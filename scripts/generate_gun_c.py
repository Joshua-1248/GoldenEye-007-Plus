#!/usr/bin/env python3
"""
Gun Model Parser for GoldenEye 007

Parses N64 binary gun files (GnameZ.bin) and generates C source code (Model.c).

Binary Format (same as props):
    Offset 0x00: Switches array (if numSwitches > 0)
    Offset 0x??: Texture table (ModelFileTextures[numtextures])  
    Offset 0x??: RootNode and complete ModelNode tree with embedded data

All pointers in the binary are stored as offsets from base address 0x05000000.

Usage:
    python3 scripts/generate_gun_c.py [--dry-run] [--force] [--cleanup] [gun ...]
"""

import struct
import sys
import re
from pathlib import Path
from typing import Dict, List, Tuple, Optional, Any
from dataclasses import dataclass, field

# Add parent directory to path and import prop parser
import importlib.util
spec = importlib.util.spec_from_file_location("generate_prop_model_c", Path(__file__).parent / "generate_prop_model_c.py")
prop_module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(prop_module)

# Import needed functions and classes from prop parser
BinaryModelParser = prop_module.BinaryModelParser
load_image_map = prop_module.load_image_map
generate_model_c = prop_module.generate_model_c

def generate_gun_c(gun_name: str, parsed_model: Dict, metadata: Dict, image_map: Dict, binary_data: bytes) -> str:
    """Generate C source code for a gun model - uses prop generator with gun-specific header"""
    
    # Use the prop model generator but replace the header include
    c_code = generate_model_c(gun_name, parsed_model, metadata, image_map, binary_data)
    
    # Replace prop-specific header include with gun-specific one
    c_code = c_code.replace(
        f'#include "{gun_name}/ModelFileHeader.inc.c"',
        f'#include "{gun_name}/ModelFileHeader.inc.c"'  # Gun uses same case as prop
    )
    
    # Update the source file reference
    c_code = c_code.replace(
        f"// Source: P{gun_name}Z.bin",
        f"// Source: G{gun_name}Z.bin"
    )
    
    return c_code


def main():
    import argparse
    
    parser = argparse.ArgumentParser(description="Generate Model.c from binary gun files")
    parser.add_argument('guns', nargs='*', help="Gun names to process (default: all)")
    parser.add_argument('--dry-run', action='store_true', help="Show what would be done")
    parser.add_argument('--force', action='store_true', help="Overwrite existing Model.c files")
    parser.add_argument('--cleanup', action='store_true', help="Delete source GnameZ.bin files after successful conversion")
    args = parser.parse_args()
    
    # Load image mapping
    image_map = load_image_map()
    print(f"Loaded {len(image_map)} image definitions")
    
    # Find all guns
    gun_dir = Path("assets/obseg/gun")
    all_guns = []
    gun_name_map = {}  # lowercase -> actual case
    
    for bin_file in gun_dir.glob("G*Z.bin"):
        # Extract name between G and Z, preserve original case
        actual_name = bin_file.stem[1:-1]
        lower_name = actual_name.lower()
        all_guns.append(lower_name)
        gun_name_map[lower_name] = actual_name
    
    guns_to_process = args.guns if args.guns else all_guns
    
    stats = {'processed': 0, 'skipped': 0, 'errors': 0}
    
    for gun_name_input in sorted(guns_to_process):
        gun_name_lower = gun_name_input.lower()
        
        # Get the actual case from the binary file
        if gun_name_lower not in gun_name_map:
            print(f"  ✗ {gun_name_input}: Binary file not found")
            stats['errors'] += 1
            continue
        
        actual_gun_name = gun_name_map[gun_name_lower]
        bin_file = gun_dir / f"G{actual_gun_name}Z.bin"
        
        # Find the actual directory name (case-insensitive search)
        gun_subdirs = list(gun_dir.glob(f"{actual_gun_name}"))
        if not gun_subdirs:
            # Try case-insensitive
            gun_subdirs = [d for d in gun_dir.iterdir() 
                           if d.is_dir() and d.name.lower() == gun_name_lower]
        
        if not gun_subdirs:
            print(f"  ⊘ {actual_gun_name}: Missing metadata directory")
            stats['skipped'] += 1
            continue
        
        actual_dir_name = gun_subdirs[0].name
        
        # Parse metadata using the actual directory name  
        metadata_dir = gun_dir / actual_dir_name
        metadata = {}
        
        # Check for gun-specific metadata files (same format as prop)
        header_file = metadata_dir / "ModelFileHeader.inc.c"
        estimated_metadata = False
        
        if header_file.exists():
            with open(header_file, 'r') as f:
                header_content = f.read()
                metadata['header'] = header_content
                
                # Parse MODELFILEHEADER macro to extract metadata
                # Format: MODELFILEHEADER(name, rootnode, skeleton, switches, numswitches, nummatrices, boundingradius, numrecords, numtextures)
                match = re.search(r'MODELFILEHEADER\([^,]+,\s*([^,]+),\s*[^,]+,\s*([^,]+),\s*(\d+|0x[0-9A-Fa-f]+),\s*[^,]+,\s*([^,]+),\s*[^,]+,\s*(\d+|0x[0-9A-Fa-f]+)\)', header_content)
                if match:
                    rootnode = match.group(1).strip()
                    switches_ptr = match.group(2).strip()
                    num_switches = int(match.group(3), 0)  # 0 base auto-detects hex/decimal
                    bounding_radius = match.group(4).strip()
                    num_textures = int(match.group(5), 0)
                    metadata['rootnode'] = rootnode
                    metadata['switches_ptr'] = switches_ptr
                    metadata['num_switches'] = num_switches
                    metadata['bounding_radius'] = bounding_radius
                    metadata['num_textures'] = num_textures
        
        # If no metadata, try to estimate from binary
        if not metadata or 'num_switches' not in metadata:
            print(f"  ⚙ {actual_gun_name}: Estimating metadata from binary...")
            try:
                with open(bin_file, 'rb') as f:
                    binary_data = f.read()
                
                # Estimate parameters by scanning binary structure
                # First, find where actual data starts (after null padding)
                # Switches are at the start, followed by optional texture table, then nodes
                
                base_addr = 0x05000000
                num_textures = 0
                num_switches = 0
                
                # Find first non-zero 4-byte word to determine switch array size
                data_start = 0
                for offset in range(0, min(len(binary_data) - 3, 0x200), 4):
                    word = struct.unpack('>I', binary_data[offset:offset+4])[0]
                    if word != 0:
                        data_start = offset
                        break
                
                # Number of switches = data_start / 4
                if data_start > 0:
                    num_switches = data_start // 4
                
                # Check if there's a texture table after switches
                # Texture table has 12-byte entries, and the first word should be a pointer
                # Look at what's right after the switch array
                if data_start > 0 and data_start + 12 <= len(binary_data):
                    # Check if data_start looks like a node (has node type marker)
                    # or a texture table (has multiple valid pointers)
                    first_word = struct.unpack('>I', binary_data[data_start:data_start+4])[0]
                    
                    # If first word looks like a node type (0x0401, 0x0501, etc), no texture table
                    if first_word < 0x1000:
                        num_textures = 0
                    else:
                        # Try to count texture entries
                        test_offset = data_start
                        while test_offset + 12 <= len(binary_data):
                            try:
                                tex_data = struct.unpack('>III', binary_data[test_offset:test_offset+12])
                                # All three words should be valid addresses
                                if (tex_data[0] >= base_addr and tex_data[0] < base_addr + 0x100000 and
                                    tex_data[1] >= base_addr and tex_data[1] < base_addr + 0x100000 and
                                    tex_data[2] >= base_addr and tex_data[2] < base_addr + 0x100000):
                                    num_textures += 1
                                    test_offset += 12
                                else:
                                    break
                            except:
                                break
                
                # Estimate bounding radius from vertex data
                # For now use a default value
                bounding_radius = "100.0"
                
                metadata = {
                    'rootnode': f'&ROOTNODE({actual_dir_name})',
                    'switches_ptr': '0' if num_switches == 0 else f'SWITCHES({actual_dir_name})',
                    'num_switches': num_switches,
                    'bounding_radius': bounding_radius,
                    'num_textures': num_textures,
                    'header': '// Estimated metadata (not compiled)\n'
                }
                estimated_metadata = True
                
                print(f"    Estimated: {num_switches} switches, {num_textures} textures, radius {bounding_radius}")
                
            except Exception as e:
                print(f"  ✗ {actual_gun_name}: Could not estimate metadata - {e}")
                stats['errors'] += 1
                import traceback
                traceback.print_exc()
                continue
        
        output_file = gun_dir / actual_dir_name / "Model.c"
        if output_file.exists() and not args.force:
            print(f"  ⊘ {actual_gun_name}: Model.c already exists (use --force)")
            stats['skipped'] += 1
            continue
        
        try:
            # Parse binary (same format as props)
            with open(bin_file, 'rb') as f:
                binary_data = f.read()
            
            parser = BinaryModelParser(binary_data, metadata, image_map)
            parsed_model = parser.parse()
            
            # Generate C code
            c_code = generate_gun_c(actual_dir_name, parsed_model, metadata, image_map, binary_data)
            
            if args.dry_run:
                if len(parsed_model['switches']) > 0:
                    print(f"  ✓ {actual_gun_name}: Would generate Model.c ({len(parsed_model['nodes'])} nodes, {len(parsed_model['textures'])} textures, {len(parsed_model['switches'])} switches)")
                else:
                    print(f"  ✓ {actual_gun_name}: Would generate Model.c ({len(parsed_model['nodes'])} nodes, {len(parsed_model['textures'])} textures)")
            else:
                output_file.parent.mkdir(parents=True, exist_ok=True)
                with open(output_file, 'w') as f:
                    f.write(c_code)
                    
                # If metadata was estimated, create commented header file
                if estimated_metadata:
                    header_file = metadata_dir / "ModelFileHeader.inc.c"
                    switches_ptr = metadata['switches_ptr']
                    num_switches_hex = f"0x{metadata['num_switches']:02X}"
                    num_textures_hex = f"0x{metadata['num_textures']:02X}"
                    bounding_radius = metadata['bounding_radius']
                    
                    with open(header_file, 'w') as f:
                        f.write(f"// Estimated metadata - decoded from G{actual_gun_name}Z.bin\n")
                        f.write(f"// This is NOT compiled - uncomment and verify values if needed\n")
                        f.write(f"//\n")
                        f.write(f"// MODELFILEHEADER({actual_gun_name},\n")
                        f.write(f"//     &ROOTNODE({actual_gun_name}),\n")
                        f.write(f"//     &SKELETON(standard_gun),\n")
                        f.write(f"//     {switches_ptr},\n")
                        f.write(f"//     {num_switches_hex},\n")
                        f.write(f"//     4,\n")
                        f.write(f"//     {bounding_radius},\n")
                        f.write(f"//     0,\n")
                        f.write(f"//     {num_textures_hex}\n")
                        f.write(f"// )\n")
                    
                    print(f"    Created commented ModelFileHeader.inc.c")
                    
                # Report generation
                if len(parsed_model['switches']) > 0:
                    print(f"  ✓ {actual_gun_name}: Generated Model.c ({len(parsed_model['nodes'])} nodes, {len(parsed_model['textures'])} textures, {len(parsed_model['switches'])} switches)")
                else:
                    print(f"  ✓ {actual_gun_name}: Generated Model.c ({len(parsed_model['nodes'])} nodes, {len(parsed_model['textures'])} textures)")
                
                # Clean up source binary file after successful conversion
                if args.cleanup and bin_file.exists():
                    bin_file.unlink()
                    print(f"    Cleaned up {bin_file.name}")
            
            stats['processed'] += 1
            
        except Exception as e:
            print(f"  ✗ {actual_gun_name}: Error - {e}")
            stats['errors'] += 1
            import traceback
            traceback.print_exc()
    
    print(f"\n=== Summary ===")
    print(f"Processed: {stats['processed']}")
    print(f"Skipped: {stats['skipped']}")
    print(f"Errors: {stats['errors']}")


if __name__ == "__main__":
    main()
