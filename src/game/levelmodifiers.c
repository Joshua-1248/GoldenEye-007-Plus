#include <ultra64.h>
#include <bondconstants.h>
#include "levelmodifiers.h"
#include "stan.h"
#include "bgfog.h"

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
};

static const char *g_LevelModifierMiscNames[] = {
    "Cuba",
    "Citadel",
};

static s32 g_LevelModifierCurrentStage = LEVELID_NONE;
static s32 g_LevelModifierSiloBetaVentPreload = FALSE;
static s32 g_LevelModifierSiloBetaVentActive = FALSE;
static s32 g_LevelModifierCitadelWaterPreload = FALSE;
static s32 g_LevelModifierCitadelWaterActive = FALSE;

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
    g_LevelModifierCurrentStage = levelid;
    g_LevelModifierSiloBetaVentActive = FALSE;
    g_LevelModifierCitadelWaterActive = FALSE;

    if (levelid == LEVELID_SILO && g_LevelModifierSiloBetaVentPreload)
    {
        g_LevelModifierSiloBetaVentActive = TRUE;
        levelModifiersApplySiloBetaVentStan(stan);
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

s32 levelModifiersGetCurrentStageModifierCount(void)
{
    if (g_LevelModifierCurrentStage == LEVELID_SILO)
        return 1;
    if (g_LevelModifierCurrentStage == LEVELID_CITADEL)
        return 1;
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
    if (category == LEVELMOD_CATEGORY_SINGLE_PLAYER && index == 5) /* Silo */
        return TRUE;
    if (category == LEVELMOD_CATEGORY_MISCELLANEOUS && index == 1) /* Citadel */
        return TRUE;
    return FALSE;
}

s32 levelModifiersLevelAvailableInCurrentStage(s32 category, s32 index)
{
    if (category == LEVELMOD_CATEGORY_SINGLE_PLAYER && index == 5)
        return g_LevelModifierCurrentStage == LEVELID_SILO;
    if (category == LEVELMOD_CATEGORY_MISCELLANEOUS && index == 1)
        return g_LevelModifierCurrentStage == LEVELID_CITADEL;
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
}

LevelModifierPolicy levelModifiersGetCitadelWaterPolicy(void)
{
    return LEVELMOD_POLICY_REVERSIBLE;
}
