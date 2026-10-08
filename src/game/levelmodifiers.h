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

s32 levelModifiersGetCurrentStage(void);
const char *levelModifiersGetCurrentStageName(void);
s32 levelModifiersGetCurrentStageModifierCount(void);

s32 levelModifiersGetLevelCount(s32 category);
const char *levelModifiersGetLevelName(s32 category, s32 index);
s32 levelModifiersLevelImplemented(s32 category, s32 index);
s32 levelModifiersLevelAvailableInCurrentStage(s32 category, s32 index);

s32 levelModifiersGetSiloBetaVentPreload(void);
void levelModifiersSetSiloBetaVentPreload(s32 enabled);
s32 levelModifiersSiloBetaVentActive(void);
s32 levelModifiersActivateSiloBetaVent(void);
LevelModifierPolicy levelModifiersGetSiloBetaVentPolicy(void);

s32 levelModifiersCitadelWaterActive(void);
s32 levelModifiersGetCitadelWaterPreload(void);
void levelModifiersSetCitadelWaterPreload(s32 enabled);
s32 levelModifiersSetCitadelWater(s32 enabled);
void levelModifiersOnEnvironmentLoaded(s32 levelid);
LevelModifierPolicy levelModifiersGetCitadelWaterPolicy(void);


/* R27Q_DAM_DOCK_RESTORATION */
void levelModifiersOnSetupReady(s32 levelid);
void levelModifiersOnPropsLoaded(s32 levelid);
void levelModifiersOnStageCleanup(s32 levelid);
s32 levelModifiersGetReservedObjectCount(s32 levelid);

s32 levelModifiersDamDoorsActive(void);
s32 levelModifiersGetDamDoorsPreload(void);
void levelModifiersSetDamDoorsPreload(s32 enabled);
s32 levelModifiersSetDamDoors(s32 enabled);
LevelModifierPolicy levelModifiersGetDamDoorsPolicy(void);

s32 levelModifiersDamSpeedboatActive(void);
s32 levelModifiersGetDamSpeedboatPreload(void);
void levelModifiersSetDamSpeedboatPreload(s32 enabled);
s32 levelModifiersSetDamSpeedboat(s32 enabled);
LevelModifierPolicy levelModifiersGetDamSpeedboatPolicy(void);

/* R27R_DRIVABLE_SPEEDBOAT */
s32 levelModifiersDamDrivableSpeedboatActive(void);
s32 levelModifiersGetDamDrivableSpeedboatPreload(void);
void levelModifiersSetDamDrivableSpeedboatPreload(s32 enabled);
s32 levelModifiersActivateDamDrivableSpeedboat(void);
LevelModifierPolicy levelModifiersGetDamDrivableSpeedboatPolicy(void);

const char *levelModifiersGetCurrentStageModifierName(s32 index);
const char *levelModifiersGetCurrentStageModifierValue(s32 index);
s32 levelModifiersToggleCurrentStageModifier(s32 index);
s32 levelModifiersCurrentStageModifierLocked(s32 index);

s32 levelModifiersGetFrontendModifierCount(s32 category, s32 levelIndex);
const char *levelModifiersGetFrontendModifierName(s32 category, s32 levelIndex, s32 modifierIndex);
const char *levelModifiersGetFrontendModifierValue(s32 category, s32 levelIndex, s32 modifierIndex);
s32 levelModifiersToggleFrontendModifier(s32 category, s32 levelIndex, s32 modifierIndex);

/* R27S R2 multi-level beta modifiers are exposed through the generic dispatch API. */

/* R27S R4 Temple / Complex / Frigate reversible modifiers. */
void levelModifiersOnStartPadsLoaded(void);
const char *levelModifiersGetFrontendModifierDescription(s32 category, s32 levelIndex, s32 modifierIndex);
const char *levelModifiersGetFrontendModifierCredit(s32 category, s32 levelIndex, s32 modifierIndex);


#ifdef GE_MODDED_CHEATS
s32 levelModifiersUseCurrentStanRoomForHoleTraversal(void);
#endif
s32 levelModifiersCradleKillPlaneShouldKill(f32 playerY);

#endif
