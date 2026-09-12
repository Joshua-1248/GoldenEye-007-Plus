#ifndef _SKY_H_
#define _SKY_H_
#include <ultra64.h>
#include <bondconstants.h>
#include <bondtypes.h>
#include "bondview.h"

void skySetStageNum(s32 stagenum);
void skyTick(void);
Gfx * skyRender(Gfx *arg0);
void sub_GAME_7F097388(SkyRelated18 *arg0, Mtxf *arg1, u16 arg2, f32 arg3, f32 arg4, SkyRelated38 *arg5);
Gfx *skyRenderTri(Gfx *gdl, SkyRelated38 *arg1, SkyRelated38 *arg2, SkyRelated38 *arg3, f32 arg4, bool textured);
Gfx *skyRenderFull(Gfx *gdl, SkyRelated38 *arg1, SkyRelated38 *arg2, SkyRelated38 *arg3, SkyRelated38 *arg4, f32 arg5);

#endif
