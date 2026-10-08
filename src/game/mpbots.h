#ifndef _GAME_MPBOTS_H_
#define _GAME_MPBOTS_H_

#include <ultra64.h>
#include <bondtypes.h>

#ifdef GE_MODDED_CHEATS

#define MOD_MP_BOT_MAX 8
#define MOD_MP_BOT_HUMAN_TARGET_BASE 0
#define MOD_MP_BOT_BOT_TARGET_BASE MAX_PLAYER_COUNT

typedef enum ModMpBotDifficulty
{
    MODBOT_DIFFICULTY_MEAT = 0,
    MODBOT_DIFFICULTY_EASY,
    MODBOT_DIFFICULTY_NORMAL,
    MODBOT_DIFFICULTY_HARD,
    MODBOT_DIFFICULTY_PERFECT,
    MODBOT_DIFFICULTY_DARK,
    MODBOT_DIFFICULTY_MAX
} ModMpBotDifficulty;

typedef enum ModMpBotTrait
{
    MODBOT_TRAIT_PACIFIST       = 1 << 0,
    MODBOT_TRAIT_VINDICTIVE     = 1 << 1,
    MODBOT_TRAIT_PSYCHOTIC      = 1 << 2,
    MODBOT_TRAIT_JUGGERNAUT     = 1 << 3,
    MODBOT_TRAIT_ARMOR_SPECIALIST = 1 << 4,
    MODBOT_TRAIT_PYROMANIAC     = 1 << 5,
    MODBOT_TRAIT_BULLY          = 1 << 6,
    MODBOT_TRAIT_FEARFUL        = 1 << 7,
    MODBOT_TRAIT_EQUALIZER      = 1 << 8,
    MODBOT_TRAIT_STALKER        = 1 << 9,
    MODBOT_TRAIT_MELEE          = 1 << 10,
    MODBOT_TRAIT_SPEEDY         = 1 << 11,
    MODBOT_TRAIT_CHEAP          = 1 << 12,
    MODBOT_TRAIT_COUNT          = 13
} ModMpBotTrait;

#define MOD_MP_BOT_NAME_LEN 16

struct ModMpBotConfig
{
    u8 character;
    u8 difficulty;
    u16 traits;
    char name[MOD_MP_BOT_NAME_LEN];
};

extern u8 g_ModMpBotCount;
extern struct ModMpBotConfig g_ModMpBotConfigs[MOD_MP_BOT_MAX];

s32 modMpBotsGetCount(void);
void modMpBotsSetCount(s32 count);
void modMpBotsCycleCount(void);

const char *modMpBotGetDifficultyName(s32 difficulty);
const char *modMpBotGetTraitName(s32 traitindex);
const char *modMpBotGetDisplayName(s32 slot);

void modMpBotCycleCharacter(s32 slot, s32 direction);
void modMpBotCycleDifficulty(s32 slot, s32 direction);
void modMpBotToggleTrait(s32 slot, s32 traitindex);
s32 modMpBotHasTrait(s32 slot, u16 trait);

void modMpBotsResetRuntime(void);
void modMpBotsPrepareStage(void);
void modMpBotsTick(void);
void modMpBotsPostChrTick(void);
void modMpBotsNotifyPropFreed(PropRecord *prop);

s32 modMpBotsGetSlotForChr(ChrRecord *chr);
PropRecord *modMpBotsGetTargetProp(ChrRecord *chr);
PropRecord *modMpBotsGetBotProp(s32 slot);
f32 modMpBotsGetMoveMultiplier(ChrRecord *chr);
s32 modMpBotsApplyLateral(ChrRecord *chr, coord3d *src, coord3d *dst);

s32 modMpBotsCanTakeDamage(ChrRecord *victim);
void modMpBotsOnDamageAccepted(ChrRecord *victim);
void modMpBotsNotifyHumanHit(ChrRecord *victim, s32 playernum);
void modMpBotsNotifyBotHit(ChrRecord *victim, ChrRecord *attacker);
void modMpBotsSetDamageOwner(s32 slot);
void modMpBotsClearDamageOwner(void);
s32 modMpBotsGetDamageOwner(void);
void modMpBotsDamageViewer(s32 slot, PropRecord *viewer, ITEM_IDS weaponid, coord3d *direction);

s32 modMpBotsGetHumanKills(s32 playernum);
s32 modMpBotsGetBotKills(s32 slot);
s32 modMpBotsGetBotDeaths(s32 slot);
void modMpBotsRecordBotKillOnHuman(s32 slot, s32 playernum);

#endif

#endif
