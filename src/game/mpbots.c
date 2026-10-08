#include <ultra64.h>
#include <bondgame.h>
#include <bondconstants.h>
#include <bondaicommands.h>
#include <bondtypes.h>
#include <math.h>
#include <random.h>
#include <music.h>
#include "bondview.h"
#include "bondhead.h"
#include "chraction.h"
#include "chrobjdata.h"
#include "chrai.h"
#include "chr.h"
#include "file.h"
#include "front.h"
#include "gun.h"
#include "image.h"
#include "lv.h"
#include "initanitable.h"
#include "memp.h"
#include "ob.h"
#include "math_atan2f.h"

/*
 * R27Z/R28 R3: do not remove math_atan2f.h.
 * GoldenEye's active include/math.h does not prototype atan2f. IDO otherwise
 * applies default argument promotion and implicit-int return ABI, corrupting
 * targetyaw at runtime.
 */
#include "mp_weapon.h"
#include "model.h"
#include "mpbots.h"
#include "objecthandler.h"
#include "player.h"
#include "propobj.h"
#include "stan.h"
#include "bg.h"
#include "matrixmath.h"

/* R30 P3 GE compatibility declarations missing from the original headers. */
extern PROP getPropForHeldItem(ITEM_IDS item);
extern bool objCanPickupFromSafe(ObjectRecord *obj);
extern f32 chrGetArmor(ChrRecord *chr);
extern s32 get_ammo_type_for_weapon(ITEM_IDS weapon);
extern struct firing_anim_struct firing_animation_groups[][6];
extern void chrlvUpdateAimendbackShoulders(ChrRecord *self, void *arg1, s32 same, s32 swap, f32 next);
extern void play_sound_for_shot_actor(ChrRecord *self);
extern resource_lookup_data_entry resource_lookup_data_array[];
#if defined(GE_PHYSICAL_CODE) && defined(GE_PHYSICAL_FASTPATHS)
extern u8 _modMpBotModelCacheStart[];
extern u8 _modMpBotModelCacheEnd[];
extern u8 _modMpBotTextureCacheStart[];
extern u8 _modMpBotTextureCacheEnd[];
#endif

#ifdef GE_MODDED_CHEATS

#define MODBOT_TARGET_NONE (-1)
#define MODBOT_ATTACKER_NONE (-1)
#define MODBOT_PD_MAX_PARTICIPANTS (MAX_PLAYER_COUNT + MOD_MP_BOT_MAX)

#define MODBOT_PD_DISTMODE_BACKUP  1
#define MODBOT_PD_DISTMODE_OK      2
#define MODBOT_PD_DISTMODE_ADVANCE 3
#define MODBOT_PD_DISTMODE_GOTO    4

#define MODBOT_PD_DISTCFG_CLOSE            0
#define MODBOT_PD_DISTCFG_PISTOL           1
#define MODBOT_PD_DISTCFG_DEFAULT          2
#define MODBOT_PD_DISTCFG_SHOOTEXPLOSIVE   3
#define MODBOT_PD_DISTCFG_KAZE             4
#define MODBOT_PD_DISTCFG_FARSIGHT         5
#define MODBOT_PD_DISTCFG_FOLLOW           6
#define MODBOT_PD_DISTCFG_THROWEXPLOSIVE   7

#define MODBOT_PD_DTOR(x) ((x) * (M_TAU_F / 360.0f))

static void modMpBotsPlayLocalSfx(ChrRecord *chr, s16 sound)
{
    if (chr != NULL && chr->prop != NULL && sound != 0)
        chrobjSndCreatePostEventDefault(sndPlaySfx(g_musicSfxBufferPtr, sound, NULL), &chr->prop->pos);
}

static s16 modMpBotsPickupSfx(ITEM_IDS item)
{
    if (item == ITEM_LASER) return PICKUP_LASER_SFX;
    if (item == ITEM_KNIFE || item == ITEM_THROWKNIFE) return PICKUP_KNIFE_SFX;
    if (item == ITEM_TIMEDMINE || item == ITEM_PROXIMITYMINE || item == ITEM_REMOTEMINE) return PICKUP_MINE_SFX;
    if (item == ITEM_UNARMED || item == ITEM_FIST || item == ITEM_GRENADE) return 0;
    return PICKUP_GUN_SFX;
}

static s16 modMpBotsAmmoPickupSfx(s32 ammotype)
{
    if (ammotype == AMMO_REMOTEMINE || ammotype == AMMO_PROXMINE
        || ammotype == AMMO_TIMEDMINE || ammotype == AMMO_BOMBCASE
        || ammotype == AMMO_BUG || ammotype == AMMO_MICRO_CAMERA
        || ammotype == AMMO_PLASTIQUE)
        return PICKUP_MINE_SFX;
    if (ammotype == AMMO_KNIFE)
        return PICKUP_KNIFE_SFX;
    return PICKUP_AMMO_SFX;
}

/*
 * P11: Simulant-owned navigation now walks GoldenEye's native STAN polygon
 * links directly.  No guard waypoints/path nodes are required.  Route cost
 * deliberately includes clearance/portal/turn comfort so bots prefer the
 * middle of rooms and walkways rather than TAS-like shortest-edge lines.
 */
#define MODBOT_STAN_SEARCH_MAX 352
#define MODBOT_STAN_HASH_SIZE  512
#define MODBOT_STAN_DIRECT_RANGE_SQ (240.0f * 240.0f)
#define MODBOT_STAN_REACH_SQ        (70.0f * 70.0f)
#define MODBOT_STAN_CLEARANCE_GOAL  90.0f
#define MODBOT_STAN_PORTAL_GOAL     110.0f
#define MODBOT_STAN_CLEARANCE_COST  0.35f
#define MODBOT_STAN_PORTAL_COST     0.20f
#define MODBOT_STAN_TURN_COST       22.0f

/* R30 P5: load lifecycle and player-presentation hardening. */
#define MODBOT_MODEL_LOAD_MIN_FREE 0x10000
#define MODBOT_MODEL_LOAD_ROM_GUARD 0x100

/*
 * Direct GoldenEye adaptation of Perfect Dark's struct aibot fields used by
 * bot.c/botcmd.c.  GoldenEye-only ownership/scoring fields stay at the top.
 */
#define MODBOT_PD_INV_MAX 10
#define MODBOT_PD_FUNC_PRIMARY 0

struct ModMpBotRuntime
{
    ChrRecord *chr;
    PropRecord *targetprop;
    s16 lastattacker;
    s16 vindicttarget;
    s16 stalktarget;
    s16 respawntimer;
    s16 damagegracetimer60;
    u8 deathrecorded;
    u8 spawnready;
    u8 resourcesready;
    s32 kills;
    s32 deaths;

    f32 speedmultforwards;
    f32 speedmultsideways;
    s8 distmode;
    u8 manualbackup;
    s16 attackingparticipant;
    s16 distoverridecode;
    s16 pad1;
    f32 roty;
    f32 angleoffset;
    f32 speedtheta;
    f32 lookangle;
    f32 moveratex;
    f32 moveratez;
    s32 distmodettl60;
    s32 distoverridetimer60;
    s32 shootdelaytimer60;
    s32 targetlastseen60;
    s32 lastseenanytarget60;
    u8 targetinsight;
    u8 pad2[3];
    s32 queryplayernum;
    s8 chrnumsbydistanceasc[MODBOT_PD_MAX_PARTICIPANTS];
    f32 chrdistances[MODBOT_PD_MAX_PARTICIPANTS];
    u8 chrsinsight[MODBOT_PD_MAX_PARTICIPANTS];
    s32 chrslastseen60[MODBOT_PD_MAX_PARTICIPANTS];
    f32 zeroangle;
    f32 zerospeed;
    f32 zeroinc;
    s32 random3ttl60;
    u32 random3;
    f32 curzerotimer60;
    s32 abortattacktimer60;
    s32 realignangleframe;

    /* R30 P2 R1: PD botact/botinv combat state. */
    ITEM_IDS weaponnum;
    s8 gunfunc;
    u8 ismeleeweapon;
    u8 inventorycount;
    u8 pad3;
    s32 loadedammo[2];
    s16 timeuntilreload60[2];
    s16 nextbullettimer60[2];
    s16 punchtimer60[2];
    s32 ammoheld[AMMOTYPE_MAX];
    s32 changeguntimer60;
    s32 throwtimer60;
    u8 burstsdone[2];
    u8 pad4[2];
    u32 random1;
    s32 random1ttl60;
    ITEM_IDS inventory[MODBOT_PD_INV_MAX];

    /* R30 P3: PD pickup/switch/throw state. */
    u8 inventorycopies[MODBOT_PD_INV_MAX];
    s16 inventorypads[MODBOT_PD_INV_MAX];

    /* P11: bot-owned STAN corridor; never aliases guard ACT_GOPOS state. */
    StandTile *navtiles[MAX_CHRWAYPOINTS];
    StandTile *navgoalstan;
    coord3d navgoalpos;
    s16 navtargetcode;
    s16 navage;
    s8 navindex;
    s8 navcount;
    u8 navactive;
};

struct ModMpBotPdDifficulty
{
    u8 shootdelay60;
    f32 minzerospeed;
    f32 maxzerospeed;
    u16 zerotime60;
    f32 turnunzeromult;
    f32 zerocloakspeed;
    f32 forcezerominspeed;
    s32 dizzyamount;
};


/*
 * GoldenEye weapon IDs are not Perfect Dark weapon IDs.  P2 therefore maps
 * each GE weapon onto the closest *live* PD g_BotWeaponConfigs primary-fire
 * role.  The numeric score/distance/reload values below are PD values, not
 * newly tuned GoldenEye values.  This adapter is the unavoidable engine seam;
 * the PD table's legacy PP9I/KL01313/KF7SPECIAL rows are zeroed and cannot
 * provide useful Simulant behaviour for GoldenEye weapons.
 */
struct ModMpBotPdWeaponConfig
{
    s16 score1;
    s16 score2;
    u8 supported;
    u8 haspriammogoal;
    u8 pridistconfig;
    u16 targetammopri;
    u16 criticalammopri;
    u8 reloaddelay;
    u8 allowpartialreloaddelay;
    u8 throwable;
    u8 melee;
};

u8 g_ModMpBotCount = 0;

struct ModMpBotConfig g_ModMpBotConfigs[MOD_MP_BOT_MAX] = {
    {0, MODBOT_DIFFICULTY_NORMAL, 0, "Bot 1"},
    {1, MODBOT_DIFFICULTY_NORMAL, 0, "Bot 2"},
    {2, MODBOT_DIFFICULTY_NORMAL, 0, "Bot 3"},
    {3, MODBOT_DIFFICULTY_NORMAL, 0, "Bot 4"},
    {4, MODBOT_DIFFICULTY_NORMAL, 0, "Bot 5"},
    {5, MODBOT_DIFFICULTY_NORMAL, 0, "Bot 6"},
    {6, MODBOT_DIFFICULTY_NORMAL, 0, "Bot 7"},
    {7, MODBOT_DIFFICULTY_NORMAL, 0, "Bot 8"}
};

static struct ModMpBotRuntime g_ModMpBotRuntime[MOD_MP_BOT_MAX];

struct ModMpBotStanSearchNode
{
    StandTile *tile;
    f32 g;
    f32 f;
    s16 parent;
    s16 heappos;
    u8 state;
    u8 pad[3];
};

static struct ModMpBotStanSearchNode g_ModMpBotStanNodes[MODBOT_STAN_SEARCH_MAX];
static s16 g_ModMpBotStanHeap[MODBOT_STAN_SEARCH_MAX];
static s16 g_ModMpBotStanHash[MODBOT_STAN_HASH_SIZE];
static s16 g_ModMpBotStanReverse[MODBOT_STAN_SEARCH_MAX];
static s16 g_ModMpBotStanNodeCount;
static s16 g_ModMpBotStanHeapCount;

static s32 g_ModMpBotHumanKills[MAX_PLAYER_COUNT];
static s32 g_ModMpBotDamageOwner = -1;
static s32 g_ModMpBotSpawnCursor;

static void modMpBotsPdNavClear(struct ModMpBotRuntime *runtime);

#if defined(GE_PHYSICAL_CODE) && defined(GE_PHYSICAL_FASTPATHS)
static u8 *g_ModMpBotModelCacheCursor;
static struct texpool g_ModMpBotTexturePool;
#endif

static const char *g_ModMpBotDifficultyNames[MODBOT_DIFFICULTY_MAX] = {
    "Very Easy", "Easy", "Normal", "Hard", "Very Hard", "Extreme"
};

static const char *g_ModMpBotTraitNames[MODBOT_TRAIT_COUNT] = {
    "Pacifist", "Vindictive", "Psychotic", "Juggernaut",
    "Armor Specialist", "Pyromaniac", "Bully", "Fearful",
    "Equalizer", "Stalker", "Melee", "Speedy", "Cheap"
};



/* Perfect Dark bot.c difficulty table, NTSC timing. */
static struct ModMpBotPdDifficulty g_ModMpBotPdDifficulties[MODBOT_DIFFICULTY_MAX] = {
    {90, MODBOT_PD_DTOR(15.0f),  MODBOT_PD_DTOR(30.0f), 600, 10.0f, MODBOT_PD_DTOR(40.0f),   MODBOT_PD_DTOR(20.0f), 1000},
    {60, MODBOT_PD_DTOR(7.0f),   MODBOT_PD_DTOR(14.0f), 360, 10.0f, MODBOT_PD_DTOR(28.5f),  MODBOT_PD_DTOR(8.0f),  1000},
    {30, MODBOT_PD_DTOR(4.0f),   MODBOT_PD_DTOR(8.0f),  180, 4.0f,  MODBOT_PD_DTOR(20.0f),   MODBOT_PD_DTOR(5.0f),  1500},
    {15, MODBOT_PD_DTOR(1.5f),   MODBOT_PD_DTOR(4.0f),   90, 2.0f,  MODBOT_PD_DTOR(14.0f),   MODBOT_PD_DTOR(2.0f),  2500},
    { 0, MODBOT_PD_DTOR(0.0f),   MODBOT_PD_DTOR(2.0f),   45, 1.0f,  MODBOT_PD_DTOR(10.0f),   MODBOT_PD_DTOR(0.0f),  4000},
    { 0, MODBOT_PD_DTOR(0.0f),   MODBOT_PD_DTOR(0.0f),    0, 1.0f,  MODBOT_PD_DTOR(10.0f),   MODBOT_PD_DTOR(0.0f),  4000}
};

/* Perfect Dark botcmd.c g_BotDistConfigs. */
static f32 g_ModMpBotPdDistConfigs[][3] = {
    {0.0f,    120.0f,  10000.0f},
    {300.0f,  450.0f,   4500.0f},
    {300.0f,  600.0f,   4500.0f},
    {600.0f, 1200.0f,   4500.0f},
    {150.0f,  250.0f,   4500.0f},
    {1000.0f, 2000.0f,  3000.0f},
    {0.0f,    250.0f,  10000.0f},
    {450.0f,  700.0f,   4500.0f}
};

static s32 modMpBotsRuntimeAllowed(void)
{
    return gamemode == GAMEMODE_MULTI
        && g_ModMpBotCount > 0;
}

s32 modMpBotsGetCount(void)
{
    return g_ModMpBotCount;
}

void modMpBotsSetCount(s32 count)
{
    if (count < 0)
        count = 0;
    if (count > MOD_MP_BOT_MAX)
        count = MOD_MP_BOT_MAX;

    g_ModMpBotCount = (u8)count;
}

void modMpBotsCycleCount(void)
{
    modMpBotsSetCount((g_ModMpBotCount + 1) % (MOD_MP_BOT_MAX + 1));
}

const char *modMpBotGetDifficultyName(s32 difficulty)
{
    if ((u32)difficulty >= MODBOT_DIFFICULTY_MAX)
        difficulty = MODBOT_DIFFICULTY_NORMAL;
    return g_ModMpBotDifficultyNames[difficulty];
}

const char *modMpBotGetTraitName(s32 traitindex)
{
    if ((u32)traitindex >= MODBOT_TRAIT_COUNT)
        return "";
    return g_ModMpBotTraitNames[traitindex];
}


const char *modMpBotGetDisplayName(s32 slot)
{
    if ((u32)slot >= MOD_MP_BOT_MAX)
        return "Bot";
    return g_ModMpBotConfigs[slot].name;
}

s32 modMpBotHasTrait(s32 slot, u16 trait)
{
    return (u32)slot < MOD_MP_BOT_MAX && (g_ModMpBotConfigs[slot].traits & trait) != 0;
}

void modMpBotToggleTrait(s32 slot, s32 traitindex)
{
    if ((u32)slot < MOD_MP_BOT_MAX && (u32)traitindex < MODBOT_TRAIT_COUNT)
        g_ModMpBotConfigs[slot].traits ^= (u16)(1U << traitindex);
}

void modMpBotCycleCharacter(s32 slot, s32 direction)
{
    s32 count;
    s32 value;
    if ((u32)slot >= MOD_MP_BOT_MAX) return;
    count = frontGetMpCharacterCount();
    if (count <= 0) return;
    if (direction == 0) direction = 1;
    value = (s32)g_ModMpBotConfigs[slot].character + direction;
    while (value < 0) value += count;
    while (value >= count) value -= count;
    g_ModMpBotConfigs[slot].character = (u8)value;
}

void modMpBotCycleDifficulty(s32 slot, s32 direction)
{
    s32 value;
    if ((u32)slot >= MOD_MP_BOT_MAX) return;
    if (direction == 0) direction = 1;
    value = (s32)g_ModMpBotConfigs[slot].difficulty + direction;
    while (value < 0) value += MODBOT_DIFFICULTY_MAX;
    while (value >= MODBOT_DIFFICULTY_MAX) value -= MODBOT_DIFFICULTY_MAX;
    g_ModMpBotConfigs[slot].difficulty = (u8)value;
}

static s32 modMpBotsStageReadyForSpawn(void)
{
    s32 i;

    /* P11 navigation owns native STAN links directly; guard waypoint tables
     * are not a Simulant spawn prerequisite. */
    if (startpadcount <= 0 || g_CurrentSetup.pads == NULL || standTileStart == NULL)
        return FALSE;

    for (i = 0; i < getPlayerCount(); i++)
    {
        /* Viewer prop->stan is presentation/tick-maintained state and can lag
         * the authoritative player collision tile during the first MP frames.
         * Waiting on it made Simulants appear only after the local player
         * moved/aimed.  field_488.current_tile_ptr is initialized as part of
         * bondviewPlayerBeginLife and is the real movement authority. */
        if (g_playerPointers[i] == NULL || g_playerPointers[i]->prop == NULL
            || g_playerPointers[i]->field_488.current_tile_ptr == NULL)
            return FALSE;
    }

    return TRUE;
}

/*
 * GoldenEye lazily loads character bodies/heads from MEMPOOL_STAGE.  Its
 * historical loader assumes the allocation succeeds; a late Simulant body on
 * a tight Multiplayer stage can therefore enter the allocator failure loop.
 * Physical Plus builds reserve an upper-RDRAM model-file cache instead.  The
 * ordinary decoded texture pool remains authoritative (and already has its
 * Expansion-Pak overflow pool in PHYSICAL_FASTPATHS builds).
 */
static s32 modMpBotsEnsureCharacterModelLoaded(s32 modelnum)
{
    ModelFileHeader *header;
    s32 filenum;
    s32 rombytes;

    if (modelnum < 0 || c_item_entries[modelnum].header == NULL
        || c_item_entries[modelnum].filename == NULL)
        return modelnum < 0;

    header = c_item_entries[modelnum].header;
    if (header->RootNode != NULL)
        return TRUE;

    filenum = fileGetIndex((char *)c_item_entries[modelnum].filename);
    if (filenum <= 0)
        return FALSE;

    rombytes = resource_lookup_data_array[filenum].rom_size;
    if (rombytes <= 0)
        return FALSE;

#if defined(GE_PHYSICAL_CODE) && defined(GE_PHYSICAL_FASTPATHS)
    {
        s32 remaining;
        s32 used;
        u8 *cursor;

        cursor = (u8 *)(((u32)g_ModMpBotModelCacheCursor + 0xfU) & ~0xfU);
        remaining = (s32)(_modMpBotModelCacheEnd - cursor);

        /* 64 KiB is deliberately conservative: the largest authored chr
         * Model.o payload in this tree is below 40 KiB, with heads far smaller.
         * The extra room also keeps the compressed source safely behind the
         * in-place decompression destination. */
        if (remaining < MODBOT_MODEL_LOAD_MIN_FREE
            || rombytes + MODBOT_MODEL_LOAD_MIN_FREE
                + MODBOT_MODEL_LOAD_ROM_GUARD >= remaining)
        {
            /* Never fall back to GoldenEye's non-failing stage-bank loader
             * for a live Simulant.  That fallback could still overrun a tight
             * MP stage.  A model that cannot fit the bounded upper cache is
             * rejected cleanly for this stage. */
            return FALSE;
        }

        load_object_fill_header(header, (u8 *)c_item_entries[modelnum].filename,
            cursor, remaining, &g_ModMpBotTexturePool);
        used = get_pc_buffer_remaining_value((u8 *)c_item_entries[modelnum].filename);

        if (header->RootNode == NULL || used <= 0 || used > remaining)
        {
            header->RootNode = NULL;
            return FALSE;
        }

        g_ModMpBotModelCacheCursor = cursor + ((used + 0xf) & ~0xf);
        return TRUE;
    }
#else
    /* Non-physical compatibility path: refuse the lazy load before GE can
     * hand an undersized stage bank to its non-failing allocator. */
    if ((s32)mempGetBankSizeLeft(MEMPOOL_STAGE)
        <= rombytes + MODBOT_MODEL_LOAD_MIN_FREE + MODBOT_MODEL_LOAD_ROM_GUARD)
        return FALSE;

    return TRUE;
#endif
}

static s32 modMpBotsEnsureCharacterResources(s32 body, s32 head)
{
    if (!modMpBotsEnsureCharacterModelLoaded(body))
        return FALSE;

    if (head >= 0 && !modMpBotsEnsureCharacterModelLoaded(head))
        return FALSE;

    return TRUE;
}

void modMpBotsResetRuntime(void)
{
    s32 i;
    s32 j;

    for (i = 0; i < MOD_MP_BOT_MAX; i++)
    {
        struct ModMpBotRuntime *runtime = &g_ModMpBotRuntime[i];

        runtime->chr = NULL;
        runtime->targetprop = NULL;
        runtime->lastattacker = MODBOT_ATTACKER_NONE;
        runtime->vindicttarget = MODBOT_TARGET_NONE;
        runtime->stalktarget = MODBOT_TARGET_NONE;
        runtime->respawntimer = 0;
        runtime->damagegracetimer60 = 0;
        runtime->deathrecorded = FALSE;
        runtime->spawnready = FALSE;
        runtime->resourcesready = FALSE;
        runtime->kills = 0;
        runtime->deaths = 0;
        runtime->speedmultforwards = 0.0f;
        runtime->speedmultsideways = 0.0f;
        runtime->distmode = -1;
        runtime->manualbackup = FALSE;
        runtime->attackingparticipant = -1;
        runtime->distoverridecode = MODBOT_TARGET_NONE;
        runtime->roty = 0.0f;
        runtime->angleoffset = 0.0f;
        runtime->speedtheta = 0.0f;
        runtime->lookangle = 0.0f;
        runtime->moveratex = 0.0f;
        runtime->moveratez = 0.0f;
        runtime->distmodettl60 = 0;
        runtime->distoverridetimer60 = 0;
        runtime->shootdelaytimer60 = 0;
        runtime->targetlastseen60 = -1;
        runtime->lastseenanytarget60 = -1;
        runtime->targetinsight = FALSE;
        runtime->queryplayernum = -1;
        runtime->zeroangle = 0.0f;
        runtime->zerospeed = 0.0f;
        runtime->zeroinc = 0.0f;
        runtime->random3ttl60 = 0;
        runtime->random3 = randomGetNext();
        runtime->curzerotimer60 = 0.0f;
        runtime->abortattacktimer60 = -1;
        runtime->realignangleframe = 0;
        runtime->weaponnum = ITEM_UNARMED;
        runtime->gunfunc = MODBOT_PD_FUNC_PRIMARY;
        runtime->ismeleeweapon = TRUE;
        runtime->inventorycount = 1;
        runtime->loadedammo[GUNRIGHT] = 0;
        runtime->loadedammo[GUNLEFT] = 0;
        runtime->timeuntilreload60[GUNRIGHT] = 0;
        runtime->timeuntilreload60[GUNLEFT] = 0;
        runtime->nextbullettimer60[GUNRIGHT] = 0;
        runtime->nextbullettimer60[GUNLEFT] = 0;
        runtime->punchtimer60[GUNRIGHT] = 0;
        runtime->punchtimer60[GUNLEFT] = -1;
        runtime->changeguntimer60 = 0;
        runtime->throwtimer60 = 0;
        runtime->burstsdone[GUNRIGHT] = 0;
        runtime->burstsdone[GUNLEFT] = 0;
        runtime->random1 = randomGetNext();
        runtime->random1ttl60 = 0;
        runtime->inventory[0] = ITEM_UNARMED;
                runtime->navgoalstan = NULL;
        runtime->navgoalpos.x = 0.0f;
        runtime->navgoalpos.y = 0.0f;
        runtime->navgoalpos.z = 0.0f;
        runtime->navtargetcode = MODBOT_TARGET_NONE;
        runtime->navage = 0;
        runtime->navindex = 0;
        runtime->navcount = 0;
        runtime->navactive = FALSE;

        for (j = 0; j < MAX_CHRWAYPOINTS; j++)
            runtime->navtiles[j] = NULL;

        for (j = 1; j < MODBOT_PD_INV_MAX; j++)
            runtime->inventory[j] = ITEM_UNARMED;

        for (j = 0; j < AMMOTYPE_MAX; j++)
            runtime->ammoheld[j] = 0;

        for (j = 0; j < MODBOT_PD_MAX_PARTICIPANTS; j++)
        {
            runtime->chrnumsbydistanceasc[j] = -1;
            runtime->chrdistances[j] = 99999999.0f;
            runtime->chrsinsight[j] = FALSE;
            runtime->chrslastseen60[j] = -1;
        }
    }

    for (i = 0; i < MAX_PLAYER_COUNT; i++)
        g_ModMpBotHumanKills[i] = 0;

    g_ModMpBotDamageOwner = -1;
    g_ModMpBotSpawnCursor = 0;
#if defined(GE_PHYSICAL_CODE) && defined(GE_PHYSICAL_FASTPATHS)
    g_ModMpBotModelCacheCursor = _modMpBotModelCacheStart;
    texInitPool(&g_ModMpBotTexturePool, _modMpBotTextureCacheStart,
        (s32)(_modMpBotTextureCacheEnd - _modMpBotTextureCacheStart));
#endif
}

/* R30 P6: prepare selected bot body/head resources during stage load, before
 * the active prop world begins ticking.  Runtime bot ticks never perform a
 * model-file decompression/header relocation. */
void modMpBotsPrepareStage(void)
{
    Model *primedmodels[MOD_MP_BOT_MAX];
    s32 primedcount = 0;
    s32 i;
    s32 body;
    s32 head;

    if (!modMpBotsRuntimeAllowed())
        return;

#if defined(GE_PHYSICAL_CODE) && defined(GE_PHYSICAL_FASTPATHS)
    g_ModMpBotModelCacheCursor = _modMpBotModelCacheStart;
    texInitPool(&g_ModMpBotTexturePool, _modMpBotTextureCacheStart,
        (s32)(_modMpBotTextureCacheEnd - _modMpBotTextureCacheStart));
#endif

    for (i = 0; i < g_ModMpBotCount; i++)
    {
        Model *model;

        g_ModMpBotRuntime[i].resourcesready = FALSE;

        if (!frontGetMpCharacterData(g_ModMpBotConfigs[i].character, &body, &head, NULL))
            continue;

        if (!modMpBotsEnsureCharacterResources(body, head))
            continue;

        /* modelmgrAllocateAnimModelSlots reserved one ordinary animated-model
         * slot for each configured Simulant.  Instantiate the body+head now,
         * while g_ModelIsLvResetting is still true, so that slot receives the
         * exact rwdata length this character requires.  Keep every primed
         * model occupied until all bots have been sized so repeated bodies
         * cannot reuse the same slot.  No Prop/Chr is created here. */
        model = retrieve_header_for_body_and_head(body, head, 0);

        if (model == NULL)
            continue;

        primedmodels[primedcount++] = model;
        g_ModMpBotRuntime[i].resourcesready = TRUE;
    }

    /* Mark the primed slots free only after all configured bots have obtained
     * distinct exact-size rwdata buffers.  Runtime spawn/respawn can now reuse
     * them without relying on the ten retail 140-record generic spares. */
    for (i = 0; i < primedcount; i++)
        clear_aircraft_model_obj(primedmodels[i]);
}

s32 modMpBotsGetSlotForChr(ChrRecord *chr)
{
    s32 i;

    if (chr == NULL)
        return -1;

    for (i = 0; i < MOD_MP_BOT_MAX; i++)
    {
        if (g_ModMpBotRuntime[i].chr == chr)
            return i;
    }

    return -1;
}

PropRecord *modMpBotsGetTargetProp(ChrRecord *chr)
{
    s32 slot = modMpBotsGetSlotForChr(chr);

    if (slot < 0)
        return NULL;

    return g_ModMpBotRuntime[slot].targetprop;
}

PropRecord *modMpBotsGetBotProp(s32 slot)
{
    ChrRecord *chr;

    if ((u32)slot >= MOD_MP_BOT_MAX)
        return NULL;

    chr = g_ModMpBotRuntime[slot].chr;
    if (chr == NULL || chr->prop == NULL || chr->model == NULL || chrIsDead(chr))
        return NULL;

    return chr->prop;
}

f32 modMpBotsGetMoveMultiplier(ChrRecord *chr)
{
    /*
     * R30 P1: Perfect Dark applies SpeedSim/TurtleSim scaling inside
     * bot_calculate_max_speed, not inside the generic chr GOPOS motor.
     * Returning 1 here prevents GoldenEye's old compatibility hook from
     * multiplying the PD lateral motor a second time.
     */
    return 1.0f;
}



/*
 * Feed player-like forward/side input into GoldenEye's normal chr collision
 * update.  This is the GoldenEye analogue of Perfect Dark's bot_update_lateral.
 *
 * IMPORTANT: this function only proposes X/Z.  sub_GAME_7F01FC10 still owns
 * STAN validation, wall collision, floor support, gravity and room updates.
 * Bots therefore do not teleport or bypass native collision.
 */
static f32 modMpBotsPdCalculateMaxSpeed(s32 slot)
{
    struct ModMpBotRuntime *runtime = &g_ModMpBotRuntime[slot];
    struct ModMpBotConfig *config = &g_ModMpBotConfigs[slot];
    s32 difficulty = config->difficulty;
    f32 pov = 1.0f;
    f32 speed;

    if ((u32)difficulty >= MODBOT_DIFFICULTY_MAX)
        difficulty = MODBOT_DIFFICULTY_NORMAL;

    /*
     * PD uses g_HeadsAndBodies[bodynum].height / 159 here.  GoldenEye's MP
     * character table already stores the corresponding normalized POV scale,
     * so that is the compatibility-layer input to the same formula.
     */
    frontGetMpCharacterData(config->character, NULL, NULL, &pov);
    if (pov <= 0.0f)
        pov = 1.0f;

    speed = pov;
    speed = speed * 0.002830188954249f + 1.0f;

    switch (difficulty)
    {
        case MODBOT_DIFFICULTY_MEAT:    speed *= 5.0f;  break;
        case MODBOT_DIFFICULTY_EASY:    speed *= 6.2f;  break;
        default:
        case MODBOT_DIFFICULTY_NORMAL:  speed *= 7.6f;  break;
        case MODBOT_DIFFICULTY_HARD:    speed *= 9.4f;  break;
        case MODBOT_DIFFICULTY_PERFECT: speed *= 11.2f; break;
        case MODBOT_DIFFICULTY_DARK:    speed *= 11.2f; break;
    }

    /* PD TurtleSim uses 3.5 where a NormalSim uses 7.6.  Because Juggernaut
     * is now an independent trait, apply the same relative slowdown to the
     * selected difficulty rather than replacing that difficulty's speed. */
    if (config->traits & MODBOT_TRAIT_JUGGERNAUT) speed *= (3.5f / 7.6f);

    /* Traits intentionally stack without a cap. Speedy + Cheap is 4x, and
     * either/both can progressively overcome the Juggernaut slowdown. */
    if (config->traits & MODBOT_TRAIT_SPEEDY) speed *= 2.0f;
    if (config->traits & MODBOT_TRAIT_CHEAP) speed *= 2.0f;

    if (runtime->chr == NULL)
        return 0.0f;

    return speed;
}

/*
 * Direct port of Perfect Dark bot.c:bot_update_lateral (NTSC 1.0 path), with
 * GE's g_ClockTimer/g_GlobalTimerDelta substituted for lvupdate240 and
 * lvupdate60freal.  R30 P4 makes aibot->roty authoritative; navigation no
 * longer reads travel heading back out of GoldenEye's guard action state.
 */
s32 modMpBotsApplyLateral(ChrRecord *chr, coord3d *src, coord3d *dst)
{
    struct ModMpBotRuntime *runtime;
    s32 slot;
    s32 i;
    s32 numupdates;
    f32 cosine;
    f32 sine;
    f32 desiredx;
    f32 desiredz;
    f32 speedsideways;
    f32 speedforwards;
    f32 speed;
    f32 tmp;
    f32 movex = 0.0f;
    f32 movez = 0.0f;

    if (chr == NULL || src == NULL || dst == NULL || chr->model == NULL)
        return FALSE;

    slot = modMpBotsGetSlotForChr(chr);
    if (slot < 0)
        return FALSE;

    runtime = &g_ModMpBotRuntime[slot];

    if (chrIsDead(chr))
    {
        runtime->moveratex = 0.0f;
        runtime->moveratez = 0.0f;
        return FALSE;
    }

    /* PD does not move Simulants during its initial 145-frame spawn window. */
    if (g_GlobalTimer < 145)
    {
        runtime->moveratex = 0.0f;
        runtime->moveratez = 0.0f;
        dst->x = src->x;
        dst->z = src->z;
        return TRUE;
    }

    speedsideways = runtime->speedmultsideways;
    speedforwards = runtime->speedmultforwards;
    speed = modMpBotsPdCalculateMaxSpeed(slot);

    speedsideways *= speed;
    speedforwards *= speed;

    cosine = cosf(runtime->roty);
    sine = sinf(runtime->roty);

    desiredx = speedsideways * cosine + speedforwards * sine;
    desiredz = -speedsideways * sine + speedforwards * cosine;

    numupdates = g_ClockTimer;
    if (numupdates <= 0)
    {
        dst->x = src->x;
        dst->z = src->z;
        return TRUE;
    }

    tmp = 0.055000007152557f * g_GlobalTimerDelta / (f32)numupdates;

    for (i = 0; i < numupdates; i++)
    {
        runtime->moveratex = 0.945f * runtime->moveratex + desiredx;
        runtime->moveratez = 0.945f * runtime->moveratez + desiredz;
        movex += runtime->moveratex * tmp;
        movez += runtime->moveratez * tmp;
    }

    dst->x = src->x + movex;
    dst->z = src->z + movez;

    return TRUE;
}


void modMpBotsSetDamageOwner(s32 slot)
{
    g_ModMpBotDamageOwner = ((u32)slot < MOD_MP_BOT_MAX) ? slot : -1;
}

void modMpBotsClearDamageOwner(void)
{
    g_ModMpBotDamageOwner = -1;
}

s32 modMpBotsGetDamageOwner(void)
{
    return g_ModMpBotDamageOwner;
}

static s16 modMpBotsTargetCodeFromChr(ChrRecord *chr)
{
    s32 slot;
    s32 i;

    if (chr == NULL || chr->prop == NULL)
        return MODBOT_TARGET_NONE;

    slot = modMpBotsGetSlotForChr(chr);
    if (slot >= 0)
        return (s16)(MOD_MP_BOT_BOT_TARGET_BASE + slot);

    if (chr->prop->type == PROP_TYPE_VIEWER)
    {
        i = getPlayerPointerIndex(chr->prop);
        if ((u32)i < MAX_PLAYER_COUNT)
            return (s16)i;
    }

    return MODBOT_TARGET_NONE;
}

s32 modMpBotsCanTakeDamage(ChrRecord *victim)
{
    s32 slot = modMpBotsGetSlotForChr(victim);

    if (slot < 0)
        return TRUE;

    if (victim == NULL || victim->actiontype == ACT_DIE || victim->actiontype == ACT_DEAD)
        return FALSE;

    return g_ModMpBotRuntime[slot].damagegracetimer60 <= 0;
}

void modMpBotsOnDamageAccepted(ChrRecord *victim)
{
    s32 slot = modMpBotsGetSlotForChr(victim);
    s32 damagetype;
    s32 duration;
    f32 health;

    if (slot < 0 || victim == NULL
        || victim->actiontype == ACT_DIE || victim->actiontype == ACT_DEAD)
        return;

    /* Match the human multiplayer damage grace window.  Human players select
     * one of g_DamageTypes[] from remaining health, then refuse subsequent
     * damage until that damage-show interval finishes.  Simulants use the same
     * table and timer, without the HUD flash itself. */
    health = 1.0f;
    if (victim->maxdamage > 0.0f && victim->damage > 0.0f)
        health = 1.0f - victim->damage / victim->maxdamage;
    if (health < 0.0f)
        health = 0.0f;
    if (health > 1.0f)
        health = 1.0f;

    damagetype = (s32)(health * 8.0f);
    if (damagetype >= 8)
        damagetype = 7;
    if (damagetype < 0)
        damagetype = 0;

    duration = (s32)g_DamageTypes[damagetype].field_0x8;
    if ((s32)g_DamageTypes[damagetype].flashEndFrame > duration)
        duration = (s32)g_DamageTypes[damagetype].flashEndFrame;

    g_ModMpBotRuntime[slot].damagegracetimer60 = (s16)duration;

    modMpBotsPlayLocalSfx(victim, BOND_GET_HIT1_SFX);
    play_sound_for_shot_actor(victim);
}

void modMpBotsNotifyHumanHit(ChrRecord *victim, s32 playernum)
{
    s32 slot = modMpBotsGetSlotForChr(victim);

    if (slot >= 0 && (u32)playernum < MAX_PLAYER_COUNT
        && victim != NULL && victim->actiontype != ACT_DIE
        && victim->actiontype != ACT_DEAD)
        g_ModMpBotRuntime[slot].lastattacker = (s16)playernum;
}

void modMpBotsNotifyBotHit(ChrRecord *victim, ChrRecord *attacker)
{
    s32 victimslot = modMpBotsGetSlotForChr(victim);
    s32 attackerslot = modMpBotsGetSlotForChr(attacker);

    if (victimslot >= 0 && attackerslot >= 0 && victimslot != attackerslot
        && victim != NULL && victim->actiontype != ACT_DIE
        && victim->actiontype != ACT_DEAD)
        g_ModMpBotRuntime[victimslot].lastattacker =
            (s16)(MOD_MP_BOT_BOT_TARGET_BASE + attackerslot);
}

s32 modMpBotsGetHumanKills(s32 playernum)
{
    if ((u32)playernum >= MAX_PLAYER_COUNT)
        return 0;

    return g_ModMpBotHumanKills[playernum];
}

s32 modMpBotsGetBotKills(s32 slot)
{
    if ((u32)slot >= MOD_MP_BOT_MAX)
        return 0;

    return g_ModMpBotRuntime[slot].kills;
}

s32 modMpBotsGetBotDeaths(s32 slot)
{
    if ((u32)slot >= MOD_MP_BOT_MAX)
        return 0;

    return g_ModMpBotRuntime[slot].deaths;
}

void modMpBotsRecordBotKillOnHuman(s32 slot, s32 playernum)
{
    if ((u32)slot < MOD_MP_BOT_MAX && (u32)playernum < MAX_PLAYER_COUNT)
        g_ModMpBotRuntime[slot].kills++;
}

void modMpBotsDamageViewer(s32 slot, PropRecord *viewer, ITEM_IDS weaponid, coord3d *direction)
{
    s32 oldplayer;
    s32 victim;
    f32 damage;

    if ((u32)slot >= MOD_MP_BOT_MAX || viewer == NULL || viewer->type != PROP_TYPE_VIEWER)
        return;

    victim = getPlayerPointerIndex(viewer);
    if ((u32)victim >= (u32)getPlayerCount())
        return;

    damage = 0.125f * gunItemGetDestructionAmount(weaponid) * g_AiDamageModifier * get_007_damage_mod();
    if (weaponid == ITEM_SHOTGUN || weaponid == ITEM_AUTOSHOT)
        damage *= 3.0f;

    oldplayer = get_cur_playernum();
    set_cur_player(victim);
    modMpBotsSetDamageOwner(slot);
    record_damage_kills(damage, direction->x, direction->z, victim, 1);
    modMpBotsClearDamageOwner();
    set_cur_player(oldplayer);
}

static PropRecord *modMpBotsResolveTargetCode(s16 code)
{
    s32 index;

    if (code >= MOD_MP_BOT_BOT_TARGET_BASE)
    {
        index = code - MOD_MP_BOT_BOT_TARGET_BASE;
        if ((u32)index < MOD_MP_BOT_MAX
            && g_ModMpBotRuntime[index].chr != NULL
            && g_ModMpBotRuntime[index].chr->prop != NULL
            && !chrIsDead(g_ModMpBotRuntime[index].chr))
            return g_ModMpBotRuntime[index].chr->prop;
        return NULL;
    }

    if ((u32)code < (u32)getPlayerCount()
        && g_playerPointers[code] != NULL
        && g_playerPointers[code]->prop != NULL
        && !g_playerPointers[code]->bonddead)
        return g_playerPointers[code]->prop;

    return NULL;
}

static f32 modMpBotsTargetHealth(PropRecord *prop)
{
    s32 playernum;
    ChrRecord *chr;

    if (prop == NULL)
        return 999999.0f;

    if (prop->type == PROP_TYPE_VIEWER)
    {
        playernum = getPlayerPointerIndex(prop);
        if ((u32)playernum < (u32)getPlayerCount())
            return g_playerPointers[playernum]->bondhealth + g_playerPointers[playernum]->bondarmour;
        return 999999.0f;
    }

    if (prop->type == PROP_TYPE_CHR && prop->chr != NULL)
    {
        chr = prop->chr;
        if (chr->maxdamage > 0.0f)
            return (chr->maxdamage - chr->damage) / chr->maxdamage;
        return 999999.0f;
    }

    return 999999.0f;
}

static s32 modMpBotsHumanScore(s32 playernum)
{
    s32 i;
    s32 points = modMpBotsGetHumanKills(playernum);

    if ((u32)playernum >= (u32)getPlayerCount())
        return -9999;

    for (i = 0; i < getPlayerCount(); i++)
    {
        if (i != playernum)
            points += g_playerPlayerData[playernum].kill_counts[i];
        else
            points -= g_playerPlayerData[i].kill_counts[playernum];
    }

    return points;
}




/* R30 P2 - direct botinv/botact compatibility layer. */
static void modMpBotsPdGetWeaponConfig(ITEM_IDS item, struct ModMpBotPdWeaponConfig *cfg)
{
    cfg->score1 = 0;
    cfg->score2 = 0;
    cfg->supported = 1;
    cfg->haspriammogoal = 1;
    cfg->pridistconfig = MODBOT_PD_DISTCFG_DEFAULT;
    cfg->targetammopri = 0;
    cfg->criticalammopri = 0;
    cfg->reloaddelay = 1;
    cfg->allowpartialreloaddelay = 0;
    cfg->throwable = 0;
    cfg->melee = 0;

    switch (item)
    {
        case ITEM_UNARMED:
        case ITEM_FIST:
            /* PD WEAPON_UNARMED. */
            cfg->score1 = 13; cfg->score2 = 13;
            cfg->pridistconfig = MODBOT_PD_DISTCFG_CLOSE;
            cfg->targetammopri = 0; cfg->criticalammopri = 0;
            cfg->reloaddelay = 0; cfg->melee = 1;
            break;
        case ITEM_KNIFE:
            /* PD WEAPON_COMBATKNIFE primary. */
            cfg->score1 = 20; cfg->score2 = 40;
            cfg->pridistconfig = MODBOT_PD_DISTCFG_CLOSE;
            cfg->targetammopri = 0; cfg->criticalammopri = 0;
            cfg->reloaddelay = 1; cfg->melee = 1;
            break;
        case ITEM_THROWKNIFE:
            /* PD combat-knife ranged role; GE has no alternate-fire selector. */
            cfg->score1 = 40; cfg->score2 = 40;
            cfg->pridistconfig = MODBOT_PD_DISTCFG_DEFAULT;
            cfg->targetammopri = 5; cfg->criticalammopri = 1;
            cfg->reloaddelay = 1; cfg->throwable = 1;
            break;
        case ITEM_WPPK:
        case ITEM_TT33:
            /* PD Falcon 2. */
            cfg->score1 = 56; cfg->score2 = 60;
            cfg->pridistconfig = MODBOT_PD_DISTCFG_PISTOL;
            cfg->targetammopri = 30; cfg->criticalammopri = 10;
            cfg->reloaddelay = 1;
            break;
        case ITEM_WPPKSIL:
            /* PD Falcon 2 Silencer. */
            cfg->score1 = 52; cfg->score2 = 60;
            cfg->pridistconfig = MODBOT_PD_DISTCFG_PISTOL;
            cfg->targetammopri = 30; cfg->criticalammopri = 10;
            cfg->reloaddelay = 1;
            break;
        case ITEM_SILVERWPPK:
            /* PD MagSec 4 primary. */
            cfg->score1 = 76; cfg->score2 = 88;
            cfg->pridistconfig = MODBOT_PD_DISTCFG_PISTOL;
            cfg->targetammopri = 30; cfg->criticalammopri = 10;
            cfg->reloaddelay = 1;
            break;
        case ITEM_RUGER:
            /* PD DY357 Magnum. */
            cfg->score1 = 68; cfg->score2 = 76;
            cfg->pridistconfig = MODBOT_PD_DISTCFG_PISTOL;
            cfg->targetammopri = 30; cfg->criticalammopri = 8;
            cfg->reloaddelay = 3;
            break;
        case ITEM_GOLDENGUN:
        case ITEM_GOLDWPPK:
            /* PD DY357-LX high-value pistol role. */
            cfg->score1 = 180; cfg->score2 = 188;
            cfg->pridistconfig = MODBOT_PD_DISTCFG_PISTOL;
            cfg->targetammopri = 20; cfg->criticalammopri = 6;
            cfg->reloaddelay = 3;
            break;
        case ITEM_SKORPION:
        case ITEM_UZI:
        case ITEM_MP5K:
        case ITEM_MP5KSIL:
            /* PD CMP150. */
            cfg->score1 = 116; cfg->score2 = 128;
            cfg->pridistconfig = MODBOT_PD_DISTCFG_DEFAULT;
            cfg->targetammopri = 100; cfg->criticalammopri = 30;
            cfg->reloaddelay = 2;
            break;
        case ITEM_SPECTRE:
            /* PD Cyclone. */
            cfg->score1 = 120; cfg->score2 = 128;
            cfg->pridistconfig = MODBOT_PD_DISTCFG_DEFAULT;
            cfg->targetammopri = 150; cfg->criticalammopri = 50;
            cfg->reloaddelay = 2;
            break;
        case ITEM_AK47:
            /* PD K7 Avenger. */
            cfg->score1 = 156; cfg->score2 = 180;
            cfg->pridistconfig = MODBOT_PD_DISTCFG_DEFAULT;
            cfg->targetammopri = 150; cfg->criticalammopri = 40;
            cfg->reloaddelay = 2;
            break;
        case ITEM_M16:
            /* PD AR34. */
            cfg->score1 = 148; cfg->score2 = 176;
            cfg->pridistconfig = MODBOT_PD_DISTCFG_DEFAULT;
            cfg->targetammopri = 120; cfg->criticalammopri = 40;
            cfg->reloaddelay = 2;
            break;
        case ITEM_FNP90:
            /* PD RCP-120 primary. */
            cfg->score1 = 172; cfg->score2 = 188;
            cfg->pridistconfig = MODBOT_PD_DISTCFG_DEFAULT;
            cfg->targetammopri = 300; cfg->criticalammopri = 40;
            cfg->reloaddelay = 2;
            break;
        case ITEM_SHOTGUN:
        case ITEM_AUTOSHOT:
            /* PD Shotgun primary. */
            cfg->score1 = 140; cfg->score2 = 156;
            cfg->pridistconfig = MODBOT_PD_DISTCFG_PISTOL;
            cfg->targetammopri = 18; cfg->criticalammopri = 8;
            cfg->reloaddelay = 6; cfg->allowpartialreloaddelay = 1;
            break;
        case ITEM_SNIPERRIFLE:
            /* PD Sniper Rifle primary. */
            cfg->score1 = 28; cfg->score2 = 40;
            cfg->pridistconfig = MODBOT_PD_DISTCFG_DEFAULT;
            cfg->targetammopri = 30; cfg->criticalammopri = 10;
            cfg->reloaddelay = 2;
            break;
        case ITEM_LASER:
        case ITEM_WATCHLASER:
            /* PD Laser primary. */
            cfg->score1 = 112; cfg->score2 = 112;
            cfg->pridistconfig = MODBOT_PD_DISTCFG_DEFAULT;
            cfg->targetammopri = 0; cfg->criticalammopri = 0;
            cfg->reloaddelay = 1;
            break;
        case ITEM_GRENADELAUNCH:
            /* PD Devastator primary. */
            cfg->score1 = 176; cfg->score2 = 188;
            cfg->pridistconfig = MODBOT_PD_DISTCFG_SHOOTEXPLOSIVE;
            cfg->targetammopri = 20; cfg->criticalammopri = 4;
            cfg->reloaddelay = 2;
            break;
        case ITEM_ROCKETLAUNCH:
            /* PD Rocket Launcher primary. */
            cfg->score1 = 160; cfg->score2 = 188;
            cfg->pridistconfig = MODBOT_PD_DISTCFG_SHOOTEXPLOSIVE;
            cfg->targetammopri = 2; cfg->criticalammopri = 1;
            cfg->reloaddelay = 2;
            break;
        case ITEM_GRENADE:
            /* PD Grenade primary. */
            cfg->score1 = 36; cfg->score2 = 172;
            cfg->pridistconfig = MODBOT_PD_DISTCFG_THROWEXPLOSIVE;
            cfg->targetammopri = 6; cfg->criticalammopri = 2;
            cfg->reloaddelay = 1; cfg->throwable = 1;
            break;
        case ITEM_TIMEDMINE:
            /* PD Timed Mine primary: usable, but haspriammogoal is false. */
            cfg->score1 = 12; cfg->score2 = 12;
            cfg->haspriammogoal = 0;
            cfg->pridistconfig = MODBOT_PD_DISTCFG_THROWEXPLOSIVE;
            cfg->targetammopri = 5; cfg->criticalammopri = 1;
            cfg->reloaddelay = 1; cfg->throwable = 1;
            break;
        case ITEM_PROXIMITYMINE:
            /* PD Proximity Mine primary: usable, but haspriammogoal is false. */
            cfg->score1 = 40; cfg->score2 = 176;
            cfg->haspriammogoal = 0;
            cfg->pridistconfig = MODBOT_PD_DISTCFG_THROWEXPLOSIVE;
            cfg->targetammopri = 5; cfg->criticalammopri = 1;
            cfg->reloaddelay = 1; cfg->throwable = 1;
            break;
        case ITEM_REMOTEMINE:
            /* PD Remote Mine primary. */
            cfg->score1 = 44; cfg->score2 = 156;
            cfg->pridistconfig = MODBOT_PD_DISTCFG_DEFAULT;
            cfg->targetammopri = 5; cfg->criticalammopri = 2;
            cfg->reloaddelay = 1; cfg->throwable = 1;
            break;
        default:
            /* Unsupported/gadget rows remain neutral like PD's zero rows. */
            cfg->supported = 0;
            cfg->haspriammogoal = 0;
            break;
    }
}

static s32 modMpBotsPdAmmoType(ITEM_IDS item)
{
    WeaponStats *stats;

    if (item == ITEM_UNARMED || item == ITEM_FIST || item == ITEM_KNIFE)
        return AMMO_NONE;

    stats = get_ptr_item_statistics(item);
    if (stats == NULL)
        return AMMO_NONE;

    return stats->AmmoType;
}

static s32 modMpBotsPdClipCapacity(ITEM_IDS item)
{
    WeaponStats *stats;

    if (item == ITEM_UNARMED || item == ITEM_FIST || item == ITEM_KNIFE)
        return 0;

    stats = get_ptr_item_statistics(item);
    if (stats == NULL || stats->MagSize < 0)
        return 0;

    return stats->MagSize;
}

static s32 modMpBotsPdAmmoQuantity(struct ModMpBotRuntime *runtime, ITEM_IDS item, s32 includeequipped)
{
    s32 ammotype = modMpBotsPdAmmoType(item);
    s32 qty;

    if (ammotype == AMMO_NONE)
        return 0x7fff;
    if ((u32)ammotype >= AMMOTYPE_MAX)
        return 0;

    qty = runtime->ammoheld[ammotype];
    if (includeequipped && item == runtime->weaponnum)
        qty += runtime->loadedammo[GUNRIGHT] + runtime->loadedammo[GUNLEFT];

    return qty;
}

static void modMpBotsPdGiveAmmo(struct ModMpBotRuntime *runtime, s32 ammotype, s32 qty)
{
    s32 max;

    if ((u32)ammotype >= AMMOTYPE_MAX || ammotype == AMMO_NONE || qty <= 0)
        return;

    runtime->ammoheld[ammotype] += qty;
    max = get_max_ammo_for_type(ammotype);
    if (runtime->ammoheld[ammotype] > max)
        runtime->ammoheld[ammotype] = max;
}

static s32 modMpBotsPdTakeAmmo(struct ModMpBotRuntime *runtime, s32 ammotype, s32 qty)
{
    s32 actual;

    if ((u32)ammotype >= AMMOTYPE_MAX || ammotype == AMMO_NONE || qty <= 0)
        return 0;

    actual = qty;
    if (actual > runtime->ammoheld[ammotype])
        actual = runtime->ammoheld[ammotype];

    runtime->ammoheld[ammotype] -= actual;
    return actual;
}

static s32 modMpBotsPdInventoryHas(struct ModMpBotRuntime *runtime, ITEM_IDS item)
{
    s32 i;

    if (item == ITEM_UNARMED)
        return TRUE;

    for (i = 0; i < runtime->inventorycount; i++)
    {
        if (runtime->inventory[i] == item)
            return TRUE;
    }

    return FALSE;
}

static s32 modMpBotsPdInventoryGive(struct ModMpBotRuntime *runtime, ITEM_IDS item, s16 pad)
{
    s32 i;

    if (item == ITEM_UNARMED)
        return 1;

    for (i = 0; i < runtime->inventorycount; i++)
    {
        if (runtime->inventory[i] == item)
        {
            /* PD: a second copy becomes INVITEMTYPE_DUAL only from a different pad. */
            if (runtime->inventorycopies[i] < 2
                && runtime->inventorypads[i] != pad
                && bondwalkItemCheckBitflags(item, WEAPONSTATBITFLAG_CAN_DUAL_WIELD))
            {
                runtime->inventorycopies[i] = 2;
            }

            return runtime->inventorycopies[i];
        }
    }

    if (runtime->inventorycount >= MODBOT_PD_INV_MAX)
        return 0;

    i = runtime->inventorycount++;
    runtime->inventory[i] = item;
    runtime->inventorycopies[i] = 1;
    runtime->inventorypads[i] = pad;
    return 1;
}

static s32 modMpBotsPdInventoryCopies(struct ModMpBotRuntime *runtime, ITEM_IDS item)
{
    s32 i;

    if (item == ITEM_UNARMED)
        return 1;

    for (i = 0; i < runtime->inventorycount; i++)
    {
        if (runtime->inventory[i] == item)
            return runtime->inventorycopies[i];
    }

    return 0;
}

static s32 modMpBotsPdWeaponScore(s32 slot, ITEM_IDS item, s32 comparewithtarget)
{
    struct ModMpBotRuntime *runtime = &g_ModMpBotRuntime[slot];
    struct ModMpBotPdWeaponConfig cfg;
    s32 difficulty = g_ModMpBotConfigs[slot].difficulty;
    s32 score;
    s32 extra = 0;

    modMpBotsPdGetWeaponConfig(item, &cfg);
    if (!cfg.supported)
        return 0;

    if (modMpBotHasTrait(slot, MODBOT_TRAIT_MELEE)
        && cfg.pridistconfig != MODBOT_PD_DISTCFG_CLOSE)
        return 0;

    /* PD botinv_score_weapon scores the weapon role itself; ammo availability
     * is checked by botinv_tick after scoring.  Keeping those steps separate
     * is also required for CowardSim to score an opponent's weapon correctly. */
    score = cfg.score1;

    /* Exact PD RocketSim weighting, adapted onto the corresponding GE roles. */
    if (difficulty == MODBOT_DIFFICULTY_MEAT)
        extra = 100;
    else if (difficulty == MODBOT_DIFFICULTY_EASY)
        extra = 50;

    if (modMpBotHasTrait(slot, MODBOT_TRAIT_PYROMANIAC))
    {
        if (item == ITEM_ROCKETLAUNCH)
            score = extra + 300;
        else if (item == ITEM_GRENADELAUNCH)
            score = extra + 280;
        else if (item == ITEM_GRENADE)
            score = extra + 240;
    }

    /* PD perturbs some equal-function choices with random1. */
    if (item == ITEM_SHOTGUN || item == ITEM_AUTOSHOT || item == ITEM_ROCKETLAUNCH)
    {
        if ((runtime->random1 & 1U) == 0)
            score--;
    }

    (void)comparewithtarget;
    return score;
}

static s32 modMpBotsPdTargetWeaponNum(PropRecord *prop)
{
    s32 playernum;
    s32 botslot;
    PropRecord *weaponprop;

    if (prop == NULL)
        return ITEM_UNARMED;

    if (prop->type == PROP_TYPE_VIEWER)
    {
        playernum = getPlayerPointerIndex(prop);
        if ((u32)playernum < (u32)getPlayerCount() && g_playerPointers[playernum] != NULL)
            return g_playerPointers[playernum]->hands[GUNRIGHT].weaponnum;
        return ITEM_UNARMED;
    }

    if (prop->type == PROP_TYPE_CHR && prop->chr != NULL)
    {
        botslot = modMpBotsGetSlotForChr(prop->chr);
        if (botslot >= 0)
            return g_ModMpBotRuntime[botslot].weaponnum;

        weaponprop = chrGetEquippedWeaponProp(prop->chr, GUNRIGHT);
        if (weaponprop != NULL && weaponprop->weapon != NULL)
            return weaponprop->weapon->weaponnum;
    }

    return ITEM_UNARMED;
}

static s32 modMpBotsPdPassesCowardCheck(s32 slot, PropRecord *prop)
{
    s32 myscore;
    s32 theirscore;

    if (!modMpBotHasTrait(slot, MODBOT_TRAIT_FEARFUL))
        return TRUE;

    myscore = modMpBotsPdWeaponScore(slot, g_ModMpBotRuntime[slot].weaponnum, FALSE);

    {
        ITEM_IDS other = (ITEM_IDS)modMpBotsPdTargetWeaponNum(prop);
        theirscore = modMpBotsPdWeaponScore(slot, other, FALSE);
    }

    /* Exact PD CowardSim threshold: theirscore1 >= myscore1 - 30. */
    return theirscore < myscore - 30;
}

static void modMpBotsPdReload(s32 slot, s32 hand)
{
    struct ModMpBotRuntime *runtime = &g_ModMpBotRuntime[slot];
    s32 capacity = modMpBotsPdClipCapacity(runtime->weaponnum);
    s32 ammotype = modMpBotsPdAmmoType(runtime->weaponnum);
    s32 tryamount;
    s32 actual;

    runtime->timeuntilreload60[hand] = 0;

    if (capacity <= 0 || ammotype == AMMO_NONE)
        return;
    if (runtime->chr->weapons_held[hand] == NULL)
        return;

    tryamount = capacity - runtime->loadedammo[hand];
    actual = modMpBotsPdTakeAmmo(runtime, ammotype, tryamount);
    runtime->loadedammo[hand] += actual;
}

static void modMpBotsPdScheduleReload(s32 slot, s32 hand)
{
    struct ModMpBotRuntime *runtime = &g_ModMpBotRuntime[slot];
    struct ModMpBotPdWeaponConfig cfg;
    s32 capacity;

    modMpBotsPdGetWeaponConfig(runtime->weaponnum, &cfg);
    runtime->timeuntilreload60[hand] = (s16)(cfg.reloaddelay * 60);
    modMpBotsPlayLocalSfx(runtime->chr, GUN_RIFLECOCK_SFX);

    if (cfg.allowpartialreloaddelay)
    {
        capacity = modMpBotsPdClipCapacity(runtime->weaponnum);
        if (capacity > 0)
        {
            runtime->timeuntilreload60[hand] = (s16)(runtime->timeuntilreload60[hand]
                * (capacity - runtime->loadedammo[hand]) / capacity);
        }
    }
}

static s32 modMpBotsPdShotInterval(ITEM_IDS item)
{
    WeaponStats *stats;
    s32 automatic;
    s32 single;

    stats = get_ptr_item_statistics(item);
    if (stats == NULL)
        return 1;

    automatic = (s8)stats->AutomaticFiringRate;
    single = stats->SingleFiringRate;

    if (automatic > 0)
        return automatic;
    if (single > 0)
        return single;
    return 1;
}

static s32 modMpBotsWeaponIsExplosive(s32 item)
{
    return item == ITEM_ROCKETLAUNCH
        || item == ITEM_GRENADELAUNCH
        || item == ITEM_GRENADE
        || item == ITEM_TIMEDMINE
        || item == ITEM_PROXIMITYMINE
        || item == ITEM_REMOTEMINE;
}

static s32 modMpBotsChooseWeaponIndex(s32 slot)
{
    struct s_mp_weapon_set *set = getPtrMPWeaponSetData();
    s32 i;
    s32 best = -1;

    if (modMpBotHasTrait(slot, MODBOT_TRAIT_MELEE))
        return -1;

    if (modMpBotHasTrait(slot, MODBOT_TRAIT_PYROMANIAC))
    {
        for (i = 7; i >= 0; i--)
        {
            if (set[i].allowpickup && modMpBotsWeaponIsExplosive(set[i].itemID)
                && (set[i].itemID == ITEM_ROCKETLAUNCH || set[i].itemID == ITEM_GRENADELAUNCH))
                return i;
        }
    }

    for (i = 7; i >= 0; i--)
    {
        if (set[i].allowpickup && set[i].itemID != ITEM_UNARMED
            && !modMpBotsWeaponIsExplosive(set[i].itemID))
        {
            best = i;
            break;
        }
    }

    if (best < 0)
    {
        for (i = 7; i >= 0; i--)
        {
            if (set[i].allowpickup && set[i].itemID != ITEM_UNARMED)
                return i;
        }
    }

    return best;
}

static s32 modMpBotsSpawn(s32 slot)
{
    struct ModMpBotRuntime *runtime = &g_ModMpBotRuntime[slot];
    struct ModMpBotConfig *config = &g_ModMpBotConfigs[slot];
    struct s_mp_weapon_set *set;
    PadRecord *pad;
    PropRecord *prop;
    ChrRecord *chr;
    s32 body;
    s32 head;
    s32 weaponindex;
    f32 angle;
    coord3d spawnpos;

    if (startpadcount <= 0 || !frontGetMpCharacterData(config->character, &body, &head, NULL))
        return FALSE;

    /* Resources are prepared while lvlReset still owns the stage-load phase. */
    if (!runtime->resourcesready)
        return FALSE;

    pad = g_Startpad[(slot + (randomGetNext() % (u32)startpadcount)) % startpadcount];
    if (pad == NULL || pad->stan == NULL)
        return FALSE;

    angle = atan2f(pad->look.x, pad->look.z);
    spawnpos = pad->pos;
    spawnpos.y = stanGetPositionYValue(pad->stan, spawnpos.x, spawnpos.z);
    prop = chrSpawnAtCoord(body, head, &spawnpos, pad->stan, angle, NULL, 0);
    if (prop == NULL || prop->chr == NULL)
        return FALSE;

    chr = prop->chr;
    /* R30 P1: aim/movement difficulty now lives in the PD bot state. */
    chr->accuracyrating = 0;
    chr->speedrating = 0;
    chrSetMaxDamage(chr, 4.0f);
    chr->damage = 0.0f;
    chr->chrflags |= CHRFLAG_CAN_SHOOT_CHRS;

    /* PD ShieldSim/TurtleSim durability maps to GoldenEye's native character
     * armor representation: negative chr->damage, exposed by chrGetArmor(). */
    if (config->traits & MODBOT_TRAIT_JUGGERNAUT)
        chrAddHealth(chr, 8.0f);
    else if (config->traits & MODBOT_TRAIT_ARMOR_SPECIALIST)
        chrAddHealth(chr, 4.0f);

    runtime->chr = chr;
    runtime->targetprop = NULL;
    runtime->lastattacker = MODBOT_ATTACKER_NONE;
    runtime->respawntimer = 0;
    runtime->damagegracetimer60 = 0;
    runtime->deathrecorded = FALSE;
    runtime->spawnready = FALSE;
    runtime->speedmultforwards = 0.0f;
    runtime->speedmultsideways = 0.0f;
    runtime->distmode = -1;
    runtime->manualbackup = FALSE;
    runtime->attackingparticipant = -1;
    runtime->distoverridecode = MODBOT_TARGET_NONE;
    runtime->roty = angle;
    runtime->angleoffset = 0.0f;
    runtime->speedtheta = 0.0f;
    runtime->lookangle = angle;
    runtime->moveratex = 0.0f;
    runtime->moveratez = 0.0f;
    runtime->distmodettl60 = 0;
    runtime->distoverridetimer60 = 0;
    runtime->shootdelaytimer60 = 0;
    runtime->targetlastseen60 = -1;
    runtime->lastseenanytarget60 = -1;
    runtime->targetinsight = FALSE;
    runtime->queryplayernum = -1;
    runtime->zeroangle = 0.0f;
    runtime->zerospeed = 0.0f;
    runtime->zeroinc = 0.0f;
    runtime->random3ttl60 = 0;
    runtime->random3 = randomGetNext();
    runtime->curzerotimer60 = 0.0f;
    runtime->abortattacktimer60 = -1;
    runtime->realignangleframe = g_GlobalTimer;
    runtime->weaponnum = ITEM_UNARMED;
    runtime->gunfunc = MODBOT_PD_FUNC_PRIMARY;
    runtime->ismeleeweapon = TRUE;
    runtime->inventorycount = 1;
    runtime->loadedammo[GUNRIGHT] = 0;
    runtime->loadedammo[GUNLEFT] = 0;
    runtime->timeuntilreload60[GUNRIGHT] = 0;
    runtime->timeuntilreload60[GUNLEFT] = 0;
    runtime->nextbullettimer60[GUNRIGHT] = 0;
    runtime->nextbullettimer60[GUNLEFT] = 0;
    runtime->punchtimer60[GUNRIGHT] = 0;
    runtime->punchtimer60[GUNLEFT] = -1;
    runtime->changeguntimer60 = 0;
    runtime->throwtimer60 = 0;
    runtime->burstsdone[GUNRIGHT] = 0;
    runtime->burstsdone[GUNLEFT] = 0;
    runtime->random1 = randomGetNext();
    runtime->random1ttl60 = 0;
    runtime->inventory[0] = ITEM_UNARMED;
    runtime->inventorycopies[0] = 1;
    runtime->inventorypads[0] = -1;
        runtime->navgoalstan = NULL;
    runtime->navgoalpos = prop->pos;
    runtime->navtargetcode = MODBOT_TARGET_NONE;
    runtime->navage = 0;
    runtime->navindex = 0;
    runtime->navcount = 0;
    runtime->navactive = FALSE;

    {
        s32 j;
        for (j = 0; j < MAX_CHRWAYPOINTS; j++)
            runtime->navtiles[j] = NULL;
        for (j = 1; j < MODBOT_PD_INV_MAX; j++)
        {
            runtime->inventory[j] = ITEM_UNARMED;
            runtime->inventorycopies[j] = 0;
            runtime->inventorypads[j] = -1;
        }
        for (j = 0; j < AMMOTYPE_MAX; j++)
            runtime->ammoheld[j] = 0;
        for (j = 0; j < MODBOT_PD_MAX_PARTICIPANTS; j++)
        {
            runtime->chrnumsbydistanceasc[j] = -1;
            runtime->chrdistances[j] = 99999999.0f;
            runtime->chrsinsight[j] = FALSE;
            runtime->chrslastseen60[j] = -1;
        }
    }

    check_set_actor_standing_still(chr, 0, 0);

    /* PD's ordinary Simulants enter play unarmed and acquire their loadout
     * through bot inventory/pickup logic.  Preserve the requested Dark/
     * unfair starting-weapon privilege only for the Cheap trait. */
    weaponindex = -1;
    if (config->traits & MODBOT_TRAIT_CHEAP)
        weaponindex = modMpBotsChooseWeaponIndex(slot);

    if (weaponindex >= 0)
    {
        ITEM_IDS item;
        s32 ammotype;
        s32 capacity;
        s32 totalammo;
        PropRecord *givenweapon;
        struct ModMpBotPdWeaponConfig weaponcfg;

        set = getPtrMPWeaponSetData();
        item = (ITEM_IDS)set[weaponindex].itemID;
        givenweapon = chrGiveWeapon(chr, set[weaponindex].propID, item, 0);
        if (givenweapon == NULL)
            return TRUE;

        modMpBotsPdInventoryGive(runtime, item, -1);
        runtime->weaponnum = item;
        runtime->gunfunc = MODBOT_PD_FUNC_PRIMARY;
        modMpBotsPdGetWeaponConfig(item, &weaponcfg);
        runtime->ismeleeweapon = weaponcfg.melee;

        ammotype = modMpBotsPdAmmoType(item);
        capacity = modMpBotsPdClipCapacity(item);
        totalammo = set[weaponindex].ammoamount;

        if (ammotype != AMMO_NONE && capacity > 0)
        {
            if (totalammo < capacity)
                totalammo = capacity;
            runtime->loadedammo[GUNRIGHT] = capacity;
            modMpBotsPdGiveAmmo(runtime, ammotype, totalammo - capacity);
        }
        else if (ammotype == AMMO_NONE)
        {
            /* Laser-style weapons are ammo-less in GE multiplayer. */
            runtime->loadedammo[GUNRIGHT] = 1;
        }
    }

    return TRUE;
}

static void modMpBotsInvalidateTargetProp(PropRecord *victim)
{
    s32 i;

    if (victim == NULL)
        return;

    for (i = 0; i < g_ModMpBotCount; i++)
    {
        if (g_ModMpBotRuntime[i].targetprop == victim)
        {
            g_ModMpBotRuntime[i].targetprop = NULL;
            g_ModMpBotRuntime[i].attackingparticipant = -1;
            g_ModMpBotRuntime[i].targetinsight = FALSE;
            modMpBotsPdNavClear(&g_ModMpBotRuntime[i]);
        }
    }
}

static void modMpBotsRecordDeath(s32 slot)
{
    struct ModMpBotRuntime *runtime = &g_ModMpBotRuntime[slot];
    s32 attacker = runtime->lastattacker;

    runtime->deaths++;

    if (modMpBotHasTrait(slot, MODBOT_TRAIT_VINDICTIVE)
        && attacker != MODBOT_ATTACKER_NONE)
    {
        runtime->vindicttarget = (s16)attacker;
    }

    if (modMpBotHasTrait(slot, MODBOT_TRAIT_STALKER)
        && runtime->stalktarget == MODBOT_TARGET_NONE
        && attacker != MODBOT_ATTACKER_NONE)
    {
        /* FeudSim latches an opponent; last killer is PD's preferred seed. */
        runtime->stalktarget = (s16)attacker;
    }

    if ((u32)attacker < MAX_PLAYER_COUNT)
    {
        g_ModMpBotHumanKills[attacker]++;
    }
    else if (attacker >= MOD_MP_BOT_BOT_TARGET_BASE)
    {
        s32 killerslot = attacker - MOD_MP_BOT_BOT_TARGET_BASE;
        if ((u32)killerslot < MOD_MP_BOT_MAX && killerslot != slot)
            g_ModMpBotRuntime[killerslot].kills++;
    }
}




/* R30 P1 helpers: direct translations of PD bot.c/botcmd.c semantics. */
static f32 modMpBotsPdNormalizeAngle(f32 angle)
{
    if (angle >= M_TAU_F)
        angle -= M_TAU_F;
    else if (angle < 0.0f)
        angle += M_TAU_F;
    return angle;
}

static f32 modMpBotsPdAngleDelta(f32 from, f32 to)
{
    f32 delta = to - from;
    if (delta < -M_PI_F)
        delta += M_TAU_F;
    else if (delta >= M_PI_F)
        delta -= M_TAU_F;
    return delta;
}

static s32 modMpBotsPdPropAlive(PropRecord *prop)
{
    s32 playernum;

    if (prop == NULL)
        return FALSE;

    if (prop->type == PROP_TYPE_VIEWER)
    {
        playernum = getPlayerPointerIndex(prop);
        return (u32)playernum < (u32)getPlayerCount()
            && g_playerPointers[playernum] != NULL
            && !g_playerPointers[playernum]->bonddead;
    }

    return prop->type == PROP_TYPE_CHR && prop->chr != NULL && !chrIsDead(prop->chr);
}

static PropRecord *modMpBotsPdGetParticipantProp(s32 participant)
{
    s32 slot;

    if ((u32)participant < MAX_PLAYER_COUNT)
    {
        if ((u32)participant < (u32)getPlayerCount()
            && g_playerPointers[participant] != NULL
            && g_playerPointers[participant]->prop != NULL
            && !g_playerPointers[participant]->bonddead)
            return g_playerPointers[participant]->prop;
        return NULL;
    }

    slot = participant - MAX_PLAYER_COUNT;
    if ((u32)slot < MOD_MP_BOT_MAX
        && g_ModMpBotRuntime[slot].chr != NULL
        && g_ModMpBotRuntime[slot].chr->prop != NULL
        && !chrIsDead(g_ModMpBotRuntime[slot].chr))
        return g_ModMpBotRuntime[slot].chr->prop;

    return NULL;
}

static s32 modMpBotsPdFindParticipant(PropRecord *prop)
{
    s32 i;

    if (prop == NULL)
        return -1;

    if (prop->type == PROP_TYPE_VIEWER)
        return getPlayerPointerIndex(prop);

    if (prop->type == PROP_TYPE_CHR && prop->chr != NULL)
    {
        i = modMpBotsGetSlotForChr(prop->chr);
        if (i >= 0)
            return MAX_PLAYER_COUNT + i;
    }

    return -1;
}

static s16 modMpBotsPdTargetCodeForProp(PropRecord *prop)
{
    s32 participant = modMpBotsPdFindParticipant(prop);
    if (participant < 0)
        return MODBOT_TARGET_NONE;
    if (participant < MAX_PLAYER_COUNT)
        return (s16)participant;
    return (s16)(MOD_MP_BOT_BOT_TARGET_BASE + participant - MAX_PLAYER_COUNT);
}

static f32 modMpBotsPdDistance(ChrRecord *chr, PropRecord *prop)
{
    f32 dx = prop->pos.x - chr->prop->pos.x;
    f32 dy = prop->pos.y - chr->prop->pos.y;
    f32 dz = prop->pos.z - chr->prop->pos.z;
    return sqrtf(dx * dx + dy * dy + dz * dz);
}

static s32 modMpBotsPdHasSight(ChrRecord *chr, PropRecord *prop)
{
    if (chr == NULL || chr->prop == NULL || chr->prop->stan == NULL
        || prop == NULL || prop->stan == NULL)
        return FALSE;

    /* GE compatibility shim for PD chr_has_los_to_chr. */
    return check_if_position_in_same_room(chr, &prop->pos, prop->stan);
}

static s32 modMpBotsPdTargetIsArmed(PropRecord *prop)
{
    s32 playernum;

    if (prop == NULL)
        return FALSE;

    if (prop->type == PROP_TYPE_VIEWER)
    {
        playernum = getPlayerPointerIndex(prop);
        if ((u32)playernum >= (u32)getPlayerCount() || g_playerPointers[playernum] == NULL)
            return FALSE;
        return g_playerPointers[playernum]->hands[GUNRIGHT].weaponnum != ITEM_UNARMED
            && g_playerPointers[playernum]->hands[GUNRIGHT].weaponnum != ITEM_FIST;
    }

    if (prop->type == PROP_TYPE_CHR && prop->chr != NULL)
        return chrGetEquippedWeaponProp(prop->chr, GUNRIGHT) != NULL
            || chrGetEquippedWeaponProp(prop->chr, GUNLEFT) != NULL;

    return FALSE;
}

static s32 modMpBotsPdPassesPeaceCheck(s32 slot, PropRecord *prop)
{
    if (modMpBotHasTrait(slot, MODBOT_TRAIT_PACIFIST))
        return modMpBotsPdTargetIsArmed(prop);
    return TRUE;
}

static void modMpBotsPdSetTarget(s32 slot, PropRecord *prop)
{
    struct ModMpBotRuntime *runtime = &g_ModMpBotRuntime[slot];
    s32 participant = modMpBotsPdFindParticipant(prop);
    s32 changed = runtime->targetprop != prop;

    if (participant >= 0)
    {
        runtime->targetinsight = runtime->chrsinsight[participant];
        runtime->targetlastseen60 = runtime->chrslastseen60[participant];
    }
    else if (prop != NULL)
    {
        runtime->targetinsight = modMpBotsPdHasSight(runtime->chr, prop);
        runtime->targetlastseen60 = runtime->targetinsight ? g_GlobalTimer : -1;
    }
    else
    {
        runtime->targetinsight = FALSE;
        runtime->targetlastseen60 = -1;
    }

    if (runtime->targetlastseen60 > runtime->lastseenanytarget60)
        runtime->lastseenanytarget60 = runtime->targetlastseen60;

    runtime->targetprop = prop;

    if (changed)
    {
        runtime->shootdelaytimer60 = 0;
    }
    else if (runtime->targetinsight)
    {
        runtime->shootdelaytimer60 += g_ClockTimer;
    }
    else
    {
        runtime->shootdelaytimer60 -= g_ClockTimer;
        if (runtime->shootdelaytimer60 < 0)
            runtime->shootdelaytimer60 = 0;
    }
}

static void modMpBotsPdUpdateZeroAngle(s32 slot)
{
    struct ModMpBotRuntime *runtime = &g_ModMpBotRuntime[slot];
    struct ModMpBotPdDifficulty *diff;
    s32 difficulty = g_ModMpBotConfigs[slot].difficulty;
    s32 i;
    f32 minspeed;
    f32 maxspeed;
    f32 tmp;
    f32 frac;

    if ((u32)difficulty >= MODBOT_DIFFICULTY_MAX)
        difficulty = MODBOT_DIFFICULTY_NORMAL;

    diff = &g_ModMpBotPdDifficulties[difficulty];
    runtime->random3ttl60 -= g_ClockTimer;

    if (runtime->random3ttl60 <= 0)
    {
        runtime->random3 = randomGetNext();
        runtime->random3ttl60 = 20 + (s32)(randomGetNext() % 20U);
    }

    if (g_ClockTimer > 0)
    {
        if (runtime->targetinsight)
            runtime->curzerotimer60 += g_GlobalTimerDelta;
        else
            runtime->curzerotimer60 -= g_GlobalTimerDelta;

        tmp = diff->turnunzeromult * (runtime->speedtheta * g_GlobalTimerDelta);
        if (tmp < 0.0f)
            tmp = -tmp;
        runtime->curzerotimer60 -= tmp;
    }

    if (runtime->curzerotimer60 > runtime->shootdelaytimer60)
        runtime->curzerotimer60 = runtime->shootdelaytimer60;
    if (runtime->curzerotimer60 < 0.0f)
        runtime->curzerotimer60 = 0.0f;

    if (runtime->curzerotimer60 >= diff->zerotime60)
    {
        runtime->curzerotimer60 = diff->zerotime60;
        minspeed = 0.0f;
        maxspeed = 0.0f;
    }
    else
    {
        frac = (diff->zerotime60 - runtime->curzerotimer60) / (f32)diff->zerotime60;
        minspeed = diff->minzerospeed * frac;
        maxspeed = diff->maxzerospeed * frac;
    }

    if (maxspeed < diff->forcezerominspeed)
        maxspeed = diff->forcezerominspeed;

    runtime->zeroinc = (maxspeed - minspeed)
        * (runtime->random3 % 0x10000) * (1.0f / 65535.0f) + minspeed;

    if (runtime->random3 & 0x10000)
        runtime->zeroinc = -runtime->zeroinc;

    for (i = 0; i < g_ClockTimer; i++)
        runtime->zerospeed = runtime->zerospeed * 0.97500002384186f + runtime->zeroinc;

    runtime->zeroangle = runtime->zerospeed * 0.024999976158142f;
}

static void modMpBotsPdChooseGeneralTarget(s32 slot)
{
    struct ModMpBotRuntime *runtime = &g_ModMpBotRuntime[slot];
    PropRecord *candidate;
    PropRecord *closestoutofsight = NULL;
    s32 distancesdone[MODBOT_PD_MAX_PARTICIPANTS];
    s32 selfparticipant = MAX_PLAYER_COUNT + slot;
    s32 difficulty = g_ModMpBotConfigs[slot].difficulty;
    s32 query;
    s32 tries;
    s32 i;
    s32 j;

    if ((u32)difficulty >= MODBOT_DIFFICULTY_MAX)
        difficulty = MODBOT_DIFFICULTY_NORMAL;

    /* PD amortises LOS/distance work by updating one multiplayer chr per tick. */
    query = runtime->queryplayernum;
    for (tries = 0; tries < MODBOT_PD_MAX_PARTICIPANTS; tries++)
    {
        query = (query + 1) % MODBOT_PD_MAX_PARTICIPANTS;
        candidate = modMpBotsPdGetParticipantProp(query);
        if (candidate != NULL && query != selfparticipant)
        {
            runtime->queryplayernum = query;
            runtime->chrdistances[query] = modMpBotsPdDistance(runtime->chr, candidate);
            runtime->chrsinsight[query] = modMpBotsPdHasSight(runtime->chr, candidate);
            break;
        }
    }

    for (i = 0; i < MODBOT_PD_MAX_PARTICIPANTS; i++)
    {
        if (runtime->chrsinsight[i])
            runtime->chrslastseen60[i] = g_GlobalTimer;
        distancesdone[i] = FALSE;
        runtime->chrnumsbydistanceasc[i] = -1;
    }

    for (i = 0; i < MODBOT_PD_MAX_PARTICIPANTS; i++)
    {
        s32 closest = -1;
        f32 closestdistance = 0.0f;

        for (j = 0; j < MODBOT_PD_MAX_PARTICIPANTS; j++)
        {
            candidate = modMpBotsPdGetParticipantProp(j);
            if (!distancesdone[j] && j != selfparticipant && candidate != NULL
                && (closest < 0 || runtime->chrdistances[j] < closestdistance))
            {
                closest = j;
                closestdistance = runtime->chrdistances[j];
            }
        }

        if (closest >= 0)
        {
            runtime->chrnumsbydistanceasc[i] = (s8)closest;
            distancesdone[closest] = TRUE;
        }
    }

    modMpBotsPdUpdateZeroAngle(slot);

    if (runtime->targetprop != NULL)
    {
        if (!modMpBotsPdPropAlive(runtime->targetprop)
            || !modMpBotsPdPassesPeaceCheck(slot, runtime->targetprop)
            || !modMpBotsPdPassesCowardCheck(slot, runtime->targetprop))
            runtime->targetprop = NULL;
    }

    if (runtime->targetprop == NULL)
    {
        for (i = 0; i < MODBOT_PD_MAX_PARTICIPANTS; i++)
        {
            s32 participant = runtime->chrnumsbydistanceasc[i];
            if (participant < 0)
                continue;

            candidate = modMpBotsPdGetParticipantProp(participant);
            if (candidate == NULL
                || !modMpBotsPdPassesPeaceCheck(slot, candidate)
                || !modMpBotsPdPassesCowardCheck(slot, candidate))
                continue;

            if (runtime->chrsinsight[participant])
            {
                modMpBotsPdSetTarget(slot, candidate);
                return;
            }

            if (difficulty == MODBOT_DIFFICULTY_MEAT || difficulty == MODBOT_DIFFICULTY_EASY)
            {
                modMpBotsPdSetTarget(slot, candidate);
                return;
            }

            if (closestoutofsight == NULL)
                closestoutofsight = candidate;
        }

        modMpBotsPdSetTarget(slot, closestoutofsight);
        return;
    }

    i = modMpBotsPdFindParticipant(runtime->targetprop);
    if (i >= 0 && runtime->chrsinsight[i])
    {
        modMpBotsPdSetTarget(slot, runtime->targetprop);
        return;
    }

    /* PD opportunistically changes to another visible enemy by distance. */
    for (i = 0; i < MODBOT_PD_MAX_PARTICIPANTS; i++)
    {
        s32 participant = runtime->chrnumsbydistanceasc[i];
        if (participant < 0 || !runtime->chrsinsight[participant])
            continue;

        candidate = modMpBotsPdGetParticipantProp(participant);
        if (candidate != NULL
            && modMpBotsPdPassesPeaceCheck(slot, candidate)
            && modMpBotsPdPassesCowardCheck(slot, candidate))
        {
            modMpBotsPdSetTarget(slot, candidate);
            return;
        }
    }

    modMpBotsPdSetTarget(slot, runtime->targetprop);
}

static PropRecord *modMpBotsCoopChooseTarget(s32 slot, s32 enemy)
{
    struct ModMpBotRuntime *runtime = &g_ModMpBotRuntime[slot];
    PropRecord *best = NULL;
    f32 bestdist = 0.0f;
    s32 i;

    if (enemy)
    {
        for (i = 0; i < g_NumChrSlots; i++)
        {
            ChrRecord *candidate = &g_ChrSlots[i];
            f32 dist;

            if (candidate == runtime->chr || candidate->model == NULL
                || candidate->prop == NULL || candidate->prop->type != PROP_TYPE_CHR
                || chrIsDead(candidate) || modMpBotsGetSlotForChr(candidate) >= 0
                || chrCoopIsEscortCandidate(candidate)
                || !modMpBotsPdHasSight(runtime->chr, candidate->prop))
                continue;

            dist = modMpBotsPdDistance(runtime->chr, candidate->prop);
            if (best == NULL || dist < bestdist)
            {
                best = candidate->prop;
                bestdist = dist;
            }
        }
        return best;
    }

    for (i = 0; i < getPlayerCount(); i++)
    {
        PropRecord *playerprop;
        f32 dist;

        if (g_playerPointers[i] == NULL || g_playerPointers[i]->bonddead
            || g_playerPointers[i]->prop == NULL)
            continue;

        playerprop = g_playerPointers[i]->prop;
        dist = modMpBotsPdDistance(runtime->chr, playerprop);
        if (best == NULL || dist < bestdist)
        {
            best = playerprop;
            bestdist = dist;
        }
    }

    return best;
}

static PropRecord *modMpBotsPdChoosePersonalityAttack(s32 slot)
{
    struct ModMpBotRuntime *runtime = &g_ModMpBotRuntime[slot];
    struct ModMpBotConfig *config = &g_ModMpBotConfigs[slot];
    PropRecord *candidate;
    PropRecord *choice[4];
    s32 votes[MODBOT_PD_MAX_PARTICIPANTS];
    f32 besthealth = 0.0f;
    f32 health;
    s32 bestscore = -99999;
    s32 score;
    s32 bestvotes = 0;
    s32 bestparticipant = -1;
    s32 participant;
    s32 i;
    s32 j;

    for (i = 0; i < MODBOT_PD_MAX_PARTICIPANTS; i++)
        votes[i] = 0;
    for (i = 0; i < 4; i++)
        choice[i] = NULL;

    if (config->traits & MODBOT_TRAIT_VINDICTIVE)
        choice[0] = modMpBotsResolveTargetCode(runtime->vindicttarget);
    if (config->traits & MODBOT_TRAIT_STALKER)
        choice[1] = modMpBotsResolveTargetCode(runtime->stalktarget);

    if (config->traits & MODBOT_TRAIT_EQUALIZER)
    {
        for (i = 0; i < MODBOT_PD_MAX_PARTICIPANTS; i++)
        {
            candidate = modMpBotsPdGetParticipantProp(i);
            if (candidate == NULL || candidate == runtime->chr->prop)
                continue;
            score = i < MAX_PLAYER_COUNT
                ? modMpBotsHumanScore(i)
                : g_ModMpBotRuntime[i - MAX_PLAYER_COUNT].kills;
            if (choice[2] == NULL || score > bestscore)
            {
                choice[2] = candidate;
                bestscore = score;
            }
        }
    }

    if (config->traits & MODBOT_TRAIT_BULLY)
    {
        for (i = 0; i < MODBOT_PD_MAX_PARTICIPANTS; i++)
        {
            candidate = modMpBotsPdGetParticipantProp(i);
            if (candidate == NULL || candidate == runtime->chr->prop)
                continue;
            health = modMpBotsTargetHealth(candidate);
            if (choice[3] == NULL || health < besthealth)
            {
                choice[3] = candidate;
                besthealth = health;
            }
        }
    }

    /* Traits are independent. Each targeting trait casts one vote; matching
     * preferences reinforce each other. Ties retain the original PD-style
     * priority order: Vindictive, Stalker, Equalizer, then Bully. */
    for (j = 0; j < 4; j++)
    {
        candidate = choice[j];
        if (candidate == NULL
            || !modMpBotsPdPassesPeaceCheck(slot, candidate)
            || !modMpBotsPdPassesCowardCheck(slot, candidate))
            continue;
        participant = modMpBotsPdFindParticipant(candidate);
        if (participant >= 0 && ++votes[participant] > bestvotes)
        {
            bestvotes = votes[participant];
            bestparticipant = participant;
        }
    }

    if (bestparticipant >= 0)
        return modMpBotsPdGetParticipantProp(bestparticipant);

    return runtime->targetprop;
}

static s32 modMpBotsPdGetDistConfig(s32 slot)
{
    struct ModMpBotPdWeaponConfig cfg;

    if (modMpBotHasTrait(slot, MODBOT_TRAIT_PSYCHOTIC))
        return MODBOT_PD_DISTCFG_KAZE;

    modMpBotsPdGetWeaponConfig(g_ModMpBotRuntime[slot].weaponnum, &cfg);
    return cfg.pridistconfig;
}

static void modMpBotsPdReleaseHeldHand(ChrRecord *chr, s32 hand)
{
    PropRecord *held;
    ObjectRecord *obj;

    held = chr->weapons_held[hand];
    if (held == NULL)
        return;

    obj = held->obj;

    /* Canonical GE ownership transition; never null weapons_held manually. */
    objDetach(held);

    if (obj != NULL && obj->prop != NULL)
        objFreePermanently(obj, TRUE);
}

static s32 modMpBotsPdMaterializeHand(s32 slot, ITEM_IDS item, s32 hand)
{
    struct ModMpBotRuntime *runtime = &g_ModMpBotRuntime[slot];
    PROP propid;
    u32 flags;

    propid = getPropForHeldItem(item);
    if ((s32)propid < 0)
        return FALSE;

    flags = hand == GUNLEFT ? PROPFLAG_WEAPON_LEFTHANDED : 0;
    return chrGiveWeapon(runtime->chr, propid, item, flags) != NULL;
}

static s32 modMpBotsPdSwitchToWeapon(s32 slot, ITEM_IDS item)
{
    struct ModMpBotRuntime *runtime = &g_ModMpBotRuntime[slot];
    struct ModMpBotPdWeaponConfig cfg;
    s32 oldammotype;
    s32 changinggun;
    s32 wantdual;
    s32 hand;

    if (item != ITEM_UNARMED && !modMpBotsPdInventoryHas(runtime, item))
        item = ITEM_UNARMED;

    changinggun = item != runtime->weaponnum;

    if (changinggun)
    {
        oldammotype = modMpBotsPdAmmoType(runtime->weaponnum);

        if (oldammotype != AMMO_NONE)
        {
            for (hand = GUNRIGHT; hand <= GUNLEFT; hand++)
            {
                if (runtime->loadedammo[hand] > 0)
                    modMpBotsPdGiveAmmo(runtime, oldammotype, runtime->loadedammo[hand]);
                runtime->loadedammo[hand] = 0;
            }
        }

        modMpBotsPdReleaseHeldHand(runtime->chr, GUNLEFT);
        modMpBotsPdReleaseHeldHand(runtime->chr, GUNRIGHT);

        runtime->weaponnum = item;
        runtime->gunfunc = MODBOT_PD_FUNC_PRIMARY;
        runtime->changeguntimer60 = 60;
        runtime->burstsdone[GUNRIGHT] = 0;
        runtime->burstsdone[GUNLEFT] = 0;
        runtime->nextbullettimer60[GUNRIGHT] = 0;
        runtime->nextbullettimer60[GUNLEFT] = 0;
        runtime->timeuntilreload60[GUNRIGHT] = 0;
        runtime->timeuntilreload60[GUNLEFT] = 0;

        modMpBotsPdGetWeaponConfig(item, &cfg);
        runtime->ismeleeweapon = item == ITEM_UNARMED || cfg.melee;
        modMpBotsPlayLocalSfx(runtime->chr, modMpBotsPickupSfx(item));
    }

    if (item == ITEM_UNARMED)
        return TRUE;

    if (runtime->chr->weapons_held[GUNRIGHT] == NULL)
    {
        if (!modMpBotsPdMaterializeHand(slot, item, GUNRIGHT))
        {
            runtime->weaponnum = ITEM_UNARMED;
            runtime->ismeleeweapon = TRUE;
            return FALSE;
        }
        modMpBotsPdReload(slot, GUNRIGHT);
    }

    wantdual = modMpBotsPdInventoryCopies(runtime, item) >= 2
        && bondwalkItemCheckBitflags(item, WEAPONSTATBITFLAG_CAN_DUAL_WIELD);

    if (wantdual && runtime->chr->weapons_held[GUNLEFT] == NULL)
    {
        if (modMpBotsPdMaterializeHand(slot, item, GUNLEFT))
            modMpBotsPdReload(slot, GUNLEFT);
    }
    else if (!wantdual && runtime->chr->weapons_held[GUNLEFT] != NULL)
    {
        modMpBotsPdReleaseHeldHand(runtime->chr, GUNLEFT);
        runtime->loadedammo[GUNLEFT] = 0;
    }

    if (runtime->chr->weapons_held[GUNRIGHT] != NULL
        && runtime->chr->weapons_held[GUNLEFT] != NULL
        && runtime->chr->weapons_held[GUNRIGHT]->weapon != NULL
        && runtime->chr->weapons_held[GUNLEFT]->weapon != NULL)
    {
        propweaponSetDual(runtime->chr->weapons_held[GUNLEFT]->weapon,
            runtime->chr->weapons_held[GUNRIGHT]->weapon);
    }

    return TRUE;
}

static void modMpBotsPdBotInvTick(s32 slot)
{
    struct ModMpBotRuntime *runtime = &g_ModMpBotRuntime[slot];
    ITEM_IDS bestweapon = ITEM_UNARMED;
    s32 bestscore = 0;
    s32 score;
    s32 i;

    runtime->random1ttl60 -= g_ClockTimer;
    if (runtime->random1ttl60 < 0)
    {
        /* PD: 2-12 seconds at NTSC 60 Hz. */
        runtime->random1ttl60 = 120 + (s32)(randomGetNext() % 600U);
        runtime->random1 = randomGetNext();
    }

    /* PeaceSim keeps its current weapon in PD. GE has no secondary function. */
    if (modMpBotHasTrait(slot, MODBOT_TRAIT_PACIFIST))
        return;

    for (i = -1; i < runtime->inventorycount; i++)
    {
        ITEM_IDS item = i < 0 ? ITEM_UNARMED : runtime->inventory[i];
        score = modMpBotsPdWeaponScore(slot, item, TRUE);

        if (score >= bestscore && modMpBotsPdAmmoQuantity(runtime, item, TRUE) > 0)
        {
            bestscore = score;
            bestweapon = item;
        }
    }

    /* PD botinv_switch_to_weapon owns the selected inventory role. */
    modMpBotsPdSwitchToWeapon(slot, bestweapon);
}

static s32 modMpBotsPdTargetInFov(s32 slot, f32 degrees)
{
    struct ModMpBotRuntime *runtime = &g_ModMpBotRuntime[slot];
    f32 angle;
    f32 diff;
    f32 dx;
    f32 dz;

    if (!modMpBotsPdPropAlive(runtime->targetprop))
        return FALSE;

    dx = runtime->targetprop->pos.x - runtime->chr->prop->pos.x;
    dz = runtime->targetprop->pos.z - runtime->chr->prop->pos.z;
    angle = atan2f(dx, dz);
    diff = modMpBotsPdAngleDelta(runtime->lookangle, angle);
    if (diff < 0.0f)
        diff = -diff;

    return diff <= MODBOT_PD_DTOR(degrees);
}

static void modMpBotsPdApplyAimEnd(s32 slot)
{
    struct ModMpBotRuntime *runtime = &g_ModMpBotRuntime[slot];
    PropRecord *target = runtime->targetprop;
    f32 dx;
    f32 dy;
    f32 dz;
    f32 pitch;
    f32 horiz;
    s32 playernum;

    if (runtime->ismeleeweapon || !modMpBotsPdPropAlive(target))
    {
        runtime->chr->aimendcount = 10;
        runtime->chr->aimendrshoulder = 0.0f;
        runtime->chr->aimendlshoulder = 0.0f;
        runtime->chr->aimendback = 0.0f;
        runtime->chr->aimendsideback = 0.0f;
        return;
    }

    dx = target->pos.x - runtime->chr->prop->pos.x;
    dy = target->pos.y - runtime->chr->prop->pos.y;
    dz = target->pos.z - runtime->chr->prop->pos.z;

    /* PD aibot branch of chr_calculate_aimend(..., animcfg, ..., 0):
     * for player targets, aim 40% of eye height below prop origin. */
    if (target->type == PROP_TYPE_VIEWER)
    {
        playernum = getPlayerPointerIndex(target);
        if ((u32)playernum < (u32)getPlayerCount() && g_playerPointers[playernum] != NULL)
            dy -= g_playerPointers[playernum]->eyeheight * 0.4f;
    }

    horiz = sqrtf(dx * dx + dz * dz);
    pitch = atan2f(dy, horiz);
    if (pitch >= M_PI_F)
        pitch -= M_TAU_F;

    /* P2 uses PD's valid animcfg==NULL branch.  The later animation tranche
     * will supply player_choose_third_person_animation's attackanimconfig. */
    runtime->chr->aimendrshoulder = pitch;
    runtime->chr->aimendlshoulder = 0.0f;
    runtime->chr->aimendback = 0.0f;
    runtime->chr->aimendsideback = 0.0f;
    runtime->chr->aimendcount = 10;
}

static f32 modMpBotsPdMeleeReach(struct ModMpBotRuntime *runtime)
{
    return runtime->weaponnum == ITEM_SNIPERRIFLE ? 100.0f : 50.0f;
}

static f32 modMpBotsPdMeleeDistanceSq(struct ModMpBotRuntime *runtime,
    PropRecord *target)
{
    f32 dx = target->pos.x - runtime->chr->prop->pos.x;
    f32 dz = target->pos.z - runtime->chr->prop->pos.z;

    /* Human Slappers/Sniper Butt reach is horizontal/camera-space.  Do not
     * include prop-origin Y: doing so made a standing player effectively out
     * of range while crouching moved the same player into range. */
    return dx * dx + dz * dz;
}

static s32 modMpBotsPdMeleeImpact(s32 slot)
{
    struct ModMpBotRuntime *runtime = &g_ModMpBotRuntime[slot];
    PropRecord *target = runtime->targetprop;
    coord3d direction;
    StandTile *tile;
    f32 reach;

    if (!modMpBotsPdPropAlive(target))
        return FALSE;

    reach = modMpBotsPdMeleeReach(runtime);
    if (modMpBotsPdMeleeDistanceSq(runtime, target) > reach * reach)
        return FALSE;

    tile = runtime->chr->prop->stan;
    if (tile == NULL || target->stan == NULL
        || !stanTestLineUnobstructed(&tile,
            runtime->chr->prop->pos.x, runtime->chr->prop->pos.z,
            target->pos.x, target->pos.z,
            CDTYPE_OBJS | CDTYPE_DOORS | CDTYPE_PATHBLOCKER,
            runtime->chr->chrheight - 20.0f, runtime->chr->chrheight - 20.0f,
            0.0f, 1.0f))
        return FALSE;

    direction.x = target->pos.x - runtime->chr->prop->pos.x;
    direction.y = target->pos.y - runtime->chr->prop->pos.y;
    direction.z = target->pos.z - runtime->chr->prop->pos.z;

    if (target->type == PROP_TYPE_VIEWER)
        modMpBotsDamageViewer(slot, target, ITEM_FIST, &direction);
    else if (target->type == PROP_TYPE_CHR && target->chr != NULL)
    {
        modMpBotsNotifyBotHit(target->chr, runtime->chr);
        handles_shot_actors(target->chr, HIT_CHEST, &direction, ITEM_FIST, FALSE);
    }
    else
        return FALSE;

    return TRUE;
}

static void modMpBotsPdTickMelee(s32 slot)
{
    struct ModMpBotRuntime *runtime = &g_ModMpBotRuntime[slot];
    s32 difficulty = g_ModMpBotConfigs[slot].difficulty;
    f32 reach = modMpBotsPdMeleeReach(runtime);

    if (runtime->punchtimer60[GUNRIGHT] >= 0 && runtime->timeuntilreload60[GUNRIGHT] <= 0)
    {
        runtime->punchtimer60[GUNRIGHT] -= g_ClockTimer;

        if (runtime->targetprop == NULL
            || !runtime->targetinsight
            || runtime->shootdelaytimer60 < g_ModMpBotPdDifficulties[difficulty].shootdelay60
            || !modMpBotsPdTargetInFov(slot, 40.0f)
            || modMpBotsPdMeleeDistanceSq(runtime, runtime->targetprop) > reach * reach)
        {
            runtime->punchtimer60[GUNRIGHT] = 0;
        }

        if (runtime->punchtimer60[GUNRIGHT] < 0)
        {
            if (modMpBotsPdMeleeImpact(slot))
                modMpBotsPlayLocalSfx(runtime->chr, PUNCH1_SFX + (randomGetNext() % 3U));
            else
                modMpBotsPlayLocalSfx(runtime->chr, PUNCHING_AIR_SFX);

            if (difficulty == MODBOT_DIFFICULTY_MEAT)
                runtime->punchtimer60[GUNRIGHT] = 120;
            else if (difficulty == MODBOT_DIFFICULTY_EASY)
                runtime->punchtimer60[GUNRIGHT] = 60;
            else
                runtime->punchtimer60[GUNRIGHT] = 30;

            if ((randomGetNext() % 3U) == 0)
                runtime->punchtimer60[GUNLEFT] = runtime->punchtimer60[GUNRIGHT] - 20;
        }
    }
}

static void modMpBotsPdTickReload(s32 slot)
{
    struct ModMpBotRuntime *runtime = &g_ModMpBotRuntime[slot];
    s32 capacity = modMpBotsPdClipCapacity(runtime->weaponnum);
    s32 ammotype = modMpBotsPdAmmoType(runtime->weaponnum);
    s32 hand;

    for (hand = GUNRIGHT; hand <= GUNLEFT; hand++)
    {
        if (runtime->timeuntilreload60[hand] > 0)
        {
            runtime->timeuntilreload60[hand] -= g_ClockTimer;
            if (runtime->timeuntilreload60[hand] <= 0)
                modMpBotsPdReload(slot, hand);
            continue;
        }

        if (capacity <= 0 || ammotype == AMMO_NONE
            || runtime->chr->weapons_held[hand] == NULL)
            continue;

        if (runtime->loadedammo[hand] <= 0 && runtime->ammoheld[ammotype] > 0)
        {
            modMpBotsPdScheduleReload(slot, hand);
        }
        else if (runtime->loadedammo[hand] < capacity / 2
            && runtime->ammoheld[ammotype] > 0
            && runtime->lastseenanytarget60 >= 0
            && runtime->lastseenanytarget60 < g_GlobalTimer - 120)
        {
            modMpBotsPdScheduleReload(slot, hand);
        }
    }
}

#ifdef REFRESH_PAL
#define MODBOT_PD_PROJECTILE_REFRESH 50
#define MODBOT_PD_THROW_TIMER_MULTI 150
#define MODBOT_PD_THROW_TIMER_DEFAULT 200
#else
#define MODBOT_PD_PROJECTILE_REFRESH 60
#define MODBOT_PD_THROW_TIMER_MULTI 180
#define MODBOT_PD_THROW_TIMER_DEFAULT 240
#endif

static void modMpBotsPdCalculateTrajectory(coord3d *frompos, f32 speed, coord3d *aimpos, coord3d *dir)
{
    f32 xvel;
    f32 yvel;
    f32 zvel;
    f32 latvel;
    f32 vel;
    f32 cosine;
    f32 baseangle;
    f32 term;
    f32 angle;

    /* Perfect Dark chr_calculate_trajectory arithmetic and constants. */
    speed *= 0.59999999f;
    xvel = (aimpos->x - frompos->x) * 0.01f;
    yvel = (aimpos->y - frompos->y) * 0.01f;
    zvel = (aimpos->z - frompos->z) * 0.01f;

    vel = sqrtf(xvel * xvel + yvel * yvel + zvel * zvel);
    latvel = sqrtf(xvel * xvel + zvel * zvel);

    if (vel <= 0.0001f || latvel <= 0.0001f)
    {
        dir->x = 0.0f;
        dir->y = 1.0f;
        dir->z = 0.0f;
        return;
    }

    cosine = latvel / vel;
    baseangle = acosf(cosine);
    if (yvel < 0.0f)
        baseangle = -baseangle;

    term = (vel * 9.81f * cosine * cosine) / (speed * speed) + yvel / vel;
    if (term < -1.0f)
        term = -1.0f;
    else if (term > 1.0f)
        term = 1.0f;

    angle = (asinf(term) - baseangle) * 0.5f + baseangle;
    cosine = cosf(angle);
    dir->x = xvel / latvel * cosine;
    dir->y = sinf(angle);
    dir->z = zvel / latvel * cosine;
}

static s32 modMpBotsPdThrowInterval(ITEM_IDS item)
{
    if (item == ITEM_THROWKNIFE)
        return 120;
    if (item == ITEM_GRENADE)
        return 90;
    return 60;
}

static s32 modMpBotsPdIsLauncherProjectile(ITEM_IDS item)
{
    return item == ITEM_ROCKETLAUNCH || item == ITEM_GRENADELAUNCH;
}

static s32 modMpBotsPdSpawnProjectile(s32 slot)
{
    struct ModMpBotRuntime *runtime = &g_ModMpBotRuntime[slot];
    ITEM_IDS item = runtime->weaponnum;
    WeaponObjRecord *weaponobj;
    PropRecord *target;
    coord3d spawnpos;
    coord3d aimpos;
    coord3d dir;
    coord3d velocity;
    coord3d unscaled;
    Mtxf matrix;
    Mtxf tmp;
    Mtxf identity;
    PROP modelnum;
    ITEM_IDS projectileitem;
    f32 yaw;
    f32 pitch;
    f32 dist;
    s32 thrown;

    if (runtime->chr == NULL || runtime->chr->prop == NULL || runtime->chr->prop->stan == NULL)
        return FALSE;

    target = runtime->targetprop;
    weaponobj = NULL;
    modelnum = (PROP)-1;
    projectileitem = item;
    thrown = FALSE;
    spawnpos = runtime->chr->prop->pos;
    spawnpos.y += 30.0f; /* GE's own off-screen chr gun-position fallback. */
    yaw = runtime->lookangle;
    pitch = runtime->chr->aimendrshoulder;

    if (item == ITEM_THROWKNIFE || item == ITEM_GRENADE
        || item == ITEM_TIMEDMINE || item == ITEM_PROXIMITYMINE
        || item == ITEM_REMOTEMINE)
    {
        thrown = TRUE;

        switch (item)
        {
            case ITEM_THROWKNIFE:    modelnum = PROP_CHRKNIFE;         break;
            case ITEM_GRENADE:       modelnum = PROP_CHRGRENADE;       break;
            case ITEM_TIMEDMINE:     modelnum = PROP_CHRTIMEDMINE;     break;
            case ITEM_PROXIMITYMINE: modelnum = PROP_CHRPROXIMITYMINE; break;
            case ITEM_REMOTEMINE:    modelnum = PROP_CHRREMOTEMINE;    break;
            default: break;
        }

        if (target != NULL && modMpBotsPdPropAlive(target)
            && modMpBotsPdTargetInFov(slot, 30.0f))
        {
            aimpos = target->pos;
            if (item == ITEM_GRENADE && target->chr != NULL)
                aimpos.y = target->chr->ground;
            else if (target->type == PROP_TYPE_VIEWER)
                aimpos.y -= 25.0f;

            modMpBotsPdCalculateTrajectory(&spawnpos, 16.666666f, &aimpos, &dir);
        }
        else
        {
            dir.x = cosf(MODBOT_PD_DTOR(20.0f)) * sinf(yaw);
            dir.y = sinf(MODBOT_PD_DTOR(20.0f));
            dir.z = cosf(MODBOT_PD_DTOR(20.0f)) * cosf(yaw);
        }

        velocity.x = dir.x * 16.666666f;
        velocity.y = dir.y * 16.666666f;
        velocity.z = dir.z * 16.666666f;

        matrix_4x4_set_identity(&matrix);
        if (item == ITEM_THROWKNIFE)
        {
            matrix_4x4_set_rotation_around_z(M_PI_F * 1.5f, &matrix);
            matrix_4x4_set_rotation_around_x(M_PI_F, &tmp);
            matrix_4x4_multiply_homogeneous_in_place(&tmp, &matrix);
        }
        matrix_4x4_set_rotation_around_x(MODBOT_PD_DTOR(20.0f), &tmp);
        matrix_4x4_multiply_homogeneous_in_place(&tmp, &matrix);
        matrix_4x4_set_rotation_around_y(yaw, &tmp);
        matrix_4x4_multiply_homogeneous_in_place(&tmp, &matrix);
    }
    else if (item == ITEM_ROCKETLAUNCH || item == ITEM_GRENADELAUNCH)
    {
        if (target == NULL || !modMpBotsPdPropAlive(target))
            return FALSE;

        dist = modMpBotsPdDistance(runtime->chr, target);
        if (dist <= 400.0f)
            return FALSE;

        dir.x = cosf(pitch) * sinf(yaw);
        dir.y = sinf(pitch);
        dir.z = cosf(pitch) * cosf(yaw);

        matrix_4x4_set_rotation_around_x(pitch, &matrix);
        matrix_4x4_set_rotation_around_y(yaw, &tmp);
        matrix_4x4_multiply_homogeneous_in_place(&tmp, &matrix);

        if (item == ITEM_ROCKETLAUNCH)
        {
            modelnum = PROP_CHRROCKET;
            projectileitem = ITEM_ROCKETROUND;
            unscaled.x = dir.x * 1.111111f;
            unscaled.y = dir.y * 1.111111f;
            unscaled.z = dir.z * 1.111111f;
            velocity.x = unscaled.x * g_GlobalTimerDelta;
            velocity.y = unscaled.y * g_GlobalTimerDelta;
            velocity.z = unscaled.z * g_GlobalTimerDelta;
        }
        else
        {
            modelnum = PROP_CHRGRENADEROUND;
            projectileitem = ITEM_GRENADEROUND;
            velocity.x = dir.x * 33.333332f;
            velocity.y = dir.y * 33.333332f;
            velocity.z = dir.z * 33.333332f;
        }
    }
    else
    {
        return FALSE;
    }

    if ((s32)modelnum < 0)
        return FALSE;

    weaponobj = (WeaponObjRecord *)create_new_item_instance_of_model(modelnum, projectileitem);
    if (weaponobj == NULL)
        return FALSE;

    matrix_4x4_set_identity(&identity);

    if (item == ITEM_GRENADELAUNCH)
        weaponobj->timer = CHRLV_DEFAULT_TIMER;
    else if (item == ITEM_ROCKETLAUNCH)
        weaponobj->timer = -1;
    else if (item == ITEM_GRENADE)
        weaponobj->timer = MODBOT_PD_THROW_TIMER_DEFAULT;
    else if (item == ITEM_TIMEDMINE || item == ITEM_PROXIMITYMINE || item == ITEM_REMOTEMINE)
        weaponobj->timer = MODBOT_PD_THROW_TIMER_MULTI;

    gunInitProjectileObject((ObjectRecord *)weaponobj, &spawnpos, runtime->chr->prop->stan,
        &matrix, &velocity, &identity, runtime->chr->prop);

    if ((weaponobj->runtime_bitflags & RUNTIMEBITFLAG_HASPROJECTILE) == 0)
        return FALSE;

    if (item == ITEM_ROCKETLAUNCH)
    {
        weaponobj->projectile->flags |= 0x80;
        weaponobj->projectile->flags |= 0x20;
        weaponobj->projectile->unkB0 = weaponobj->runtime_pos.y;
        weaponobj->projectile->unkB4 = weaponobj->projectile->speed.y;
        weaponobj->projectile->unk10.x = unscaled.x;
        weaponobj->projectile->unk10.y = unscaled.y;
        weaponobj->projectile->unk10.z = unscaled.z;
    }
    else if (item == ITEM_GRENADELAUNCH)
    {
        weaponobj->projectile->unk8C = 0.3f;
        weaponobj->projectile->unk94 = 0.13333333f;
        weaponobj->projectile->refreshrate = MODBOT_PD_PROJECTILE_REFRESH;
    }
    else if (thrown)
    {
        weaponobj->projectile->flags |= 2;
        weaponobj->projectile->refreshrate = MODBOT_PD_PROJECTILE_REFRESH;
        if (item == ITEM_GRENADE)
        {
            weaponobj->projectile->unk8C = 0.3f;
            weaponobj->projectile->unk94 = 0.13333333f;
        }
        else
        {
            weaponobj->projectile->unk8C = 0.1f;
        }

        if (item == ITEM_THROWKNIFE)
            weaponobj->runtime_bitflags |= RUNTIMEBITFLAG_THROWING_KNIFE_RELATED;

        /* PD remote-mine detonation decision/function state remains P4. */
    }

    return TRUE;
}

static void modMpBotsPdTickGunfire(s32 slot)
{
    struct ModMpBotRuntime *runtime = &g_ModMpBotRuntime[slot];
    struct ModMpBotPdWeaponConfig cfg;
    s32 difficulty = g_ModMpBotConfigs[slot].difficulty;
    s32 ammotype;
    s32 hand;
    s32 canfire;
    s32 projectileweapon;
    s32 firing[2];

    firing[GUNRIGHT] = FALSE;
    firing[GUNLEFT] = FALSE;
    modMpBotsPdGetWeaponConfig(runtime->weaponnum, &cfg);

    for (hand = GUNRIGHT; hand <= GUNLEFT; hand++)
    {
        if (runtime->nextbullettimer60[hand] > 0)
            runtime->nextbullettimer60[hand] -= g_ClockTimer;
    }

    if (runtime->changeguntimer60 > 0)
        runtime->changeguntimer60 -= g_ClockTimer;

    modMpBotsPdTickReload(slot);

    canfire = runtime->changeguntimer60 <= 0
        && runtime->targetprop != NULL
        && runtime->targetinsight
        && runtime->shootdelaytimer60 >= g_ModMpBotPdDifficulties[difficulty].shootdelay60
        && modMpBotsPdPropAlive(runtime->targetprop);

    projectileweapon = cfg.throwable || modMpBotsPdIsLauncherProjectile(runtime->weaponnum);

    if (projectileweapon)
    {
        chrSetFiring(runtime->chr, GUNRIGHT, FALSE);
        chrSetFiring(runtime->chr, GUNLEFT, FALSE);

        if (canfire
            && runtime->timeuntilreload60[GUNRIGHT] <= 0
            && runtime->nextbullettimer60[GUNRIGHT] <= 0
            && runtime->chr->weapons_held[GUNRIGHT] != NULL
            && modMpBotsPdTargetInFov(slot, cfg.throwable ? 30.0f : 45.0f))
        {
            ammotype = modMpBotsPdAmmoType(runtime->weaponnum);
            if (ammotype == AMMO_NONE || runtime->loadedammo[GUNRIGHT] > 0)
            {
                if (modMpBotsPdSpawnProjectile(slot))
                {
                    if (ammotype != AMMO_NONE)
                        runtime->loadedammo[GUNRIGHT]--;
                    runtime->nextbullettimer60[GUNRIGHT] = (s16)(cfg.throwable
                        ? modMpBotsPdThrowInterval(runtime->weaponnum)
                        : modMpBotsPdShotInterval(runtime->weaponnum));
                }
            }
        }
        return;
    }

    if (canfire && modMpBotsPdTargetInFov(slot, 45.0f))
    {
        ammotype = modMpBotsPdAmmoType(runtime->weaponnum);

        for (hand = GUNRIGHT; hand <= GUNLEFT; hand++)
        {
            if (runtime->chr->weapons_held[hand] == NULL
                || runtime->timeuntilreload60[hand] > 0
                || runtime->nextbullettimer60[hand] > 0)
                continue;

            if (ammotype == AMMO_NONE || runtime->loadedammo[hand] > 0)
            {
                firing[hand] = TRUE;
                runtime->nextbullettimer60[hand] = (s16)modMpBotsPdShotInterval(runtime->weaponnum);
                if (ammotype != AMMO_NONE)
                    runtime->loadedammo[hand]--;
            }
        }
    }

    for (hand = GUNRIGHT; hand <= GUNLEFT; hand++)
    {
        if (firing[hand])
        {
            runtime->chr->seen_bond_time = g_GlobalTimer;
            if (hand == GUNRIGHT)
                runtime->chr->hidden |= CHRHIDDEN_FIRE_WEAPON_RIGHT;
            else
                runtime->chr->hidden |= CHRHIDDEN_FIRE_WEAPON_LEFT;
            chrSetFiring(runtime->chr, hand, TRUE);
        }
        else
        {
            chrSetFiring(runtime->chr, hand, FALSE);
        }
    }
}

static void modMpBotsPdCombatTick(s32 slot)
{
    struct ModMpBotRuntime *runtime = &g_ModMpBotRuntime[slot];

    modMpBotsPdApplyAimEnd(slot);

    if (runtime->ismeleeweapon)
        modMpBotsPdTickMelee(slot);
    else
        modMpBotsPdTickGunfire(slot);
}

static ITEM_IDS modMpBotsPdWeaponForAmmo(s32 ammotype)
{
    switch (ammotype)
    {
        case AMMO_GRENADE:    return ITEM_GRENADE;
        case AMMO_REMOTEMINE: return ITEM_REMOTEMINE;
        case AMMO_PROXMINE:   return ITEM_PROXIMITYMINE;
        case AMMO_TIMEDMINE:  return ITEM_TIMEDMINE;
        case AMMO_KNIFE:      return ITEM_THROWKNIFE;
        default:              return ITEM_UNARMED;
    }
}

static s32 modMpBotsGeMagazineQty(AmmoCrateRecord *crate)
{
    s32 qty = 1;

    switch (crate->ammoType)
    {
        case AMMO_9MM:
        case AMMO_9MM_2:
        case AMMO_RIFLE:   qty = 10; break;
        case AMMO_SHOTGUN:
        case AMMO_MAGNUM:  qty = 5; break;
        case AMMO_GGUN:    qty = 3; break;
        case AMMO_DARTS:   qty = 4; break;
        default: break;
    }

    /* One-human Multiplayer is still Multiplayer: no GE solo ammo multiplier. */
    return qty;
}

static s32 modMpBotsGeWeaponPickupQty(WeaponObjRecord *weapon)
{
    s32 ammotype;
    s32 qty = 1;

    if (weapon->flags & PROPFLAG_NO_AMMO)
        return 0;

    ammotype = get_ammo_type_for_weapon(weapon->weaponnum);
    switch (ammotype)
    {
        case AMMO_9MM:
        case AMMO_9MM_2:
        case AMMO_RIFLE:        qty = 10; break;
        case AMMO_SHOTGUN:
        case AMMO_MAGNUM:       qty = 5; break;
        case AMMO_GGUN:         qty = 3; break;
        case AMMO_DARTS:        qty = 4; break;
        case AMMO_GRENADEROUND: qty = 3; break;
        default: break;
    }
    return qty;
}

static s32 modMpBotsPdRoomsNear(PropRecord *botprop, PropRecord *prop)
{
    s32 i;
    s32 j;

    for (i = 0; i < PROPRECORD_STAN_ROOM_LEN && botprop->rooms[i] != 0xff; i++)
    {
        for (j = 0; j < PROPRECORD_STAN_ROOM_LEN && prop->rooms[j] != 0xff; j++)
        {
            if (botprop->rooms[i] == prop->rooms[j]
                || bgRoomsSharePortal(botprop->rooms[i], prop->rooms[j]))
                return TRUE;
        }
    }
    return FALSE;
}

static s32 modMpBotsPdCanBenefitFromPickup(s32 slot, PropRecord *prop)
{
    struct ModMpBotRuntime *runtime = &g_ModMpBotRuntime[slot];
    ObjectRecord *obj = prop->obj;
    WeaponObjRecord *weapon;
    AmmoCrateRecord *crate;
    MultiAmmoCrateRecord *multicrate;
    BodyArmourRecord *armour;
    ITEM_IDS item;
    s32 ammotype;
    s32 copies;
    s32 i;

    if (obj == NULL)
        return FALSE;

    switch (obj->type)
    {
        case PROPDEF_COLLECTABLE:
            weapon = (WeaponObjRecord *)obj;
            item = (ITEM_IDS)weapon->weaponnum;

            if ((item == ITEM_GRENADE || item == ITEM_GRENADEROUND
                    || item == ITEM_REMOTEMINE || item == ITEM_PROXIMITYMINE
                    || item == ITEM_TIMEDMINE)
                && (weapon->timer >= 0 || (obj->runtime_bitflags & RUNTIMEBITFLAG_HASPROJECTILE)))
                return FALSE;

            if (item == ITEM_ROCKETROUND && (obj->runtime_bitflags & RUNTIMEBITFLAG_HASPROJECTILE))
                return FALSE;

            copies = modMpBotsPdInventoryCopies(runtime, item);
            if (copies == 0)
                return TRUE;
            if (copies < 2 && bondwalkItemCheckBitflags(item, WEAPONSTATBITFLAG_CAN_DUAL_WIELD))
                return TRUE;

            ammotype = modMpBotsPdAmmoType(item);
            return ammotype != AMMO_NONE && (u32)ammotype < AMMOTYPE_MAX
                && runtime->ammoheld[ammotype] < get_max_ammo_for_type(ammotype);

        case PROPDEF_MAGAZINE:
            crate = (AmmoCrateRecord *)obj;
            ammotype = crate->ammoType == AMMO_9MM_2 ? AMMO_9MM : crate->ammoType;
            return ammotype > AMMO_NONE && (u32)ammotype < AMMOTYPE_MAX
                && runtime->ammoheld[ammotype] < get_max_ammo_for_type(ammotype);

        case PROPDEF_AMMO:
            multicrate = (MultiAmmoCrateRecord *)obj;
            for (i = 0; i < AMMOTYPE_GLOBAL_MAX; i++)
            {
                ammotype = i + 1;
                if (ammotype == AMMO_9MM_2)
                    ammotype = AMMO_9MM;
                if (multicrate->slots[i].quantity > 0
                    && ammotype > AMMO_NONE && (u32)ammotype < AMMOTYPE_MAX
                    && runtime->ammoheld[ammotype] < get_max_ammo_for_type(ammotype))
                    return TRUE;
            }
            return FALSE;

        case PROPDEF_ARMOUR:
            armour = (BodyArmourRecord *)obj;
            return armour->amount > chrGetArmor(runtime->chr);

        default:
            return FALSE;
    }
}

static s32 modMpBotsPdTestPickup(s32 slot, PropRecord *prop)
{
    struct ModMpBotRuntime *runtime = &g_ModMpBotRuntime[slot];
    ObjectRecord *obj;
    StandTile *tile;
    f32 dx;
    f32 dy;
    f32 dz;

    if (runtime->chr == NULL || runtime->chr->prop == NULL
        || prop == NULL || prop == runtime->chr->prop)
        return FALSE;
    if (prop->type != PROP_TYPE_OBJ && prop->type != PROP_TYPE_WEAPON)
        return FALSE;
    if (prop->parent != NULL || prop->timetoregen != 0)
        return FALSE;

    obj = prop->obj;
    if (obj == NULL)
        return FALSE;

    if (objIsCollectable((PropDefHeaderRecord *)obj))
    {
        if (obj->flags & PROPFLAG_UNCOLLECTABLE)
            return FALSE;
    }
    else if (!(obj->flags & PROPFLAG_00040000))
    {
        return FALSE;
    }

    if ((obj->flags & PROPFLAG_00080000) || !objCanPickupFromSafe(obj))
        return FALSE;

    if ((obj->runtime_bitflags & RUNTIMEBITFLAG_HASPROJECTILE)
        && obj->projectile != NULL && obj->projectile->refreshrate > 0
        && obj->projectile->unk90 == 0)
        return FALSE;

    if (!modMpBotsPdRoomsNear(runtime->chr->prop, prop)
        || !modMpBotsPdCanBenefitFromPickup(slot, prop))
        return FALSE;

    dx = prop->pos.x - runtime->chr->prop->pos.x;
    dy = prop->pos.y - runtime->chr->prop->pos.y;
    dz = prop->pos.z - runtime->chr->prop->pos.z;

    /* PD normal Simulant pickup radius 100 and vertical range +/-200. */
    if (dx * dx + dz * dz > 10000.0f || dy < -200.0f || dy > 200.0f)
        return FALSE;

    if (!(obj->flags2 & 0x1000))
    {
        tile = runtime->chr->prop->stan;
        if (tile == NULL)
            return FALSE;
        if (!stanTestLineUnobstructed(&tile,
                runtime->chr->prop->pos.x, runtime->chr->prop->pos.z,
                prop->pos.x, prop->pos.z, CDTYPE_DOORS | CDTYPE_BG,
                30.0f, 30.0f, 0.0f, 1.0f))
            return FALSE;
    }

    return TRUE;
}

static void modMpBotsPdGivePickupAmmoItem(struct ModMpBotRuntime *runtime, s32 ammotype, s32 qty, s16 pad)
{
    ITEM_IDS item;

    if (ammotype == AMMO_9MM_2)
        ammotype = AMMO_9MM;
    modMpBotsPdGiveAmmo(runtime, ammotype, qty);
    item = modMpBotsPdWeaponForAmmo(ammotype);
    if (item != ITEM_UNARMED)
        modMpBotsPdInventoryGive(runtime, item, pad);
}

static void modMpBotsPdConsumePickupProp(PropRecord *prop)
{
    ObjectRecord *obj;

    if (prop == NULL)
        return;

    obj = prop->obj;

    /* Match the retail player pickup lifetime: clean/detach the ObjectRecord
     * before the PropRecord tick operation releases or regenerates it. */
    if (obj != NULL)
        objFree(obj, 0, obj->state & RUNTIMEBITFLAG_REMOVE);

    propExecuteTickOperation(prop, TICKOP_FREE);
}

static s32 modMpBotsPdPickupProp(s32 slot, PropRecord *prop)
{
    struct ModMpBotRuntime *runtime = &g_ModMpBotRuntime[slot];
    ObjectRecord *obj = prop->obj;
    WeaponObjRecord *weapon;
    AmmoCrateRecord *crate;
    MultiAmmoCrateRecord *multicrate;
    BodyArmourRecord *armour;
    ITEM_IDS item;
    s32 ammotype;
    s32 qty;
    s32 i;

    if (obj == NULL)
        return FALSE;

    switch (obj->type)
    {
        case PROPDEF_COLLECTABLE:
            weapon = (WeaponObjRecord *)obj;
            item = (ITEM_IDS)weapon->weaponnum;
            ammotype = modMpBotsPdAmmoType(item);
            qty = modMpBotsGeWeaponPickupQty(weapon);
            if (ammotype != AMMO_NONE)
                modMpBotsPdGivePickupAmmoItem(runtime, ammotype, qty, (s16)obj->pad);
            if ((s32)getPropForHeldItem(item) >= 0)
                modMpBotsPdInventoryGive(runtime, item, (s16)obj->pad);
            modMpBotsPlayLocalSfx(runtime->chr, modMpBotsPickupSfx(item));
            modMpBotsPdConsumePickupProp(prop);
            return TRUE;

        case PROPDEF_MAGAZINE:
            crate = (AmmoCrateRecord *)obj;
            modMpBotsPdGivePickupAmmoItem(runtime, crate->ammoType,
                modMpBotsGeMagazineQty(crate), (s16)obj->pad);
            modMpBotsPlayLocalSfx(runtime->chr, modMpBotsAmmoPickupSfx(crate->ammoType));
            modMpBotsPdConsumePickupProp(prop);
            return TRUE;

        case PROPDEF_AMMO:
            multicrate = (MultiAmmoCrateRecord *)obj;
            for (i = 0; i < AMMOTYPE_GLOBAL_MAX; i++)
            {
                if (multicrate->slots[i].quantity > 0)
                    modMpBotsPdGivePickupAmmoItem(runtime, i + 1,
                        multicrate->slots[i].quantity, (s16)obj->pad);
            }
            modMpBotsPlayLocalSfx(runtime->chr, PICKUP_AMMO_SFX);
            modMpBotsPdConsumePickupProp(prop);
            return TRUE;

        case PROPDEF_ARMOUR:
            armour = (BodyArmourRecord *)obj;
            /* GE actor armour is represented as negative ChrRecord damage. */
            if (armour->amount > chrGetArmor(runtime->chr))
                runtime->chr->damage = -armour->amount;
            modMpBotsPlayLocalSfx(runtime->chr, ARMOUR_COLLECT_SFX);
            modMpBotsPdConsumePickupProp(prop);
            return TRUE;

        default:
            return FALSE;
    }
}

static void modMpBotsPdCheckPickups(s32 slot)
{
    PropRecord *prop;
    PropRecord *prev;

    prop = chrpropGetActiveTail();
    while (prop != NULL)
    {
        prev = prop->prev;
        if (modMpBotsPdTestPickup(slot, prop))
            modMpBotsPdPickupProp(slot, prop);
        prop = prev;
    }
}

/* ShieldSim's defining inventory behavior is that shield collection takes
 * precedence once its shield falls into the normal fetch range.  GE armour is
 * a world object rather than a PD inventory weapon, so choose the nearest live
 * armour prop here and let the same STAN A* motor reach it. */
static PropRecord *modMpBotsPdFindArmorTarget(s32 slot)
{
    struct ModMpBotRuntime *runtime = &g_ModMpBotRuntime[slot];
    PropRecord *prop;
    PropRecord *best = NULL;
    f32 current = 0.0f;
    f32 dx;
    f32 dz;
    f32 dist;
    f32 bestdist = 0.0f;
    s32 needweapon = TRUE;
    s32 wantarmor;
    s32 i;

    if (runtime->chr == NULL || runtime->chr->prop == NULL)
        return NULL;

    for (i = 0; i < runtime->inventorycount; i++)
    {
        if (runtime->inventory[i] != ITEM_UNARMED)
        {
            needweapon = FALSE;
            break;
        }
    }

    wantarmor = modMpBotHasTrait(slot, MODBOT_TRAIT_ARMOR_SPECIALIST);
    if (wantarmor)
    {
        current = chrGetArmor(runtime->chr);
        if (current >= 6.0f)
            wantarmor = FALSE;
    }

    if (!needweapon && !wantarmor)
        return NULL;

    for (prop = chrpropGetActiveTail(); prop != NULL; prop = prop->prev)
    {
        ObjectRecord *obj;

        if ((prop->type != PROP_TYPE_OBJ && prop->type != PROP_TYPE_WEAPON)
            || prop->obj == NULL || prop->stan == NULL || prop->parent != NULL
            || prop->timetoregen != 0 || !objCanPickupFromSafe(prop->obj))
            continue;

        obj = prop->obj;

        if (needweapon)
        {
            WeaponObjRecord *weapon;
            ITEM_IDS item;

            if (obj->type != PROPDEF_COLLECTABLE)
                continue;

            weapon = (WeaponObjRecord *)obj;
            item = (ITEM_IDS)weapon->weaponnum;
            if (item == ITEM_UNARMED || (s32)getPropForHeldItem(item) < 0)
                continue;
        }
        else
        {
            BodyArmourRecord *armour;

            if (!wantarmor || obj->type != PROPDEF_ARMOUR)
                continue;

            armour = (BodyArmourRecord *)obj;
            if (armour->amount <= current)
                continue;
        }

        dx = prop->pos.x - runtime->chr->prop->pos.x;
        dz = prop->pos.z - runtime->chr->prop->pos.z;
        dist = dx * dx + dz * dz;

        if (best == NULL || dist < bestdist)
        {
            best = prop;
            bestdist = dist;
        }
    }

    return best;
}

static s32 modMpBotsPdMoveTargetValid(PropRecord *prop)
{
    if (modMpBotsPdFindParticipant(prop) >= 0)
        return modMpBotsPdPropAlive(prop);

    return prop != NULL
        && (prop->type == PROP_TYPE_OBJ || prop->type == PROP_TYPE_WEAPON)
        && prop->obj != NULL && prop->obj->type == PROPDEF_ARMOUR
        && prop->stan != NULL && prop->parent == NULL && prop->timetoregen == 0;
}

static void modMpBotsPdNavClear(struct ModMpBotRuntime *runtime)
{
    s32 i;

    runtime->navgoalstan = NULL;
    runtime->navtargetcode = MODBOT_TARGET_NONE;
    runtime->navindex = 0;
    runtime->navcount = 0;
    runtime->navactive = FALSE;

    for (i = 0; i < MAX_CHRWAYPOINTS; i++)
        runtime->navtiles[i] = NULL;
}

static f32 modMpBotsPdNavSqDist(const coord3d *a, const coord3d *b)
{
    f32 dx = b->x - a->x;
    f32 dz = b->z - a->z;
    return dx * dx + dz * dz;
}

static u32 modMpBotsStanHashKey(StandTile *tile)
{
    u32 key = (u32)(((u8 *)tile - (u8 *)standTileStart) >> 3);

    key ^= key >> 7;
    key *= 2654435761U;
    return key;
}

static void modMpBotsStanSearchReset(void)
{
    s32 i;

    g_ModMpBotStanNodeCount = 0;
    g_ModMpBotStanHeapCount = 0;

    for (i = 0; i < MODBOT_STAN_HASH_SIZE; i++)
        g_ModMpBotStanHash[i] = -1;
}

static s16 modMpBotsStanFindNode(StandTile *tile, s32 create)
{
    u32 slot;
    s16 index;
    s32 probes;

    if (tile == NULL)
        return -1;

    slot = modMpBotsStanHashKey(tile) & (MODBOT_STAN_HASH_SIZE - 1);

    for (probes = 0; probes < MODBOT_STAN_HASH_SIZE; probes++)
    {
        index = g_ModMpBotStanHash[slot];

        if (index < 0)
        {
            if (!create || g_ModMpBotStanNodeCount >= MODBOT_STAN_SEARCH_MAX)
                return -1;

            index = g_ModMpBotStanNodeCount++;
            g_ModMpBotStanHash[slot] = index;
            g_ModMpBotStanNodes[index].tile = tile;
            g_ModMpBotStanNodes[index].g = 0.0f;
            g_ModMpBotStanNodes[index].f = 0.0f;
            g_ModMpBotStanNodes[index].parent = -1;
            g_ModMpBotStanNodes[index].heappos = -1;
            g_ModMpBotStanNodes[index].state = 0;
            return index;
        }

        if (g_ModMpBotStanNodes[index].tile == tile)
            return index;

        slot = (slot + 1) & (MODBOT_STAN_HASH_SIZE - 1);
    }

    return -1;
}

static void modMpBotsStanHeapSwap(s32 a, s32 b)
{
    s16 tmp = g_ModMpBotStanHeap[a];

    g_ModMpBotStanHeap[a] = g_ModMpBotStanHeap[b];
    g_ModMpBotStanHeap[b] = tmp;
    g_ModMpBotStanNodes[g_ModMpBotStanHeap[a]].heappos = a;
    g_ModMpBotStanNodes[g_ModMpBotStanHeap[b]].heappos = b;
}

static void modMpBotsStanHeapBubbleUp(s32 pos)
{
    s32 parent;

    while (pos > 0)
    {
        parent = (pos - 1) >> 1;

        if (g_ModMpBotStanNodes[g_ModMpBotStanHeap[parent]].f
            <= g_ModMpBotStanNodes[g_ModMpBotStanHeap[pos]].f)
            break;

        modMpBotsStanHeapSwap(parent, pos);
        pos = parent;
    }
}

static void modMpBotsStanHeapPush(s16 index)
{
    s32 pos;

    if (g_ModMpBotStanHeapCount >= MODBOT_STAN_SEARCH_MAX)
        return;

    pos = g_ModMpBotStanHeapCount++;
    g_ModMpBotStanHeap[pos] = index;
    g_ModMpBotStanNodes[index].heappos = pos;
    modMpBotsStanHeapBubbleUp(pos);
}

static s16 modMpBotsStanHeapPop(void)
{
    s16 result;
    s32 pos;
    s32 left;
    s32 right;
    s32 best;

    if (g_ModMpBotStanHeapCount <= 0)
        return -1;

    result = g_ModMpBotStanHeap[0];
    g_ModMpBotStanNodes[result].heappos = -1;
    g_ModMpBotStanHeapCount--;

    if (g_ModMpBotStanHeapCount <= 0)
        return result;

    g_ModMpBotStanHeap[0] = g_ModMpBotStanHeap[g_ModMpBotStanHeapCount];
    g_ModMpBotStanNodes[g_ModMpBotStanHeap[0]].heappos = 0;
    pos = 0;

    while (1)
    {
        left = pos * 2 + 1;
        right = left + 1;
        best = pos;

        if (left < g_ModMpBotStanHeapCount
            && g_ModMpBotStanNodes[g_ModMpBotStanHeap[left]].f
                < g_ModMpBotStanNodes[g_ModMpBotStanHeap[best]].f)
            best = left;

        if (right < g_ModMpBotStanHeapCount
            && g_ModMpBotStanNodes[g_ModMpBotStanHeap[right]].f
                < g_ModMpBotStanNodes[g_ModMpBotStanHeap[best]].f)
            best = right;

        if (best == pos)
            break;

        modMpBotsStanHeapSwap(pos, best);
        pos = best;
    }

    return result;
}

static f32 modMpBotsStanDistance(StandTile *a, StandTile *b)
{
    coord3d pa;
    coord3d pb;
    f32 dx;
    f32 dy;
    f32 dz;

    getTileMidPoint(a, &pa);
    getTileMidPoint(b, &pb);
    dx = pb.x - pa.x;
    dy = pb.y - pa.y;
    dz = pb.z - pa.z;
    return sqrtf(dx * dx + dy * dy + dz * dz);
}

static f32 modMpBotsStanTurnPenalty(s16 current, StandTile *next)
{
    s16 parent = g_ModMpBotStanNodes[current].parent;
    coord3d a;
    coord3d b;
    coord3d c;
    f32 abx;
    f32 abz;
    f32 bcx;
    f32 bcz;
    f32 ablen;
    f32 bclen;
    f32 cosine;

    if (parent < 0)
        return 0.0f;

    getTileMidPoint(g_ModMpBotStanNodes[parent].tile, &a);
    getTileMidPoint(g_ModMpBotStanNodes[current].tile, &b);
    getTileMidPoint(next, &c);

    abx = b.x - a.x;
    abz = b.z - a.z;
    bcx = c.x - b.x;
    bcz = c.z - b.z;
    ablen = sqrtf(abx * abx + abz * abz);
    bclen = sqrtf(bcx * bcx + bcz * bcz);

    if (ablen < 1.0f || bclen < 1.0f)
        return 0.0f;

    cosine = (abx * bcx + abz * bcz) / (ablen * bclen);

    if (cosine < -1.0f)
        cosine = -1.0f;
    else if (cosine > 1.0f)
        cosine = 1.0f;

    return (1.0f - cosine) * MODBOT_STAN_TURN_COST;
}

static f32 modMpBotsStanEdgeCost(s16 current, StandTile *next, s32 edgeIndex)
{
    f32 cost;
    f32 clearance;
    f32 width = 0.0f;
    coord3d portal;

    cost = modMpBotsStanDistance(g_ModMpBotStanNodes[current].tile, next);
    clearance = stanGetTileCenterClearanceWorld(next);

    if (clearance < MODBOT_STAN_CLEARANCE_GOAL)
        cost += (MODBOT_STAN_CLEARANCE_GOAL - clearance) * MODBOT_STAN_CLEARANCE_COST;

    if (stanGetEdgeMidPointWorld(g_ModMpBotStanNodes[current].tile, edgeIndex, &portal, &width)
        && width < MODBOT_STAN_PORTAL_GOAL)
    {
        cost += (MODBOT_STAN_PORTAL_GOAL - width) * MODBOT_STAN_PORTAL_COST;
    }

    cost += modMpBotsStanTurnPenalty(current, next);
    return cost;
}

static s32 modMpBotsPdNavBuildRoute(s32 slot, PropRecord *targetprop)
{
    struct ModMpBotRuntime *runtime = &g_ModMpBotRuntime[slot];
    StandTile *starttile;
    StandTile *goaltile;
    StandTile *nexttile;
    s16 start;
    s16 goal = -1;
    s16 current;
    s16 next;
    s16 parent;
    s32 edge;
    s32 pointcount;
    s32 reversecount;
    s32 routecount;
    s32 i;
    f32 tentative;
    f32 heuristic;

    if (runtime->chr == NULL || runtime->chr->prop == NULL
        || targetprop == NULL || targetprop->stan == NULL
        || runtime->chr->prop->stan == NULL)
    {
        modMpBotsPdNavClear(runtime);
        return FALSE;
    }

    starttile = runtime->chr->prop->stan;
    goaltile = targetprop->stan;

    for (i = 0; i < MAX_CHRWAYPOINTS; i++)
        runtime->navtiles[i] = NULL;

    runtime->navgoalstan = goaltile;
    runtime->navgoalpos = targetprop->pos;
    runtime->navtargetcode = modMpBotsPdTargetCodeForProp(targetprop);
    runtime->navindex = 0;
    runtime->navcount = 0;
    runtime->navactive = FALSE;

    if (starttile == goaltile)
    {
        runtime->navtiles[0] = starttile;
        runtime->navcount = 1;
        runtime->navindex = 1;
        runtime->navactive = TRUE;
        return TRUE;
    }

    modMpBotsStanSearchReset();
    start = modMpBotsStanFindNode(starttile, TRUE);

    if (start < 0)
        return FALSE;

    g_ModMpBotStanNodes[start].g = 0.0f;
    g_ModMpBotStanNodes[start].f = modMpBotsStanDistance(starttile, goaltile);
    g_ModMpBotStanNodes[start].state = 1;
    modMpBotsStanHeapPush(start);

    while (g_ModMpBotStanHeapCount > 0)
    {
        current = modMpBotsStanHeapPop();

        if (current < 0)
            break;

        if (g_ModMpBotStanNodes[current].tile == goaltile)
        {
            goal = current;
            break;
        }

        g_ModMpBotStanNodes[current].state = 2;
        pointcount = stanGetTilePointCount(g_ModMpBotStanNodes[current].tile);

        for (edge = 0; edge < pointcount; edge++)
        {
            nexttile = stanGetLinkedTileAtEdge(g_ModMpBotStanNodes[current].tile, edge);

            if (nexttile == NULL)
                continue;

            next = modMpBotsStanFindNode(nexttile, TRUE);

            if (next < 0 || g_ModMpBotStanNodes[next].state == 2)
                continue;

            tentative = g_ModMpBotStanNodes[current].g
                + modMpBotsStanEdgeCost(current, nexttile, edge);

            if (g_ModMpBotStanNodes[next].state == 0
                || tentative < g_ModMpBotStanNodes[next].g)
            {
                g_ModMpBotStanNodes[next].parent = current;
                g_ModMpBotStanNodes[next].g = tentative;
                heuristic = modMpBotsStanDistance(nexttile, goaltile);
                g_ModMpBotStanNodes[next].f = tentative + heuristic;

                if (g_ModMpBotStanNodes[next].state == 0)
                {
                    g_ModMpBotStanNodes[next].state = 1;
                    modMpBotsStanHeapPush(next);
                }
                else
                {
                    modMpBotsStanHeapBubbleUp(g_ModMpBotStanNodes[next].heappos);
                }
            }
        }
    }

    if (goal < 0)
        return FALSE;

    reversecount = 0;
    current = goal;

    while (current >= 0 && reversecount < MODBOT_STAN_SEARCH_MAX)
    {
        g_ModMpBotStanReverse[reversecount++] = current;
        parent = g_ModMpBotStanNodes[current].parent;
        current = parent;
    }

    if (reversecount <= 0)
        return FALSE;

    routecount = reversecount;

    if (routecount > MAX_CHRWAYPOINTS)
        routecount = MAX_CHRWAYPOINTS;

    for (i = 0; i < routecount; i++)
        runtime->navtiles[i] = g_ModMpBotStanNodes[g_ModMpBotStanReverse[reversecount - 1 - i]].tile;

    runtime->navcount = (s8)routecount;
    runtime->navindex = (runtime->navtiles[0] == starttile && routecount > 1) ? 1 : 0;
    runtime->navactive = TRUE;
    return TRUE;
}

static s32 modMpBotsStanFindEdgeTo(StandTile *from, StandTile *to)
{
    s32 edge;
    s32 pointcount;

    if (from == NULL || to == NULL)
        return -1;

    pointcount = stanGetTilePointCount(from);

    for (edge = 0; edge < pointcount; edge++)
    {
        if (stanGetLinkedTileAtEdge(from, edge) == to)
            return edge;
    }

    return -1;
}

static void modMpBotsPdNavSetTravelYaw(struct ModMpBotRuntime *runtime, coord3d *goal)
{
    f32 dx = goal->x - runtime->chr->prop->pos.x;
    f32 dz = goal->z - runtime->chr->prop->pos.z;

    if (dx * dx + dz * dz > 1.0f)
        runtime->roty = modMpBotsPdNormalizeAngle(atan2f(dx, dz));
}

static void modMpBotsPdCheckDoor(struct ModMpBotRuntime *runtime, coord3d *goal)
{
    PropRecord *doorprop;
    PropRecord *scan;
    ObjectRecord *obj;
    f32 gx;
    f32 gz;
    f32 len;
    f32 endx;
    f32 endz;
    f32 dx;
    f32 dy;
    f32 dz;

    if (runtime->chr == NULL || runtime->chr->prop == NULL || goal == NULL
        || (runtime->navage % 10) != 0)
        return;

    gx = goal->x - runtime->chr->prop->pos.x;
    gz = goal->z - runtime->chr->prop->pos.z;
    len = sqrtf(gx * gx + gz * gz);
    endx = goal->x;
    endz = goal->z;

    if (len > 1.0f)
    {
        endx += gx * (220.0f / len);
        endz += gz * (220.0f / len);
    }

    doorprop = sub_GAME_7F0B1410(runtime->chr->prop->stan,
        runtime->chr->prop->pos.x, runtime->chr->prop->pos.z,
        endx, endz, 0x5000);

    /* Temple's short portal approaches can stop the route probe just before
     * the door polygon.  Fall back to a player-like local USE search: choose
     * only a nearby door lying in the current travel corridor. */
    if (doorprop == NULL && len > 1.0f)
    {
        f32 invlen = 1.0f / len;

        for (scan = chrpropGetActiveTail(); scan != NULL; scan = scan->prev)
        {
            f32 along;
            f32 side;
            f32 sx;
            f32 sz;

            if (scan->type != PROP_TYPE_DOOR || scan->door == NULL
                || scan->obj == NULL)
                continue;

            sx = scan->pos.x - runtime->chr->prop->pos.x;
            sz = scan->pos.z - runtime->chr->prop->pos.z;
            along = (sx * gx + sz * gz) * invlen;

            if (along <= 0.0f || along > 280.0f)
                continue;

            side = sx * (-gz * invlen) + sz * (gx * invlen);
            if (side < -120.0f || side > 120.0f)
                continue;

            dy = scan->pos.y - runtime->chr->prop->pos.y;
            if (dy < -120.0f || dy > 120.0f)
                continue;

            doorprop = scan;
            break;
        }
    }

    if (doorprop == NULL || doorprop->door == NULL)
        return;

    obj = doorprop->obj;
    if (obj == NULL)
        return;

    dx = doorprop->pos.x - runtime->chr->prop->pos.x;
    dy = doorprop->pos.y - runtime->chr->prop->pos.y;
    dz = doorprop->pos.z - runtime->chr->prop->pos.z;

    if (dx * dx + dy * dy + dz * dz < 78400.0f)
    {
        doorsChooseSwingDirection(runtime->chr->prop, doorprop->door);
        doorActivate(doorprop->door, 1);
    }
}

static void modMpBotsPdNavTick(s32 slot, PropRecord *targetprop)
{
    struct ModMpBotRuntime *runtime = &g_ModMpBotRuntime[slot];
    StandTile *currenttile;
    StandTile *nexttile;
    coord3d portal;
    coord3d center;
    coord3d goal;
    f32 portalwidth;
    s16 targetcode;
    s32 edge;
    s32 rebuild = FALSE;
    s32 direct = FALSE;

    if (!modMpBotsPdMoveTargetValid(targetprop))
    {
        modMpBotsPdNavClear(runtime);
        return;
    }

    if (runtime->chr == NULL || runtime->chr->prop == NULL
        || runtime->chr->prop->stan == NULL || targetprop->stan == NULL)
    {
        modMpBotsPdNavClear(runtime);
        return;
    }

    targetcode = modMpBotsPdTargetCodeForProp(targetprop);
    currenttile = runtime->chr->prop->stan;

    if (!runtime->navactive
        || runtime->navtargetcode != targetcode
        || runtime->navgoalstan != targetprop->stan)
    {
        rebuild = TRUE;
    }

    if (!rebuild && runtime->navindex > 0 && runtime->navindex <= runtime->navcount)
    {
        StandTile *previous = runtime->navtiles[runtime->navindex - 1];
        StandTile *expected = runtime->navindex < runtime->navcount
            ? runtime->navtiles[runtime->navindex] : runtime->navgoalstan;

        if (currenttile != previous && currenttile != expected)
            rebuild = TRUE;
    }

    if (rebuild)
        modMpBotsPdNavBuildRoute(slot, targetprop);

    runtime->navage += g_ClockTimer;
    runtime->navgoalpos = targetprop->pos;

    if (runtime->targetinsight
        && (currenttile == targetprop->stan
            || modMpBotsPdNavSqDist(&runtime->chr->prop->pos, &targetprop->pos)
                <= MODBOT_STAN_DIRECT_RANGE_SQ))
    {
        direct = TRUE;
    }

    if (direct)
    {
        goal = targetprop->pos;
    }
    else if (runtime->navactive)
    {
        while (runtime->navindex < runtime->navcount
            && currenttile == runtime->navtiles[runtime->navindex])
        {
            runtime->navindex++;
        }

        if (runtime->navindex >= runtime->navcount)
        {
            if (currenttile != runtime->navgoalstan)
            {
                if (!modMpBotsPdNavBuildRoute(slot, targetprop))
                {
                    if (runtime->targetinsight)
                        goal = targetprop->pos;
                    else
                    {
                        modMpBotsPdNavClear(runtime);
                        return;
                    }
                }
                currenttile = runtime->chr->prop->stan;
            }
        }

        if (runtime->navactive && runtime->navindex < runtime->navcount)
        {
            nexttile = runtime->navtiles[runtime->navindex];
            edge = modMpBotsStanFindEdgeTo(currenttile, nexttile);

            if (edge < 0)
            {
                if (!modMpBotsPdNavBuildRoute(slot, targetprop))
                {
                    if (runtime->targetinsight)
                        goal = targetprop->pos;
                    else
                    {
                        modMpBotsPdNavClear(runtime);
                        return;
                    }
                }
                currenttile = runtime->chr->prop->stan;
                nexttile = runtime->navindex < runtime->navcount
                    ? runtime->navtiles[runtime->navindex] : NULL;
                edge = modMpBotsStanFindEdgeTo(currenttile, nexttile);
            }

            if (edge >= 0 && nexttile != NULL
                && stanGetEdgeMidPointWorld(currenttile, edge, &portal, &portalwidth))
            {
                getTileMidPoint(nexttile, &center);

                /* Aim through the middle of the portal, but pull the steering
                 * point inward toward the next STAN centre.  This prevents
                 * wall/ledge shaving without forcing robotic centroid hops. */
                goal.x = portal.x * 0.60f + center.x * 0.40f;
                goal.y = portal.y * 0.60f + center.y * 0.40f;
                goal.z = portal.z * 0.60f + center.z * 0.40f;

                if (modMpBotsPdNavSqDist(&runtime->chr->prop->pos, &goal)
                    <= MODBOT_STAN_REACH_SQ)
                {
                    goal = center;
                }
            }
            else
            {
                getTileMidPoint(nexttile, &goal);
            }
        }
        else
        {
            goal = targetprop->pos;
        }
    }
    else if (runtime->targetinsight)
    {
        goal = targetprop->pos;
    }
    else
    {
        return;
    }

    modMpBotsPdNavSetTravelYaw(runtime, &goal);
    modMpBotsPdCheckDoor(runtime, &goal);
}

static void modMpBotsPdTickDistMode(s32 slot, PropRecord *targetprop)
{
    struct ModMpBotRuntime *runtime = &g_ModMpBotRuntime[slot];
    struct ModMpBotConfig *config = &g_ModMpBotConfigs[slot];
    f32 *limits = g_ModMpBotPdDistConfigs[modMpBotsPdGetDistConfig(slot)];
    f32 distance;
    f32 minattackdistance;
    f32 maxattackdistance;
    f32 limit3;
    s32 targetparticipant;
    s16 targetcode;
    s32 insight;
    s32 prevmode = runtime->distmode;
    s32 newmode;

    if (!modMpBotsPdMoveTargetValid(targetprop))
        return;

    targetparticipant = modMpBotsPdFindParticipant(targetprop);
    insight = targetparticipant >= 0 ? runtime->chrsinsight[targetparticipant] : FALSE;
    targetcode = modMpBotsPdTargetCodeForProp(targetprop);
    distance = modMpBotsPdDistance(runtime->chr, targetprop);
    minattackdistance = limits[0];
    maxattackdistance = limits[1];
    limit3 = limits[2];

    if (config->difficulty == MODBOT_DIFFICULTY_MEAT)
        minattackdistance *= 0.35f;
    else if (config->difficulty == MODBOT_DIFFICULTY_EASY)
        minattackdistance *= 0.5f;

    if (runtime->distmode == MODBOT_PD_DISTMODE_BACKUP)
        minattackdistance += 25.0f;
    else if (runtime->distmode == MODBOT_PD_DISTMODE_ADVANCE
        || runtime->distmode == MODBOT_PD_DISTMODE_GOTO)
        maxattackdistance -= 25.0f;

    if (distance < minattackdistance)
        newmode = MODBOT_PD_DISTMODE_BACKUP;
    else if (distance < maxattackdistance)
        newmode = MODBOT_PD_DISTMODE_OK;
    else if (distance < limit3)
        newmode = MODBOT_PD_DISTMODE_ADVANCE;
    else
        newmode = MODBOT_PD_DISTMODE_GOTO;

    if (!(newmode == MODBOT_PD_DISTMODE_BACKUP && insight
        && runtime->distoverridecode == targetcode))
    {
        runtime->distoverridecode = MODBOT_TARGET_NONE;
        runtime->distoverridetimer60 = 0;
    }

    if (newmode == MODBOT_PD_DISTMODE_OK)
    {
        if (!insight)
            newmode = MODBOT_PD_DISTMODE_ADVANCE;
    }
    else if (newmode == MODBOT_PD_DISTMODE_BACKUP)
    {
        if (!insight)
        {
            newmode = MODBOT_PD_DISTMODE_ADVANCE;
            runtime->distoverridecode = targetcode;
            runtime->distoverridetimer60 = 20 + (s32)(randomGetNext() % 120U);
        }
        else if (runtime->distoverridecode != MODBOT_TARGET_NONE)
        {
            if (g_ClockTimer < runtime->distoverridetimer60)
            {
                runtime->distoverridetimer60 -= g_ClockTimer;
                newmode = MODBOT_PD_DISTMODE_OK;
            }
            else
            {
                runtime->distoverridecode = MODBOT_TARGET_NONE;
                runtime->distoverridetimer60 = 0;
            }
        }
    }

    runtime->distmode = (s8)newmode;

    if (runtime->distmodettl60 >= 0)
        runtime->distmodettl60 -= g_ClockTimer;

    switch (newmode)
    {
        case MODBOT_PD_DISTMODE_BACKUP:
        {
            f32 dx = targetprop->pos.x - runtime->chr->prop->pos.x;
            f32 dz = targetprop->pos.z - runtime->chr->prop->pos.z;
            runtime->manualbackup = TRUE;
            runtime->roty = modMpBotsPdNormalizeAngle(atan2f(dx, dz));
            modMpBotsPdNavClear(runtime);
            break;
        }
        case MODBOT_PD_DISTMODE_OK:
            runtime->manualbackup = FALSE;
            modMpBotsPdNavClear(runtime);
            break;
        case MODBOT_PD_DISTMODE_ADVANCE:
        case MODBOT_PD_DISTMODE_GOTO:
            runtime->manualbackup = FALSE;
            modMpBotsPdNavTick(slot, targetprop);
            break;
    }

    if (newmode != prevmode || runtime->distmodettl60 <= 0)
        runtime->distmodettl60 = 60;
}

static void modMpBotsPdTickCore(s32 slot)
{
    struct ModMpBotRuntime *runtime = &g_ModMpBotRuntime[slot];
    PropRecord *attackprop;
    PropRecord *moveprop;
    PropRecord *aimprop;
    PropRecord *followprop = NULL;
    f32 oldangle;
    f32 targetangle;
    f32 tweenangle;
    f32 diffangle;
    f32 newangle;
    f32 dx;
    f32 dz;

    /* R30 P4: Simulant travel yaw is independent from visible look yaw and
     * from every GoldenEye guard action state. */
    setsubroty(runtime->chr->model, runtime->roty);

    if (scenario == SCENARIO_COOP)
    {
        modMpBotsPdSetTarget(slot, modMpBotsCoopChooseTarget(slot, TRUE));
        attackprop = runtime->targetprop;
        followprop = modMpBotsCoopChooseTarget(slot, FALSE);
    }
    else
    {
        modMpBotsPdChooseGeneralTarget(slot);
        attackprop = modMpBotsPdChoosePersonalityAttack(slot);
    }

    if (attackprop != NULL && modMpBotHasTrait(slot, MODBOT_TRAIT_STALKER)
        && runtime->stalktarget == MODBOT_TARGET_NONE)
        runtime->stalktarget = modMpBotsPdTargetCodeForProp(attackprop);

    runtime->attackingparticipant = (s16)modMpBotsPdFindParticipant(attackprop);

    /* An unarmed Simulant first looks for a real world weapon.  Armor
     * Specialist keeps its normal armour priority once the bot is armed.
     * Co-Op follows the nearest living human whenever no encountered hostile
     * guard is taking movement priority. */
    moveprop = modMpBotsPdFindArmorTarget(slot);
    if (moveprop == NULL)
        moveprop = attackprop;
    if (moveprop == NULL)
        moveprop = followprop;

    if (moveprop != NULL)
    {
        if (scenario == SCENARIO_COOP && moveprop == followprop
            && attackprop == NULL)
        {
            /* Following a teammate is not combat range management.  Stay with
             * the human instead of stopping at the normal weapon standoff
             * distance or backing away from them. */
            if (modMpBotsPdNavSqDist(&runtime->chr->prop->pos,
                    &followprop->pos) > (180.0f * 180.0f))
            {
                runtime->manualbackup = FALSE;
                modMpBotsPdNavTick(slot, followprop);
            }
            else
            {
                runtime->manualbackup = FALSE;
                /* A teammate can be just across a closed door while already
                 * inside the follow radius.  Probe/use that door before
                 * clearing the route so the bot does not stop on its near side. */
                modMpBotsPdCheckDoor(runtime, &followprop->pos);
                modMpBotsPdNavClear(runtime);
            }
        }
        else
        {
            modMpBotsPdTickDistMode(slot, moveprop);
        }
    }
    else
    {
        runtime->manualbackup = FALSE;
        modMpBotsPdNavClear(runtime);
    }

    oldangle = runtime->lookangle;
    aimprop = runtime->targetprop;

    if (aimprop != NULL && runtime->targetinsight)
    {
        dx = aimprop->pos.x - runtime->chr->prop->pos.x;
        dz = aimprop->pos.z - runtime->chr->prop->pos.z;
        targetangle = atan2f(dx, dz) + runtime->zeroangle;
        targetangle = modMpBotsPdNormalizeAngle(targetangle);
    }
    else
    {
        targetangle = runtime->roty;
    }

    tweenangle = g_GlobalTimerDelta * 0.061590049415827f;
    diffangle = modMpBotsPdAngleDelta(oldangle, targetangle);

    if (diffangle >= 0.0f)
    {
        if (diffangle <= tweenangle)
            newangle = targetangle;
        else
            newangle = modMpBotsPdNormalizeAngle(oldangle + tweenangle);
    }
    else
    {
        if (diffangle >= -tweenangle)
            newangle = targetangle;
        else
            newangle = modMpBotsPdNormalizeAngle(oldangle - tweenangle);
    }

    runtime->speedtheta = modMpBotsPdAngleDelta(oldangle, newangle);
    if (g_GlobalTimerDelta > 0.0f)
    {
        runtime->speedtheta /= g_GlobalTimerDelta;
        runtime->speedtheta *= 16.236389160156f;
    }
    else
    {
        runtime->speedtheta = 0.0f;
    }

    runtime->lookangle = newangle;

    if (runtime->manualbackup)
    {
        runtime->speedmultforwards = -1.0f;
        runtime->speedmultsideways = 0.0f;
    }
    else if (runtime->navactive)
    {
        /* Perfect Dark's Simulant route movement is forward-only here.
         * Do not invent lateral stuck-recovery input in the GE adaptation. */
        runtime->speedmultforwards = 1.0f;
        runtime->speedmultsideways = 0.0f;
    }
    else
    {
        runtime->speedmultforwards = 0.0f;
        runtime->speedmultsideways = 0.0f;
        runtime->realignangleframe = g_GlobalTimer;
    }

    /* PD botinv_tick precedes the two-hand combat loop in bot_tick_unpaused. */
    modMpBotsPdBotInvTick(slot);
    if (!(scenario == SCENARIO_COOP && runtime->targetprop != NULL
        && runtime->targetprop->type == PROP_TYPE_VIEWER))
        modMpBotsPdCombatTick(slot);
}

/*
 * PD calls playerChooseThirdPersonAnimation for Simulants.  GoldenEye already
 * owns equivalent authored remote-player firing/locomotion tables, so feed the
 * Simulant movement state into those tables rather than leaving the chr in a
 * static guard standing pose.
 */
static void modMpBotsApplyPlayerAnimation(s32 slot)
{
    struct ModMpBotRuntime *runtime = &g_ModMpBotRuntime[slot];
    ChrRecord *chr = runtime->chr;
    PropRecord *leftprop;
    PropRecord *rightprop;
    struct WeaponObjRecord *leftobj;
    struct WeaponObjRecord *rightobj;
    struct firing_anim_struct *fa;
    void *firingtable;
    ModelAnimation *anim;
    ModelAnimation *cur;
    f32 fwd;
    f32 side;
    f32 turn;
    f32 amount;
    f32 pitch;
    f32 startframe;
    s32 group;
    s32 sub;

    if (chr == NULL || chr->model == NULL || chr->prop == NULL || chrIsDead(chr))
        return;

    leftprop = chrGetEquippedWeaponProp(chr, GUNLEFT);
    rightprop = chrGetEquippedWeaponProp(chr, GUNRIGHT);
    leftobj = leftprop != NULL ? leftprop->weapon : NULL;
    rightobj = rightprop != NULL ? rightprop->weapon : NULL;

    if (leftprop != NULL && rightprop != NULL)
        group = 3;
    else if (leftprop == NULL && rightprop == NULL)
        group = 2;
    else if ((leftobj != NULL
            && !bondwalkItemCheckBitflags(leftobj->weaponnum, WEAPONSTATBITFLAG_HOLD_AS_GUN))
        || (rightobj != NULL
            && !bondwalkItemCheckBitflags(rightobj->weaponnum, WEAPONSTATBITFLAG_HOLD_AS_GUN)))
        group = 2;
    else if ((leftobj != NULL
            && bondwalkItemCheckBitflags(leftobj->weaponnum, WEAPONSTATBITFLAG_ONLY_1_HANDED))
        || (rightobj != NULL
            && bondwalkItemCheckBitflags(rightobj->weaponnum, WEAPONSTATBITFLAG_ONLY_1_HANDED)))
        group = 0;
    else
        group = 1;

    fwd = runtime->speedmultforwards;
    side = runtime->speedmultsideways;
    turn = runtime->speedtheta;
    if (turn < 0.0f)
        turn = -turn;

    amount = fwd < 0.0f ? -fwd : fwd;
    if (amount < (side < 0.0f ? -side : side))
        amount = side < 0.0f ? -side : side;
    if (amount < turn)
        amount = turn;

    if (amount < 0.05f)
    {
        sub = 0;
        amount = 1.0f;
    }
    else if (side < -0.05f && firing_animation_groups[group][4].pointer != NULL)
    {
        sub = 4;
        amount = -side;
    }
    else if (side > 0.05f && firing_animation_groups[group][3].pointer != NULL)
    {
        sub = 3;
        amount = side;
    }
    else
    {
        sub = amount >= 0.4f ? 2 : 1;
        if (sub == 1)
            amount += amount;
        if (amount > 1.0f)
            amount = 1.0f;
        if (fwd < -0.05f)
            amount = -amount;
    }

    fa = &firing_animation_groups[group][sub];
    firingtable = fa->pointer;
    anim = NULL;

    if (fa->anim != 0)
        anim = (ModelAnimation *)(fa->anim + (s32)ptr_animation_table);
    else if (firingtable != NULL)
        anim = (ModelAnimation *)*((s32 *)firingtable);

    if (anim == NULL)
        return;

    amount *= fa->x;
    cur = objecthandlerGetModelAnim(chr->model);

    if (cur != anim && chr->model->anim2 == NULL)
    {
        startframe = fa->y >= 0.0f ? fa->y : 0.0f;
        if (amount < 0.0f && fa->y < 0.0f)
            startframe = anim->unk04 - 1.0f;

        modelSetAnimation(chr->model, anim, 0, startframe, amount, 16.0f);
        if (fa->y >= 0.0f)
            modelSetAnimLooping(chr->model, fa->y, 16.0f);
        if (fa->z >= 0.0f)
            modelSetAnimEndFrame(chr->model, fa->z);
    }
    else if (cur == anim && modelGetAnimSpeed(chr->model) != amount)
    {
        modelSetAnimSpeed(chr->model, amount, 1.0f);
    }

    pitch = chr->aimendrshoulder;
    if (firingtable != NULL)
    {
        chr->hidden &= ~CHRHIDDEN_0400;
        chrlvUpdateAimendbackShoulders(chr, firingtable, group == 3, 1, pitch);
    }
    else
    {
        chr->aimendrshoulder = 0.0f;
        chr->aimendlshoulder = 0.0f;
        chr->aimendback = 0.0f;
        chr->hidden |= CHRHIDDEN_0400;
    }
}

void modMpBotsNotifyPropFreed(PropRecord *prop)
{
    s32 i;

    for (i = 0; i < g_ModMpBotCount; i++)
    {
        struct ModMpBotRuntime *runtime = &g_ModMpBotRuntime[i];

        if (runtime->chr != NULL && runtime->chr->prop == prop)
        {
            runtime->chr = NULL;
            runtime->targetprop = NULL;
            runtime->spawnready = FALSE;
            runtime->respawntimer = runtime->deathrecorded ? -1 : 0;
            runtime->deathrecorded = FALSE;
            return;
        }
    }
}

void modMpBotsTick(void)
{
    struct ModMpBotRuntime *runtime;
    s32 i;
    s32 n;
    s32 slot;

    if (!modMpBotsRuntimeAllowed())
        return;

    /* Existing Simulants tick first.  Spawning is intentionally done after
     * this loop so a new chr can never execute bot AI in its creation frame. */
    for (i = 0; i < g_ModMpBotCount; i++)
    {
        runtime = &g_ModMpBotRuntime[i];

        /* A fading corpse can lose its model before GE releases its prop.
         * Keep ownership until the prop is actually gone; otherwise the slot
         * can respawn while its previous body is still in the world. */
        if (runtime->chr == NULL || runtime->chr->prop == NULL)
        {
            s32 finisheddeath = runtime->deathrecorded;

            runtime->chr = NULL;
            runtime->targetprop = NULL;
            runtime->spawnready = FALSE;
            runtime->deathrecorded = FALSE;

            /* GE owns ACT_DIE -> ACT_DEAD -> corpse fade -> TICKOP_FREE.
             * -1 means the corpse was released since the previous bot tick.
             * Convert it to a one-tick hold here so the replacement can never
             * be spawned in the same Simulant tick that observed TICKOP_FREE. */
            if (finisheddeath || runtime->respawntimer < 0)
                runtime->respawntimer = 1;
            else if (runtime->respawntimer > 0)
                runtime->respawntimer--;
            continue;
        }

        if (runtime->damagegracetimer60 > 0)
        {
            runtime->damagegracetimer60 -= g_ClockTimer;
            if (runtime->damagegracetimer60 < 0)
                runtime->damagegracetimer60 = 0;
        }

        if (chrIsDead(runtime->chr))
        {
            runtime->speedmultforwards = 0.0f;
            runtime->speedmultsideways = 0.0f;
            runtime->moveratex = 0.0f;
            runtime->moveratez = 0.0f;

            if (!runtime->deathrecorded)
            {
                modMpBotsInvalidateTargetProp(runtime->chr->prop);
                modMpBotsRecordDeath(i);
                runtime->deathrecorded = TRUE;
                runtime->respawntimer = 0;
            }
            continue;
        }

        /* The first normal chr tick after creation owns initialization.  P3
         * pickup mutation and P1/P2 decision/combat cannot run before that. */
        if (!runtime->spawnready)
        {
            runtime->speedmultforwards = 0.0f;
            runtime->speedmultsideways = 0.0f;
            runtime->moveratex = 0.0f;
            runtime->moveratez = 0.0f;
            continue;
        }

        runtime->deathrecorded = FALSE;
        modMpBotsPdTickCore(i);
    }

    if (!modMpBotsStageReadyForSpawn())
        return;

    /* Materialize at most one bot per frame.  This prevents a single MP load
     * tick from mutating the active prop list and loading up to eight new
     * character model pairs at once.  Rotate the starting slot so one bad or
     * memory-heavy character cannot starve the others. */
    for (n = 0; n < g_ModMpBotCount; n++)
    {
        slot = (g_ModMpBotSpawnCursor + n) % g_ModMpBotCount;
        runtime = &g_ModMpBotRuntime[slot];

        if (runtime->chr == NULL && runtime->respawntimer <= 0)
        {
            modMpBotsSpawn(slot);
            g_ModMpBotSpawnCursor = (slot + 1) % g_ModMpBotCount;
            break;
        }
    }
}

void modMpBotsPostChrTick(void)
{
    struct ModMpBotRuntime *runtime;
    s32 i;

    if (!modMpBotsRuntimeAllowed())
        return;

    for (i = 0; i < g_ModMpBotCount; i++)
    {
        runtime = &g_ModMpBotRuntime[i];

        if (runtime->chr == NULL || runtime->chr->model == NULL
            || runtime->chr->prop == NULL || chrIsDead(runtime->chr))
            continue;

        /* Reaching this seam proves the newly-created chr survived one normal
         * chrlvAllChrTick.  Do not mutate pickups or animation in that same
         * creation frame; simply arm it for the next Simulant tick. */
        if (!runtime->spawnready)
        {
            runtime->spawnready = TRUE;
            continue;
        }

        modMpBotsApplyPlayerAnimation(i);

        /* Travel yaw remains private Simulant state.  Only the independent PD
         * look yaw is published to the visible GE character model. */
        setsubroty(runtime->chr->model, runtime->lookangle);

        /* PD bot_check_pickups runs after the character movement tick. */
        modMpBotsPdCheckPickups(i);
    }
}


#endif
