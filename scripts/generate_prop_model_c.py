#!/usr/bin/env python3
"""
Comprehensive Binary Model Parser for GoldenEye 007

Parses N64 binary model files (.bin) and generates C source code (Model.c).

Binary Format:
    Offset 0x00: Switches array (if numSwitches > 0)
    Offset 0x??: Texture table (ModelFileTextures[numtextures])
    Offset 0x??: RootNode and complete ModelNode tree with embedded data

All pointers in the binary are stored as offsets from base address 0x05000000.
The game's load_object_fill_header() converts these to RAM pointers at runtime.

Usage:
    python3 scripts/generate_prop_model_c.py [--dry-run] [--force] [prop ...]
"""

import struct
import sys
import re
from pathlib import Path
from typing import Dict, List, Tuple, Optional, Any
from dataclasses import dataclass, field

# Constants
BASE_ADDRESS = 0x05000000
VERTEX_SIZE = 16  # sizeof(Vertex)
GFX_SIZE = 8  # sizeof(Gfx)

# ModelNode Opcodes
OPCODES = {
    0: "NULL", 1: "HEADER", 2: "GROUP", 3: "OP03", 4: "DL", 5: "OP05",
    6: "OP06", 7: "OP07", 8: "LOD", 9: "BSP", 10: "BBOX", 11: "OP11",
    12: "GUNFIRE", 13: "SHADOW", 14: "OP14", 15: "INTERLINK", 16: "OP16",
    17: "OP17", 18: "SWITCH", 19: "OP19", 20: "OP20", 21: "GROUPSIMPLE",
    22: "DLPRIMARY", 23: "HEAD", 24: "DLCOLLISION",
}

# Binary reader utilities
def read_u32(data, offset): return struct.unpack('>I', data[offset:offset+4])[0]
def read_s32(data, offset): return struct.unpack('>i', data[offset:offset+4])[0]
def read_u16(data, offset): return struct.unpack('>H', data[offset:offset+2])[0]
def read_s16(data, offset): return struct.unpack('>h', data[offset:offset+2])[0]
def read_u8(data, offset): return data[offset]
def read_s8(data, offset): return struct.unpack('>b', bytes([data[offset]]))[0]
def read_float(data, offset): return struct.unpack('>f', data[offset:offset+4])[0]

def float_to_c(f: float) -> str:
    """Convert float to C literal (IDO doesn't support hex float literals)"""
    if f == 0.0:
        return "0.0f"
    elif f == 1.0:
        return "1.0f"
    elif f == -1.0:
        return "-1.0f"
    else:
        # Use sufficient precision for exact binary representation
        return f"{f}f"

@dataclass
class ModelTexture:
    texture_id: int
    width: int
    height: int
    mipmaptiles: int
    type: int
    renderdepth: int
    sflags: int
    tflags: int

@dataclass
class Vertex:
    x: int
    y: int
    z: int
    flag: int
    s: int
    t: int
    r: int
    g: int
    b: int
    a: int

@dataclass
class ModelNode:
    offset: int  # Offset in binary where this node is located
    opcode: int
    opcode_flags: int = 0  # High byte of the opcode field (UseAdditionalMatrices flag for chr models)
    data_offset: int = 0
    parent_offset: int = 0
    next_offset: int = 0
    prev_offset: int = 0
    child_offset: int = 0
    data: Any = None  # Parsed data structure based on opcode

class BinaryModelParser:
    def __init__(self, binary_data: bytes, metadata: dict, image_map: dict = None):
        self.data = binary_data
        self.metadata = metadata
        self.nodes = {}  # offset -> ModelNode
        self.referenced_data = {}  # offset -> parsed data structure (no ModelNode)
        self.textures = []
        self.switches = []
        self.base_address = BASE_ADDRESS
        self.image_map = image_map or {}

    def to_file_offset(self, addr: int) -> int:
        """Convert base-relative address to file offset. Returns -1 for NULL (0x00000000)"""
        if addr == 0:
            return -1  # NULL pointer
        if addr < self.base_address:
            return addr  # Already a file offset
        return addr - self.base_address

    def offset_to_ptr_name(self, offset: int) -> str:
        """Convert binary offset to C pointer/symbol name"""
        if offset == 0:
            return "NULL"
        # Check if it's a known node
        if offset in self.nodes:
            return f"&ModelNode_0x{offset:03x}"
        # It's some data structure
        return f"0x{offset:08x}"  # Will need resolving later

    def parse(self):
        """Parse the complete binary model file"""
        num_switches = self.metadata.get('num_switches', 0)
        num_textures = self.metadata['num_textures']

        offset = 0

        # Parse switches array (array of u32 pointers)
        if num_switches > 0:
            for i in range(num_switches):
                switch_ptr = read_u32(self.data, offset)
                # Convert absolute address (0x05xxxxxx) to relative offset
                if switch_ptr > 0:
                    switch_offset = switch_ptr - BASE_ADDRESS
                else:
                    switch_offset = 0
                self.switches.append(switch_offset)
                offset += 4

        # Parse texture table (12 bytes per texture)
        for i in range(num_textures):
            tex = ModelTexture(
                texture_id=read_u32(self.data, offset),
                width=read_u8(self.data, offset + 4),
                height=read_u8(self.data, offset + 5),
                mipmaptiles=read_u8(self.data, offset + 6),
                type=read_u8(self.data, offset + 7),
                renderdepth=read_u8(self.data, offset + 8),
                sflags=read_u8(self.data, offset + 9),
                tflags=read_u8(self.data, offset + 10),
            )
            self.textures.append(tex)
            offset += 12

        # Parse ModelNode tree (starts after textures)
        # RootNode is at this offset
        root_offset = offset
        self.parse_node_tree(root_offset)

        # Second pass: parse structures referenced by pointers but not in ModelNode tree
        self.parse_referenced_structures()

        return {
            'switches': self.switches,
            'textures': self.textures,
            'nodes': self.nodes,
            'referenced_data': self.referenced_data,
            'root_offset': root_offset
        }

    def parse_node_tree(self, node_offset: int):
        """Recursively parse ModelNode tree structure"""
        if node_offset in self.nodes:
            return

        # Parse ModelNode structure (24 bytes)
        # typedef struct ModelNode {
        #     u8 UseAdditionalMatrices; /*0x00 - high byte of opcode field, used in chr models*/
        #     u8 Opcode;                /*0x01 - low byte*/
        #     u16 padding;              /*0x02*/
        #     union ModelRoData *Data;  /*0x04*/
        #     struct ModelNode *Parent; /*0x08*/
        #     struct ModelNode *Next;   /*0x0c*/
        #     struct ModelNode *Prev;   /*0x10*/
        #     struct ModelNode *Child;  /*0x14*/
        # } ModelNode;
        # Note: Officially it's "u16 Opcode" but chr models use both bytes separately

        opcode_u16 = read_u16(self.data, node_offset)
        opcode_flags = (opcode_u16 >> 8) & 0xFF  # High byte
        opcode = opcode_u16 & 0xFF  # Low byte
        data_offset = self.to_file_offset(read_u32(self.data, node_offset + 4))
        parent_offset = self.to_file_offset(read_u32(self.data, node_offset + 8))
        next_offset = self.to_file_offset(read_u32(self.data, node_offset + 12))
        prev_offset = self.to_file_offset(read_u32(self.data, node_offset + 16))
        child_offset = self.to_file_offset(read_u32(self.data, node_offset + 20))

        # Create node
        node = ModelNode(
            offset=node_offset,
            opcode=opcode,
            opcode_flags=opcode_flags,
            data_offset=data_offset,
            parent_offset=parent_offset,
            next_offset=next_offset,
            prev_offset=prev_offset,
            child_offset=child_offset
        )

        self.nodes[node_offset] = node

        # Parse the data structure for this opcode
        if data_offset > 0:
            node.data = self.parse_node_data(node.opcode, data_offset)

        # Recursively parse linked nodes (but not parent to avoid cycles)
        if child_offset > 0:
            self.parse_node_tree(child_offset)
        if next_offset > 0:
            self.parse_node_tree(next_offset)

    def parse_referenced_structures(self):
        """Parse structures referenced by pointers that aren't in ModelNode tree"""
        # Collect all referenced offsets
        referenced_offsets = set()

        for node_offset, node in self.nodes.items():
            if not node.data:
                continue

            dtype = node.data.get('_type')

            # HeaderRecord.FirstGroup actually points to a ModelNode (not directly to GroupRecord)
            # The ModelNode is already in the tree, so no need to parse as referenced structure

            # LODRecord.Children point to ModelNodes with data
            if dtype == 'LODRecord':
                for i in range(3):
                    child_offset = node.data.get(f'child{i}_offset', 0)
                    if child_offset > 0 and child_offset in self.nodes:
                        child_node = self.nodes[child_offset]
                        if child_node.data_offset > 0:
                            # Determine opcode from child node
                            ref_opcode = child_node.opcode
                            referenced_offsets.add((child_node.data_offset, ref_opcode))

        # Parse any referenced structures that don't already have a ModelNode
        for ref_offset, ref_opcode in referenced_offsets:
            # Check if this offset already has a node pointing to it
            already_exists = any(
                node.data_offset == ref_offset
                for node in self.nodes.values()
            )

            if not already_exists and ref_offset not in self.referenced_data:
                # Parse and store as a standalone data structure (not a ModelNode)
                self.referenced_data[ref_offset] = self.parse_node_data(ref_opcode, ref_offset)

    def parse_node_data(self, opcode: int, data_offset: int) -> Dict:
        """Parse the data structure for a specific opcode"""

        if opcode == 2:  # GROUP
            return self.parse_group_record(data_offset)
        elif opcode == 9:  # BSP
            return self.parse_bsp_record(data_offset)
        elif opcode == 10:  # BBOX
            return self.parse_bbox_record(data_offset)
        elif opcode == 18:  # SWITCH
            return self.parse_switch_record(data_offset)
        elif opcode == 21:  # GROUPSIMPLE
            return self.parse_groupsimple_record(data_offset)
        elif opcode == 1:  # HEADER
            return self.parse_header_record(data_offset)
        elif opcode == 12:  # GUNFIRE
            return self.parse_gunfire_record(data_offset)
        elif opcode == 22:  # DLPRIMARY
            return self.parse_dlprimary_record(data_offset)
        elif opcode == 24:  # DLCOLLISION
            return self.parse_dlcollision_record(data_offset)
        elif opcode == 4:  # DL
            return self.parse_dl_record(data_offset)
        elif opcode == 8:  # LOD
            return self.parse_lod_record(data_offset)
        elif opcode == 23:  # HEAD
            return self.parse_head_placeholder_record(data_offset)
        elif opcode == 13:  # SHADOW
            return self.parse_shadow_record(data_offset)
        elif opcode == 15:  # INTERLINK
            return self.parse_interlink_record(data_offset)
        # Add more as needed
        else:
            return {'_raw_offset': data_offset}

    def parse_group_record(self, offset: int) -> Dict:
        """Parse ModelRoData_GroupRecord (28 bytes)"""
        return {
            '_type': 'GroupRecord',
            'origin_x': read_float(self.data, offset),
            'origin_y': read_float(self.data, offset + 4),
            'origin_z': read_float(self.data, offset + 8),
            'joint_id': read_u16(self.data, offset + 12),
            'matrix_id0': read_s16(self.data, offset + 14),
            'matrix_id1': read_s16(self.data, offset + 16),
            'matrix_id2': read_s16(self.data, offset + 18),
            'child_group_offset': self.to_file_offset(read_u32(self.data, offset + 20)),
            'bounding_volume_radius': read_float(self.data, offset + 24),
        }

    def parse_bbox_record(self, offset: int) -> Dict:
        """Parse ModelRoData_BoundingBoxRecord (28 bytes)"""
        return {
            '_type': 'BoundingBoxRecord',
            'model_number': read_u32(self.data, offset),
            'xmin': read_float(self.data, offset + 4),
            'xmax': read_float(self.data, offset + 8),
            'ymin': read_float(self.data, offset + 12),
            'ymax': read_float(self.data, offset + 16),
            'zmin': read_float(self.data, offset + 20),
            'zmax': read_float(self.data, offset + 24),
        }

    def parse_lod_record(self, offset: int) -> Dict:
        """Parse ModelRoData_LODRecord (16 bytes)

        Structure (from bondtypes.h):
            f32 MinDistance;    // 0x0
            f32 MaxDistance;    // 0x4
            ModelNode *Affects; // 0x8 (runtime pointer, set to NULL in binary)
            u16 RwDataIndex;    // 0xC
            u16 reserved;       // 0xE
        Total: 16 bytes
        """
        return {
            '_type': 'LODRecord',
            'min_distance': read_float(self.data, offset + 0x0),
            'max_distance': read_float(self.data, offset + 0x4),
            'child_node_ptr': read_u32(self.data, offset + 0x8),  # Runtime pointer
            'rw_data_index': read_u16(self.data, offset + 0xC),
            'reserved': read_u16(self.data, offset + 0xE),
            '_binary_size': 16,  # Actual size in binary
        }

    def parse_dlcollision_record(self, offset: int) -> Dict:
        """Parse ModelRoData_DisplayList_CollisionRecord (32 bytes)

        Structure (from bondtypes.h):
            Gfx    *Primary;              // 0x00 - Primary display list
            Gfx    *Secondary;            // 0x04 - Secondary display list (optional)
            Vertex *Vertices;             // 0x08 - Render vertices
            s16     numVertices;          // 0x0C - Number of render vertices
            s16     numCollisionVertices; // 0x0E - Number of collision vertices
            Vertex *CollisionVertices;    // 0x10 - Collision vertices
            s16    *PointUsage;           // 0x14 - Point usage array
            s16     ModelType;            // 0x18 - Model type flags
            u16     RwDataIndex;          // 0x1A - Runtime data index
            void   *BaseAddr;             // 0x1C - Runtime base address
        Total: 32 bytes
        """
        primary_offset = self.to_file_offset(read_u32(self.data, offset + 0x0))
        secondary_offset = self.to_file_offset(read_u32(self.data, offset + 0x4))
        vertices_offset = self.to_file_offset(read_u32(self.data, offset + 0x8))
        num_vertices = read_s16(self.data, offset + 0xC)
        num_collision_vertices = read_s16(self.data, offset + 0xE)
        collision_vertices_offset = self.to_file_offset(read_u32(self.data, offset + 0x10))
        point_usage_offset = self.to_file_offset(read_u32(self.data, offset + 0x14))
        model_type = read_s16(self.data, offset + 0x18)
        rw_data_index = read_u16(self.data, offset + 0x1A)
        # base_addr at 0x1C is runtime pointer, not needed

        # Parse vertex arrays
        vertices = self.parse_vertex_array(vertices_offset, num_vertices) if vertices_offset > 0 else []
        collision_vertices = self.parse_vertex_array(collision_vertices_offset, num_collision_vertices) if collision_vertices_offset > 0 else []
        point_usage = self.parse_point_usage(point_usage_offset, num_vertices) if point_usage_offset > 0 else []

        # Parse GDL commands
        primary_gfx = self.parse_gfx_list(primary_offset, vertices_offset) if primary_offset > 0 else []
        secondary_gfx = self.parse_gfx_list(secondary_offset, vertices_offset) if secondary_offset > 0 else []

        return {
            '_type': 'DisplayListCollisionRecord',
            'primary_offset': primary_offset,
            'secondary_offset': secondary_offset,
            'vertices_offset': vertices_offset,
            'collision_vertices_offset': collision_vertices_offset,
            'point_usage_offset': point_usage_offset,
            'vertices': vertices,
            'collision_vertices': collision_vertices,
            'point_usage': point_usage,
            'model_type': model_type,
            'rw_data_index': rw_data_index,
            'primary_gfx': primary_gfx,
            'secondary_gfx': secondary_gfx,
        }

    def parse_dl_record(self, offset: int) -> Dict:
        """Parse ModelRoData_DisplayListRecord (19 bytes logical, 20 with padding)"""
        return {
            '_type': 'DisplayListRecord',
            'primary_offset': self.to_file_offset(read_u32(self.data, offset)),
            'secondary_offset': self.to_file_offset(read_u32(self.data, offset + 4)),
            # More fields...
        }

    def parse_bsp_record(self, offset: int) -> Dict:
        """Parse ModelRoData_BSPRecord (36 bytes total)"""
        return {
            '_type': 'BSPRecord',
            'point_x': read_float(self.data, offset),
            'point_y': read_float(self.data, offset + 4),
            'point_z': read_float(self.data, offset + 8),
            'vector_x': read_float(self.data, offset + 12),
            'vector_y': read_float(self.data, offset + 16),
            'vector_z': read_float(self.data, offset + 20),
            'left_child_offset': self.to_file_offset(read_u32(self.data, offset + 24)),
            'right_child_offset': self.to_file_offset(read_u32(self.data, offset + 28)),
            'reserved': read_s16(self.data, offset + 32),
            'rw_data_index': read_u16(self.data, offset + 34),
        }

    def parse_switch_record(self, offset: int) -> Dict:
        """Parse ModelRoData_SwitchRecord (8 bytes)"""
        return {
            '_type': 'SwitchRecord',
            'controls_offset': self.to_file_offset(read_u32(self.data, offset)),
            'rw_data_index': read_u16(self.data, offset + 4),
            'reserved': read_u16(self.data, offset + 6),
        }

    def parse_interlink_record(self, offset: int) -> Dict:
        """Parse ModelRoData_InterlinkageRecord (28 bytes)

        Structure (from bondtypes.h):
            coord3d pos;      // 0x0 - position (3 floats)
            u32     unknown1; // 0xC
            u32     unknown2; // 0x10
            u32     unknown3; // 0x14
            f32     Scale;    // 0x18
        Total: 28 bytes (0x1C)
        """
        return {
            '_type': 'InterlinkageRecord',
            'pos_x': read_float(self.data, offset + 0x0),
            'pos_y': read_float(self.data, offset + 0x4),
            'pos_z': read_float(self.data, offset + 0x8),
            'unknown1': read_u32(self.data, offset + 0xC),
            'unknown2': read_u32(self.data, offset + 0x10),
            'unknown3': read_u32(self.data, offset + 0x14),
            'scale': read_float(self.data, offset + 0x18),
        }

    def parse_groupsimple_record(self, offset: int) -> Dict:
        """Parse ModelRoData_GroupSimpleRecord (20 bytes)"""
        return {
            '_type': 'GroupSimpleRecord',
            'origin_x': read_float(self.data, offset),
            'origin_y': read_float(self.data, offset + 4),
            'origin_z': read_float(self.data, offset + 8),
            'group1': read_s16(self.data, offset + 12),
            'group2': read_u16(self.data, offset + 14),
            'bounding_volume_radius': read_float(self.data, offset + 16),
        }

    def parse_header_record(self, offset: int) -> Dict:
        """Parse ModelRoData_HeaderRecord (16 bytes)"""
        return {
            '_type': 'HeaderRecord',
            'anim_part': read_u16(self.data, offset),
            'matrix_index': read_s16(self.data, offset + 2),
            'first_group_offset': self.to_file_offset(read_u32(self.data, offset + 4)),
            'group1': read_u16(self.data, offset + 8),
            'group2': read_u16(self.data, offset + 10),
            'rw_data_index': read_u16(self.data, offset + 12),
            'reserved': read_u16(self.data, offset + 14),
        }

    def parse_head_placeholder_record(self, offset: int) -> Dict:
        """Parse ModelRoData_HeadPlaceholderRecord (4 bytes with padding)

        Structure (from bondtypes.h):
            u16 RwDataIndex;     // 0x0
            u16 padding;         // 0x2 (implicit padding to 4 bytes)
        Total: 4 bytes
        """
        return {
            '_type': 'HeadPlaceholderRecord',
            'rw_data_index': read_u16(self.data, offset),
            'padding': read_u16(self.data, offset + 2),
        }

    def parse_shadow_record(self, offset: int) -> Dict:
        """Parse ModelRoData_ShadowRecord (32 bytes)

        Structure (from bondtypes.h):
            coord2d pos;                   // 0x0 (2 floats)
            coord2d size;                  // 0x8 (2 floats)
            void *image;                   // 0x10
            ModelRoData_HeaderRecord *Header;  // 0x14 (pointer to header node)
            f32 Scale;                     // 0x18
            void *BaseAddr;                // 0x1C
        Total: 32 bytes (0x20)
        """
        image_ptr = read_u32(self.data, offset + 0x10)
        image_offset = self.to_file_offset(image_ptr) if image_ptr != 0 else 0

        header_ptr = read_u32(self.data, offset + 0x14)
        header_offset = self.to_file_offset(header_ptr) if header_ptr != 0 else 0

        return {
            '_type': 'ShadowRecord',
            'pos_x': read_float(self.data, offset + 0x0),
            'pos_y': read_float(self.data, offset + 0x4),
            'size_x': read_float(self.data, offset + 0x8),
            'size_y': read_float(self.data, offset + 0xC),
            'image_offset': image_offset,
            'header_offset': header_offset,
            'scale': read_float(self.data, offset + 0x18),
            'base_addr': read_u32(self.data, offset + 0x1C),
        }

    def parse_gunfire_record(self, offset: int) -> Dict:
        """Parse ModelRoData_GunfireRecord (40 bytes)

        Structure (from bondtypes.h):
            coord3d Offset;      // 0x0 (3 floats)
            coord3d Size;        // 0xC (3 floats)
            void *Image;         // 0x18 (pointer to texture)
            f32 Scale;           // 0x1C
            u16 RwDataIndex;     // 0x20
            u16 reserved;        // 0x22
            u32 BaseAddr;        // 0x24
        Total: 40 bytes (0x28)
        """
        image_ptr = read_u32(self.data, offset + 0x18)
        image_offset = self.to_file_offset(image_ptr) if image_ptr != 0 else 0

        return {
            '_type': 'GunfireRecord',
            'offset_x': read_float(self.data, offset + 0x0),
            'offset_y': read_float(self.data, offset + 0x4),
            'offset_z': read_float(self.data, offset + 0x8),
            'size_x': read_float(self.data, offset + 0xC),
            'size_y': read_float(self.data, offset + 0x10),
            'size_z': read_float(self.data, offset + 0x14),
            'image_offset': image_offset,
            'scale': read_float(self.data, offset + 0x1C),
            'rw_data_index': read_u16(self.data, offset + 0x20),
            'reserved': read_u16(self.data, offset + 0x22),
            'base_addr': read_u32(self.data, offset + 0x24),
            '_binary_size': 40,
        }

    def parse_dlprimary_record(self, offset: int) -> Dict:
        """Parse ModelRoData_DisplayListPrimaryRecord (16 bytes)

        Structure (from bondtypes.h):
            s32 numVertices;    // 0x0
            Vertex *Vertices;   // 0x4
            Gfx *Primary;       // 0x8
            void *BaseAddr;     // 0xC
        Total: 16 bytes
        """
        num_vertices = read_s32(self.data, offset)
        vertices_ptr = read_u32(self.data, offset + 4)
        primary_ptr = read_u32(self.data, offset + 8)
        base_addr = read_u32(self.data, offset + 12)

        vertices_offset = self.to_file_offset(vertices_ptr) if vertices_ptr != 0 else 0
        primary_offset = self.to_file_offset(primary_ptr) if primary_ptr != 0 else 0

        # Parse vertex array if present
        vertices = []
        if vertices_offset > 0 and num_vertices > 0:
            for i in range(num_vertices):
                v_offset = vertices_offset + i * 16
                vertices.append(Vertex(
                    x=read_s16(self.data, v_offset + 0),
                    y=read_s16(self.data, v_offset + 2),
                    z=read_s16(self.data, v_offset + 4),
                    flag=read_u16(self.data, v_offset + 6),
                    s=read_s16(self.data, v_offset + 8),
                    t=read_s16(self.data, v_offset + 10),
                    r=read_u8(self.data, v_offset + 12),
                    g=read_u8(self.data, v_offset + 13),
                    b=read_u8(self.data, v_offset + 14),
                    a=read_u8(self.data, v_offset + 15)
                ))

        # Parse Gfx array
        primary_gfx, _ = parse_gfx_array(self.data, primary_offset, self.base_address) if primary_offset > 0 else ([], 0)

        return {
            '_type': 'DisplayListPrimaryRecord',
            'num_vertices': num_vertices,
            'vertices_offset': vertices_offset,
            'primary_offset': primary_offset,
            'base_addr': base_addr,
            'vertices': vertices,
            'primary_gfx': primary_gfx,
            '_binary_size': 16,
        }

    def parse_dl_record(self, offset: int) -> Dict:
        """Parse ModelRoData_DisplayListRecord (19 bytes)"""
        primary_offset = self.to_file_offset(read_u32(self.data, offset))
        secondary_offset = self.to_file_offset(read_u32(self.data, offset + 4))
        vertices_offset = self.to_file_offset(read_u32(self.data, offset + 12))
        num_vertices = read_u16(self.data, offset + 16)

        # Extract vertex array if present
        vertices = []
        if vertices_offset > 0 and num_vertices > 0:
            vertices = self.parse_vertex_array(vertices_offset, num_vertices)

        # Extract Gfx display lists if present
        primary_gfx = []
        if primary_offset > 0:
            primary_gfx = self.parse_gfx_list(primary_offset, vertices_offset)

        secondary_gfx = []
        if secondary_offset > 0:
            secondary_gfx = self.parse_gfx_list(secondary_offset, vertices_offset)

        return {
            '_type': 'DisplayListRecord',
            'primary_offset': primary_offset,
            'secondary_offset': secondary_offset,
            'base_addr': read_u32(self.data, offset + 8),
            'vertices_offset': vertices_offset,
            'num_vertices': num_vertices,
            'model_type': read_s8(self.data, offset + 18),
            'vertices': vertices,
            'primary_gfx': primary_gfx,
            'secondary_gfx': secondary_gfx,
        }

    def parse_vertex_array(self, offset: int, count: int) -> List[Vertex]:
        """Parse Vertex array (16 bytes per vertex)"""
        vertices = []
        for i in range(count):
            v_off = offset + i * 16
            v = Vertex(
                x=read_s16(self.data, v_off),
                y=read_s16(self.data, v_off + 2),
                z=read_s16(self.data, v_off + 4),
                flag=read_u16(self.data, v_off + 6),
                s=read_s16(self.data, v_off + 8),
                t=read_s16(self.data, v_off + 10),
                r=read_u8(self.data, v_off + 12),
                g=read_u8(self.data, v_off + 13),
                b=read_u8(self.data, v_off + 14),
                a=read_u8(self.data, v_off + 15),
            )
            vertices.append(v)
        return vertices

    def parse_point_usage(self, offset: int, count: int) -> List[int]:
        """Parse point usage array (s16[])"""
        return [read_s16(self.data, offset + i * 2) for i in range(count)]

    def parse_gfx_list(self, offset: int, vertices_offset: int = None) -> List[str]:
        """Parse Gfx display list commands (8 bytes each) until final end marker
        Returns decoded command strings ready for C output

        vertices_offset: File offset where vertex array starts (for resolving vertex addresses)
        """
        if offset <= 0 or offset >= len(self.data):
            return []

        # Prepare vertex array name for address resolution
        vtx_array_name = f"Vertex_0x{vertices_offset:03x}" if vertices_offset else None

        gfx_cmds = []
        while offset + 8 <= len(self.data):
            w0 = read_u32(self.data, offset)
            w1 = read_u32(self.data, offset + 4)

            # Decode command to macro format with image_map for texture lookup
            decoded = decode_gfx_command(w0, w1, vtx_array_name, vertices_offset, self.image_map)
            gfx_cmds.append(decoded)

            opcode = (w0 >> 24) & 0xFF

            # Stop at B8 (final end marker), but continue past E7 (initial marker)
            if opcode == 0xB8:
                break

            offset += 8

            # Safety limit to prevent infinite loops
            if len(gfx_cmds) > 10000:
                break

        return gfx_cmds


def load_image_map():
    """Load IMAGE_* to ID mapping from images.def"""
    image_map = {}
    images_file = Path("assets/images.def")
    if images_file.exists():
        with open(images_file, 'r') as f:
            for idx, line in enumerate(f):
                line = line.strip()
                if line.startswith('IMAGE('):
                    # Extract the name from IMAGE(NAME, ...)
                    match = re.match(r'IMAGE\(([^,]+),', line)
                    if match:
                        name = match.group(1).strip()
                        # Map index to IMAGE_NAME format (no _ID suffix)
                        image_map[idx] = f"IMAGE_{name}"
    return image_map


def parse_metadata_files(prop_name: str) -> Optional[Dict]:
    """Parse ModelFileHeader.inc.c and propFileRecord.inc.c"""
    prop_dir = Path("assets/obseg/prop") / prop_name

    header_file = prop_dir / "ModelFileHeader.inc.c"
    record_file = prop_dir / "propFileRecord.inc.c"

    if not header_file.exists() or not record_file.exists():
        return None

    metadata = {}

    # Parse ModelFileHeader.inc.c
    with open(header_file, 'r') as f:
        content = f.read()
        # Allow hex or decimal for all numeric fields
        match = re.search(r'MODELFILEHEADER\s*\(\s*(\w+)\s*,\s*(0x[0-9A-Fa-f]+|\d+)\s*,\s*&SKELETON\((\w+)\)\s*,\s*(0x[0-9A-Fa-f]+|\d+)\s*,\s*(0x[0-9A-Fa-f]+|\d+)\s*,\s*(0x[0-9A-Fa-f]+|\d+)\s*,\s*([\d.]+)\s*,\s*(0x[0-9A-Fa-f]+|\d+)\s*,\s*(0x[0-9A-Fa-f]+|\d+)\s*\)', content)
        if match:
            metadata['name'] = match.group(1)
            metadata['skeleton'] = match.group(3)
            metadata['num_switches'] = int(match.group(5), 0)
            metadata['bounding_radius'] = float(match.group(7))
            metadata['num_textures'] = int(match.group(9), 0)  # 0 base auto-detects hex/decimal

    # Parse propFileRecord.inc.c
    with open(record_file, 'r') as f:
        content = f.read()
        match = re.search(r'PROPFILERECORD\s*\(\s*\w+\s*,\s*([\d.]+)\s*\)', content)
        if match:
            metadata['scale'] = float(match.group(1))

    return metadata if 'num_textures' in metadata else None


def decode_gfx_command(w0: int, w1: int, vertex_array_name: str = None, vertex_array_offset: int = None, image_map: dict = None) -> str:
    """
    Decode a single Gfx command (w0, w1) to its GBI macro representation.
    Returns the macro call as a string (e.g., "gsDPPipeSync()").
    Falls back to raw hex format if command is unknown.
    image_map: dict mapping texture_id -> IMAGE_NAME for G_SETTEX

    Reference: include/PR/gbi.h

    vertex_array_name: Name of the vertex array (e.g., "Vertex_0x098")
    vertex_array_offset: File offset where vertex array starts
    """
    opcode = (w0 >> 24) & 0xFF

    # Helper to resolve segment addresses to symbol names
    def resolve_address(addr):
        # All segment addresses (0xXX000000 format) are runtime-resolved
        # Leave them as raw hex for the game engine to resolve
        # The segments (3, 4, 5, etc.) are set up at runtime
        return f"0x{addr:08X}"

    # Helper to extract bit fields using _SHIFTR logic
    def extract_bits(value, shift, width):
        return (value >> shift) & ((1 << width) - 1)

    # G_IM_FMT_ constants from gbi.h
    FMT_NAMES = {
        0: "G_IM_FMT_RGBA",
        1: "G_IM_FMT_YUV",
        2: "G_IM_FMT_CI",
        3: "G_IM_FMT_IA",
        4: "G_IM_FMT_I",
    }

    # G_IM_SIZ_ constants from gbi.h
    SIZ_NAMES = {
        0: "G_IM_SIZ_4b",
        1: "G_IM_SIZ_8b",
        2: "G_IM_SIZ_16b",
        3: "G_IM_SIZ_32b",
        5: "G_IM_SIZ_DD",
    }

    # RDP Sync commands (no parameters)
    if opcode == 0xE7:
        return "gsDPPipeSync()"
    elif opcode == 0xE6:
        return "gsDPLoadSync()"
    elif opcode == 0xE8:
        return "gsDPTileSync()"
    elif opcode == 0xE9:
        return "gsDPFullSync()"
    # G_SETTEX (0xC0) - GoldenEye custom command for texture setup
    # gsSPUseTexture(cms, cmt, tile, shifts, shiftt, type, minlevel, detail_id, texture_id)
    elif opcode == 0xC0:
        cms = extract_bits(w0, 22, 2)
        cmt = extract_bits(w0, 20, 2)
        tile = extract_bits(w0, 18, 2)
        shifts = extract_bits(w0, 14, 4)
        shiftt = extract_bits(w0, 10, 4)
        type_val = extract_bits(w0, 0, 3)
        minlevel = extract_bits(w1, 24, 8)
        detail_id = extract_bits(w1, 12, 12)
        texture_id = extract_bits(w1, 0, 12)

        # Map type value to TextureTypes enum
        type_names = [
            "TEXTURETYPE_LOD",
            "TEXTURETYPE_DETAIL",
            "TEXTURETYPE_MIPMAP",
            "TEXTURETYPE_TILE",
            "TEXTURETYPE_TILE_PRESWAPPED"
        ]
        type_str = type_names[type_val] if type_val < len(type_names) else str(type_val)

        # Look up IMAGE enum from texture_id
        if image_map and texture_id in image_map:
            image_name = image_map[texture_id]
            return f"gsSPUseTexture({cms}, {cmt}, {tile}, {shifts}, {shiftt}, {type_str}, {minlevel}, {detail_id}, {image_name})"
        else:
            # Fallback to raw texture_id if not found in map
            return f"gsSPUseTexture({cms}, {cmt}, {tile}, {shifts}, {shiftt}, {type_str}, {minlevel}, {detail_id}, 0x{texture_id:03X})"

    elif opcode == 0x00:
        return "gsDPNoOp()"

    # gsSPEndDisplayList (0xB8 or 0xDF depending on mode)
    elif opcode == 0xB8 or opcode == 0xDF:
        return "gsSPEndDisplayList()"

    # gsSPMatrixGE (0x01 G_MTX) - GoldenEye matrix command
    # Format: w0 = cmd(24-31) | params(16-23) | sizeof(Mtx)(0-15), w1 = address
    # params bits: projection(0) | load(1) | push(2)
    elif opcode == 0x01:
        params = extract_bits(w0, 16, 8)
        size = extract_bits(w0, 0, 16)
        addr = w1
        addr_str = resolve_address(addr)

        # Decode matrix parameters
        param_str = ""
        if params & 0x01:
            param_str = "G_MTX_PROJECTION"
        else:
            param_str = "G_MTX_MODELVIEW"

        if params & 0x02:
            param_str += " | G_MTX_LOAD"
        else:
            param_str += " | G_MTX_MUL"

        if params & 0x04:
            param_str += " | G_MTX_PUSH"
        else:
            param_str += " | G_MTX_NOPUSH"

        # Output raw bytes with decoded comment
        return f"gsSPMatrix({addr_str}, {param_str})"

    elif opcode == 0x04:
        # RARE vertex format: w0 = cmd(24-31) | encoded(16-23) | sizeof(Vtx)*n(0-15)
        # encoded byte format: ((v0+n) << 1) | flag
        # Different from standard F3DEX which uses v0*2 and ((n)<<10)|(sizeof(Vtx)*(n)-1)
        encoded_value = extract_bits(w0, 16, 8)
        length = extract_bits(w0, 0, 16)
        flag = encoded_value & 1
        n = encoded_value >> 4
        #n = length // 16  # sizeof(Vtx) = 16
        v0 = encoded_value & 15
        addr = w1
        addr_str = resolve_address(addr)
        return f"gsSPVertex({addr_str}, {n+1}, {v0})"


    # G_TRI4 (0xB1) - GoldenEye extension packing 4 triangles with 4-bit vertex indices
    # gsSP4Triangles(x1, y1, z1, x2, y2, z2, x3, y3, z3, x4, y4, z4)
    elif opcode == 0xB1:
        # Extract from w0: z4(12-15) | z3(8-11) | z2(4-7) | z1(0-3)
        z1 = extract_bits(w0, 0, 4)
        z2 = extract_bits(w0, 4, 4)
        z3 = extract_bits(w0, 8, 4)
        z4 = extract_bits(w0, 12, 4)

        # Extract from w1: y4(28-31) | x4(24-27) | y3(20-23) | x3(16-19) | y2(12-15) | x2(8-11) | y1(4-7) | x1(0-3)
        x1 = extract_bits(w1, 0, 4)
        y1 = extract_bits(w1, 4, 4)
        x2 = extract_bits(w1, 8, 4)
        y2 = extract_bits(w1, 12, 4)
        x3 = extract_bits(w1, 16, 4)
        y3 = extract_bits(w1, 20, 4)
        x4 = extract_bits(w1, 24, 4)
        y4 = extract_bits(w1, 28, 4)

        # If z3 and z4 are both zero, produce a shorter gsSP2Triangles macro as requested
        if x3 == 0 and x4 == 0 and y3 == 0 and y4 == 0 and z3 == 0 and z4 == 0:
            return f"gsSP2Triangles({x1}, {y1}, {z1},{x1}, {x2}, {y2}, {z2}, {x2})"

        return f"gsSP4Triangles({x1}, {y1}, {z1}, {x2}, {y2}, {z2}, {x3}, {y3}, {z3}, {x4}, {y4}, {z4})"

    # gsSP1Triangle (0x05 in F3DEX_GBI_2, 0xBF in F3DEX/classic)
    elif opcode == 0x05:
        v0 = extract_bits(w1, 16, 8) // 2
        v1 = extract_bits(w1, 8, 8) // 2
        v2 = extract_bits(w1, 0, 8) // 2
        flag = 0
        return f"gsSP1Triangle({v0}, {v1}, {v2}, {flag})"

    # 0xBF - F3DEX gsSP1Triangle (GoldenEye version without flag rotation)
    elif opcode == 0xBF:
        v0 = extract_bits(w1, 16, 8) // 10
        v1 = extract_bits(w1, 8, 8) // 10
        v2 = extract_bits(w1, 0, 8) // 10
        flag = 0
        return f"gsSP1Triangle({v0}, {v1}, {v2}, {flag})"

    # gsSPTexture (0xD7 in F3DEX_GBI_2, 0xBB in classic)
    elif opcode == 0xD7 or opcode == 0xBB:
        # w0 = cmd(24-31) | bowtie(16-23) | level(11-13) | tile(8-10) | on(0-7)
        # w1 = s(16-31) | t(0-15)
        bowtie = extract_bits(w0, 16, 8)
        level = extract_bits(w0, 11, 3)
        tile = extract_bits(w0, 8, 3)
        on = extract_bits(w0, 0, 8)
        s = extract_bits(w1, 16, 16)
        t = extract_bits(w1, 0, 16)

        # Use gsSPTextureL if bowtie != 0, otherwise gsSPTexture
        if bowtie != 0:
            return f"gsSPTextureL({s}, {t}, {level}, 0x{bowtie:02X}, {tile}, {on})"
        else:
            return f"gsSPTexture({s}, {t}, {level}, {tile}, {on})"

    # gsDPSetCombineMode / gsDPSetCombineLERP (0xFC)
    elif opcode == 0xFC:
        # Complex color combiner - show as raw for now
        muxs0 = w0 & 0xFFFFFF
        muxs1 = w1
        return f"gsDPSetCombine(0x{muxs0:06X}, 0x{muxs1:08X})"

    # gsSPSetOtherMode_H (0xE3 in F3DEX_GBI_2, 0xBA in classic)
    elif opcode == 0xE3 or opcode == 0xBA:
        sft = extract_bits(w0, 8, 8)
        length = extract_bits(w0, 0, 8)
        data = w1

        # Decode common high-level macros
        if sft == 16 and length == 1:
            # gsDPSetTextureLOD
            if data == 0x00000000:
                return "gsDPSetTextureLOD(G_TL_TILE)"
            elif data == 0x00010000:
                return "gsDPSetTextureLOD(G_TL_LOD)"
        elif sft == 17 and length == 2:
            # gsDPSetTextureDetail
            if data == 0x00000000:
                return "gsDPSetTextureDetail(G_TD_CLAMP)"
            elif data == 0x00020000:
                return "gsDPSetTextureDetail(G_TD_SHARPEN)"
            elif data == 0x00040000:
                return "gsDPSetTextureDetail(G_TD_DETAIL)"
        elif sft == 12 and length == 2:
            # gsDPSetTextureFilter
            if data == 0x00000000:
                return "gsDPSetTextureFilter(G_TF_POINT)"
            elif data == 0x00002000:
                return "gsDPSetTextureFilter(G_TF_BILERP)"
            elif data == 0x00003000:
                return "gsDPSetTextureFilter(G_TF_AVERAGE)"

        # Fallback to generic macro
        return f"gsSPSetOtherMode(G_SETOTHERMODE_H, {sft}, {length}, 0x{data:08X})"

    # gsSPSetOtherMode_L (0xE2 in F3DEX_GBI_2, 0xB9 in classic)
    elif opcode == 0xE2 or opcode == 0xB9:
        sft = extract_bits(w0, 8, 8)
        length = extract_bits(w0, 0, 8)
        data = w1
        return f"gsSPSetOtherMode(G_SETOTHERMODE_L, {sft}, {length}, 0x{data:08X})"

    # gsSPGeometryMode (0xD9 in F3DEX_GBI_2)
    elif opcode == 0xD9:
        clearbits = (~w0) & 0xFFFFFF
        setbits = w1
        return f"gsSPGeometryMode(0x{clearbits:08X}, 0x{setbits:08X})"

    # gsSPSetGeometryMode (0xB7 classic, 0xD9 F3DEX2 variant)
    elif opcode == 0xB7:
        mode = w1
        return f"gsSPSetGeometryMode(0x{mode:08X})"

    # gsSPClearGeometryMode (0xB6)
    elif opcode == 0xB6:
        mode = w1
        return f"gsSPClearGeometryMode(0x{mode:08X})"

    # gsDPSetPrimColor (0xFA)
    elif opcode == 0xFA:
        m = extract_bits(w0, 8, 8)
        l = extract_bits(w0, 0, 8)
        r = extract_bits(w1, 24, 8)
        g = extract_bits(w1, 16, 8)
        b = extract_bits(w1, 8, 8)
        a = extract_bits(w1, 0, 8)
        return f"gsDPSetPrimColor({m}, {l}, {r}, {g}, {b}, {a})"

    # gsDPSetEnvColor (0xFB)
    elif opcode == 0xFB:
        r = extract_bits(w1, 24, 8)
        g = extract_bits(w1, 16, 8)
        b = extract_bits(w1, 8, 8)
        a = extract_bits(w1, 0, 8)
        return f"gsDPSetEnvColor({r}, {g}, {b}, {a})"

    # gsDPSetTextureImage (0xFD)
    elif opcode == 0xFD:
        fmt = extract_bits(w0, 21, 3)
        siz = extract_bits(w0, 19, 2)
        width = extract_bits(w0, 0, 12) + 1
        addr = w1
        fmt_name = FMT_NAMES.get(fmt, f"0x{fmt:X}")
        siz_name = SIZ_NAMES.get(siz, f"0x{siz:X}")
        return f"gsDPSetTextureImage({fmt_name}, {siz_name}, {width}, 0x{addr:08X})"

    # gsDPSetTile (0xF5) - output as raw bytes due to complex parameter encoding
    elif opcode == 0xF5:
        fmt = extract_bits(w0, 21, 3)
        siz = extract_bits(w0, 19, 2)
        line = extract_bits(w0, 9, 9)
        tmem = extract_bits(w0, 0, 9)
        tile = extract_bits(w1, 24, 3)
        palette = extract_bits(w1, 20, 4)
        ct = extract_bits(w1, 18, 2)
        mt = extract_bits(w1, 8, 2)
        maskt = extract_bits(w1, 14, 4)
        shiftt = extract_bits(w1, 10, 4)
        cs = extract_bits(w1, 2, 2)
        ms = extract_bits(w1, 8, 2)
        masks = extract_bits(w1, 4, 4)
        shifts = extract_bits(w1, 0, 4)
        fmt_name = FMT_NAMES.get(fmt, f"0x{fmt:X}")
        siz_name = SIZ_NAMES.get(siz, f"0x{siz:X}")
        return f"{{{{ 0x{w0:08X}, 0x{w1:08X} }}}}  /* gsDPSetTile({fmt_name}, {siz_name}, {line}, {tmem}, {tile}, {palette}, {ct}, {maskt}, {shiftt}, {cs}, {masks}, {shifts}) */"

    # gsDPLoadBlock (0xF3)
    elif opcode == 0xF3:
        uls = extract_bits(w0, 12, 12)
        ult = extract_bits(w0, 0, 12)
        tile = extract_bits(w1, 24, 3)
        lrs = extract_bits(w1, 12, 12)
        dxt = extract_bits(w1, 0, 12)
        return f"gsDPLoadBlock({tile}, {uls}, {ult}, {lrs}, {dxt})"

    # gsDPSetTileSize (0xF2) - output as raw bytes due to precision issues
    elif opcode == 0xF2:
        uls = extract_bits(w0, 12, 12)
        ult = extract_bits(w0, 0, 12)
        tile = extract_bits(w1, 24, 3)
        lrs = extract_bits(w1, 12, 12)
        lrt = extract_bits(w1, 0, 12)
        return f"{{{{ 0x{w0:08X}, 0x{w1:08X} }}}}  /* gsDPSetTileSize({tile}, {uls}, {ult}, {lrs}, {lrt}) */"

    # gsSPDisplayList (0xDE in F3DEX_GBI_2, 0x06 in classic)
    elif opcode == 0xDE or opcode == 0x06:
        addr = w1
        return f"gsSPDisplayList(0x{addr:08X})"

    # gsSPBranchList (0xDE with different flags)
    # Distinguishing from DisplayList requires looking at push/nopush flag
    # For now, treat as DisplayList

    # gsSPMatrix (0xDA in F3DEX_GBI_2, 0x01 in classic)
    elif opcode == 0xDA or opcode == 0x01:
        params = extract_bits(w0, 0, 8)
        addr = w1
        return f"gsSPMatrix(0x{addr:08X}, {params})"

    # gsSPPopMatrix (0xD8 in F3DEX_GBI_2, 0xBD in classic)
    elif opcode == 0xD8 or opcode == 0xBD:
        n = w1
        return f"gsSPPopMatrix(G_MTX_MODELVIEW, {n})"

    # gsDPLoadTLUT (0xF0)
    elif opcode == 0xF0:
        tile = extract_bits(w1, 24, 3)
        count = extract_bits(w1, 14, 10)
        return f"gsDPLoadTLUT({tile}, {count})"

    # gsDPSetScissor (0xED)
    elif opcode == 0xED:
        mode = extract_bits(w0, 0, 2)
        ulx = extract_bits(w0, 12, 12)
        uly = extract_bits(w0, 0, 12)
        lrx = extract_bits(w1, 12, 12)
        lry = extract_bits(w1, 0, 12)
        return f"gsDPSetScissor(G_SC_NON_INTERLACE, {ulx}, {uly}, {lrx}, {lry})"

    # Unknown command - output as raw hex with comment
    else:
        return "{{0x%08X, 0x%08X}} /* unknown opcode 0x%02X */" % (w0, w1, opcode)


def parse_gfx_array(data: bytes, offset: int, base_addr: int) -> Tuple[List[str], int]:
    """Parse Gfx display list commands until end marker (0xB8 or 0xDF)"""
    if offset == 0:
        return [], 0

    commands = []
    pos = offset
    max_commands = 1000  # Safety limit to prevent infinite loops

    for _ in range(max_commands):
        if pos + 8 > len(data):
            break

        # Read Gfx command (8 bytes)
        w0 = read_u32(data, pos)
        w1 = read_u32(data, pos + 4)

        opcode = (w0 >> 24) & 0xFF

        # Decode command to macro format
        decoded = decode_gfx_command(w0, w1)
        commands.append(decoded)
        pos += 8

        # Stop at B8 (gsSPEndDisplayList) or DF end markers
        if opcode == 0xB8 or opcode == 0xDF:
            break

    return commands, pos - offset


def generate_model_c(prop_name: str, parsed_model: Dict, metadata: Dict, image_map: Dict, binary_data: bytes) -> str:
    """Generate complete Model.c source code from parsed model data"""

    switches = parsed_model['switches']
    textures = parsed_model['textures']
    nodes = parsed_model['nodes']
    referenced_data = parsed_model.get('referenced_data', {})
    root_offset = parsed_model['root_offset']

    lines = []
    lines.append('#include "bondtypes.h"')
    lines.append('#include "bondconstants.h"')
    lines.append('#include "gbi_extension.h"')
    lines.append("")
    lines.append(f"#define TEXTURECOUNT {len(textures)}")
    lines.append("")

    # Calculate vertex counts from DisplayList nodes to generate #define statements
    vertex_counts = {}
    collision_vertex_counts = {}
    vertex_array_index = 0
    collision_array_index = 0

    for node_offset in sorted(nodes.keys()):
        node = nodes[node_offset]
        # DisplayListCollisionRecord (opcode 24)
        if node.opcode == 24 and node.data and node.data.get('_type') == 'DisplayListCollisionRecord':
            vertex_counts[vertex_array_index] = len(node.data['vertices'])
            collision_vertex_counts[collision_array_index] = len(node.data['collision_vertices'])
            vertex_array_index += 1
            collision_array_index += 1
        # DisplayListPrimaryRecord (opcode 22)
        elif node.opcode == 22 and node.data and node.data.get('_type') == 'DisplayListPrimaryRecord':
            if node.data.get('vertices'):
                vertex_counts[vertex_array_index] = len(node.data['vertices'])
                vertex_array_index += 1
        # DisplayListRecord (opcode 4)
        elif node.opcode == 4 and node.data and node.data.get('_type') == 'DisplayListRecord':
            if node.data.get('vertices'):
                vertex_counts[vertex_array_index] = len(node.data['vertices'])
                vertex_array_index += 1

    # Generate vertex count defines
    for idx, count in vertex_counts.items():
        lines.append(f"#define VERTEXGROUPCOUNT{idx} {count}")
    for idx, count in collision_vertex_counts.items():
        lines.append(f"#define COLLISIONVERTEXCOUNT{idx} {count}")
    lines.append("")

    # Forward declarations (will be populated after structure collection)
    lines.append("// Forward declarations")

    # First, declare all ModelNodes
    for node_offset in sorted(nodes.keys()):
        lines.append(f"extern ModelNode ModelNode_0x{node_offset:03x};")
    lines.append("")

    # Placeholder for data structure forward declarations (will be filled in later)
    forward_decl_placeholder_index = len(lines)

    # For props with switches: generate switch array at start of file
    # Game code expects: filedata[0..numSwitches*4-1] = switches, then textures, then nodes
    if len(switches) > 0:
        lines.append(f"// Switch array ({len(switches)} switches) - game loads this as part of binary")
        lines.append(f"u32 SwitchNodes[{len(switches)}] = ")
        lines.append("{")
        for switch_offset in switches:
            if switch_offset > 0:
                # Use label reference instead of absolute address
                lines.append(f"    (u32)&ModelNode_0x{switch_offset:03x},  // ModelNode at offset 0x{switch_offset:03X}")
            else:
                lines.append(f"    0x00000000,  // NULL")
        lines.append("};")
        lines.append("")

    # Generate texture table
    lines.append("//base address is 0x05000000")
    if len(textures) > 0:
        lines.append(f"ModelFileTextures proptextures[TEXTURECOUNT] = ")
        lines.append("{")
        for tex in textures:
            img_name = image_map.get(tex.texture_id, f"0x{tex.texture_id:X}")
            lines.append(f"    {{{img_name}, {tex.width}, {tex.height}, 0x{tex.mipmaptiles:02X}, 0x{tex.type:X}, 0x{tex.renderdepth:02X}, 0x{tex.sflags:X}, 0x{tex.tflags:X}}},")
        lines.append("};")
    else:
        # Zero-sized arrays are invalid in C, so don't generate them
        lines.append("// No textures for this prop")
    lines.append("")

    # Generate ModelNode tree
    lines.append("// ModelNode tree")
    for node_offset in sorted(nodes.keys()):
        node = nodes[node_offset]

        # Format the opcode field - need to include both flag byte and opcode byte
        # In the binary it's stored as u16 big-endian: (flags << 8) | opcode
        # We output it as a compound literal to ensure correct byte layout
        if node.opcode_flags != 0:
            # Need both bytes: output as hex u16 to ensure exact byte layout
            opcode_value = f"0x{(node.opcode_flags << 8) | node.opcode:04X}"
        else:
            # Just the opcode for prop models (flags=0)
            opcode_name = f"MODELNODE_OPCODE_{OPCODES.get(node.opcode, 'UNKNOWN')}"
            opcode_value = opcode_name

        # Format pointers
        data_ptr = f"&{get_data_symbol_name(node)}" if node.data else "NULL"
        parent_ptr = f"&ModelNode_0x{node.parent_offset:03x}" if node.parent_offset is not None and node.parent_offset >= 0 else "NULL"
        next_ptr = f"&ModelNode_0x{node.next_offset:03x}" if node.next_offset is not None and node.next_offset >= 0 else "NULL"
        prev_ptr = f"&ModelNode_0x{node.prev_offset:03x}" if node.prev_offset is not None and node.prev_offset >= 0 else "NULL"
        child_ptr = f"&ModelNode_0x{node.child_offset:03x}" if node.child_offset is not None and node.child_offset >= 0 else "NULL"

        lines.append(f"ModelNode ModelNode_0x{node_offset:03x} = {{{opcode_value}, {data_ptr}, {parent_ptr}, {next_ptr}, {prev_ptr}, {child_ptr}}};")
    lines.append("")

    # CRITICAL: IDO compiler places structures in .data section in exact C file declaration order
    # We MUST output all structures sorted by their original binary offset to match the layout

    # Step 1: Collect all structures with their (file_offset, code_lines, dependencies)
    all_structures = []

    # Map node offset to symbolic names for each data array
    node_to_vtx_name = {}
    node_to_colvtx_name = {}
    node_to_ptusage_name = {}
    node_to_gdl_prim_name = {}
    node_to_gdl_sec_name = {}

    # Create mapping from node_offset to vertex/collision array indices
    # This must match the indices generated in the first loop
    node_to_vtx_index = {}
    node_to_colvtx_index = {}
    vtx_counter = 0
    colvtx_counter = 0
    for node_offset in sorted(nodes.keys()):
        node = nodes[node_offset]
        if node.opcode == 24 and node.data and node.data.get('_type') == 'DisplayListCollisionRecord':
            node_to_vtx_index[node_offset] = vtx_counter
            node_to_colvtx_index[node_offset] = colvtx_counter
            vtx_counter += 1
            colvtx_counter += 1
        elif node.opcode == 22 and node.data and node.data.get('_type') == 'DisplayListPrimaryRecord':
            if node.data.get('vertices'):
                node_to_vtx_index[node_offset] = vtx_counter
                vtx_counter += 1
        elif node.opcode == 4 and node.data and node.data.get('_type') == 'DisplayListRecord':
            if node.data.get('vertices'):
                node_to_vtx_index[node_offset] = vtx_counter
                vtx_counter += 1

    # Counters for generating unique names
    ptusage_idx = 0
    gdl_prim_idx = 0
    gdl_sec_idx = 0

    # Step 2: Process nodes to collect all structures with their offsets
    for node_offset in sorted(nodes.keys()):
        node = nodes[node_offset]
        if not node.data:
            continue

        dtype = node.data.get('_type')

        # GroupRecord structures
        if dtype == 'GroupRecord':
            struct_lines = []
            struct_lines.append(f"ModelRoData_GroupRecord GroupRecord_0x{node.data_offset:03x} = ")
            struct_lines.append("{")
            ox, oy, oz = node.data['origin_x'], node.data['origin_y'], node.data['origin_z']
            struct_lines.append(f"    {{{ox}, {oy}, {oz}}}, //origin {{x,y,z}}")
            struct_lines.append(f"    0x{node.data['joint_id']:X}, //JointID")
            mid0 = node.data['matrix_id0']
            mid1 = node.data['matrix_id1']
            mid2 = node.data['matrix_id2']
            struct_lines.append(f"    0x{mid0 & 0xFFFF:X}, //MatrixID0")
            struct_lines.append(f"    0x{mid1 & 0xFFFF:X}, //MatrixID1")
            struct_lines.append(f"    0x{mid2 & 0xFFFF:X}, //MatrixID2")
            # ChildGroup pointer (points to ModelNode, not directly to GroupRecord)
            child_offset = node.data.get('child_group_offset', -1)
            if child_offset > 0:
                struct_lines.append(f"    (struct ModelRoData_GroupRecord *)&ModelNode_0x{child_offset:03x}, //ChildGroup")
            else:
                struct_lines.append(f"    NULL, //ChildGroup")
            struct_lines.append(f"    {node.data['bounding_volume_radius']} //BoundingVolumeRadius")
            struct_lines.append("};")
            all_structures.append((node.data_offset, '\n'.join(struct_lines)))

        # BoundingBoxRecord structures
        elif dtype == 'BoundingBoxRecord':
            struct_lines = []
            struct_lines.append(f"ModelRoData_BoundingBoxRecord BoundingBoxRecord_0x{node.data_offset:03x} = ")
            struct_lines.append("{")
            struct_lines.append(f"    0x{node.data['model_number']:X},")
            xmin, xmax = node.data['xmin'], node.data['xmax']
            ymin, ymax = node.data['ymin'], node.data['ymax']
            zmin, zmax = node.data['zmin'], node.data['zmax']
            struct_lines.append(f"    {{{xmin}, {xmax}, {ymin}, {ymax}, {zmin}, {zmax}}}")
            struct_lines.append("};")
            all_structures.append((node.data_offset, '\n'.join(struct_lines)))

            # Note: Padding is handled by gap detection, not added here

        # LODRecord structures
        elif dtype == 'LODRecord':
            struct_lines = []
            struct_lines.append(f"ModelRoData_LODRecord LODRecord_0x{node.data_offset:03x} = ")
            struct_lines.append("{")
            struct_lines.append(f"    {node.data['min_distance']}f, //MinDistance")
            struct_lines.append(f"    {node.data['max_distance']}f, //MaxDistance")
            # The Affects field points to the child1 node
            child1_ptr = f"&ModelNode_0x{node.child_offset:03x}" if node.child_offset > 0 else "NULL"
            struct_lines.append(f"    {child1_ptr}, //Affects (child node)")
            rw_idx = node.data.get('rw_data_index', 0)
            res = node.data.get('reserved', 0)
            struct_lines.append(f"    0x{rw_idx:X}, 0x{res:X} //RwDataIndex, reserved")
            struct_lines.append("};")
            all_structures.append((node.data_offset, '\n'.join(struct_lines)))
            # Note: Padding is detected and added separately

        # BSPRecord structures
        elif dtype == 'BSPRecord':
            struct_lines = []
            struct_lines.append(f"ModelRoData_BSPRecord BSPRecord_0x{node.data_offset:03x} = ")
            struct_lines.append("{")
            px, py, pz = node.data['point_x'], node.data['point_y'], node.data['point_z']
            vx, vy, vz = node.data['vector_x'], node.data['vector_y'], node.data['vector_z']
            struct_lines.append(f"    {{{px}, {py}, {pz}}}, //Point")
            struct_lines.append(f"    {{{vx}, {vy}, {vz}}}, //Vector")
            # Left and right children point to ModelNodes
            left_offset = node.data.get('left_child_offset', -1)
            right_offset = node.data.get('right_child_offset', -1)
            left_ptr = f"&ModelNode_0x{left_offset:03x}" if left_offset > 0 else "NULL"
            right_ptr = f"&ModelNode_0x{right_offset:03x}" if right_offset > 0 else "NULL"
            struct_lines.append(f"    {left_ptr}, //leftChild")
            struct_lines.append(f"    {right_ptr}, //rightChild")
            struct_lines.append(f"    0x{node.data['reserved']:X}, 0x{node.data['rw_data_index']:X} //reserved, RwDataIndex")
            struct_lines.append("};")
            all_structures.append((node.data_offset, '\n'.join(struct_lines)))

        # SwitchRecord structures
        elif dtype == 'SwitchRecord':
            struct_lines = []
            struct_lines.append(f"ModelRoData_SwitchRecord SwitchRecord_0x{node.data_offset:03x} = ")
            struct_lines.append("{")
            controls_offset = node.data.get('controls_offset', -1)
            controls_ptr = f"&ModelNode_0x{controls_offset:03x}" if controls_offset > 0 else "NULL"
            struct_lines.append(f"    {controls_ptr}, //Controls")
            struct_lines.append(f"    0x{node.data['rw_data_index']:X}, 0x{node.data['reserved']:X} //RwDataIndex, reserved")
            struct_lines.append("};")
            all_structures.append((node.data_offset, '\n'.join(struct_lines)))

        # InterlinkageRecord structures (28 bytes)
        elif dtype == 'InterlinkageRecord':
            struct_lines = []
            struct_lines.append(f"ModelRoData_InterlinkageRecord InterlinkageRecord_0x{node.data_offset:03x} = ")
            struct_lines.append("{")
            px, py, pz = node.data['pos_x'], node.data['pos_y'], node.data['pos_z']
            struct_lines.append(f"    {{{px}, {py}, {pz}}}, //pos")
            struct_lines.append(f"    0x{node.data['unknown1']:08X}, //unknown1")
            struct_lines.append(f"    0x{node.data['unknown2']:08X}, //unknown2")
            struct_lines.append(f"    0x{node.data['unknown3']:08X}, //unknown3")
            struct_lines.append(f"    {node.data['scale']} //Scale")
            struct_lines.append("};")
            all_structures.append((node.data_offset, '\n'.join(struct_lines)))

        # GroupSimpleRecord structures (20 bytes)
        elif dtype == 'GroupSimpleRecord':
            struct_lines = []
            struct_lines.append(f"ModelRoData_GroupSimpleRecord GroupSimpleRecord_0x{node.data_offset:03x} = ")
            struct_lines.append("{")
            ox, oy, oz = node.data['origin_x'], node.data['origin_y'], node.data['origin_z']
            struct_lines.append(f"    {{{ox}, {oy}, {oz}}}, //Origin")
            struct_lines.append(f"    0x{node.data['group1']:X}, 0x{node.data['group2']:X}, //Group1, Group2")
            struct_lines.append(f"    {node.data['bounding_volume_radius']} //BoundingVolumeRadius")
            struct_lines.append("};")
            all_structures.append((node.data_offset, '\n'.join(struct_lines)))

        # HeaderRecord structures (16 bytes)
        elif dtype == 'HeaderRecord':
            struct_lines = []
            struct_lines.append(f"ModelRoData_HeaderRecord HeaderRecord_0x{node.data_offset:03x} = ")
            struct_lines.append("{")
            struct_lines.append(f"    0x{node.data['anim_part']:X}, //AnimPart")
            struct_lines.append(f"    {node.data['matrix_index']}, //MatrixIndex")
            # FirstGroup points to the first child node in the header subtree.
            first_group_offset = node.data.get('first_group_offset', 0)
            group_ptr = f"&ModelNode_0x{first_group_offset:03x}" if first_group_offset > 0 else "NULL"
            struct_lines.append(f"    {group_ptr}, //FirstGroup")
            struct_lines.append(f"    0x{node.data['group1']:X}, 0x{node.data['group2']:X}, //Group1, Group2")
            struct_lines.append(f"    0x{node.data['rw_data_index']:X}, 0x{node.data['reserved']:X} //RwDataIndex, reserved")
            struct_lines.append("};")
            all_structures.append((node.data_offset, '\n'.join(struct_lines)))

        # HeadPlaceholderRecord structures (4 bytes)
        elif dtype == 'HeadPlaceholderRecord':
            struct_lines = []
            struct_lines.append(f"ModelRoData_HeadPlaceholderRecord HeadPlaceholderRecord_0x{node.data_offset:03x} = ")
            struct_lines.append("{")
            struct_lines.append(f"    0x{node.data['rw_data_index']:X} //RwDataIndex")
            struct_lines.append("};")
            all_structures.append((node.data_offset, '\n'.join(struct_lines)))

        # ShadowRecord structures (32 bytes)
        elif dtype == 'ShadowRecord':
            data = node.data
            struct_lines = []
            struct_lines.append(f"ModelRoData_ShadowRecord ShadowRecord_0x{node.data_offset:03x} = ")
            struct_lines.append("{")
            struct_lines.append(f"    {{{float_to_c(data['pos_x'])}, {float_to_c(data['pos_y'])}}}, //pos")
            struct_lines.append(f"    {{{float_to_c(data['size_x'])}, {float_to_c(data['size_y'])}}}, //size")

            # Image pointer
            if data['image_offset'] > 0:
                struct_lines.append(f"    (void *)(0x05000000 + 0x{data['image_offset']:03x}), //image")
            else:
                struct_lines.append("    NULL, //image")

            # Header pointer - points to a ModelNode with HEADER opcode usually
            if data['header_offset'] > 0:
                struct_lines.append(f"    (struct ModelRoData_HeaderRecord *)(0x05000000 + 0x{data['header_offset']:03x}), //Header")
            else:
                struct_lines.append("    NULL, //Header")

            struct_lines.append(f"    {float_to_c(data['scale'])}, //Scale")
            struct_lines.append(f"    (void *)0x{data['base_addr']:X} //BaseAddr")
            struct_lines.append("};")
            all_structures.append((node.data_offset, '\n'.join(struct_lines)))

        # GunfireRecord structures (40 bytes)
        elif dtype == 'GunfireRecord':
            data = node.data
            struct_lines = []
            struct_lines.append(f"ModelRoData_GunfireRecord GunfireRecord_0x{node.data_offset:03x} = ")
            struct_lines.append("{")
            # Offset coord3d
            struct_lines.append("    {")
            struct_lines.append(f"        {float_to_c(data['offset_x'])}, //{data['offset_x']:.6f}")
            struct_lines.append(f"        {float_to_c(data['offset_y'])}, //{data['offset_y']:.6f}")
            struct_lines.append(f"        {float_to_c(data['offset_z'])}, //{data['offset_z']:.6f}")
            struct_lines.append("    },")
            # Size coord3d
            struct_lines.append("    {")
            struct_lines.append(f"        {float_to_c(data['size_x'])}, //{data['size_x']:.6f}")
            struct_lines.append(f"        {float_to_c(data['size_y'])}, //{data['size_y']:.6f}")
            struct_lines.append(f"        {float_to_c(data['size_z'])}, //{data['size_z']:.6f}")
            struct_lines.append("    },")
            # Image pointer (NULL for now, or reference texture)
            if data.get('image_offset', 0) > 0:
                struct_lines.append(f"    (void *)(0x05000000 + 0x{data['image_offset']:03x}), //Image")
            else:
                struct_lines.append("    NULL, //Image")
            struct_lines.append(f"    {float_to_c(data['scale'])}, //{data['scale']:.6f}")
            struct_lines.append(f"    0x{data['rw_data_index']:X}, //RwDataIndex")
            struct_lines.append(f"    0x{data['reserved']:X}, //reserved")
            struct_lines.append(f"    0x{data['base_addr']:08X} //BaseAddr")
            struct_lines.append("};")
            all_structures.append((node.data_offset, '\n'.join(struct_lines)))

        # DisplayListPrimaryRecord structures (16 bytes) with sub-arrays
        elif dtype == 'DisplayListPrimaryRecord':
            data = node.data
            dl_offset = node.data_offset

            # Extract file offsets
            vertices_offset = data.get('vertices_offset', 0)
            primary_offset = data['primary_offset']

            vtx_name = None
            prim_name = None

            # Vertex array
            if data.get('vertices') and vertices_offset > 0:
                vtx_name = f"Vertex_0x{vertices_offset:03x}"
                vtx_index = node_to_vtx_index.get(node_offset, 0)
                node_to_vtx_name[node_offset] = (vtx_name, vtx_index)

                struct_lines = []
                struct_lines.append(f"Vertex {vtx_name}[VERTEXGROUPCOUNT{vtx_index}] = ")
                struct_lines.append("{ //{ {   x,    y,   z}, index,     s,     t,    r,    g,    b,    a }")
                for v in data['vertices']:
                    s_str = f"0x{v.s:X}" if v.s >= 0 else f"{v.s}"
                    t_str = f"0x{v.t:X}" if v.t >= 0 else f"{v.t}"
                    struct_lines.append(f"    {{ {{ {v.x:4d}, {v.y:4d}, {v.z:4d}}}, 0x{v.flag:X}, {s_str:>6}, {t_str:>6}, 0x{v.r:02X}, 0x{v.g:02X}, 0x{v.b:02X}, 0x{v.a:02X} }},")
                struct_lines.append("};")
                all_structures.append((vertices_offset, '\n'.join(struct_lines)))

            # Primary Gfx array
            if data['primary_gfx'] and primary_offset > 0:
                prim_name = f"GDL_0x{primary_offset:03x}"
                node_to_gdl_prim_name[node_offset] = prim_name

                struct_lines = []
                struct_lines.append(f"Gfx {prim_name}[] = ")
                struct_lines.append("{")
                for cmd in data['primary_gfx']:
                    struct_lines.append(f"    {cmd},")
                struct_lines.append("};")
                all_structures.append((primary_offset, '\n'.join(struct_lines)))

            # DLPrimary record itself
            struct_lines = []
            struct_lines.append(f"ModelRoData_DisplayListPrimaryRecord DLPrimaryRecord_0x{dl_offset:03x} = ")
            struct_lines.append("{")
            struct_lines.append(f"    {data['num_vertices']}, //numVertices")
            if vtx_name:
                struct_lines.append(f"    {vtx_name}, //Vertices")
            else:
                struct_lines.append(f"    NULL, //Vertices")
            if prim_name:
                struct_lines.append(f"    {prim_name}, //Primary")
            else:
                struct_lines.append(f"    NULL, //Primary")
            struct_lines.append(f"    (void *)0x{data['base_addr']:08X} //BaseAddr")
            struct_lines.append("};")
            all_structures.append((dl_offset, '\n'.join(struct_lines)))

        # DisplayListRecord structures (19 bytes) with sub-arrays
        elif dtype == 'DisplayListRecord':
            data = node.data
            dl_offset = node.data_offset

            # Extract file offsets for all sub-structures
            vertices_offset = data.get('vertices_offset', 0)
            primary_offset = data['primary_offset']
            secondary_offset = data['secondary_offset']

            # Assign unique symbolic names
            vtx_name = None
            prim_name = None
            sec_name = None

            # Vertex array
            if data.get('vertices') and vertices_offset > 0:
                vtx_name = f"Vertex_0x{vertices_offset:03x}"
                vtx_index = node_to_vtx_index.get(node_offset, 0)
                node_to_vtx_name[node_offset] = (vtx_name, vtx_index)

                struct_lines = []
                struct_lines.append(f"Vertex {vtx_name}[VERTEXGROUPCOUNT{vtx_index}] = ")
                struct_lines.append("{ //{ {   x,    y,   z}, index,     s,     t,    r,    g,    b,    a }")
                for v in data['vertices']:
                    s_str = f"0x{v.s:X}" if v.s >= 0 else f"{v.s}"
                    t_str = f"0x{v.t:X}" if v.t >= 0 else f"{v.t}"
                    struct_lines.append(f"    {{ {{ {v.x:4d}, {v.y:4d}, {v.z:4d}}}, 0x{v.flag:X}, {s_str:>6}, {t_str:>6}, 0x{v.r:02X}, 0x{v.g:02X}, 0x{v.b:02X}, 0x{v.a:02X} }},")
                struct_lines.append("};")
                all_structures.append((vertices_offset, '\n'.join(struct_lines)))

            # Primary GFX array
            if data.get('primary_gfx') and primary_offset > 0:
                prim_name = f"GFX_PRIMARY_0x{primary_offset:03x}"
                node_to_gdl_prim_name[node_offset] = prim_name
                gdl_prim_idx += 1

                struct_lines = []
                struct_lines.append(f"Gfx {prim_name}[] = ")
                struct_lines.append("{")
                for cmd_str in data['primary_gfx']:
                    struct_lines.append(f"    {cmd_str},")
                struct_lines.append("};")
                all_structures.append((primary_offset, '\n'.join(struct_lines)))

            # Secondary GFX array
            if data.get('secondary_gfx') and secondary_offset > 0:
                sec_name = f"GFX_SECONDARY_0x{secondary_offset:03x}"
                node_to_gdl_sec_name[node_offset] = sec_name
                gdl_sec_idx += 1

                struct_lines = []
                struct_lines.append(f"Gfx {sec_name}[] = ")
                struct_lines.append("{")
                for cmd_str in data['secondary_gfx']:
                    struct_lines.append(f"    {cmd_str},")
                struct_lines.append("};")
                all_structures.append((secondary_offset, '\n'.join(struct_lines)))

            # Now the DisplayListRecord itself
            struct_lines = []
            struct_lines.append(f"ModelRoData_DisplayListRecord DisplayListRecord_0x{dl_offset:03x} = ")
            struct_lines.append("{")

            primary_ref = f"&{node_to_gdl_prim_name[node_offset]}" if node_offset in node_to_gdl_prim_name else "NULL"
            secondary_ref = f"&{node_to_gdl_sec_name[node_offset]}" if node_offset in node_to_gdl_sec_name else "NULL"

            struct_lines.append(f"    {primary_ref}, //PrimaryDisplayList")
            struct_lines.append(f"    {secondary_ref}, //SecondaryDisplayList")

            # BaseAddr pointer (raw value from binary)
            base_addr = data.get('base_addr', 0)
            struct_lines.append(f"    (void *)0x{base_addr:08X}, //BaseAddr")

            # Vertices pointer
            if node_offset in node_to_vtx_name:
                vtx_ref = f"&{node_to_vtx_name[node_offset][0]}"
            elif data.get('vertices_offset', -1) >= 0:
                # Vertices pointer exists but no vertices were parsed (num_vertices=0)
                vtx_ref = f"(Vtx *)0x{BASE_ADDRESS + data['vertices_offset']:08X}"
            else:
                vtx_ref = "NULL"
            struct_lines.append(f"    {vtx_ref}, //Vertices")
            struct_lines.append(f"    0x{data['num_vertices']:X}, //NumVertices")
            struct_lines.append(f"    {data['model_type']} //ModelType")
            struct_lines.append("};")
            all_structures.append((dl_offset, '\n'.join(struct_lines)))

        # DisplayListCollisionRecord and all its sub-structures
        elif dtype == 'DisplayListCollisionRecord':
            data = node.data

            # DLCollisionRecord itself (32 bytes at node.data_offset)
            dlcoll_offset = node.data_offset

            # Extract file offsets for all sub-structures
            vertices_offset = data.get('vertices_offset', 0)
            collision_vertices_offset = data.get('collision_vertices_offset', 0)
            point_usage_offset = data.get('point_usage_offset', 0)
            primary_offset = data['primary_offset']
            secondary_offset = data['secondary_offset']

            # Assign unique symbolic names for this node's arrays
            vtx_name = None
            colvtx_name = None
            ptusage_name = None
            prim_name = None
            sec_name = None

            # Vertex array
            if data.get('vertices') and vertices_offset > 0:
                vtx_name = f"Vertex_0x{vertices_offset:03x}"
                vtx_index = node_to_vtx_index.get(node_offset, 0)
                node_to_vtx_name[node_offset] = (vtx_name, vtx_index)

                struct_lines = []
                struct_lines.append(f"Vertex {vtx_name}[VERTEXGROUPCOUNT{vtx_index}] = ")
                struct_lines.append("{ //{ {   x,    y,   z}, index,     s,     t,    r,    g,    b,    a }")
                for v in data['vertices']:
                    s_str = f"0x{v.s:X}" if v.s >= 0 else f"{v.s}"
                    t_str = f"0x{v.t:X}" if v.t >= 0 else f"{v.t}"
                    struct_lines.append(f"    {{ {{ {v.x:4d}, {v.y:4d}, {v.z:4d}}}, 0x{v.flag:X}, {s_str:>6}, {t_str:>6}, 0x{v.r:02X}, 0x{v.g:02X}, 0x{v.b:02X}, 0x{v.a:02X} }},")
                struct_lines.append("};")
                all_structures.append((vertices_offset, '\n'.join(struct_lines)))

            # Collision vertex array
            if data['collision_vertices'] and collision_vertices_offset > 0:
                colvtx_name = f"Collision_Vertex_0x{collision_vertices_offset:03x}"
                colvtx_index = node_to_colvtx_index.get(node_offset, 0)
                node_to_colvtx_name[node_offset] = (colvtx_name, colvtx_index)

                struct_lines = []
                struct_lines.append(f"Vertex {colvtx_name}[COLLISIONVERTEXCOUNT{colvtx_index}] = ")
                struct_lines.append("{ //{ {   x,    y,   z}, index,     s,     t,    r,    g,    b,    a }")
                for v in data['collision_vertices']:
                    s_str = f"0x{v.s:X}" if v.s >= 0 else f"{v.s}"
                    t_str = f"0x{v.t:X}" if v.t >= 0 else f"{v.t}"
                    struct_lines.append(f"    {{ {{ {v.x:4d}, {v.y:4d}, {v.z:4d}}}, 0x{v.flag:X}, {s_str:>6}, {t_str:>6}, 0x{v.r:02X}, 0x{v.g:02X}, 0x{v.b:02X}, 0x{v.a:02X} }},")
                struct_lines.append("};")
                all_structures.append((collision_vertices_offset, '\n'.join(struct_lines)))

            # Point usage array
            if data['point_usage'] and point_usage_offset > 0:
                ptusage_name = f"POINT_USAGE_0x{point_usage_offset:03x}"
                node_to_ptusage_name[node_offset] = (ptusage_name, ptusage_idx)
                ptusage_idx += 1

                struct_lines = []
                struct_lines.append(f"s16 {ptusage_name}[VERTEXGROUPCOUNT{node_to_vtx_name[node_offset][1]}] = ")
                struct_lines.append("{")
                for i in range(0, len(data['point_usage']), 4):
                    chunk = data['point_usage'][i:i+4]
                    formatted = ', '.join(f"0x{v:04X}" if v >= 0 else f"{v}" for v in chunk)
                    struct_lines.append(f"    {formatted},")
                struct_lines.append("};")
                all_structures.append((point_usage_offset, '\n'.join(struct_lines)))

                # Note: Padding is detected via gap analysis, not added explicitly here

            # Primary GFX array
            if data.get('primary_gfx') and primary_offset > 0:
                prim_name = f"GFX_PRIMARY_0x{primary_offset:03x}"
                node_to_gdl_prim_name[node_offset] = prim_name
                gdl_prim_idx += 1

                struct_lines = []
                struct_lines.append(f"Gfx {prim_name}[] = ")
                struct_lines.append("{")
                for cmd_str in data['primary_gfx']:
                    struct_lines.append(f"    {cmd_str},")
                struct_lines.append("};")
                all_structures.append((primary_offset, '\n'.join(struct_lines)))

            # Secondary GFX array
            if data.get('secondary_gfx') and secondary_offset > 0:
                sec_name = f"GFX_SECONDARY_0x{secondary_offset:03x}"
                node_to_gdl_sec_name[node_offset] = sec_name
                gdl_sec_idx += 1

                struct_lines = []
                struct_lines.append(f"Gfx {sec_name}[] = ")
                struct_lines.append("{")
                for cmd_str in data['secondary_gfx']:
                    struct_lines.append(f"    {cmd_str},")
                struct_lines.append("};")
                all_structures.append((secondary_offset, '\n'.join(struct_lines)))

            # Now the DLCollisionRecord itself, which references the arrays above
            struct_lines = []
            struct_lines.append(f"ModelRoData_DisplayList_CollisionRecord DLCollisionRecord_0x{dlcoll_offset:03x} = ")
            struct_lines.append("{")

            primary_ref = node_to_gdl_prim_name.get(node_offset, "NULL")
            secondary_ref = node_to_gdl_sec_name.get(node_offset, "NULL")

            if node_offset in node_to_vtx_name:
                vtx_ref_name = node_to_vtx_name[node_offset][0]
                vtx_count_ref = f"VERTEXGROUPCOUNT{node_to_vtx_name[node_offset][1]}"
            else:
                vtx_ref_name = "NULL"
                vtx_count_ref = "0"

            if node_offset in node_to_colvtx_name:
                colvtx_ref_name = node_to_colvtx_name[node_offset][0]
                colvtx_count_ref = f"COLLISIONVERTEXCOUNT{node_to_colvtx_name[node_offset][1]}"
            else:
                colvtx_ref_name = "NULL"
                colvtx_count_ref = "0"

            ptusage_ref = node_to_ptusage_name.get(node_offset, ("NULL", -1))[0]

            struct_lines.append(f"    {primary_ref}, //primary")
            struct_lines.append(f"    {secondary_ref}, //secondary")
            struct_lines.append(f"    {vtx_ref_name}, {vtx_count_ref}, //vertices,vcount")
            struct_lines.append(f"    {colvtx_count_ref}, {colvtx_ref_name}, //ncolvtx,collision vertices")
            struct_lines.append(f"    {ptusage_ref}, //point usage")
            struct_lines.append(f"    0x{data['model_type']:X}, 0x{data['rw_data_index']:X}, //type, index")
            struct_lines.append(f"    0x0 //baseaddr")
            struct_lines.append("};")
            all_structures.append((dlcoll_offset, '\n'.join(struct_lines)))

    # Step 2.1: Process referenced_data structures (not in ModelNode tree)
    for ref_offset in sorted(referenced_data.keys()):
        ref_data = referenced_data[ref_offset]
        dtype = ref_data.get('_type')

        # Only handle GroupRecord for now (main use case)
        if dtype == 'GroupRecord':
            struct_lines = []
            struct_lines.append(f"ModelRoData_GroupRecord GroupRecord_0x{ref_offset:03x} = ")
            struct_lines.append("{")
            ox, oy, oz = ref_data['origin_x'], ref_data['origin_y'], ref_data['origin_z']
            struct_lines.append(f"    {{{ox}, {oy}, {oz}}}, //origin {{x,y,z}}")
            struct_lines.append(f"    0x{ref_data['joint_id']:X}, //JointID")
            mid0 = ref_data['matrix_id0']
            mid1 = ref_data['matrix_id1']
            mid2 = ref_data['matrix_id2']
            struct_lines.append(f"    0x{mid0 & 0xFFFF:X}, //MatrixID0")
            struct_lines.append(f"    0x{mid1 & 0xFFFF:X}, //MatrixID1")
            struct_lines.append(f"    0x{mid2 & 0xFFFF:X}, //MatrixID2")
            # ChildGroup points to a ModelNode
            child_offset = ref_data.get('child_group_offset', -1)
            if child_offset > 0:
                struct_lines.append(f"    (struct ModelRoData_GroupRecord *)&ModelNode_0x{child_offset:03x}, //ChildGroup")
            else:
                struct_lines.append(f"    NULL, //ChildGroup")
            struct_lines.append(f"    {ref_data['bounding_volume_radius']} //BoundingVolumeRadius")
            struct_lines.append("};")
            all_structures.append((ref_offset, '\n'.join(struct_lines)))

    # Step 2.5: Detect padding by finding gaps between sorted structures
    # Mark all structure bytes, find gaps
    if binary_data:
        binary_size = len(binary_data)
        byte_map = [False] * binary_size

        # Mark switches (handled separately in Switches.c, not padding)
        switch_size = len(switches) * 4 if switches else 0
        for i in range(switch_size):
            byte_map[i] = True

        # Mark textures
        tex_end = switch_size + len(textures) * 12
        for i in range(switch_size, tex_end):
            byte_map[i] = True

        # Mark nodes
        nodes_start = tex_end
        nodes_end = nodes_start + len(nodes) * 24
        for i in range(nodes_start, nodes_end):
            byte_map[i] = True

        # Mark all structures by their offsets in all_structures list
        for offset, _ in all_structures:
            # Determine size based on what's at this offset
            for node_offset in sorted(nodes.keys()):
                node = nodes[node_offset]
                if not node.data:
                    continue
                dtype = node.data.get('_type')
                data = node.data

                # GroupRecord or BoundingBoxRecord
                if offset == node.data_offset and dtype in ('GroupRecord', 'BoundingBoxRecord'):
                    for i in range(offset, offset + 28):
                        byte_map[i] = True

                # LODRecord (16 bytes)
                if offset == node.data_offset and dtype == 'LODRecord':
                    for i in range(offset, offset + 16):
                        byte_map[i] = True

                # BSPRecord (36 bytes)
                if offset == node.data_offset and dtype == 'BSPRecord':
                    for i in range(offset, offset + 36):
                        byte_map[i] = True

                # SwitchRecord (8 bytes)
                if offset == node.data_offset and dtype == 'SwitchRecord':
                    for i in range(offset, offset + 8):
                        byte_map[i] = True

                # GroupSimpleRecord (20 bytes)
                if offset == node.data_offset and dtype == 'GroupSimpleRecord':
                    for i in range(offset, offset + 20):
                        byte_map[i] = True

                # HeaderRecord (16 bytes)
                if offset == node.data_offset and dtype == 'HeaderRecord':
                    for i in range(offset, offset + 16):
                        byte_map[i] = True

                # DisplayListRecord (19 bytes logical, 20 bytes with compiler padding)
                if offset == node.data_offset and dtype == 'DisplayListRecord':
                    for i in range(offset, offset + 20):
                        byte_map[i] = True

                # GunfireRecord (40 bytes)
                if offset == node.data_offset and dtype == 'GunfireRecord':
                    for i in range(offset, offset + 40):
                        byte_map[i] = True

                # ShadowRecord (32 bytes)
                if offset == node.data_offset and dtype == 'ShadowRecord':
                    for i in range(offset, offset + 32):
                        byte_map[i] = True

                # HeadPlaceholderRecord (4 bytes)
                if offset == node.data_offset and dtype == 'HeadPlaceholderRecord':
                    for i in range(offset, offset + 4):
                        byte_map[i] = True

                # InterlinkageRecord (28 bytes)
                if offset == node.data_offset and dtype == 'InterlinkageRecord':
                    for i in range(offset, offset + 28):
                        byte_map[i] = True

                # DisplayListPrimaryRecord (16 bytes)
                if offset == node.data_offset and dtype == 'DisplayListPrimaryRecord':
                    for i in range(offset, offset + 16):
                        byte_map[i] = True

                # DLCollisionRecord (32 bytes)
                if offset == node.data_offset and dtype == 'DisplayListCollisionRecord':
                    for i in range(offset, offset + 32):
                        byte_map[i] = True

                # Vertices
                if data.get('vertices_offset') and offset == data['vertices_offset']:
                    vsize = len(data['vertices']) * 16
                    for i in range(offset, offset + vsize):
                        byte_map[i] = True

                # Collision vertices
                if data.get('collision_vertices_offset') and offset == data['collision_vertices_offset']:
                    csize = len(data['collision_vertices']) * 16
                    for i in range(offset, offset + csize):
                        byte_map[i] = True

                # Point usage
                if data.get('point_usage_offset') and offset == data['point_usage_offset']:
                    psize = len(data['point_usage']) * 2
                    for i in range(offset, offset + psize):
                        byte_map[i] = True

                # GFX arrays
                if data.get('primary_offset') and offset == data['primary_offset']:
                    gsize = len(data['primary_gfx']) * 8
                    for i in range(offset, offset + gsize):
                        byte_map[i] = True

                if data.get('secondary_offset') and offset == data['secondary_offset']:
                    gsize = len(data['secondary_gfx']) * 8
                    for i in range(offset, offset + gsize):
                        byte_map[i] = True

        # Find last real structure (handle empty case)
        if all_structures:
            last_offset = max(off for off, _ in all_structures)
        else:
            last_offset = binary_size - 1

        # Calculate switch size to skip that region (switches handled separately in Switches.c)
        switch_size = len(switches) * 4 if switches else 0

        # Find padding gaps (up to last structure only, SKIP switch region at start)
        # Don't scan past last_offset + 256 bytes to avoid trailing data
        scan_limit = min(last_offset + 256, binary_size)

        i = switch_size  # Start AFTER switches
        while i < scan_limit:
            if not byte_map[i]:
                pad_start = i
                while i < binary_size and not byte_map[i]:
                    i += 1
                pad_end = i
                pad_size = pad_end - pad_start

                # ONLY add padding for 4+ byte gaps that are BETWEEN structures (not at the end)
                # 2-byte gaps are natural compiler alignment - don't add variables for them
                # Skip padding that extends to near the end of the file (trailing data)
                if pad_size >= 4 and pad_end < scan_limit - 32:
                    # Read padding bytes
                    pad_bytes = binary_data[pad_start:pad_end]

                    # Generate padding as u32 array for proper alignment
                    if pad_size == 4:
                        val = struct.unpack('>I', pad_bytes)[0]
                        all_structures.append((pad_start, f"u32 PADDING_0x{pad_start:03x} = 0x{val:08X};"))
                    elif pad_size % 4 == 0:
                        values = [struct.unpack('>I', pad_bytes[j:j+4])[0] for j in range(0, pad_size, 4)]
                        vals_str = ', '.join(f"0x{v:08X}" for v in values)
                        all_structures.append((pad_start, f"u32 PADDING_0x{pad_start:03x}[{pad_size // 4}] = {{{vals_str}}};"))
                    else:
                        # Odd size - use byte array
                        vals_str = ', '.join(f"0x{b:02X}" for b in pad_bytes)
                        all_structures.append((pad_start, f"u8 PADDING_0x{pad_start:03x}[{pad_size}] = {{{vals_str}}};"))
            else:
                i += 1

    # Step 2.5.1: REMOVED - Trailing padding generation
    # Trailing padding after arrays is compiler-generated for alignment, not source data
    # The compiler automatically adds padding between structures as needed
    # DO NOT generate PADDING_TRAILING variables - they cause incorrect binary sizes

    # Step 2.6: Generate forward declarations with correct names
    forward_decl_lines = []

    # Declare referenced data structures (not in ModelNode tree)
    for ref_offset in sorted(referenced_data.keys()):
        ref_data = referenced_data[ref_offset]
        dtype = ref_data.get('_type')
        if dtype == 'GroupRecord':
            forward_decl_lines.append(f"extern ModelRoData_GroupRecord GroupRecord_0x{ref_offset:03x};")

    # Declare all data structures
    for node_offset in sorted(nodes.keys()):
        node = nodes[node_offset]
        if node.data:
            dtype = node.data.get('_type')
            if dtype == 'GroupRecord':
                forward_decl_lines.append(f"extern ModelRoData_GroupRecord GroupRecord_0x{node.data_offset:03x};")
            elif dtype == 'BoundingBoxRecord':
                forward_decl_lines.append(f"extern ModelRoData_BoundingBoxRecord BoundingBoxRecord_0x{node.data_offset:03x};")
            elif dtype == 'LODRecord':
                forward_decl_lines.append(f"extern ModelRoData_LODRecord LODRecord_0x{node.data_offset:03x};")
            elif dtype == 'BSPRecord':
                forward_decl_lines.append(f"extern ModelRoData_BSPRecord BSPRecord_0x{node.data_offset:03x};")
            elif dtype == 'SwitchRecord':
                forward_decl_lines.append(f"extern ModelRoData_SwitchRecord SwitchRecord_0x{node.data_offset:03x};")
            elif dtype == 'InterlinkageRecord':
                forward_decl_lines.append(f"extern ModelRoData_InterlinkageRecord InterlinkageRecord_0x{node.data_offset:03x};")
            elif dtype == 'GroupSimpleRecord':
                forward_decl_lines.append(f"extern ModelRoData_GroupSimpleRecord GroupSimpleRecord_0x{node.data_offset:03x};")
            elif dtype == 'HeaderRecord':
                forward_decl_lines.append(f"extern ModelRoData_HeaderRecord HeaderRecord_0x{node.data_offset:03x};")
            elif dtype == 'HeadPlaceholderRecord':
                forward_decl_lines.append(f"extern ModelRoData_HeadPlaceholderRecord HeadPlaceholderRecord_0x{node.data_offset:03x};")
            elif dtype == 'ShadowRecord':
                forward_decl_lines.append(f"extern ModelRoData_ShadowRecord ShadowRecord_0x{node.data_offset:03x};")
            elif dtype == 'GunfireRecord':
                forward_decl_lines.append(f"extern ModelRoData_GunfireRecord GunfireRecord_0x{node.data_offset:03x};")
            elif dtype == 'DisplayListPrimaryRecord':
                forward_decl_lines.append(f"extern ModelRoData_DisplayListPrimaryRecord DLPrimaryRecord_0x{node.data_offset:03x};")

                # Declare arrays for DisplayListPrimaryRecord
                if node_offset in node_to_vtx_name:
                    vtx_name, vtx_idx = node_to_vtx_name[node_offset]
                    forward_decl_lines.append(f"extern Vertex {vtx_name}[VERTEXGROUPCOUNT{vtx_idx}];")

                if node_offset in node_to_gdl_prim_name:
                    prim_name = node_to_gdl_prim_name[node_offset]
                    forward_decl_lines.append(f"extern Gfx {prim_name}[];")
            elif dtype == 'DisplayListRecord':
                forward_decl_lines.append(f"extern ModelRoData_DisplayListRecord DisplayListRecord_0x{node.data_offset:03x};")

                # Declare arrays for DisplayListRecord
                if node_offset in node_to_vtx_name:
                    vtx_name, vtx_idx = node_to_vtx_name[node_offset]
                    forward_decl_lines.append(f"extern Vertex {vtx_name}[VERTEXGROUPCOUNT{vtx_idx}];")

                if node_offset in node_to_gdl_prim_name:
                    prim_name = node_to_gdl_prim_name[node_offset]
                    forward_decl_lines.append(f"extern Gfx {prim_name}[];")

                if node_offset in node_to_gdl_sec_name:
                    sec_name = node_to_gdl_sec_name[node_offset]
                    forward_decl_lines.append(f"extern Gfx {sec_name}[];")
            elif dtype == 'DisplayListCollisionRecord':
                forward_decl_lines.append(f"extern ModelRoData_DisplayList_CollisionRecord DLCollisionRecord_0x{node.data_offset:03x};")

                # Declare arrays using the SAME names we assigned during structure collection
                if node_offset in node_to_vtx_name:
                    vtx_name, vtx_idx = node_to_vtx_name[node_offset]
                    forward_decl_lines.append(f"extern Vertex {vtx_name}[VERTEXGROUPCOUNT{vtx_idx}];")

                if node_offset in node_to_colvtx_name:
                    colvtx_name, colvtx_idx = node_to_colvtx_name[node_offset]
                    forward_decl_lines.append(f"extern Vertex {colvtx_name}[COLLISIONVERTEXCOUNT{colvtx_idx}];")

                if node_offset in node_to_ptusage_name:
                    ptusage_name, _ = node_to_ptusage_name[node_offset]
                    vtx_idx = node_to_vtx_name[node_offset][1]
                    forward_decl_lines.append(f"extern s16 {ptusage_name}[VERTEXGROUPCOUNT{vtx_idx}];")

                if node_offset in node_to_gdl_prim_name:
                    prim_name = node_to_gdl_prim_name[node_offset]
                    forward_decl_lines.append(f"extern Gfx {prim_name}[];")

                if node_offset in node_to_gdl_sec_name:
                    sec_name = node_to_gdl_sec_name[node_offset]
                    forward_decl_lines.append(f"extern Gfx {sec_name}[];")

    forward_decl_lines.append("")

    # Insert forward declarations at the placeholder position
    lines[forward_decl_placeholder_index:forward_decl_placeholder_index] = forward_decl_lines

    # Step 3: Sort ALL structures by their original binary file offset
    all_structures.sort(key=lambda x: x[0])

    # Step 4: Output structures in sorted offset order
    for offset, code in all_structures:
        lines.append(code)
        lines.append("")

    return "\n".join(lines)


def get_data_symbol_name(node: ModelNode) -> str:
    """Get the C symbol name for a node's data structure"""
    if not node.data:
        return "NULL"

    dtype = node.data.get('_type')
    if dtype == 'GroupRecord':
        return f"GroupRecord_0x{node.data_offset:03x}"
    elif dtype == 'BoundingBoxRecord':
        return f"BoundingBoxRecord_0x{node.data_offset:03x}"
    elif dtype == 'LODRecord':
        return f"LODRecord_0x{node.data_offset:03x}"
    elif dtype == 'BSPRecord':
        return f"BSPRecord_0x{node.data_offset:03x}"
    elif dtype == 'SwitchRecord':
        return f"SwitchRecord_0x{node.data_offset:03x}"
    elif dtype == 'InterlinkageRecord':
        return f"InterlinkageRecord_0x{node.data_offset:03x}"
    elif dtype == 'GroupSimpleRecord':
        return f"GroupSimpleRecord_0x{node.data_offset:03x}"
    elif dtype == 'HeaderRecord':
        return f"HeaderRecord_0x{node.data_offset:03x}"
    elif dtype == 'HeadPlaceholderRecord':
        return f"HeadPlaceholderRecord_0x{node.data_offset:03x}"
    elif dtype == 'ShadowRecord':
        return f"ShadowRecord_0x{node.data_offset:03x}"
    elif dtype == 'GunfireRecord':
        return f"GunfireRecord_0x{node.data_offset:03x}"
    elif dtype == 'DisplayListPrimaryRecord':
        return f"DLPrimaryRecord_0x{node.data_offset:03x}"
    elif dtype == 'DisplayListRecord':
        return f"DisplayListRecord_0x{node.data_offset:03x}"
    elif dtype == 'DisplayListCollisionRecord':
        return f"DLCollisionRecord_0x{node.data_offset:03x}"
    else:
        return f"Data_0x{node.data_offset:03x}"


def main():
    import argparse

    parser = argparse.ArgumentParser(description="Generate Model.c from binary prop files")
    parser.add_argument('props', nargs='*', help="Prop names to process (default: all)")
    parser.add_argument('--dry-run', action='store_true', help="Show what would be done")
    parser.add_argument('--force', action='store_true', help="Overwrite existing Model.c files")
    parser.add_argument('--cleanup', action='store_true', help="Delete source PnameZ.bin files after successful conversion")
    args = parser.parse_args()

    # Load image mapping
    image_map = load_image_map()
    print(f"Loaded {len(image_map)} image definitions")

    # Find all props
    prop_dir = Path("assets/obseg/prop")
    all_props = []
    prop_name_map = {}  # Map lowercase name to actual binary filename

    for bin_file in prop_dir.glob("P*Z.bin"):
        # Extract name between P and Z, preserve original case
        actual_name = bin_file.stem[1:-1]
        lower_name = actual_name.lower()
        all_props.append(lower_name)
        prop_name_map[lower_name] = actual_name

    props_to_process = args.props if args.props else all_props

    stats = {'processed': 0, 'skipped': 0, 'errors': 0}

    for prop_name_input in sorted(props_to_process):
        prop_name_lower = prop_name_input.lower()

        # Get the actual case from the binary file
        if prop_name_lower not in prop_name_map:
            print(f"  ✗ {prop_name_input}: Binary file not found")
            stats['errors'] += 1
            continue

        actual_prop_name = prop_name_map[prop_name_lower]
        bin_file = prop_dir / f"P{actual_prop_name}Z.bin"

        # Find the actual directory name (case-insensitive search)
        prop_subdirs = list(prop_dir.glob(f"{actual_prop_name}"))
        if not prop_subdirs:
            # Try case-insensitive
            prop_subdirs = [d for d in prop_dir.iterdir()
                           if d.is_dir() and d.name.lower() == prop_name_lower]

        if not prop_subdirs:
            # Try fuzzy match for names with suffixes like _lg
            prop_subdirs = [d for d in prop_dir.iterdir()
                           if d.is_dir() and d.name.lower().startswith(prop_name_lower)]

        if not prop_subdirs:
            print(f"  ⊘ {actual_prop_name}: Missing metadata directory")
            stats['skipped'] += 1
            continue

        actual_dir_name = prop_subdirs[0].name

        # Parse metadata using the actual directory name
        metadata = parse_metadata_files(actual_dir_name)
        if not metadata:
            print(f"  ⊘ {actual_prop_name}: Missing metadata files")
            stats['skipped'] += 1
            continue

        output_file = prop_dir / actual_dir_name / "Model.c"
        if output_file.exists() and not args.force:
            print(f"  ⊘ {actual_prop_name}: Model.c already exists (use --force)")
            stats['skipped'] += 1
            continue

        try:
            # Parse binary
            with open(bin_file, 'rb') as f:
                binary_data = f.read()

            parser = BinaryModelParser(binary_data, metadata, image_map)
            parsed_model = parser.parse()

            # Generate C code - use the directory name for output
            c_code = generate_model_c(actual_dir_name, parsed_model, metadata, image_map, binary_data)

            if args.dry_run:
                if len(parsed_model['switches']) > 0:
                    print(f"  ✓ {actual_prop_name}: Would generate Model.c ({len(parsed_model['nodes'])} nodes, {len(parsed_model['textures'])} textures, {len(parsed_model['switches'])} switches)")
                else:
                    print(f"  ✓ {actual_prop_name}: Would generate Model.c ({len(parsed_model['nodes'])} nodes, {len(parsed_model['textures'])} textures)")
            else:
                output_file.parent.mkdir(parents=True, exist_ok=True)
                with open(output_file, 'w') as f:
                    f.write(c_code)

                # Report generation
                if len(parsed_model['switches']) > 0:
                    print(f"  ✓ {actual_prop_name}: Generated Model.c ({len(parsed_model['nodes'])} nodes, {len(parsed_model['textures'])} textures, {len(parsed_model['switches'])} switches)")
                else:
                    print(f"  ✓ {actual_prop_name}: Generated Model.c ({len(parsed_model['nodes'])} nodes, {len(parsed_model['textures'])} textures)")

                # Clean up source binary file after successful conversion
                if args.cleanup and bin_file.exists():
                    bin_file.unlink()
                    print(f"    Cleaned up {bin_file.name}")

            stats['processed'] += 1

        except Exception as e:
            print(f"  ✗ {actual_prop_name}: Error - {e}")
            stats['errors'] += 1
            import traceback
            traceback.print_exc()

    print(f"\n=== Summary ===")
    print(f"Processed: {stats['processed']}")
    print(f"Skipped: {stats['skipped']}")
    print(f"Errors: {stats['errors']}")


if __name__ == "__main__":
    main()
