/**
 * GE's light system is quite rudimentary and does just two things when a light is destroyed:
 * 
 * 1) it darkens the light fixture's own vertices in place. Each vertex
 *      color component is shifted right by 2 so the fixture's polygons draw
 *      at a quarter brightness and read as burnt out
 * 
 * 2) it spawns glass shards from the light fixture
 * 
 * Two tables are important for this sytem.
 * 
 * light_fixture_table[100] is a directory of the fixtures currently loaded. An
 * entry is (room_index, DL start, DL end): the run of a room's display list 
 * drawn with a single light texture. Note that one entry is NOT one light fixture,
 * but rather a batch of tris in a room using the same texture. Thus one entry could
 * represent multiple light fixtures so long as those fixtures use the same texture.
 * Function lightIsCoordNearDarkenedVertex() determines what counts as one light fixture.
 * 
 * darkened_light_table[512] is the persistent record of what has been shot
 * out, as (room_index, vertex index). It has to be separate and persistent
 * because vertex data is reread from the file whenever a room is streamed
 * back in, which wipes the in-place darkening. It is a circular buffer, so on
 * a level with a great many broken lights the oldest ones silently come back.
 * 
 * Lifecycle:
 * Level start   On level start lightFixtureInitTables() zeroes both tables.
 * 
 * Room load     bgLoadRoomPrimaryGdl() calls clear_light_fixturetable_in_room(),
 *               which drops that room's stale fixture entries and latches the
 *               room number into current_light_fixture_room. Then
 *               texLoadFromGdl() walks the room's display list. Every time it
 *               emits a texture that check_if_imageID_is_light() recognises
 *               it opens an entry via lightFixtureEntryBegin() 
 *               and closes it with lightFixtureEntryEnd(). So the
 *               fixture directory is a by-product of texture processing.
 * 
 * Room unload   Nothing! delete_room_data() frees ptr_expanded_mapping_info,
 *               the display list these entries point into, but never
 *               touches light_fixture_table. Thus a level could in theory have
 *               enough lights to fill the light_fixture_table and after that
 *               point you wouldn't be able to shoot out any more lights. But I
 *               don't believe any levels actually have enough rooms and lights 
 *               to fill the table.
 *
 * Vertex load   after bgLoadRoomVtxData(), bg.c calls redarken_lights_in_room()
 *               to re-apply the >>2 to every vertex the darkened table lists for
 *               that room, restoring lights the player broke earlier.
 *
 * Bullet hit    chrprop.c checks check_if_imageID_is_light() on the hit
 *               texture and, if it matches, calls lightFixtureBreak() with the
 *               display list command of the triangle that was hit.
 * 
 * Vertex pointers inside a room display list are segmented addresses - segment
 * SPSEGMENT_BG_VTX (14) in the top byte, a 24-bit offset below - because the
 * display list was built offline. The RSP resolves them from the gSPSegment
 * that bg.c issues at draw time, but code here reads the vertices with the CPU
 * and so has to resolve them by hand: room vertex base + offset. That is the
 * "(ptr & 0xFF000000) == 0x0E000000" test in lightFindVertexBaseForTri.
 */

#include <ultra64.h>
#include "lightfixture.h"
#include "bg.h"
#include "glass.h"
#include <bondconstants.h>
#include <assets/image_externs.h>
#include <PR/gbi.h>
#include <gbi_extension.h>

#define LIGHTFIXTURE_TABLE_MAX 0x64
#define DARKENED_LIGHT_TABLE_MAX 0x200

s_lightfixture light_fixture_table[LIGHTFIXTURE_TABLE_MAX];
s16 current_light_fixture_slot;
s16 current_light_fixture_room;

/**
 * The darkened light table can hold a maximum of 512 vertices. It is a circular table so if that limit is exceeded,
 * the oldest records are overwritten first.
 */
struct s_darkened_light darkened_light_table[DARKENED_LIGHT_TABLE_MAX];

s32 dword_CODE_bss_80083318; // unused
s32 cur_entry_darkened_light_table = 0;

s32 D_80046034[] = {0, 0, 0, 0, 0, 0, 0}; // unused


void lightFixtureInitTables(void)
{
    s32 i;

    for (i = 0; i < LIGHTFIXTURE_TABLE_MAX; i++)
    {
        light_fixture_table[i].room_index = 0;
    }

    for (i = 0; i < DARKENED_LIGHT_TABLE_MAX; i++)
    {
        darkened_light_table[i].room_index = 0;
    }

    cur_entry_darkened_light_table = 0;
}


s32 lightFixtureFindFreeSlot(void)
{
    s32 i;

    for (i = 0; i != LIGHTFIXTURE_TABLE_MAX; i++)
    {
        if (light_fixture_table[i].room_index == 0)
        {
            return i;
        }
    }

    return LIGHTFIXTURE_TABLE_MAX;
}


void lightFixtureEntryBegin(Gfx *DL)
{
    current_light_fixture_slot = lightFixtureFindFreeSlot();

    if (current_light_fixture_slot != LIGHTFIXTURE_TABLE_MAX)
    {
        light_fixture_table[current_light_fixture_slot].room_index = current_light_fixture_room;
        light_fixture_table[current_light_fixture_slot].ptr_start_pertinent_DL = DL;
    }
}


void lightFixtureEntryEnd(Gfx *DL)
{
    if (current_light_fixture_slot != LIGHTFIXTURE_TABLE_MAX)
    {
        light_fixture_table[current_light_fixture_slot].ptr_end_pertinent_DL = DL;
    }
}


bool check_if_imageID_is_light(s32 imageID)
{
    if ((imageID == IMAGE_WALL_LAMP)     ||
        (imageID == IMAGE_203_LIGHT)     ||
        (imageID == IMAGE_205_LIGHT)     ||
        (imageID == IMAGE_252_LIGHT)     ||
        (imageID == IMAGE_PANEL_LAMP)    ||
        (imageID == IMAGE_255_LIGHT)     ||
        (imageID == IMAGE_256_LIGHT)     ||
        (imageID == IMAGE_HANGING_LAMP)  ||
        (imageID == IMAGE_NEON_LAMP)     ||
        (imageID == IMAGE_LINEAR_LAMP))
    {
        // Will darken when shot
        return 1;
    } 
    else
    {
        return 0;
    }
}


/**
 * Returns the vertex array that a triangle command's indices refer to.
 *
 * Scans backwards to the gSPVertex that last loaded vertices, then resolves its
 * address. That address is normally a segment-14 (SPSEGMENT_BG_VTX) reference,
 * because room display lists are built offline; the RSP resolves those itself
 * at draw time, but this runs on the CPU and so adds the room's vertex base by
 * hand.
 */
Vtx *lightFindVertexBaseForTri(Gfx *gfx, s32 room_index)
{
    Vtx * ret;

    while (gfx->dma.cmd != G_VTX )
    { 
        gfx--; 
    }

    ret = gfx->dma.addr;

    if (((s32) ret & 0xFF000000) == 0x0E000000) 
    {
        ret = (s32)g_BgRoomInfo[room_index].vertices + ((s32) ret & 0xFFFFFF);
    }

    return ret;
}


void extract_vertex_indices_from_triangle(Gfx* gfx, u32 tri_type, s32* idx1, s32* idx2, s32* idx3)
{
    switch (tri_type) 
    {
        case 0:
            *idx1 = (s32) gfx->tri.tri.v[0] / 10;
            *idx2 = (s32) gfx->tri.tri.v[1] / 10;
            *idx3 = (s32) gfx->tri.tri.v[2] / 10;
            break;
        // unsure of how to cleanly access the below versions
        case 1:
            *idx1 = ((u32*)gfx)[1] & 0xF;
            *idx2 = ((((u8*)gfx)[7]) & 0xFFFFFFFFu) >> 4;
            *idx3 = ((s32*)gfx)[0] & 0xF;
            break;
        case 2:
            *idx1 = ((u8*)gfx)[6] & 0xF;
            *idx2 = ((((u16*)gfx)[3]) & 0xFFFFFFFFu) >> 0xC;
            *idx3 = ((((u8*)gfx)[3]) & 0xFFFFFFFFu) >> 4;
            break;
        case 3:
            *idx1 = ((u16*)gfx)[2] & 0xF;
            *idx2 = ((((u8*)gfx)[5]) & 0xFFFFFFFFu) >> 4;
            *idx3 = ((u8*)gfx)[2] & 0xF;
            break;
        case 4:
            *idx1 = ((u8*)gfx)[4] & 0xF;
            *idx2 = ((u32*)gfx)[1] >> 0x1C;
            *idx3 = ((((u16*)gfx)[1]) & 0xFFFFFFFFu) >> 0xC;
            break;
    }
}


void extract_vertex_coords_from_triangle(Gfx * gfx, u32 tri_type, s32 room_index, coord16 * out1, coord16 * out2, coord16 * out3)
{
    s32 idx1;
    s32 idx2;
    s32 idx3;
    Vtx * vertices;

    extract_vertex_indices_from_triangle(gfx, tri_type, &idx1, &idx2, &idx3);
    vertices = lightFindVertexBaseForTri(gfx, room_index);

    out1->AsArray[0] = (s16) vertices[idx1].v.ob[0];
    out1->AsArray[1] = (s16) vertices[idx1].v.ob[1];
    out1->AsArray[2] = (s16) vertices[idx1].v.ob[2];

    out2->AsArray[0] = (s16) vertices[idx2].v.ob[0];
    out2->AsArray[1] = (s16) vertices[idx2].v.ob[1];
    out2->AsArray[2] = (s16) vertices[idx2].v.ob[2];

    out3->AsArray[0] = (s16) vertices[idx3].v.ob[0];
    out3->AsArray[1] = (s16) vertices[idx3].v.ob[1];
    out3->AsArray[2] = (s16) vertices[idx3].v.ob[2];
}


void redarken_lights_in_room(s32 room_index)
{
    Vtx * vertex;
    s32 i;
    struct s_darkened_light* unk;

    vertex = g_BgRoomInfo[room_index].vertices;

    for (i = 0; i < DARKENED_LIGHT_TABLE_MAX; i++)
    {
        unk = &darkened_light_table[i];

        if (room_index != unk->room_index) { continue; }

        vertex[unk->vtx_index].v.cn[0] >>= 2;
        vertex[unk->vtx_index].v.cn[1] >>= 2;
        vertex[unk->vtx_index].v.cn[2] >>= 2;
        vertex[unk->vtx_index].v.cn[3] >>= 2;
    }
}


void darken_vertex_in_room(Vtx * vertex, s32 room_index)
{
    s32 vtx_index;

    // Check if this vertex was already darkened
    if (darkened_light_table_contains_vertex(vertex, room_index) != 0) { return; }

    // weird memory stuff going on here
    vtx_index = ((u32)vertex - (u32)g_BgRoomInfo[room_index].vertices) >> 4;

    darkened_light_table[cur_entry_darkened_light_table].room_index = (u16) room_index;
    darkened_light_table[cur_entry_darkened_light_table].vtx_index = vtx_index;

    vertex->v.cn[0] >>= 2;
    vertex->v.cn[1] >>= 2;
    vertex->v.cn[2] >>= 2;
    vertex->v.cn[3] >>= 2;

    cur_entry_darkened_light_table++;

    if (cur_entry_darkened_light_table >= DARKENED_LIGHT_TABLE_MAX)
    {
        cur_entry_darkened_light_table = 0;
    }
}


s32 darkened_light_table_contains_vertex(Vtx * vertex, s32 room_index)
{
    u32 vtx_index;
    s32 i;

    // weird memory stuff going on here
    vtx_index = ((u32)vertex - (u32)g_BgRoomInfo[room_index].vertices) >> 4;

    for (i = 0; i < DARKENED_LIGHT_TABLE_MAX; i++)
    {
        if ((room_index == darkened_light_table[i].room_index) && ((s32)vtx_index == darkened_light_table[i].vtx_index))
        {
            return TRUE;
        }
    }

    return FALSE;
}


void darken_triangle_in_room(Gfx *gfx, u32 tri_type, s32 room_index)
{
    Vtx * vertex;
    s32 idx1;
    s32 idx2;
    s32 idx3;
    Vtx * vertices;

    extract_vertex_indices_from_triangle(gfx, tri_type, &idx1, &idx2, &idx3);
    vertices = lightFindVertexBaseForTri(gfx, room_index);

    darken_vertex_in_room(&vertices[idx1], room_index);
    darken_vertex_in_room(&vertices[idx2], room_index);

    vertex = &vertices[idx3];
    darken_vertex_in_room(vertex, room_index);
}


s32 darkened_light_table_contains_triangle(Gfx * gfx, u32 tri_type, s32 room_index)
{
    s32 out3;
    s32 idx1;
    s32 idx2;
    s32 idx3;
    Vtx * vertices;
    s32 out2;
    s32 out1;

    extract_vertex_indices_from_triangle(gfx, tri_type, &idx1, &idx2, &idx3);
    vertices = lightFindVertexBaseForTri(gfx, room_index);
    out1 = darkened_light_table_contains_vertex(&vertices[idx2], room_index);
    out2 = darkened_light_table_contains_vertex(&vertices[idx1], room_index);
    out3 = darkened_light_table_contains_vertex(&vertices[idx3], room_index);
    return out3 + out2 + out1;
}


/**
 * Test whether a tri belongs to a light fixture region that should also be darkened.
 */
s32 lightIsCoordNearDarkenedVertex(coord16 * coord, s32 room_index)
{
    s32 dx;
    s32 dy;
    s32 dz;
    s32 i;
    Vtx * vertex;

    i = 0;
    do
    {
        if (room_index == darkened_light_table[i].room_index)
        {
            vertex = &g_BgRoomInfo[room_index].vertices[darkened_light_table[i].vtx_index];

            dx = vertex->v.ob[0] - coord->AsArray[0];
            dy = vertex->v.ob[1] - coord->AsArray[1];
            dz = vertex->v.ob[2] - coord->AsArray[2];

            if (dx < 0) { dx = -dx; }
            if (dy < 0) { dy = -dy; }
            if (dz < 0) { dz = -dz; }

            if ((dx + dy + dz) < (s32) (get_room_data_float1() * 100.0f))
            {
                return 1;
            }
        }
    } while (++i < DARKENED_LIGHT_TABLE_MAX);

    return 0;
}


/**
 * Darken the vertices belonging to a light fixture and spawn shards of glass.
 * 
 * When a bullet hits a triangle, lightFixtureBreak searches for a light_fixture_table entry where:
 * room_index == entry->room_index
 * gfx >= entry->ptr_start_pertinent_DL
 * gfx <  entry->ptr_end_pertinent_DL
 * 
 */
void lightFixtureBreak(Gfx * hit_gfx, u32 tri_type, s32 room_index)
{
    s16 diff_z_12;
 
    // Vertices of the hit triangle
    coord16 hit_vtx1;
    coord16 hit_vtx2;
    coord16 hit_vtx3;
 
    // Corners of whichever candidate tri is currently being tested for darkening
    coord16 cand_vtx1;
    coord16 cand_vtx2;
    coord16 cand_vtx3;
 
    s16 diff_x_23;
    s16 diff_x_12;
    Gfx *fixture_gfx;
    f32 edge_frac;
    s32 j;
    s8 darken_tri1;
    s8 darken_tri4;
    s16 diff_y_23;
    s16 diff_y_12;
    s16 diff_x_13;
    s16 diff_z_23;
    f32 shard_step_12;
    f32 shard_step_13;
    f32 shard_step_23;
    coord3d room_origin;
    coord3d shard_pos;
    s32 i;
    s16 diff_z_13;
    f32 edge_length;
    s16 diff_y_13;
 
    for (i = 0; i < LIGHTFIXTURE_TABLE_MAX; i++)
    {
        /** 
          *  Check if this shot landed in the same room as this light fixture table entry. If not, skip this iteration.
          *  This is what makes stale slots in the light fixture array safe. An unloaded room's entries keep dangling
          *  DL pointers, and they are rejected here before the pointer comparisons
          *  below could ever look at them. 
          */
        if (room_index != light_fixture_table[i].room_index) 
        { 
            continue; 
        }
 
        if (hit_gfx < light_fixture_table[i].ptr_start_pertinent_DL) 
        { 
            continue; 
        }
 
        if (hit_gfx >= light_fixture_table[i].ptr_end_pertinent_DL) 
        { 
            continue; 
        }
 
        // If this tri is already darkened, do not darken it again.
        if (darkened_light_table_contains_triangle(hit_gfx, tri_type, light_fixture_table[i].room_index) != 0) 
        { 
            return; 
        }
 
        //Darken the exact triangle that was shot. The check for other tris that are part of this light fixture comes later.
        darken_triangle_in_room(hit_gfx, tri_type, light_fixture_table[i].room_index);
 
        /**
         * Measure the hit tri's three edges and turn each into a step size for the shard loops below.
         *
         * edge_length is in room units; get_room_data_float2() is 1 / level scale, so multiplying gives 
         * the edge length in world units, and 10.0f / that is the fraction of the edge 
         * spanning 10 world units. The loops add it to edge_frac from 0 to 1, so shards land every 10
         * world units and a longer edge sheds proportionally more. The per-axis diffs are kept because 
         * those loops reuse them as the direction to interpolate along.
         */
        extract_vertex_coords_from_triangle(hit_gfx, tri_type, light_fixture_table[i].room_index, &hit_vtx1, &hit_vtx2, &hit_vtx3);
 
		diff_x_12 = hit_vtx1.AsArray[0] - hit_vtx2.AsArray[0];
		diff_x_13 = hit_vtx1.AsArray[0] - hit_vtx3.AsArray[0];
		diff_x_23 = hit_vtx2.AsArray[0] - hit_vtx3.AsArray[0];
 
		diff_y_12 = hit_vtx1.AsArray[1] - hit_vtx2.AsArray[1];
		diff_y_13 = hit_vtx1.AsArray[1] - hit_vtx3.AsArray[1];
		diff_y_23 = hit_vtx2.AsArray[1] - hit_vtx3.AsArray[1];
 
		diff_z_12 = hit_vtx1.AsArray[2] - hit_vtx2.AsArray[2];
		diff_z_13 = hit_vtx1.AsArray[2] - hit_vtx3.AsArray[2];
		diff_z_23 = hit_vtx2.AsArray[2] - hit_vtx3.AsArray[2];
 
        edge_length = sqrtf((diff_x_12 * diff_x_12) + (diff_y_12 * diff_y_12) + (diff_z_12 * diff_z_12));
        shard_step_12 = 10.0f / (get_room_data_float2() * edge_length);
 
        edge_length = sqrtf((diff_x_13 * diff_x_13) + (diff_y_13 * diff_y_13) + (diff_z_13 * diff_z_13));
        shard_step_13 = 10.0f / (get_room_data_float2() * edge_length);
 
        edge_length = sqrtf((diff_x_23 * diff_x_23) + (diff_y_23 * diff_y_23) + (diff_z_23 * diff_z_23));
        shard_step_23 = 10.0f / (get_room_data_float2() * edge_length);
 
        getRoomPositionScaledByIndex(light_fixture_table[i].room_index, &room_origin);
 
        /**
         * Spawn glass shards along the edges of the hit tri.
         * Shards are spawned at fixed length intervals i.e. a long edge spawns more shards than a short edge.
         * Positions are converted from room space to world space for the glassCreateShard() function.
         */
        for (edge_frac = 0.0f; edge_frac < 1.0f; edge_frac += shard_step_12)
        {
            shard_pos.x = ((hit_vtx2.AsArray[0] + (diff_x_12 * edge_frac)) * get_room_data_float2()) + room_origin.f[0];
            shard_pos.y = ((hit_vtx2.AsArray[1] + (diff_y_12 * edge_frac)) * get_room_data_float2()) + room_origin.f[1];
            shard_pos.z = ((hit_vtx2.AsArray[2] + (diff_z_12 * edge_frac)) * get_room_data_float2()) + room_origin.f[2];
            glassCreateShard(&shard_pos, 0.0f, 10.0f);
        }
 
        for (edge_frac = 0.0f; edge_frac < 1.0f; edge_frac += shard_step_13)
        {
            shard_pos.x = ((hit_vtx3.AsArray[0] + (diff_x_13 * edge_frac)) * get_room_data_float2()) + room_origin.f[0];
            shard_pos.y = ((hit_vtx3.AsArray[1] + (diff_y_13 * edge_frac)) * get_room_data_float2()) + room_origin.f[1];
            shard_pos.z = ((hit_vtx3.AsArray[2] + (diff_z_13 * edge_frac)) * get_room_data_float2()) + room_origin.f[2];
            glassCreateShard(&shard_pos, 0.0f, 10.0f);
        }
 
        for (edge_frac = 0.0f; edge_frac < 1.0f; edge_frac += shard_step_23)
        {
            shard_pos.x = ((hit_vtx3.AsArray[0] + (diff_x_23 * edge_frac)) * get_room_data_float2()) + room_origin.f[0];
            shard_pos.y = ((hit_vtx3.AsArray[1] + (diff_y_23 * edge_frac)) * get_room_data_float2()) + room_origin.f[1];
            shard_pos.z = ((hit_vtx3.AsArray[2] + (diff_z_23 * edge_frac)) * get_room_data_float2()) + room_origin.f[2];
            glassCreateShard(&shard_pos, 0.0f, 10.0f);
        }
 
        /**
         * Iterate over all tris in the fixture's display list range.
         * If any vertex of a tri is close to a previously darkened vertex,
         * darken the entire tri. This ensures the entire fixture is darkened, not just the hit tri.
         */
        for (fixture_gfx = light_fixture_table[i].ptr_start_pertinent_DL; fixture_gfx < light_fixture_table[i].ptr_end_pertinent_DL; fixture_gfx++)
        {
            if (fixture_gfx->dma.cmd == G_TRI1)
            {
                darken_tri1 = 0;
 
                extract_vertex_coords_from_triangle(fixture_gfx, 0, light_fixture_table[i].room_index, &cand_vtx1, &cand_vtx2, &cand_vtx3);
 
                if (lightIsCoordNearDarkenedVertex(&cand_vtx1, light_fixture_table[i].room_index) != 0)
                {
                    darken_tri1 = 1;
                }
                else if (lightIsCoordNearDarkenedVertex(&cand_vtx2, light_fixture_table[i].room_index) != 0)
                {
                    darken_tri1 = 1;
                }
                else if (lightIsCoordNearDarkenedVertex(&cand_vtx3, light_fixture_table[i].room_index) != 0)
                {
                    darken_tri1 = 1;
                }
 
                if (darken_tri1 != 0)
                {
                    darken_triangle_in_room(fixture_gfx, 0, light_fixture_table[i].room_index);
                }
            }
            else if (fixture_gfx->dma.cmd == G_TRI4)
            {
                for (j = 0; j < 4; j++)
                {
                    darken_tri4 = 0;
 
                    extract_vertex_coords_from_triangle(fixture_gfx, j + 1, light_fixture_table[i].room_index, &cand_vtx1, &cand_vtx2, &cand_vtx3);
 
                    if (lightIsCoordNearDarkenedVertex(&cand_vtx1, light_fixture_table[i].room_index) != 0)
                    {
                        darken_tri4 = 1;
                    }
                    else if (lightIsCoordNearDarkenedVertex(&cand_vtx2, light_fixture_table[i].room_index) != 0)
                    {
                        darken_tri4 = 1;
                    }
                    else if (lightIsCoordNearDarkenedVertex(&cand_vtx3, light_fixture_table[i].room_index) != 0)
                    {
                        darken_tri4 = 1;
                    }
 
                    if (darken_tri4 != 0)
                    {
                        darken_triangle_in_room(fixture_gfx, j + 1, light_fixture_table[i].room_index);
                    }
                }
            }
        }
        return;
    }
}


/** 
 * When a room loads bgLoadRoomPrimaryGdl() calls this function.
 * The function walks through the entire light_fixture_table and frees the slots
 * that belong to the room being loaded. The level's room numbers start at 1 so setting
 * the room index to 0 is a way to indicate the slot is free. Then texLoadFromGdl() refills slots
 * for that room via first-fit, so a reloaded room may land in entirely different slots than before.
 */
void clear_light_fixturetable_in_room(s32 room_index)
{
    s32 i;


    for (i = 0; i < LIGHTFIXTURE_TABLE_MAX; i++)
    {
        if (room_index == light_fixture_table[i].room_index)
        {
            light_fixture_table[i].room_index = 0;
        }
    }

    current_light_fixture_room = room_index;
}

