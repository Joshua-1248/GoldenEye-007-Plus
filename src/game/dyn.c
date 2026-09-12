#include <ultra64.h>
#include "dyn.h"
#ifdef GE_MODDED_CHEATS
#include "game/front.h"
#endif
#include <token.h>
#include <str.h>
#include <memp.h>
#include <macro.h>

/**
 * This file handles memory usage for graphics related tasks.
 *
 * There are two pools, "gfx" and "vtx", which are used to store different data.
 *
 * The gfx pool (g_GfxBuffers) is sized based on the stage's -mgfx
 * argument. It contains only the master display list's GBI bytecode.
 * The master gdl is passed through all rendering functions in the game engine,
 * where each appends to the display list.
 *
 * The vtx pool (g_VtxBuffers) is sized based on the stage's -mvtx argument.
 * It is used for auxiliary graphics data such as vertex arrays, matrices and
 * colours.
 *
 * Both the gfx and vtx pools are split into two buffers of equal size.
 * Only one buffer is active at a time - the other is being drawn to the screen
 * while the active one is being built. Each time a frame is finished the active
 * buffer index is swapped to the other one.
 *
 * Both the gfx and vtx pools have a third element in them, but this is just a
 * marker for the end of the second element's allocation.
 */

u8 *g_GfxBuffers[3];
u8 *g_VtxBuffers[3];
u8 *g_GfxMemPos;
u8 g_GfxActiveBufferIndex;
s32 g_GfxRequestedDisplayList;
s32 D_800482E0 = 0;
s32 g_GfxSizesByPlayerCount[] = {0x10000, 0x18000, 0x20000, 0x28000};
s32 g_VtxSizesByPlayerCount[] = {0x10000, 0x18000, 0x20000, 0x28000};

char membars_string1[] = ">>>>>>>>>>>>>>>>>>>>>>>>>";
char membars_string2[] = "=========================";
char membars_string3[] = "-------------------------";

void dynInit(void) {
    debTryAdd(&D_800482E0, "dyn_c_debug");
}

void dynInitMemory(void) {
    s32 playerindex = getPlayerCount() - 1;

#ifdef GE_MODDED_CHEATS
    /* Campaign stage tokens contain single-player display-list/vertex sizes.
     * Using those values for two or more Co-Op viewports corrupts memory as
     * soon as the first split-screen frame is built.  Keep Co-Op's render
     * scratch outside MEMPOOL_STAGE and use GoldenEye's native per-player
     * default sizes (64/96/128/160 KiB per half).  Two gfx halves plus two
     * vtx halves fit exactly in the 640 KiB Expansion Pak reservation at 4P. */
    if (gamemode == GAMEMODE_MULTI && get_scenario() == SCENARIO_COOP) {
        static const u32 coopDynSizeByPlayerCount[4] = {
            0x10000, 0x18000, 0x20000, 0x28000
        };
        extern u8 _coopDynBuffersStart[];
        extern u8 _coopDynBuffersEnd[];
        u8 *base = _coopDynBuffersStart;
        u32 size;

        if (playerindex < 0) playerindex = 0;
        if (playerindex > 3) playerindex = 3;
        size = coopDynSizeByPlayerCount[playerindex];

        g_GfxSizesByPlayerCount[playerindex] = size;
        g_VtxSizesByPlayerCount[playerindex] = size;

        g_GfxBuffers[0] = base;
        g_GfxBuffers[1] = base + size;
        g_GfxBuffers[2] = base + size * 2;
        g_VtxBuffers[0] = g_GfxBuffers[2];
        g_VtxBuffers[1] = g_VtxBuffers[0] + size;
        g_VtxBuffers[2] = g_VtxBuffers[1] + size;

        /* Linker assertion guarantees the 4P maximum fits; retain a runtime
         * guard so a bad future repartition cannot scribble into the next
         * Expansion Pak region. */
        if (g_VtxBuffers[2] > _coopDynBuffersEnd) {
            g_VtxBuffers[2] = _coopDynBuffersEnd;
        }

        g_GfxActiveBufferIndex = 0;
        g_GfxRequestedDisplayList = FALSE;
        g_GfxMemPos = g_VtxBuffers[0];
        return;
    }
#endif

    if (tokenFind(1, "-mgfx")) {
        g_GfxSizesByPlayerCount[playerindex] = strtol(tokenFind(1, "-mgfx"), NULL, 0) * 1024;
    }
    if (tokenFind(1, "-mvtx")) {
        g_VtxSizesByPlayerCount[playerindex] = strtol(tokenFind(1, "-mvtx"), NULL, 0) * 1024;
    }

    g_GfxBuffers[0] = mempAllocBytesInBank(g_GfxSizesByPlayerCount[playerindex] * 2, MEMPOOL_STAGE);
    g_GfxBuffers[1] = (g_GfxBuffers[0] + g_GfxSizesByPlayerCount[playerindex]);
    g_GfxBuffers[2] = (g_GfxBuffers[1] + g_GfxSizesByPlayerCount[playerindex]);

    g_VtxBuffers[0] = mempAllocBytesInBank(g_VtxSizesByPlayerCount[playerindex] * 2, MEMPOOL_STAGE);
    g_VtxBuffers[1] = (g_VtxBuffers[0] + g_VtxSizesByPlayerCount[playerindex]);
    g_VtxBuffers[2] = (g_VtxBuffers[1] + g_VtxSizesByPlayerCount[playerindex]);

    g_GfxActiveBufferIndex = 0;
    g_GfxRequestedDisplayList = FALSE;
    g_GfxMemPos = g_VtxBuffers[0];
}

Gfx *dynGetMasterDisplayList(void) {
    g_GfxRequestedDisplayList = TRUE;

    return (Gfx*)g_GfxBuffers[g_GfxActiveBufferIndex];
}

s32 dynGetFreeGfx2(Gfx *gdl) {
    return (Gfx*)g_GfxBuffers[g_GfxActiveBufferIndex + 1] - gdl;
}

/**
 * Address: 7F0BD6C4
 */
Vtx *dynAllocateVertices(s32 count) 
{
    void *ptr = g_GfxMemPos;
	g_GfxMemPos += count * sizeof(Vtx);
	return ptr;
}

Mtx *dynAllocateMatrix(void)
{
	void *ptr = g_GfxMemPos;
	g_GfxMemPos += sizeof(Mtx);
	return ptr;
}

/**
 * Address: 7F0BD6F8
 */
Light *dynAllocateLights(s32 count)
{
    void *ptr = g_GfxMemPos;
    g_GfxMemPos += count * sizeof(Light);
    return ptr;
}

void *dynAllocate(s32 size) {
    void *ptr = g_GfxMemPos;
	size = ALIGN16_a(size);
	g_GfxMemPos += size;
	return ptr;
}

void dynSwapBuffers(void) {
    g_GfxActiveBufferIndex = (g_GfxActiveBufferIndex ^ 1);
    g_GfxRequestedDisplayList = FALSE;
    g_GfxMemPos = g_VtxBuffers[g_GfxActiveBufferIndex];
}

void dynRemovedFunc(Gfx *gdl) {
}

s32 dynGetFreeGfx(Gfx *gdl) {
    return (Gfx*)g_GfxBuffers[g_GfxActiveBufferIndex + 1] - gdl;
}

s32 dynGetFreeVtx(void) {
	return g_VtxBuffers[g_GfxActiveBufferIndex + 1] - g_GfxMemPos;
}

// Address 0x7F0BD7CC NTSC
void dynCalculateMembarLength(const char* arg0, f32 arg1, f32 arg2)
{
    s32 len;
    f32 zero = 0;
    
    len = strlen(arg0);
    
    arg1 /= arg2;
    
    if(zero);
    
    if (arg1 < zero && len > 1)
    {
        if (len > 1)
        {
            
        }
    }
}

void dynDrawMembars(Gfx *gdl) {
    dynCalculateMembarLength(membars_string2, ((Gfx*)g_GfxBuffers[g_GfxActiveBufferIndex + 1] - gdl), ((Gfx*)g_GfxBuffers[g_GfxActiveBufferIndex + 1] - (Gfx*)g_GfxBuffers[g_GfxActiveBufferIndex]));
    dynCalculateMembarLength(membars_string2, (g_VtxBuffers[g_GfxActiveBufferIndex + 1] - g_GfxMemPos), (g_VtxBuffers[g_GfxActiveBufferIndex + 1] - g_VtxBuffers[g_GfxActiveBufferIndex]));
}
