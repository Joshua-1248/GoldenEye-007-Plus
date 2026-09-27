#ifndef _MIRRORED_LEVELS_H_
#define _MIRRORED_LEVELS_H_

#include <ultra64.h>

#ifdef GE_MODDED_CHEATS
void mirrorLevelsStageReset(void);
void mirrorLevelsStageBegin(s32 enabled);
s32 mirrorLevelsIsEnabled(void);
s32 mirrorLevelsHasPending(void);
void mirrorLevelsSetEnabled(s32 enabled);
void mirrorLevelsApplyPending(void);
void mirrorLevelsPrepareStageUnload(void);
void mirrorLevelsApplySetupIfNeeded(void);
f32 mirrorLevelsX(f32 x);
f32 mirrorLevelsDirX(f32 x);
#endif

#endif
