#include <ultra64.h>
#include <bondconstants.h>
#include "levelmodifiers.h"
#include "stan.h"
#include "bgfog.h"
#include "bondview.h"
#include "prop.h"

extern s32 gamemode;
s32 get_scenario(void);

extern s32 gptr_stan;

typedef struct LevelModifierPatchRun
{
    u32 offset;
    u16 length;
    u16 dataOffset;
} LevelModifierPatchRun;

/*
 * V89 Silo Beta Vent restoration.  These bytes are the exact delta between
 * an unmodified Silo RAM dump and a dump with the historical Start In Beta
 * Vent GameShark restoration enabled.  The original cheat wrote absolute
 * 0x801Bxxxx addresses; V89 expresses the same changes relative to the loaded
 * Tbg_silo_all_p_stanZ asset, so the source no longer depends on a RAM address.
 *
 * The generated retail STAN binary was verified byte-for-byte against the
 * unmodified dump at every patched offset before these runs were derived.
 */
static const u8 g_SiloBetaVentPatchData[] = {
    0x27, 0x24, 0x27, 0x28, 0x21, 0xea, 0xfe, 0xe8, 0x08, 0xe8, 0x21, 0x86, 0xfe, 0xe8, 0x08, 0xe8,
    0x27, 0x04, 0xfe, 0xe8, 0x08, 0x28, 0x16, 0x14, 0xfe, 0xe8, 0xfc, 0x08, 0x05, 0x28, 0x21, 0xee,
    0xfe, 0xe8, 0xfc, 0x08, 0x04, 0xe8, 0x08, 0xfe, 0x12, 0xfc, 0x08, 0x05, 0x28, 0x00, 0x00, 0x16,
    0x13, 0xfe, 0xe8, 0xfc, 0x08, 0x04, 0xe8, 0xfd, 0xc0, 0xfc, 0x08, 0x04, 0xe8, 0x0c, 0xfe, 0x12,
    0xfc, 0x08, 0x05, 0x28, 0x16, 0x14, 0xfe, 0x12, 0xfc, 0x08, 0x05, 0x28, 0x08, 0xfd, 0xc0, 0xfc,
    0x08, 0x04, 0xe8, 0xfd, 0xc0, 0xfc, 0x08, 0x05, 0x5c, 0x10, 0x16, 0x14, 0xfe, 0x12, 0xfc, 0x08,
    0x05, 0x28, 0x27, 0x0c, 0xfd, 0xc0, 0xfc, 0x08, 0x05, 0x5c, 0x14, 0xfe, 0x12, 0xfc, 0x08, 0x05,
    0x5c, 0x00, 0x00, 0x17, 0xfe, 0x12, 0xfc, 0x08, 0x05, 0x5c, 0x10, 0xfd, 0xc0, 0xfc, 0x08, 0x05,
    0x5c, 0xfd, 0x86, 0x05, 0x5c, 0x18, 0x17, 0xfe, 0x12, 0xfc, 0x08, 0x05, 0x5c, 0x27, 0x14, 0xfd,
    0x86, 0x05, 0x5c, 0x1c, 0xfe, 0x3e, 0x05, 0x5c, 0x00, 0x00, 0x17, 0xfd, 0x86, 0x05, 0x5c, 0xfe,
    0x3e, 0x06, 0x10, 0xfe, 0x3e, 0x05, 0x5c, 0x18, 0x17, 0xfd, 0x86, 0x05, 0x5c, 0xfd, 0x86, 0x06,
    0x10, 0x00, 0x00, 0xfe, 0x3e, 0x06, 0x10, 0x19, 0x33, 0xff, 0xd1, 0xfc, 0x85, 0x04, 0x79, 0x09,
    0xd6, 0x04, 0x7f, 0x00, 0x00, 0x0b, 0xfc, 0x85, 0x04, 0x79, 0x20, 0x0a, 0x19, 0x33, 0xff, 0xd1,
    0xfc, 0x85, 0x04, 0x79, 0xff, 0xd2, 0xd6, 0x04, 0x7f, 0x20, 0xbe, 0x09, 0xd6, 0x04, 0x7f,
};

static const LevelModifierPatchRun g_SiloBetaVentPatchRuns[] = {
    {0x1001a, 2, 0},
    {0x105aa, 2, 2},
    {0x10bea, 2, 4},
    {0x10f04, 2, 6},
    {0x10f07, 1, 8},
    {0x10f09, 1, 9},
    {0x10f12, 2, 10},
    {0x10f24, 2, 12},
    {0x10f27, 1, 14},
    {0x10f29, 5, 15},
    {0x10f2f, 1, 20},
    {0x10f31, 1, 21},
    {0x137cf, 2, 22},
    {0x137d4, 14, 24},
    {0x137e3, 9, 38},
    {0x137ef, 2, 47},
    {0x137f4, 6, 49},
    {0x137fc, 6, 55},
    {0x13803, 7, 61},
    {0x1380f, 2, 68},
    {0x13814, 6, 70},
    {0x1381b, 7, 76},
    {0x13824, 6, 83},
    {0x1382b, 1, 89},
    {0x1382f, 2, 90},
    {0x13834, 14, 92},
    {0x13843, 9, 106},
    {0x1384f, 1, 115},
    {0x13854, 6, 116},
    {0x1385b, 7, 122},
    {0x13864, 2, 129},
    {0x13868, 2, 131},
    {0x1386b, 1, 133},
    {0x1386f, 1, 134},
    {0x13874, 10, 135},
    {0x13880, 2, 145},
    {0x13883, 3, 147},
    {0x13888, 4, 150},
    {0x1388f, 1, 154},
    {0x13894, 2, 155},
    {0x13898, 2, 157},
    {0x1389c, 2, 159},
    {0x138a0, 2, 161},
    {0x138a4, 2, 163},
    {0x138a8, 2, 165},
    {0x138ab, 1, 167},
    {0x138af, 1, 168},
    {0x138b4, 2, 169},
    {0x138b8, 2, 171},
    {0x138bc, 2, 173},
    {0x138c0, 6, 175},
    {0x138c8, 2, 181},
    {0x138cf, 2, 183},
    {0x138d4, 6, 185},
    {0x138dd, 1, 191},
    {0x138df, 5, 192},
    {0x138e5, 7, 197},
    {0x138ef, 2, 204},
    {0x138f4, 6, 206},
    {0x138fc, 2, 212},
    {0x138ff, 5, 214},
    {0x13905, 1, 219},
    {0x13907, 3, 220},
};

static const char *g_LevelModifierSpNames[] = {
    "Dam",
    "Facility",
    "Runway",
    "Surface 1",
    "Bunker 1",
    "Silo",
    "Frigate",
    "Surface 2",
    "Bunker 2",
    "Statue",
    "Archives",
    "Streets",
    "Depot",
    "Train",
    "Jungle",
    "Control",
    "Caverns",
    "Cradle",
    "Aztec",
    "Egyptian",
};

static const char *g_LevelModifierMpNames[] = {
    "Temple",
    "Complex",
    "Caves",
    "Library",
    "Basement",
    "Stack",
    "Facility (MP)",
    "Bunker (MP)",
    "Statue (MP)",
    "Archives (MP)",
    "Caverns (MP)",
    "Cradle (MP)",
    "Egyptian (MP)",
};

static const char *g_LevelModifierMiscNames[] = {
    "Cuba",
    "Citadel",
};

static s32 g_LevelModifierCurrentStage = LEVELID_NONE;

/*
 * R27S_FACILITY_WALK_THROUGH_VENT_GRATE
 *
 * Supplied US Facility level-load RDRAM:
 *   gptr_stan @ 0x8007BF94 = 0x801A1960
 *   GS 801B3F87 000E -> STAN + 0x12627: 0x0D -> 0x0E
 *   GS 801B3F94 00FF -> STAN + 0x12634: 0xFB -> 0xFF
 *
 * Express the cheat relative to the loaded Facility STAN, never as fixed RAM.
 * The original bytes are retained so OFF is a true live reversal.
 */
#define FACILITY_VENT_GRATE_STAN_OFFSET_A 0x12627
#define FACILITY_VENT_GRATE_STAN_OFFSET_B 0x12634
#define FACILITY_VENT_GRATE_RETAIL_A      0x0d
#define FACILITY_VENT_GRATE_RETAIL_B      0xfb
#define FACILITY_VENT_GRATE_OPEN_A        0x0e
#define FACILITY_VENT_GRATE_OPEN_B        0xff

static s32 g_LevelModifierFacilityVentGratePreload = FALSE;
static s32 g_LevelModifierFacilityVentGrateActive = FALSE;
static s32 g_LevelModifierCradleKillPlanePreload = FALSE;
static s32 g_LevelModifierCradleKillPlaneActive = FALSE;

/* R27S_R2_MULTI_LEVEL_BETA_MODIFIERS
 * Dump-decoded historical restoration writes.  Setup edits are expressed
 * relative to the loaded setup propDefs (or symbolic PadRecord entries), not
 * as fixed 0x801xxxxx addresses.  OFF restores the exact clean-dump bytes.
 */
static s32 g_LevelModifierFacilityBetaDoorsPreload = FALSE;
static s32 g_LevelModifierFacilityBetaDoorsActive = FALSE;
static s32 g_LevelModifierFacilityStackedTanksPreload = FALSE;
static s32 g_LevelModifierFacilityStackedTanksActive = FALSE;
static s32 g_LevelModifierSurfaceWhiteSkyPreload = FALSE;
static s32 g_LevelModifierSurfaceWhiteSkyActive = FALSE;
static s32 g_LevelModifierSurfaceBetaDoorsPreload = FALSE;
static s32 g_LevelModifierSurfaceBetaDoorsActive = FALSE;
static s32 g_LevelModifierBunker1BetaDoorsPreload = FALSE;
static s32 g_LevelModifierBunker1BetaDoorsActive = FALSE;
static s32 g_LevelModifierFrigateKeepClearDoorsPreload = FALSE;
static s32 g_LevelModifierFrigateKeepClearDoorsActive = FALSE;
static s32 g_LevelModifierFrigateRestoreRemovedDoorsPreload = FALSE;
static s32 g_LevelModifierFrigateRestoreRemovedDoorsActive = FALSE;

/* R27S_R4_TEMPLE_COMPLEX_FRIGATE_MODIFIERS
 * Historical GameShark effects reconstructed semantically from clean US
 * level-load dumps. Collision writes are relative to each loaded STAN image;
 * OFF restores the exact clean-dump halfwords. No fixed RDRAM addresses are
 * used at runtime.
 */
static s32 g_LevelModifierTempleFallPreload = FALSE;
static s32 g_LevelModifierTempleFallActive = FALSE;
static s32 g_LevelModifierTempleBetterRespawningPreload = FALSE;
static s32 g_LevelModifierTempleBetterRespawningActive = FALSE;
static s32 g_LevelModifierTempleBetterRespawningApplied = FALSE;
static s32 g_LevelModifierComplexFallPreload = FALSE;
static s32 g_LevelModifierComplexFallActive = FALSE;
static s32 g_LevelModifierFrigatePipesPreload = FALSE;
static s32 g_LevelModifierFrigatePipesActive = FALSE;

static void levelModifiersWriteStanHalf(u8 *stan, u32 offset, u16 value)
{
    stan[offset] = (u8)(value >> 8);
    stan[offset + 1] = (u8)value;
}

static void levelModifiersApplyTempleFallStan(u8 *stan, s32 enabled)
{
    if (stan == NULL) return;
    levelModifiersWriteStanHalf(stan, 0x032a, enabled ? 0x02c0 : 0x0000);
    levelModifiersWriteStanHalf(stan, 0x0b0a, enabled ? 0x0258 : 0x0000);
    levelModifiersWriteStanHalf(stan, 0x0b4a, enabled ? 0x0258 : 0x0000);
    levelModifiersWriteStanHalf(stan, 0x0b8a, enabled ? 0x0258 : 0x0000);
    levelModifiersWriteStanHalf(stan, 0x0be2, enabled ? 0x0258 : 0x0000);
}

static void levelModifiersApplyComplexFallStan(u8 *stan, s32 enabled)
{
    if (stan == NULL) return;
    levelModifiersWriteStanHalf(stan, 0x0452, enabled ? 0x0398 : 0x0000);
    levelModifiersWriteStanHalf(stan, 0x0492, enabled ? 0x0398 : 0x0000);
    levelModifiersWriteStanHalf(stan, 0x058a, enabled ? 0x0398 : 0x0000);
    levelModifiersWriteStanHalf(stan, 0x05b2, enabled ? 0x0398 : 0x0000);
    levelModifiersWriteStanHalf(stan, 0x05ca, enabled ? 0x0398 : 0x0000);
    levelModifiersWriteStanHalf(stan, 0x05ea, enabled ? 0x0398 : 0x0000);
    levelModifiersWriteStanHalf(stan, 0x0602, enabled ? 0x0398 : 0x0000);
    levelModifiersWriteStanHalf(stan, 0x1fc2, enabled ? 0x0470 : 0x0000);
    levelModifiersWriteStanHalf(stan, 0x1ffa, enabled ? 0x0470 : 0x0000);
    levelModifiersWriteStanHalf(stan, 0x2002, enabled ? 0x0470 : 0x0000);
}

static void levelModifiersApplyFrigatePipesStan(u8 *stan, s32 enabled)
{
    if (stan == NULL) return;
    levelModifiersWriteStanHalf(stan, 0xb994, enabled ? 0x0085 : 0x0095);
    levelModifiersWriteStanHalf(stan, 0xb998, enabled ? 0xfb93 : 0xfb86);
    levelModifiersWriteStanHalf(stan, 0xb9aa, enabled ? 0x1717 : 0x0000);
    levelModifiersWriteStanHalf(stan, 0xb9b4, enabled ? 0x0085 : 0x0095);
    levelModifiersWriteStanHalf(stan, 0xb9b8, enabled ? 0xfb93 : 0xfb86);
    levelModifiersWriteStanHalf(stan, 0xba64, enabled ? 0x0085 : 0x0095);
    levelModifiersWriteStanHalf(stan, 0xba68, enabled ? 0xfb93 : 0xfb86);
    levelModifiersWriteStanHalf(stan, 0xba6a, enabled ? 0x173b : 0x0000);
    levelModifiersWriteStanHalf(stan, 0xb862, enabled ? 0x173f : 0x0000);
    levelModifiersWriteStanHalf(stan, 0xb97a, enabled ? 0x1758 : 0x0000);
    levelModifiersWriteStanHalf(stan, 0xb964, enabled ? 0x0081 : 0x0091);
    levelModifiersWriteStanHalf(stan, 0xb968, enabled ? 0xfbad : 0xfb9f);
    levelModifiersWriteStanHalf(stan, 0xb962, enabled ? 0x1707 : 0x0000);
    levelModifiersWriteStanHalf(stan, 0xba1c, enabled ? 0x0081 : 0x0091);
    levelModifiersWriteStanHalf(stan, 0xba20, enabled ? 0xfbad : 0xfb9f);
    levelModifiersWriteStanHalf(stan, 0xba22, enabled ? 0x172f : 0x0000);
    levelModifiersWriteStanHalf(stan, 0xb7e2, enabled ? 0x1737 : 0x0000);
    levelModifiersWriteStanHalf(stan, 0xb91a, enabled ? 0x174f : 0x0000);
}

static s32 levelModifiersSetTempleFall(s32 enabled)
{
    enabled = enabled ? TRUE : FALSE;
    if (g_LevelModifierCurrentStage != LEVELID_TEMPLE || gptr_stan == 0) return FALSE;
    levelModifiersApplyTempleFallStan((u8 *)gptr_stan, enabled);
    stanRebuildRoomData();
    g_LevelModifierTempleFallPreload = enabled;
    g_LevelModifierTempleFallActive = enabled;
    return TRUE;
}

static s32 levelModifiersSetTempleBetterRespawning(s32 enabled)
{
    enabled = enabled ? TRUE : FALSE;
    if (g_LevelModifierCurrentStage != LEVELID_TEMPLE) return FALSE;

    if ((enabled && !g_LevelModifierTempleBetterRespawningApplied)
        || (!enabled && g_LevelModifierTempleBetterRespawningApplied))
    {
        if (!bondviewLevelModifierSwapTempleRespawnPairs()) return FALSE;
        g_LevelModifierTempleBetterRespawningApplied = enabled;
    }

    g_LevelModifierTempleBetterRespawningPreload = enabled;
    g_LevelModifierTempleBetterRespawningActive = enabled;
    return TRUE;
}

static s32 levelModifiersSetComplexFall(s32 enabled)
{
    enabled = enabled ? TRUE : FALSE;
    if (g_LevelModifierCurrentStage != LEVELID_COMPLEX || gptr_stan == 0) return FALSE;
    levelModifiersApplyComplexFallStan((u8 *)gptr_stan, enabled);
    stanRebuildRoomData();
    g_LevelModifierComplexFallPreload = enabled;
    g_LevelModifierComplexFallActive = enabled;
    return TRUE;
}

static s32 levelModifiersSetFrigatePipes(s32 enabled)
{
    enabled = enabled ? TRUE : FALSE;
    if (g_LevelModifierCurrentStage != LEVELID_FRIGATE || gptr_stan == 0) return FALSE;
    levelModifiersApplyFrigatePipesStan((u8 *)gptr_stan, enabled);
    stanRebuildRoomData();
    g_LevelModifierFrigatePipesPreload = enabled;
    g_LevelModifierFrigatePipesActive = enabled;
    return TRUE;
}


/* R27S_R4_R1_R1_MCM_HOLE_ROOM_TP_SOLO_MP
 * Temple/Complex "fall into hole" links are intentional cross-room STAN
 * adjacencies supplied by the historical Zoinkity codes. They can move the
 * collision tile into another room without crossing a retail BG portal.
 * Only while either modifier is active may presentation-room ownership fall
 * back to the authoritative current collision tile. */
s32 levelModifiersUseCurrentStanRoomForHoleTraversal(void)
{
    return (g_LevelModifierCurrentStage == LEVELID_TEMPLE
            && g_LevelModifierTempleFallActive)
        || (g_LevelModifierCurrentStage == LEVELID_COMPLEX
            && g_LevelModifierComplexFallActive);
}

void levelModifiersOnStartPadsLoaded(void)
{
    g_LevelModifierTempleBetterRespawningApplied = FALSE;
    if (g_LevelModifierCurrentStage == LEVELID_TEMPLE
        && g_LevelModifierTempleBetterRespawningActive
        && bondviewLevelModifierSwapTempleRespawnPairs())
    {
        g_LevelModifierTempleBetterRespawningApplied = TRUE;
    }
}


extern s32 propLevelModifierApplyBetaSetupPatches(s32 levelid,
    s32 facilityDoors, s32 facilityTanks, s32 surfaceDoors,
    s32 bunkerDoors, s32 frigateKeepClear, s32 frigateRemoved);

static s32 levelModifiersApplySurfaceWhiteSky(s32 enabled)
{
    CurrentEnvironmentRecord *environment;
    if (g_LevelModifierCurrentStage != LEVELID_SURFACE)
        return FALSE;
    environment = fogGetCurrentEnvironmentp();
    if (environment == NULL)
        return FALSE;
    /* Historical 810452E4 C0C0 / 810452E6 C001 changes the Surface 1
     * environment's packed sky RGB/cloud bytes 60 60 80 01 -> C0 C0 C0 01. */
    environment->Red   = enabled ? 0xc0 : 0x60;
    environment->Green = enabled ? 0xc0 : 0x60;
    environment->Blue  = enabled ? 0xc0 : 0x80;
    g_LevelModifierSurfaceWhiteSkyPreload = enabled ? TRUE : FALSE;
    g_LevelModifierSurfaceWhiteSkyActive = enabled ? TRUE : FALSE;
    return TRUE;
}

static s32 levelModifiersApplyCurrentSetupStates(void)
{
    return propLevelModifierApplyBetaSetupPatches(g_LevelModifierCurrentStage,
        g_LevelModifierFacilityBetaDoorsActive,
        g_LevelModifierFacilityStackedTanksActive,
        g_LevelModifierSurfaceBetaDoorsActive,
        g_LevelModifierBunker1BetaDoorsActive,
        g_LevelModifierFrigateKeepClearDoorsActive,
        g_LevelModifierFrigateRestoreRemovedDoorsActive);
}


static void levelModifiersApplyFacilityVentGrateStan(u8 *stan, s32 enabled)
{
    if (stan == NULL)
        return;

    stan[FACILITY_VENT_GRATE_STAN_OFFSET_A] =
        enabled ? FACILITY_VENT_GRATE_OPEN_A : FACILITY_VENT_GRATE_RETAIL_A;
    stan[FACILITY_VENT_GRATE_STAN_OFFSET_B] =
        enabled ? FACILITY_VENT_GRATE_OPEN_B : FACILITY_VENT_GRATE_RETAIL_B;
}

static s32 levelModifiersSetFacilityVentGrate(s32 enabled)
{
    enabled = enabled ? TRUE : FALSE;

    if (g_LevelModifierCurrentStage != LEVELID_FACILITY || gptr_stan == 0)
        return FALSE;

    levelModifiersApplyFacilityVentGrateStan((u8 *)gptr_stan, enabled);
    stanRebuildRoomData();

    g_LevelModifierFacilityVentGratePreload = enabled;
    g_LevelModifierFacilityVentGrateActive = enabled;
    return TRUE;
}


static s32 g_LevelModifierSiloBetaVentPreload = FALSE;
static s32 g_LevelModifierSiloBetaVentActive = FALSE;
static s32 g_LevelModifierCitadelWaterPreload = FALSE;
static s32 g_LevelModifierCitadelWaterActive = FALSE;

/* R27Q_DAM_DOCK_RESTORATION
 *
 * Dam has two reversible restorations:
 *   0: Restore Doors Near Dam Docks
 *   1: Restore Speedboat
 *
 * Preload is the frontend selection. Active describes the live stage.
 * Runtime availability is only asserted after the campaign Dam setup has
 * supplied the preserved donor records and the unused boat pad has been
 * repaired semantically.
 */
static s32 g_LevelModifierDamDoorsPreload = FALSE;
static s32 g_LevelModifierDamDoorsActive = FALSE;
static s32 g_LevelModifierDamSpeedboatPreload = FALSE;
static s32 g_LevelModifierDamSpeedboatActive = FALSE;
/* R27R_DRIVABLE_SPEEDBOAT
 * This modifier is intentionally latched once active. Destroying a live
 * vehicle while Bond is driving or standing on it is unsafe. */
static s32 g_LevelModifierDamDrivableSpeedboatPreload = FALSE;
static s32 g_LevelModifierDamDrivableSpeedboatActive = FALSE;
static s32 g_LevelModifierDamRuntimeAvailable = FALSE;


static void levelModifiersApplySiloBetaVentStan(u8 *stan)
{
    s32 i;
    s32 j;
    const LevelModifierPatchRun *run;

    if (stan == NULL)
        return;

    for (i = 0; i < (s32)(sizeof(g_SiloBetaVentPatchRuns) / sizeof(g_SiloBetaVentPatchRuns[0])); i++)
    {
        run = &g_SiloBetaVentPatchRuns[i];
        for (j = 0; j < run->length; j++)
            stan[run->offset + j] = g_SiloBetaVentPatchData[run->dataOffset + j];
    }
}

void levelModifiersOnStanLoaded(s32 levelid, u8 *stan)
{
    g_LevelModifierTempleFallActive = FALSE;
    g_LevelModifierTempleBetterRespawningActive = FALSE;
    g_LevelModifierTempleBetterRespawningApplied = FALSE;
    g_LevelModifierComplexFallActive = FALSE;
    g_LevelModifierFrigatePipesActive = FALSE;

    g_LevelModifierFacilityVentGrateActive = FALSE;
    g_LevelModifierCurrentStage = levelid;
    g_LevelModifierCradleKillPlaneActive = FALSE;
    g_LevelModifierSiloBetaVentActive = FALSE;
    g_LevelModifierCitadelWaterActive = FALSE;
    g_LevelModifierDamDoorsActive = FALSE;
    g_LevelModifierDamSpeedboatActive = FALSE;
    g_LevelModifierDamDrivableSpeedboatActive = FALSE;
    g_LevelModifierDamRuntimeAvailable = FALSE;

    if (levelid == LEVELID_SILO && g_LevelModifierSiloBetaVentPreload)
    {
        g_LevelModifierSiloBetaVentActive = TRUE;
        levelModifiersApplySiloBetaVentStan(stan);
    }

    if (levelid == LEVELID_FACILITY)
    {
        g_LevelModifierFacilityVentGrateActive =
            g_LevelModifierFacilityVentGratePreload ? TRUE : FALSE;
        levelModifiersApplyFacilityVentGrateStan(
            stan, g_LevelModifierFacilityVentGrateActive);
    }

    if (levelid == LEVELID_TEMPLE)
    {
        g_LevelModifierTempleFallActive = g_LevelModifierTempleFallPreload;
        g_LevelModifierTempleBetterRespawningActive = g_LevelModifierTempleBetterRespawningPreload;
        levelModifiersApplyTempleFallStan(stan, g_LevelModifierTempleFallActive);
    }
    else if (levelid == LEVELID_COMPLEX)
    {
        g_LevelModifierComplexFallActive = g_LevelModifierComplexFallPreload;
        levelModifiersApplyComplexFallStan(stan, g_LevelModifierComplexFallActive);
    }
    else if (levelid == LEVELID_FRIGATE)
    {
        g_LevelModifierFrigatePipesActive = g_LevelModifierFrigatePipesPreload;
        levelModifiersApplyFrigatePipesStan(stan, g_LevelModifierFrigatePipesActive);
    }

    if (levelid == LEVELID_CRADLE
        && gamemode == GAMEMODE_MULTI
        && get_scenario() != SCENARIO_COOP
        && g_LevelModifierCradleKillPlanePreload)
    {
        g_LevelModifierCradleKillPlaneActive = TRUE;
    }
}

s32 levelModifiersGetCurrentStage(void)
{
    return g_LevelModifierCurrentStage;
}

const char *levelModifiersGetCurrentStageName(void)
{
    switch (g_LevelModifierCurrentStage)
    {
        case LEVELID_DAM: return "DAM";
        case LEVELID_FACILITY: return "FACILITY";
        case LEVELID_RUNWAY: return "RUNWAY";
        case LEVELID_SURFACE: return "SURFACE 1";
        case LEVELID_BUNKER1: return "BUNKER 1";
        case LEVELID_SILO: return "SILO";
        case LEVELID_FRIGATE: return "FRIGATE";
        case LEVELID_SURFACE2: return "SURFACE 2";
        case LEVELID_BUNKER2: return "BUNKER 2";
        case LEVELID_STATUE: return "STATUE";
        case LEVELID_ARCHIVES: return "ARCHIVES";
        case LEVELID_STREETS: return "STREETS";
        case LEVELID_DEPOT: return "DEPOT";
        case LEVELID_TRAIN: return "TRAIN";
        case LEVELID_JUNGLE: return "JUNGLE";
        case LEVELID_CONTROL: return "CONTROL";
        case LEVELID_CAVERNS: return "CAVERNS";
        case LEVELID_CRADLE: return "CRADLE";
        case LEVELID_AZTEC: return "AZTEC";
        case LEVELID_EGYPT: return "EGYPTIAN";
        case LEVELID_TEMPLE: return "TEMPLE";
        case LEVELID_COMPLEX: return "COMPLEX";
        case LEVELID_CAVES: return "CAVES";
        case LEVELID_LIBRARY: return "LIBRARY";
        case LEVELID_BASEMENT: return "BASEMENT";
        case LEVELID_STACK: return "STACK";
        case LEVELID_CUBA: return "CUBA";
        case LEVELID_CITADEL: return "CITADEL";
        default: return "LEVEL";
    }
}

static s32 levelModifiersCradleMpContext(void)
{
    return g_LevelModifierCurrentStage == LEVELID_CRADLE
        && gamemode == GAMEMODE_MULTI
        && get_scenario() != SCENARIO_COOP;
}

s32 levelModifiersGetCurrentStageModifierCount(void)
{
    if (levelModifiersCradleMpContext()) return 1;

    if (g_LevelModifierCurrentStage == LEVELID_DAM)
        return g_LevelModifierDamRuntimeAvailable ? 3 : 0;
    if (g_LevelModifierCurrentStage == LEVELID_SILO)
        return 1;
    if (g_LevelModifierCurrentStage == LEVELID_CITADEL)
        return 1;
        if (g_LevelModifierCurrentStage == LEVELID_FACILITY)
        return 3;
    if (g_LevelModifierCurrentStage == LEVELID_SURFACE) return 2;
    if (g_LevelModifierCurrentStage == LEVELID_BUNKER1) return 1;
    if (g_LevelModifierCurrentStage == LEVELID_FRIGATE) return 3;
    if (g_LevelModifierCurrentStage == LEVELID_TEMPLE) return 2;
    if (g_LevelModifierCurrentStage == LEVELID_COMPLEX) return 1;
return 0;
}

s32 levelModifiersAdjustStartPadIndex(s32 originalIndex, s32 startPadSlot)
{
    /* The historical cheat changed P1's g_Startpad pointer from Silo pad 209
     * to preserved beta pad 230.  Do that semantically instead of editing the
     * low half of the runtime pointer. */
    if (g_LevelModifierCurrentStage == LEVELID_SILO
        && g_LevelModifierSiloBetaVentActive
        && startPadSlot == 0)
    {
        return 230;
    }

    return originalIndex;
}

s32 levelModifiersGetLevelCount(s32 category)
{
    if (category == LEVELMOD_CATEGORY_SINGLE_PLAYER)
        return sizeof(g_LevelModifierSpNames) / sizeof(g_LevelModifierSpNames[0]);
    if (category == LEVELMOD_CATEGORY_MULTIPLAYER)
        return sizeof(g_LevelModifierMpNames) / sizeof(g_LevelModifierMpNames[0]);
    if (category == LEVELMOD_CATEGORY_MISCELLANEOUS)
        return sizeof(g_LevelModifierMiscNames) / sizeof(g_LevelModifierMiscNames[0]);
    return 0;
}

const char *levelModifiersGetLevelName(s32 category, s32 index)
{
    if (index < 0 || index >= levelModifiersGetLevelCount(category))
        return "";
    if (category == LEVELMOD_CATEGORY_SINGLE_PLAYER)
        return g_LevelModifierSpNames[index];
    if (category == LEVELMOD_CATEGORY_MULTIPLAYER)
        return g_LevelModifierMpNames[index];
    if (category == LEVELMOD_CATEGORY_MISCELLANEOUS)
        return g_LevelModifierMiscNames[index];
    return "";
}

s32 levelModifiersLevelImplemented(s32 category, s32 index)
{
    if (category == LEVELMOD_CATEGORY_MULTIPLAYER && index == 11) return TRUE; /* Cradle (MP) */

    if (category == LEVELMOD_CATEGORY_SINGLE_PLAYER && index == 0) /* Dam */
        return TRUE;
    if (category == LEVELMOD_CATEGORY_SINGLE_PLAYER && index == 5) /* Silo */
        return TRUE;
    if (category == LEVELMOD_CATEGORY_MISCELLANEOUS && index == 1) /* Citadel */
        return TRUE;
        if (category == LEVELMOD_CATEGORY_SINGLE_PLAYER && index == 1) /* Facility */
        return TRUE;
    if (category == LEVELMOD_CATEGORY_SINGLE_PLAYER && (index == 3 || index == 4 || index == 6)) return TRUE;
    if (category == LEVELMOD_CATEGORY_MULTIPLAYER && (index == 0 || index == 1)) return TRUE;
return FALSE;
}

s32 levelModifiersLevelAvailableInCurrentStage(s32 category, s32 index)
{
    if (category == LEVELMOD_CATEGORY_MULTIPLAYER && index == 11) return levelModifiersCradleMpContext();

    if (category == LEVELMOD_CATEGORY_SINGLE_PLAYER && index == 0)
        return g_LevelModifierCurrentStage == LEVELID_DAM && g_LevelModifierDamRuntimeAvailable;
    if (category == LEVELMOD_CATEGORY_SINGLE_PLAYER && index == 5)
        return g_LevelModifierCurrentStage == LEVELID_SILO;
    if (category == LEVELMOD_CATEGORY_MISCELLANEOUS && index == 1)
        return g_LevelModifierCurrentStage == LEVELID_CITADEL;
        if (category == LEVELMOD_CATEGORY_SINGLE_PLAYER && index == 1)
        return g_LevelModifierCurrentStage == LEVELID_FACILITY;
    if (category == LEVELMOD_CATEGORY_SINGLE_PLAYER && index == 3) return g_LevelModifierCurrentStage == LEVELID_SURFACE;
    if (category == LEVELMOD_CATEGORY_SINGLE_PLAYER && index == 4) return g_LevelModifierCurrentStage == LEVELID_BUNKER1;
    if (category == LEVELMOD_CATEGORY_SINGLE_PLAYER && index == 6) return g_LevelModifierCurrentStage == LEVELID_FRIGATE;
    if (category == LEVELMOD_CATEGORY_MULTIPLAYER && index == 0) return g_LevelModifierCurrentStage == LEVELID_TEMPLE;
    if (category == LEVELMOD_CATEGORY_MULTIPLAYER && index == 1) return g_LevelModifierCurrentStage == LEVELID_COMPLEX;
return FALSE;
}

s32 levelModifiersGetSiloBetaVentPreload(void)
{
    return g_LevelModifierSiloBetaVentPreload;
}

void levelModifiersSetSiloBetaVentPreload(s32 enabled)
{
    g_LevelModifierSiloBetaVentPreload = enabled ? TRUE : FALSE;
}

s32 levelModifiersSiloBetaVentActive(void)
{
    return g_LevelModifierSiloBetaVentActive;
}

s32 levelModifiersActivateSiloBetaVent(void)
{
    if (g_LevelModifierCurrentStage != LEVELID_SILO)
        return FALSE;

    if (g_LevelModifierSiloBetaVentActive)
        return TRUE;

    g_LevelModifierSiloBetaVentActive = TRUE;
    levelModifiersApplySiloBetaVentStan((u8 *)gptr_stan);
    stanRebuildRoomData();
    return TRUE;
}

LevelModifierPolicy levelModifiersGetSiloBetaVentPolicy(void)
{
    return LEVELMOD_POLICY_LATCHED;
}

s32 levelModifiersCitadelWaterActive(void)
{
    return g_LevelModifierCitadelWaterActive;
}

s32 levelModifiersGetCitadelWaterPreload(void)
{
    return g_LevelModifierCitadelWaterPreload;
}

void levelModifiersSetCitadelWaterPreload(s32 enabled)
{
    g_LevelModifierCitadelWaterPreload = enabled ? TRUE : FALSE;
}

s32 levelModifiersSetCitadelWater(s32 enabled)
{
    CurrentEnvironmentRecord *environment;

    if (g_LevelModifierCurrentStage != LEVELID_CITADEL)
        return FALSE;

    environment = fogGetCurrentEnvironmentp();
    if (environment == NULL)
        return FALSE;

    /*
     * Historical Citadel "Water" GameShark code patched the live
     * CurrentEnvironmentRecord at 0x80044DCC. The WATER_OFF/WATER_ON RAM
     * comparison shows that Plus already carries the intended Citadel water
     * plane parameters (sea height, image and colour); the meaningful live
     * environment change is IsWater 0 -> 1.
     *
     * The cheat's final writes at 0x80044E08/0C/10 were constructing a
     * Citadel fog-table header (ID 0xF0, blend 10.0, far fog 20000.0) for a
     * build which did not have a proper Citadel environment entry. Plus V90
     * already has dedicated 1P-4P Citadel environment records, so reproducing
     * those raw adjacent writes would corrupt unrelated table data.
     */
    environment->IsWater = enabled ? 1 : 0;
    g_LevelModifierCitadelWaterPreload = enabled ? TRUE : FALSE;
    g_LevelModifierCitadelWaterActive = enabled ? TRUE : FALSE;
    return TRUE;
}

void levelModifiersOnEnvironmentLoaded(s32 levelid)
{
    CurrentEnvironmentRecord *environment;

    g_LevelModifierCitadelWaterActive = FALSE;

    if (levelid != LEVELID_CITADEL || !g_LevelModifierCitadelWaterPreload)
        return;

    environment = fogGetCurrentEnvironmentp();
    if (environment == NULL)
        return;

    environment->IsWater = 1;
    g_LevelModifierCitadelWaterActive = TRUE;

    g_LevelModifierSurfaceWhiteSkyActive = FALSE;
    if (levelid == LEVELID_SURFACE && g_LevelModifierSurfaceWhiteSkyPreload)
    {
        CurrentEnvironmentRecord *environment = fogGetCurrentEnvironmentp();
        if (environment != NULL)
        {
            environment->Red = 0xc0;
            environment->Green = 0xc0;
            environment->Blue = 0xc0;
            g_LevelModifierSurfaceWhiteSkyActive = TRUE;
        }
    }
}

LevelModifierPolicy levelModifiersGetCitadelWaterPolicy(void)
{
    return LEVELMOD_POLICY_REVERSIBLE;
}

/* -------------------------------------------------------------------------
 * R27Q Dam dock restorations
 * ------------------------------------------------------------------------- */

void levelModifiersOnSetupReady(s32 levelid)
{
    g_LevelModifierDamRuntimeAvailable = FALSE;
    g_LevelModifierDamDoorsActive = FALSE;
    g_LevelModifierDamSpeedboatActive = FALSE;

    if (levelid == LEVELID_DAM)
        g_LevelModifierDamRuntimeAvailable = propLevelModifierPrepareDamRestorations(levelid);

    if (levelid == LEVELID_FACILITY)
    {
        g_LevelModifierFacilityBetaDoorsActive = g_LevelModifierFacilityBetaDoorsPreload;
        g_LevelModifierFacilityStackedTanksActive = g_LevelModifierFacilityStackedTanksPreload;
    }
    else if (levelid == LEVELID_SURFACE)
        g_LevelModifierSurfaceBetaDoorsActive = g_LevelModifierSurfaceBetaDoorsPreload;
    else if (levelid == LEVELID_BUNKER1)
        g_LevelModifierBunker1BetaDoorsActive = g_LevelModifierBunker1BetaDoorsPreload;
    else if (levelid == LEVELID_FRIGATE)
    {
        g_LevelModifierFrigateKeepClearDoorsActive = g_LevelModifierFrigateKeepClearDoorsPreload;
        g_LevelModifierFrigateRestoreRemovedDoorsActive = g_LevelModifierFrigateRestoreRemovedDoorsPreload;
    }

    propLevelModifierApplyBetaSetupPatches(levelid,
        g_LevelModifierFacilityBetaDoorsActive,
        g_LevelModifierFacilityStackedTanksActive,
        g_LevelModifierSurfaceBetaDoorsActive,
        g_LevelModifierBunker1BetaDoorsActive,
        g_LevelModifierFrigateKeepClearDoorsActive,
        g_LevelModifierFrigateRestoreRemovedDoorsActive);
}

s32 levelModifiersGetReservedObjectCount(s32 levelid)
{
    if (levelid == LEVELID_DAM && g_LevelModifierDamRuntimeAvailable)
        return 4; /* two restored doors + static speedboat + drivable speedboat */

    return 0;
}

void levelModifiersOnPropsLoaded(s32 levelid)
{
    if (levelid != LEVELID_DAM || !g_LevelModifierDamRuntimeAvailable)
        return;

    if (g_LevelModifierDamDoorsPreload
        && propLevelModifierSetDamDoors(TRUE))
    {
        g_LevelModifierDamDoorsActive = TRUE;
    }

    if (g_LevelModifierDamSpeedboatPreload
        && propLevelModifierSetDamSpeedboat(TRUE))
    {
        g_LevelModifierDamSpeedboatActive = TRUE;
    }

    if (g_LevelModifierDamDrivableSpeedboatPreload
        && propLevelModifierActivateDamDrivableSpeedboat())
    {
        g_LevelModifierDamDrivableSpeedboatActive = TRUE;
    }
}

void levelModifiersOnStageCleanup(s32 levelid)
{
    g_LevelModifierTempleFallActive = FALSE;
    g_LevelModifierTempleBetterRespawningActive = FALSE;
    g_LevelModifierTempleBetterRespawningApplied = FALSE;
    g_LevelModifierComplexFallActive = FALSE;
    g_LevelModifierFrigatePipesActive = FALSE;

    g_LevelModifierFacilityVentGrateActive = FALSE;
    if (levelid == LEVELID_DAM || g_LevelModifierDamRuntimeAvailable)
        propLevelModifierCleanupDamRestorations();

    g_LevelModifierDamDoorsActive = FALSE;
    g_LevelModifierDamSpeedboatActive = FALSE;
    g_LevelModifierDamRuntimeAvailable = FALSE;
}

s32 levelModifiersDamDoorsActive(void)
{
    return g_LevelModifierDamDoorsActive;
}

s32 levelModifiersGetDamDoorsPreload(void)
{
    return g_LevelModifierDamDoorsPreload;
}

void levelModifiersSetDamDoorsPreload(s32 enabled)
{
    g_LevelModifierDamDoorsPreload = enabled ? TRUE : FALSE;
}

s32 levelModifiersSetDamDoors(s32 enabled)
{
    enabled = enabled ? TRUE : FALSE;

    if (g_LevelModifierCurrentStage != LEVELID_DAM
        || !g_LevelModifierDamRuntimeAvailable)
    {
        return FALSE;
    }

    if (g_LevelModifierDamDoorsActive == enabled)
    {
        g_LevelModifierDamDoorsPreload = enabled;
        return TRUE;
    }

    if (!propLevelModifierSetDamDoors(enabled))
        return FALSE;

    g_LevelModifierDamDoorsPreload = enabled;
    g_LevelModifierDamDoorsActive = enabled;
    return TRUE;
}

LevelModifierPolicy levelModifiersGetDamDoorsPolicy(void)
{
    return LEVELMOD_POLICY_REVERSIBLE;
}

s32 levelModifiersDamSpeedboatActive(void)
{
    return g_LevelModifierDamSpeedboatActive;
}

s32 levelModifiersGetDamSpeedboatPreload(void)
{
    return g_LevelModifierDamSpeedboatPreload;
}

void levelModifiersSetDamSpeedboatPreload(s32 enabled)
{
    g_LevelModifierDamSpeedboatPreload = enabled ? TRUE : FALSE;
}

s32 levelModifiersSetDamSpeedboat(s32 enabled)
{
    enabled = enabled ? TRUE : FALSE;

    if (g_LevelModifierCurrentStage != LEVELID_DAM
        || !g_LevelModifierDamRuntimeAvailable)
    {
        return FALSE;
    }

    if (g_LevelModifierDamSpeedboatActive == enabled)
    {
        g_LevelModifierDamSpeedboatPreload = enabled;
        return TRUE;
    }

    if (!propLevelModifierSetDamSpeedboat(enabled))
        return FALSE;

    g_LevelModifierDamSpeedboatPreload = enabled;
    g_LevelModifierDamSpeedboatActive = enabled;
    return TRUE;
}

LevelModifierPolicy levelModifiersGetDamSpeedboatPolicy(void)
{
    return LEVELMOD_POLICY_REVERSIBLE;
}

s32 levelModifiersDamDrivableSpeedboatActive(void)
{
    return g_LevelModifierDamDrivableSpeedboatActive;
}

s32 levelModifiersGetDamDrivableSpeedboatPreload(void)
{
    return g_LevelModifierDamDrivableSpeedboatPreload;
}

void levelModifiersSetDamDrivableSpeedboatPreload(s32 enabled)
{
    g_LevelModifierDamDrivableSpeedboatPreload = enabled ? TRUE : FALSE;
}

s32 levelModifiersActivateDamDrivableSpeedboat(void)
{
    if (g_LevelModifierCurrentStage != LEVELID_DAM
        || !g_LevelModifierDamRuntimeAvailable)
    {
        return FALSE;
    }

    if (g_LevelModifierDamDrivableSpeedboatActive)
        return FALSE;

    if (!propLevelModifierActivateDamDrivableSpeedboat())
        return FALSE;

    g_LevelModifierDamDrivableSpeedboatPreload = TRUE;
    g_LevelModifierDamDrivableSpeedboatActive = TRUE;
    return TRUE;
}

LevelModifierPolicy levelModifiersGetDamDrivableSpeedboatPolicy(void)
{
    return LEVELMOD_POLICY_LATCHED;
}

const char *levelModifiersGetCurrentStageModifierName(s32 index)
{
    if (levelModifiersCradleMpContext() && index == 0) return "Kill Plane";

    if (levelModifiersGetCurrentStageModifierCount() == 0)
        return "No modifiers available";

    if (g_LevelModifierCurrentStage == LEVELID_DAM)
    {
        if (index == 0)
            return "Restore Doors Near Dam Docks";
        if (index == 1)
            return "Restore Speedboat";
        if (index == 2)
            return "Drivable Speedboat";
    }
    else if (g_LevelModifierCurrentStage == LEVELID_SILO && index == 0)
    {
        return "Beta Vent Start";
    }
    else if (g_LevelModifierCurrentStage == LEVELID_CITADEL && index == 0)
    {
        return "Water";
    }

        if (g_LevelModifierCurrentStage == LEVELID_FACILITY && index == 0)
        return "Walk Through Grate In Vents";

    if (g_LevelModifierCurrentStage == LEVELID_FACILITY && index == 1) return "Restore Beta Facility Doors";
    if (g_LevelModifierCurrentStage == LEVELID_FACILITY && index == 2) return "Restore Beta Facility Stacked Tanks";
    if (g_LevelModifierCurrentStage == LEVELID_SURFACE && index == 0) return "White Sky";
    if (g_LevelModifierCurrentStage == LEVELID_SURFACE && index == 1) return "Beta Doors";
    if (g_LevelModifierCurrentStage == LEVELID_BUNKER1 && index == 0) return "Beta Doors";
    if (g_LevelModifierCurrentStage == LEVELID_FRIGATE && index == 0) return "\"Keep Clear\" Doors Leading Into Engine Room";
    if (g_LevelModifierCurrentStage == LEVELID_FRIGATE && index == 1) return "Restore Removed Doors";

    if (g_LevelModifierCurrentStage == LEVELID_TEMPLE && index == 0) return "Fall Down Into Hole";
    if (g_LevelModifierCurrentStage == LEVELID_TEMPLE && index == 1) return "Better Respawning";
    if (g_LevelModifierCurrentStage == LEVELID_COMPLEX && index == 0) return "Fall Down Into Holes";
    if (g_LevelModifierCurrentStage == LEVELID_FRIGATE && index == 2) return "Clipping Change In Room With 3 Pipes";
return "No modifiers available";
}

const char *levelModifiersGetCurrentStageModifierValue(s32 index)
{
    if (levelModifiersCradleMpContext() && index == 0) return g_LevelModifierCradleKillPlaneActive ? "ACTIVE" : "ACTIVATE";

    if (levelModifiersGetCurrentStageModifierCount() == 0)
        return "";

    if (g_LevelModifierCurrentStage == LEVELID_DAM)
    {
        if (index == 0)
            return g_LevelModifierDamDoorsActive ? "ON" : "OFF";
        if (index == 1)
            return g_LevelModifierDamSpeedboatActive ? "ON" : "OFF";
        if (index == 2)
            return g_LevelModifierDamDrivableSpeedboatActive ? "ACTIVE" : "ACTIVATE";
    }
    else if (g_LevelModifierCurrentStage == LEVELID_SILO && index == 0)
    {
        return g_LevelModifierSiloBetaVentActive ? "ACTIVE" : "ACTIVATE";
    }
    else if (g_LevelModifierCurrentStage == LEVELID_CITADEL && index == 0)
    {
        return g_LevelModifierCitadelWaterActive ? "ON" : "OFF";
    }

        if (g_LevelModifierCurrentStage == LEVELID_FACILITY && index == 0)
        return g_LevelModifierFacilityVentGrateActive ? "ON" : "OFF";

    if (g_LevelModifierCurrentStage == LEVELID_FACILITY && index == 1) return g_LevelModifierFacilityBetaDoorsActive ? "ON" : "OFF";
    if (g_LevelModifierCurrentStage == LEVELID_FACILITY && index == 2) return g_LevelModifierFacilityStackedTanksActive ? "ON" : "OFF";
    if (g_LevelModifierCurrentStage == LEVELID_SURFACE && index == 0) return g_LevelModifierSurfaceWhiteSkyActive ? "ON" : "OFF";
    if (g_LevelModifierCurrentStage == LEVELID_SURFACE && index == 1) return g_LevelModifierSurfaceBetaDoorsActive ? "ON" : "OFF";
    if (g_LevelModifierCurrentStage == LEVELID_BUNKER1 && index == 0) return g_LevelModifierBunker1BetaDoorsActive ? "ON" : "OFF";
    if (g_LevelModifierCurrentStage == LEVELID_FRIGATE && index == 0) return g_LevelModifierFrigateKeepClearDoorsActive ? "ON" : "OFF";
    if (g_LevelModifierCurrentStage == LEVELID_FRIGATE && index == 1) return g_LevelModifierFrigateRestoreRemovedDoorsActive ? "ON" : "OFF";

    if (g_LevelModifierCurrentStage == LEVELID_TEMPLE && index == 0) return g_LevelModifierTempleFallActive ? "ON" : "OFF";
    if (g_LevelModifierCurrentStage == LEVELID_TEMPLE && index == 1) return g_LevelModifierTempleBetterRespawningActive ? "ON" : "OFF";
    if (g_LevelModifierCurrentStage == LEVELID_COMPLEX && index == 0) return g_LevelModifierComplexFallActive ? "ON" : "OFF";
    if (g_LevelModifierCurrentStage == LEVELID_FRIGATE && index == 2) return g_LevelModifierFrigatePipesActive ? "ON" : "OFF";
return "";
}

s32 levelModifiersToggleCurrentStageModifier(s32 index)
{
    if (levelModifiersCradleMpContext() && index == 0)
    {
        if (g_LevelModifierCradleKillPlaneActive) return FALSE;
        g_LevelModifierCradleKillPlaneActive = TRUE;
        g_LevelModifierCradleKillPlanePreload = TRUE;
        return TRUE;
    }

    if (levelModifiersGetCurrentStageModifierCount() == 0)
        return FALSE;

    if (g_LevelModifierCurrentStage == LEVELID_DAM)
    {
        if (index == 0)
            return levelModifiersSetDamDoors(!g_LevelModifierDamDoorsActive);
        if (index == 1)
            return levelModifiersSetDamSpeedboat(!g_LevelModifierDamSpeedboatActive);
        if (index == 2)
        {
            if (g_LevelModifierDamDrivableSpeedboatActive)
                return FALSE;
            return levelModifiersActivateDamDrivableSpeedboat();
        }
        return FALSE;
    }

    if (g_LevelModifierCurrentStage == LEVELID_SILO && index == 0)
    {
        if (g_LevelModifierSiloBetaVentActive)
            return FALSE;
        return levelModifiersActivateSiloBetaVent();
    }

    if (g_LevelModifierCurrentStage == LEVELID_CITADEL && index == 0)
        return levelModifiersSetCitadelWater(!g_LevelModifierCitadelWaterActive);

        if (g_LevelModifierCurrentStage == LEVELID_FACILITY && index == 0)
        return levelModifiersSetFacilityVentGrate(
            !g_LevelModifierFacilityVentGrateActive);

    if (g_LevelModifierCurrentStage == LEVELID_FACILITY && index == 1) { g_LevelModifierFacilityBetaDoorsActive = !g_LevelModifierFacilityBetaDoorsActive; g_LevelModifierFacilityBetaDoorsPreload = g_LevelModifierFacilityBetaDoorsActive; return levelModifiersApplyCurrentSetupStates(); }
    if (g_LevelModifierCurrentStage == LEVELID_FACILITY && index == 2) { g_LevelModifierFacilityStackedTanksActive = !g_LevelModifierFacilityStackedTanksActive; g_LevelModifierFacilityStackedTanksPreload = g_LevelModifierFacilityStackedTanksActive; return levelModifiersApplyCurrentSetupStates(); }
    if (g_LevelModifierCurrentStage == LEVELID_SURFACE && index == 0) return levelModifiersApplySurfaceWhiteSky(!g_LevelModifierSurfaceWhiteSkyActive);
    if (g_LevelModifierCurrentStage == LEVELID_SURFACE && index == 1) { g_LevelModifierSurfaceBetaDoorsActive = !g_LevelModifierSurfaceBetaDoorsActive; g_LevelModifierSurfaceBetaDoorsPreload = g_LevelModifierSurfaceBetaDoorsActive; return levelModifiersApplyCurrentSetupStates(); }
    if (g_LevelModifierCurrentStage == LEVELID_BUNKER1 && index == 0) { g_LevelModifierBunker1BetaDoorsActive = !g_LevelModifierBunker1BetaDoorsActive; g_LevelModifierBunker1BetaDoorsPreload = g_LevelModifierBunker1BetaDoorsActive; return levelModifiersApplyCurrentSetupStates(); }
    if (g_LevelModifierCurrentStage == LEVELID_FRIGATE && index == 0) { g_LevelModifierFrigateKeepClearDoorsActive = !g_LevelModifierFrigateKeepClearDoorsActive; g_LevelModifierFrigateKeepClearDoorsPreload = g_LevelModifierFrigateKeepClearDoorsActive; return levelModifiersApplyCurrentSetupStates(); }
    if (g_LevelModifierCurrentStage == LEVELID_FRIGATE && index == 1) { g_LevelModifierFrigateRestoreRemovedDoorsActive = !g_LevelModifierFrigateRestoreRemovedDoorsActive; g_LevelModifierFrigateRestoreRemovedDoorsPreload = g_LevelModifierFrigateRestoreRemovedDoorsActive; return levelModifiersApplyCurrentSetupStates(); }

    if (g_LevelModifierCurrentStage == LEVELID_TEMPLE && index == 0) return levelModifiersSetTempleFall(!g_LevelModifierTempleFallActive);
    if (g_LevelModifierCurrentStage == LEVELID_TEMPLE && index == 1) return levelModifiersSetTempleBetterRespawning(!g_LevelModifierTempleBetterRespawningActive);
    if (g_LevelModifierCurrentStage == LEVELID_COMPLEX && index == 0) return levelModifiersSetComplexFall(!g_LevelModifierComplexFallActive);
    if (g_LevelModifierCurrentStage == LEVELID_FRIGATE && index == 2) return levelModifiersSetFrigatePipes(!g_LevelModifierFrigatePipesActive);
return FALSE;
}

s32 levelModifiersCurrentStageModifierLocked(s32 index)
{
    if (levelModifiersCradleMpContext() && index == 0 && g_LevelModifierCradleKillPlaneActive) return TRUE;

    if (levelModifiersGetCurrentStageModifierCount() == 0)
        return TRUE;

    if (g_LevelModifierCurrentStage == LEVELID_SILO
        && index == 0
        && g_LevelModifierSiloBetaVentActive)
    {
        return TRUE;
    }

    if (g_LevelModifierCurrentStage == LEVELID_DAM
        && index == 2
        && g_LevelModifierDamDrivableSpeedboatActive)
    {
        return TRUE;
    }

    return FALSE;
}

s32 levelModifiersGetFrontendModifierCount(s32 category, s32 levelIndex)
{
    if (category == LEVELMOD_CATEGORY_MULTIPLAYER && levelIndex == 11) return 1;

    if (category == LEVELMOD_CATEGORY_SINGLE_PLAYER && levelIndex == 0) /* Dam */
        return 3;
    if (category == LEVELMOD_CATEGORY_SINGLE_PLAYER && levelIndex == 5) /* Silo */
        return 1;
    if (category == LEVELMOD_CATEGORY_MISCELLANEOUS && levelIndex == 1) /* Citadel */
        return 1;
        if (category == LEVELMOD_CATEGORY_SINGLE_PLAYER && levelIndex == 1) /* Facility */
        return 3;
    if (category == LEVELMOD_CATEGORY_SINGLE_PLAYER && levelIndex == 3) return 2;
    if (category == LEVELMOD_CATEGORY_SINGLE_PLAYER && levelIndex == 4) return 1;
    if (category == LEVELMOD_CATEGORY_SINGLE_PLAYER && levelIndex == 6) return 3;
    if (category == LEVELMOD_CATEGORY_MULTIPLAYER && levelIndex == 0) return 2;
    if (category == LEVELMOD_CATEGORY_MULTIPLAYER && levelIndex == 1) return 1;
return 0;
}

const char *levelModifiersGetFrontendModifierName(s32 category, s32 levelIndex, s32 modifierIndex)
{
    if (category == LEVELMOD_CATEGORY_MULTIPLAYER && levelIndex == 11 && modifierIndex == 0) return "Kill Plane";

    if (category == LEVELMOD_CATEGORY_SINGLE_PLAYER && levelIndex == 0)
    {
        if (modifierIndex == 0)
            return "Restore Doors Near Dam Docks";
        if (modifierIndex == 1)
            return "Restore Speedboat";
        if (modifierIndex == 2)
            return "Drivable Speedboat";
    }

    if (category == LEVELMOD_CATEGORY_SINGLE_PLAYER
        && levelIndex == 5
        && modifierIndex == 0)
    {
        return "Beta Vent Start";
    }

    if (category == LEVELMOD_CATEGORY_MISCELLANEOUS
        && levelIndex == 1
        && modifierIndex == 0)
    {
        return "Water";
    }

        if (category == LEVELMOD_CATEGORY_SINGLE_PLAYER
        && levelIndex == 1 && modifierIndex == 0)
        return "Walk Through Grate In Vents";

    if (category == LEVELMOD_CATEGORY_SINGLE_PLAYER && levelIndex == 1 && modifierIndex == 1) return "Restore Beta Facility Doors";
    if (category == LEVELMOD_CATEGORY_SINGLE_PLAYER && levelIndex == 1 && modifierIndex == 2) return "Restore Beta Facility Stacked Tanks";
    if (category == LEVELMOD_CATEGORY_SINGLE_PLAYER && levelIndex == 3 && modifierIndex == 0) return "White Sky";
    if (category == LEVELMOD_CATEGORY_SINGLE_PLAYER && levelIndex == 3 && modifierIndex == 1) return "Beta Doors";
    if (category == LEVELMOD_CATEGORY_SINGLE_PLAYER && levelIndex == 4 && modifierIndex == 0) return "Beta Doors";
    if (category == LEVELMOD_CATEGORY_SINGLE_PLAYER && levelIndex == 6 && modifierIndex == 0) return "\"Keep Clear\" Doors Leading Into Engine Room";
    if (category == LEVELMOD_CATEGORY_SINGLE_PLAYER && levelIndex == 6 && modifierIndex == 1) return "Restore Removed Doors";

    if (category == LEVELMOD_CATEGORY_MULTIPLAYER && levelIndex == 0 && modifierIndex == 0) return "Fall Down Into Hole";
    if (category == LEVELMOD_CATEGORY_MULTIPLAYER && levelIndex == 0 && modifierIndex == 1) return "Better Respawning";
    if (category == LEVELMOD_CATEGORY_MULTIPLAYER && levelIndex == 1 && modifierIndex == 0) return "Fall Down Into Holes";
    if (category == LEVELMOD_CATEGORY_SINGLE_PLAYER && levelIndex == 6 && modifierIndex == 2) return "Clipping Change In Room With 3 Pipes";
return "No modifiers available";
}

const char *levelModifiersGetFrontendModifierValue(s32 category, s32 levelIndex, s32 modifierIndex)
{
    if (category == LEVELMOD_CATEGORY_MULTIPLAYER && levelIndex == 11 && modifierIndex == 0) return g_LevelModifierCradleKillPlanePreload ? "ON" : "OFF";

    if (category == LEVELMOD_CATEGORY_SINGLE_PLAYER && levelIndex == 0)
    {
        if (modifierIndex == 0)
            return g_LevelModifierDamDoorsPreload ? "On" : "Off";
        if (modifierIndex == 1)
            return g_LevelModifierDamSpeedboatPreload ? "On" : "Off";
        if (modifierIndex == 2)
            return g_LevelModifierDamDrivableSpeedboatPreload ? "On" : "Off";
    }

    if (category == LEVELMOD_CATEGORY_SINGLE_PLAYER
        && levelIndex == 5
        && modifierIndex == 0)
    {
        return g_LevelModifierSiloBetaVentPreload ? "On" : "Off";
    }

    if (category == LEVELMOD_CATEGORY_MISCELLANEOUS
        && levelIndex == 1
        && modifierIndex == 0)
    {
        return g_LevelModifierCitadelWaterPreload ? "On" : "Off";
    }

        if (category == LEVELMOD_CATEGORY_SINGLE_PLAYER
        && levelIndex == 1 && modifierIndex == 0)
        return g_LevelModifierFacilityVentGratePreload ? "On" : "Off";

    if (category == LEVELMOD_CATEGORY_SINGLE_PLAYER && levelIndex == 1 && modifierIndex == 1) return g_LevelModifierFacilityBetaDoorsPreload ? "On" : "Off";
    if (category == LEVELMOD_CATEGORY_SINGLE_PLAYER && levelIndex == 1 && modifierIndex == 2) return g_LevelModifierFacilityStackedTanksPreload ? "On" : "Off";
    if (category == LEVELMOD_CATEGORY_SINGLE_PLAYER && levelIndex == 3 && modifierIndex == 0) return g_LevelModifierSurfaceWhiteSkyPreload ? "On" : "Off";
    if (category == LEVELMOD_CATEGORY_SINGLE_PLAYER && levelIndex == 3 && modifierIndex == 1) return g_LevelModifierSurfaceBetaDoorsPreload ? "On" : "Off";
    if (category == LEVELMOD_CATEGORY_SINGLE_PLAYER && levelIndex == 4 && modifierIndex == 0) return g_LevelModifierBunker1BetaDoorsPreload ? "On" : "Off";
    if (category == LEVELMOD_CATEGORY_SINGLE_PLAYER && levelIndex == 6 && modifierIndex == 0) return g_LevelModifierFrigateKeepClearDoorsPreload ? "On" : "Off";
    if (category == LEVELMOD_CATEGORY_SINGLE_PLAYER && levelIndex == 6 && modifierIndex == 1) return g_LevelModifierFrigateRestoreRemovedDoorsPreload ? "On" : "Off";

    if (category == LEVELMOD_CATEGORY_MULTIPLAYER && levelIndex == 0 && modifierIndex == 0) return g_LevelModifierTempleFallPreload ? "On" : "Off";
    if (category == LEVELMOD_CATEGORY_MULTIPLAYER && levelIndex == 0 && modifierIndex == 1) return g_LevelModifierTempleBetterRespawningPreload ? "On" : "Off";
    if (category == LEVELMOD_CATEGORY_MULTIPLAYER && levelIndex == 1 && modifierIndex == 0) return g_LevelModifierComplexFallPreload ? "On" : "Off";
    if (category == LEVELMOD_CATEGORY_SINGLE_PLAYER && levelIndex == 6 && modifierIndex == 2) return g_LevelModifierFrigatePipesPreload ? "On" : "Off";
return "";
}

s32 levelModifiersToggleFrontendModifier(s32 category, s32 levelIndex, s32 modifierIndex)
{
    if (category == LEVELMOD_CATEGORY_MULTIPLAYER && levelIndex == 11 && modifierIndex == 0)
    {
        g_LevelModifierCradleKillPlanePreload = !g_LevelModifierCradleKillPlanePreload;
        return TRUE;
    }

    if (category == LEVELMOD_CATEGORY_SINGLE_PLAYER && levelIndex == 0)
    {
        if (modifierIndex == 0)
        {
            levelModifiersSetDamDoorsPreload(!g_LevelModifierDamDoorsPreload);
            return TRUE;
        }

        if (modifierIndex == 1)
        {
            levelModifiersSetDamSpeedboatPreload(!g_LevelModifierDamSpeedboatPreload);
            return TRUE;
        }

        if (modifierIndex == 2)
        {
            levelModifiersSetDamDrivableSpeedboatPreload(
                !g_LevelModifierDamDrivableSpeedboatPreload);
            return TRUE;
        }

        return FALSE;
    }

    if (category == LEVELMOD_CATEGORY_SINGLE_PLAYER
        && levelIndex == 5
        && modifierIndex == 0)
    {
        levelModifiersSetSiloBetaVentPreload(!g_LevelModifierSiloBetaVentPreload);
        return TRUE;
    }

    if (category == LEVELMOD_CATEGORY_MISCELLANEOUS
        && levelIndex == 1
        && modifierIndex == 0)
    {
        levelModifiersSetCitadelWaterPreload(!g_LevelModifierCitadelWaterPreload);
        return TRUE;
    }

        if (category == LEVELMOD_CATEGORY_SINGLE_PLAYER
        && levelIndex == 1 && modifierIndex == 0)
    {
        g_LevelModifierFacilityVentGratePreload =
            !g_LevelModifierFacilityVentGratePreload;
        return TRUE;
    }

    if (category == LEVELMOD_CATEGORY_SINGLE_PLAYER && levelIndex == 1 && modifierIndex == 1) { g_LevelModifierFacilityBetaDoorsPreload = !g_LevelModifierFacilityBetaDoorsPreload; return TRUE; }
    if (category == LEVELMOD_CATEGORY_SINGLE_PLAYER && levelIndex == 1 && modifierIndex == 2) { g_LevelModifierFacilityStackedTanksPreload = !g_LevelModifierFacilityStackedTanksPreload; return TRUE; }
    if (category == LEVELMOD_CATEGORY_SINGLE_PLAYER && levelIndex == 3 && modifierIndex == 0) { g_LevelModifierSurfaceWhiteSkyPreload = !g_LevelModifierSurfaceWhiteSkyPreload; return TRUE; }
    if (category == LEVELMOD_CATEGORY_SINGLE_PLAYER && levelIndex == 3 && modifierIndex == 1) { g_LevelModifierSurfaceBetaDoorsPreload = !g_LevelModifierSurfaceBetaDoorsPreload; return TRUE; }
    if (category == LEVELMOD_CATEGORY_SINGLE_PLAYER && levelIndex == 4 && modifierIndex == 0) { g_LevelModifierBunker1BetaDoorsPreload = !g_LevelModifierBunker1BetaDoorsPreload; return TRUE; }
    if (category == LEVELMOD_CATEGORY_SINGLE_PLAYER && levelIndex == 6 && modifierIndex == 0) { g_LevelModifierFrigateKeepClearDoorsPreload = !g_LevelModifierFrigateKeepClearDoorsPreload; return TRUE; }
    if (category == LEVELMOD_CATEGORY_SINGLE_PLAYER && levelIndex == 6 && modifierIndex == 1) { g_LevelModifierFrigateRestoreRemovedDoorsPreload = !g_LevelModifierFrigateRestoreRemovedDoorsPreload; return TRUE; }

    if (category == LEVELMOD_CATEGORY_MULTIPLAYER && levelIndex == 0 && modifierIndex == 0) { g_LevelModifierTempleFallPreload = !g_LevelModifierTempleFallPreload; return TRUE; }
    if (category == LEVELMOD_CATEGORY_MULTIPLAYER && levelIndex == 0 && modifierIndex == 1) { g_LevelModifierTempleBetterRespawningPreload = !g_LevelModifierTempleBetterRespawningPreload; return TRUE; }
    if (category == LEVELMOD_CATEGORY_MULTIPLAYER && levelIndex == 1 && modifierIndex == 0) { g_LevelModifierComplexFallPreload = !g_LevelModifierComplexFallPreload; return TRUE; }
    if (category == LEVELMOD_CATEGORY_SINGLE_PLAYER && levelIndex == 6 && modifierIndex == 2) { g_LevelModifierFrigatePipesPreload = !g_LevelModifierFrigatePipesPreload; return TRUE; }
return FALSE;
}



const char *levelModifiersGetFrontendModifierDescription(s32 category, s32 levelIndex, s32 modifierIndex)
{
    if (category == LEVELMOD_CATEGORY_MULTIPLAYER && levelIndex == 11 && modifierIndex == 0) return "Kills players who fall below Y -2550.";

    if (category == LEVELMOD_CATEGORY_MULTIPLAYER && levelIndex == 0 && modifierIndex == 0)
        return "Allows players to fall through the hole in Temple.";
    if (category == LEVELMOD_CATEGORY_MULTIPLAYER && levelIndex == 0 && modifierIndex == 1)
        return "Reorders Temple respawn points for better spawning.";
    if (category == LEVELMOD_CATEGORY_MULTIPLAYER && levelIndex == 1 && modifierIndex == 0)
        return "Allows players to fall through holes in Complex.";
    if (category == LEVELMOD_CATEGORY_SINGLE_PLAYER && levelIndex == 6 && modifierIndex == 2)
        return "Changes clipping around the room with three pipes.";
    return "";
}

const char *levelModifiersGetFrontendModifierCredit(s32 category, s32 levelIndex, s32 modifierIndex)
{
    if (category == LEVELMOD_CATEGORY_MULTIPLAYER && levelIndex == 11 && modifierIndex == 0) return "Original Cradle MP modifier by Zoinkity.";

    if (category == LEVELMOD_CATEGORY_MULTIPLAYER && levelIndex == 0 && modifierIndex == 1)
        return "Code by RSB";
    if ((category == LEVELMOD_CATEGORY_MULTIPLAYER && levelIndex == 0 && modifierIndex == 0)
        || (category == LEVELMOD_CATEGORY_MULTIPLAYER && levelIndex == 1 && modifierIndex == 0)
        || (category == LEVELMOD_CATEGORY_SINGLE_PLAYER && levelIndex == 6 && modifierIndex == 2))
        return "Code by Zoinkity";
    return "";
}

/* R27Y: Zoinkity Cradle MP kill plane; signed 0xF60A == -2550. */
s32 levelModifiersCradleKillPlaneShouldKill(f32 playerY)
{
    return levelModifiersCradleMpContext()
        && g_LevelModifierCradleKillPlaneActive
        && playerY < -2550.0f;
}
