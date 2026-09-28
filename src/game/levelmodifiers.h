#ifndef _LEVELMODIFIERS_H_
#define _LEVELMODIFIERS_H_

#include <ultra64.h>

typedef enum LevelModifierCategory
{
    LEVELMOD_CATEGORY_SINGLE_PLAYER = 0,
    LEVELMOD_CATEGORY_MULTIPLAYER,
    LEVELMOD_CATEGORY_MISCELLANEOUS,
    LEVELMOD_CATEGORY_COUNT
} LevelModifierCategory;

typedef enum LevelModifierPolicy
{
    LEVELMOD_POLICY_NONE = 0,
    LEVELMOD_POLICY_LATCHED,
    LEVELMOD_POLICY_REVERSIBLE
} LevelModifierPolicy;

void levelModifiersOnStanLoaded(s32 levelid, u8 *stan);
s32 levelModifiersAdjustStartPadIndex(s32 originalIndex, s32 startPadSlot);

s32 levelModifiersGetLevelCount(s32 category);
const char *levelModifiersGetLevelName(s32 category, s32 index);
s32 levelModifiersLevelImplemented(s32 category, s32 index);
s32 levelModifiersLevelAvailableInCurrentStage(s32 category, s32 index);

s32 levelModifiersGetSiloBetaVentPreload(void);
void levelModifiersSetSiloBetaVentPreload(s32 enabled);
s32 levelModifiersSiloBetaVentActive(void);
s32 levelModifiersActivateSiloBetaVent(void);
LevelModifierPolicy levelModifiersGetSiloBetaVentPolicy(void);

#endif
