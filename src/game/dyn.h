#ifndef _DYN_H_
#define _DYN_H_

#include <ultra64.h>

void dynInit(void);
void dynInitMemory(void);
Gfx *dynGetMasterDisplayList(void);
s32 dynGetFreeGfx2(Gfx *gdl);
Vtx *dynAllocateVertices(s32 count);
Mtx *dynAllocateMatrix(void);
Light *dynAllocateLights(s32 count);
void *dynAllocate(s32 size);
void dynSwapBuffers(void);
void dynRemovedFunc(Gfx *gdl);
s32 dynGetFreeGfx(Gfx *gdl);
s32 dynGetFreeVtx(void);
void dynDrawMembars(Gfx *gdl);

#endif
