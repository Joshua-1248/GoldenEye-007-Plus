#include <ultra64.h>
#include <music.h>
#include <bondgame.h>
#include <bondinv.h>
#include <bondconstants.h>
#include <boss.h>
#include <joy.h>
#include <random.h>
#include "textrelated.h"
#include "frametiming.h"
#include "player.h"
#include "mpmenu.h"
#include "gun.h"
#include "front.h"
#include "lv.h"
#include "language.h"
#include "mp_music.h"
#include "objective_status.h"
#include "file.h"
#include "file2.h"
#include "options.h"
#include "cheat.h"
#include "assets/obseg/text/LmpmenuE.h"
#include "assets/obseg/text/LoptionE.h"

#ifdef REFRESH_PAL
#define MPMENU_YOFF 8   /* PAL: every text row sits 8 pixels lower */
#else
#define MPMENU_YOFF 0
#endif

// bss
s32 g_stopPlayFlag;

/**
 * g_gameOverFlag has two meanings. One is the standard true/false for whether the game is over.
 * The other is as a timer. Values >= 2 mean a countdown is still running.
 */
s32 g_gameOverFlag;

s32 chevron_glow;
s32 chevron_glow_timer;
s32 alt_gameover_msg;
s32 alt_gameover_msg_timer;
s32 g_pausedFlag;
s32 who_paused;

// data
u16 g_AwardNames[] = {
    getStringID(LMPMENU, MPMENU_STR_00_LEMMINGAWARD),getStringID(LMPMENU, MPMENU_STR_01_WHERESTHEAMMO),getStringID(LMPMENU, MPMENU_STR_02_WHERESTHEARMOR),getStringID(LMPMENU, MPMENU_STR_03_AC10AWARD),getStringID(LMPMENU, MPMENU_STR_04_MARKSMANSHIPAWARD),getStringID(LMPMENU, MPMENU_STR_05_MOSTPROFESSIONAL),
    getStringID(LMPMENU, MPMENU_STR_06_MOSTDEADLY),getStringID(LMPMENU, MPMENU_STR_07_MOSTLYHARMLESS),getStringID(LMPMENU, MPMENU_STR_08_MOSTCOWARD),getStringID(LMPMENU, MPMENU_STR_09_MOSTFRANTIC),getStringID(LMPMENU, MPMENU_STR_0A_MOSTHONORABLE),getStringID(LMPMENU, MPMENU_STR_0B_MOSTDISHONORABLE),
    getStringID(LMPMENU, MPMENU_STR_0C_SHORTESTINNINGS),getStringID(LMPMENU, MPMENU_STR_0D_LONGESTINNINGS),getStringID(LMPMENU, MPMENU_STR_0E_DOUBLEKILL),getStringID(LMPMENU, MPMENU_STR_0F_TRIPLEKILL),getStringID(LMPMENU, MPMENU_STR_10_QUADKILL)
};


#ifdef GE_MODDED_CHEATS
void mpwatchPlayBeep(void);
/* R21 MP watch configuration hub.  MENU_OBJECTIVES is repurposed outside the
 * old experimental objectives card as the inserted player-settings card. */
static u8 mpcfg_mode[MAX_PLAYER_COUNT];      /* 0 hub, 1 options, 2 special, 3 cheats */
static u8 mpcfg_row[MAX_PLAYER_COUNT];
static u8 mpcfg_modal[MAX_PLAYER_COUNT];
static u8 mpcfg_choice[MAX_PLAYER_COUNT];
static u8 mpcfg_yready[MAX_PLAYER_COUNT] = {1,1,1,1};

static s32 mpwatchIsDebugCheat(CHEAT_ID cheat)
{
    return cheat >= CHEAT_UNLOCK_PAINTBALL || cheat == CHEAT_EXTRA_MP_CHARS ||
        (cheat >= CHEAT_MAXAMMO && cheat <= CHEAT_2X_ARMOR) ||
        cheat == CHEAT_EXTRA_WEAPONS || cheat == CHEAT_10X_HEALTH ||
        cheat == CHEAT_BONDPHASE || cheat == CHEAT_DEBUG_POS ||
        cheat == CHEAT_NO_RELOAD || cheat == CHEAT_RAPID_FIRE;
}

static s32 mpwatchCheatUnlocked(CHEAT_ID cheat)
{
    return mpwatchIsDebugCheat(cheat) || frontCheckIfCheatIsUnlocked(cheat);
}

static s32 mpwatchConfigRows(s32 mode)
{
    return mode == 0 ? 3 : mode == 1 ? 8 : mode == 2 ? 9 : CHEAT_INVALID - 1;
}

static void mpwatchConfigMove(s32 player, s32 dir)
{
    s32 n = mpwatchConfigRows(mpcfg_mode[player]);
    s32 r = mpcfg_row[player] + dir;
    if (r < 0) r = n - 1;
    if (r >= n) r = 0;
    mpcfg_row[player] = r;
}

static void mpwatchConfigToggleSpecial(s32 player, s32 row)
{
    if (row == 0) g_MpPlayerOptions[player] ^= MP_PLAYEROPT_HEADROLL;
    else if (row == 1) g_ModGameplayOptions2 ^= MODOPT2_ENDLESS_DEATHCAM;
    else if (row == 2) g_ModGameplayOptions2 ^= MODOPT2_REALTIME_COLLAPSE;
    else if (row == 3) g_ModGameplayOptions2 ^= MODOPT2_DISABLE_HITSTUN;
    else if (row == 4) g_ModGameplayOptions2 ^= MODOPT2_DISABLE_KNOCKBACK;
    else if (row == 5) g_ModGameplayOptions2 ^= MODOPT2_DISABLE_NOISE_DITHER;
    else if (row == 6) g_ModGameplayOptions2 ^= MODOPT2_DAMAGE_FLASH;
    else if (row == 7) g_MpPlayerCrosshair[player] ^= 1;
    else g_MpPlayerOptions[player] ^= MP_PLAYEROPT_DAMAGE_SFX;
}

static s32 mpwatchConfigSpecialValue(s32 player, s32 row)
{
    if (row == 0) return (g_MpPlayerOptions[player] & MP_PLAYEROPT_HEADROLL) != 0;
    if (row == 1) return (g_ModGameplayOptions2 & MODOPT2_ENDLESS_DEATHCAM) != 0;
    if (row == 2) return (g_ModGameplayOptions2 & MODOPT2_REALTIME_COLLAPSE) != 0;
    if (row == 3) return (g_ModGameplayOptions2 & MODOPT2_DISABLE_HITSTUN) != 0;
    if (row == 4) return (g_ModGameplayOptions2 & MODOPT2_DISABLE_KNOCKBACK) != 0;
    if (row == 5) return (g_ModGameplayOptions2 & MODOPT2_DISABLE_NOISE_DITHER) != 0;
    if (row == 6) return (g_ModGameplayOptions2 & MODOPT2_DAMAGE_FLASH) != 0;
    if (row == 7) return g_MpPlayerCrosshair[player] != 0;
    return (g_MpPlayerOptions[player] & MP_PLAYEROPT_DAMAGE_SFX) != 0;
}

static s32 mpwatchConfigOptionBit(s32 player, s32 row)
{
    if (row == 0) return controlstyle_player[player] & 3;
    if (row == 7) return g_MpPlayerCrosshair[player] != 0;
    return (g_MpPlayerOptions[player] >> (row - 1)) & 1;
}

static void mpwatchConfigSetOption(s32 player, s32 row, s32 value)
{
    if (row == 0)
    {
        value &= 3;
        controlstyle_player[player] = value;
        if (player == get_cur_playernum())
        {
            g_CurrentPlayer->controldef = value;
            cur_player_set_control_type(value);
        }
    }
    else if (row == 7)
    {
        g_MpPlayerCrosshair[player] = value != 0;
    }
    else
    {
        u8 bit = 1 << (row - 1);
        if (value) g_MpPlayerOptions[player] |= bit;
        else g_MpPlayerOptions[player] &= ~bit;
        copy_aim_settings_to_playerdata();
    }
}

static s32 mpwatchConfigHandleInput(s32 player)
{
    u32 pressed = joyGetButtonsPressedThisFrame(player, 0xffff);
    s32 sy = joyGetStickYInRange(player, -2, 1);
    s32 mode = mpcfg_mode[player];

    if (sy == 0 || sy == -1) mpcfg_yready[player] = 1;

    if (mpcfg_modal[player])
    {
        if (pressed & (L_JPAD|L_CBUTTONS))
        {
            if (mode == 1 && mpcfg_row[player] == 0) mpcfg_choice[player] = (mpcfg_choice[player] + 3) & 3;
            else mpcfg_choice[player] = 0;
            mpwatchPlayBeep();
        }
        else if (pressed & (R_JPAD|R_CBUTTONS))
        {
            if (mode == 1 && mpcfg_row[player] == 0) mpcfg_choice[player] = (mpcfg_choice[player] + 1) & 3;
            else mpcfg_choice[player] = 1;
            mpwatchPlayBeep();
        }
        else if (pressed & B_BUTTON) { mpcfg_modal[player] = 0; mpwatchPlayBeep(); }
        else if (pressed & A_BUTTON)
        {
            mpwatchConfigSetOption(player, mpcfg_row[player], mpcfg_choice[player]);
            mpcfg_modal[player] = 0; mpwatchPlayBeep();
        }
        return 1;
    }

    if ((pressed & (U_JPAD|U_CBUTTONS)) || (mpcfg_yready[player] && sy > 0))
    { mpwatchConfigMove(player,-1); mpcfg_yready[player]=0; mpwatchPlayBeep(); return 1; }
    if ((pressed & (D_JPAD|D_CBUTTONS)) || (mpcfg_yready[player] && sy < -1))
    { mpwatchConfigMove(player,1); mpcfg_yready[player]=0; mpwatchPlayBeep(); return 1; }

    if (pressed & B_BUTTON)
    {
        if (mode) { mpcfg_mode[player]=0; mpcfg_row[player]=0; mpwatchPlayBeep(); return 1; }
        return 0;
    }

    if (pressed & A_BUTTON)
    {
        s32 row = mpcfg_row[player];
        if (mode == 0) { mpcfg_mode[player]=row+1; mpcfg_row[player]=0; mpwatchPlayBeep(); }
        else if (mode == 1)
        {
            mpcfg_choice[player]=mpwatchConfigOptionBit(player,row);
            mpcfg_modal[player]=1; mpwatchPlayBeep();
        }
        else if (mode == 2) { mpwatchConfigToggleSpecial(player,row); mpwatchPlayBeep(); }
        else
        {
            CHEAT_ID cheat=(CHEAT_ID)(row+1);
            if (!mpwatchCheatUnlocked(cheat)) mpwatchPlayBeep();
            else
            {
                s32 active=cheatIsActive(cheat);
                if (active) cheatButtonHandleCheatsTurnedOff(cheat); else cheatButtonHandleCheatsTurnedOn(cheat);
                g_CheatActivated[cheat]=!active; g_AppendCheatSinglePlayer=TRUE; mpwatchPlayBeep();
            }
        }
        return 1;
    }
    return mode != 0;
}

typedef enum MPWATCH_CLASS
{
    MPWATCH_CLASS_MULTIPLAYER,
    MPWATCH_CLASS_COOP,
    MPWATCH_CLASS_GAMEOVER_MULTI,
    MPWATCH_CLASS_GAMEOVER_COOP
} MPWATCH_CLASS;

static s32 mpwatchIsCampaignCoop(void)
{
    return gamemode == GAMEMODE_MULTI && get_scenario() == SCENARIO_COOP;
}

static MPWATCH_CLASS mpwatchGetClass(void)
{
    if (g_gameOverFlag) return mpwatchIsCampaignCoop() ? MPWATCH_CLASS_GAMEOVER_COOP : MPWATCH_CLASS_GAMEOVER_MULTI;
    return mpwatchIsCampaignCoop() ? MPWATCH_CLASS_COOP : MPWATCH_CLASS_MULTIPLAYER;
}

static s32 mpwatchUseCoopPauseCarousel(void)
{
    return mpwatchGetClass() == MPWATCH_CLASS_COOP;
}

static s32 mpwatchHasOptionsCard(void)
{
    MPWATCH_CLASS c = mpwatchGetClass();
    return c == MPWATCH_CLASS_MULTIPLAYER || c == MPWATCH_CLASS_COOP;
}

static void mpwatchMenuGoRight(void)
{
    MPWATCH_CLASS c = mpwatchGetClass();
    if (g_CurrentPlayer->mpmenumode == MENU_GOWOC)
    {
        g_CurrentPlayer->mpmenumode = (c == MPWATCH_CLASS_MULTIPLAYER && mpwatchHasOptionsCard()) ? MENU_OBJECTIVES : MENU_LOSSES;
        return;
    }
    if (g_CurrentPlayer->mpmenumode == MENU_OBJECTIVES)
    {
        g_CurrentPlayer->mpmenumode = c == MPWATCH_CLASS_COOP ? MENU_GOWOC : MENU_LOSSES;
        return;
    }
    if (c == MPWATCH_CLASS_COOP && g_CurrentPlayer->mpmenumode == MENU_SCORES)
    { g_CurrentPlayer->mpmenumode = MENU_EXIT; return; }
    g_CurrentPlayer->mpmenumode++;
}

static void mpwatchMenuGoLeft(void)
{
    MPWATCH_CLASS c = mpwatchGetClass();
    if (g_CurrentPlayer->mpmenumode == MENU_LOSSES)
    {
        g_CurrentPlayer->mpmenumode = mpwatchHasOptionsCard() ? (c == MPWATCH_CLASS_COOP ? MENU_GOWOC : MENU_OBJECTIVES) : MENU_GOWOC;
        return;
    }
    if (g_CurrentPlayer->mpmenumode == MENU_GOWOC && c == MPWATCH_CLASS_COOP)
    { g_CurrentPlayer->mpmenumode = MENU_OBJECTIVES; return; }
    if (g_CurrentPlayer->mpmenumode == MENU_OBJECTIVES)
    { g_CurrentPlayer->mpmenumode = MENU_GOWOC; return; }
    if (c == MPWATCH_CLASS_COOP && g_CurrentPlayer->mpmenumode == MENU_EXIT)
    { g_CurrentPlayer->mpmenumode = MENU_SCORES; return; }
    g_CurrentPlayer->mpmenumode--;
}
#endif

s32 mpwatchMenuCanGoRight(void)
{
#ifdef GE_MODDED_CHEATS
    MPWATCH_CLASS c = mpwatchGetClass();
    if (c == MPWATCH_CLASS_COOP)
        return g_CurrentPlayer->mpmenumode != MENU_EXIT && g_CurrentPlayer->mpmenumode != MENU_EXIT_CONFIRM && g_CurrentPlayer->mpmenumode != MENU_FINISHED;
    if (c == MPWATCH_CLASS_GAMEOVER_MULTI || c == MPWATCH_CLASS_GAMEOVER_COOP)
        return g_CurrentPlayer->mpmenumode == MENU_GOWOC || g_CurrentPlayer->mpmenumode == MENU_LOSSES || g_CurrentPlayer->mpmenumode == MENU_KILLS;
#endif
    switch(g_CurrentPlayer->mpmenumode)
    {
        case MENU_GOWOC:
#ifdef GE_MODDED_CHEATS
        case MENU_OBJECTIVES:
#endif
        case MENU_LOSSES:
        case MENU_KILLS:
        case MENU_PAUSE:
            return 1;
        case MENU_EXIT:
        case MENU_EXIT_CONFIRM:
        case MENU_FINISHED:
            return 0;
        case MENU_SCORES:
            return g_gameOverFlag ? 0 : 1;
        default:
            return 0;
    }
}

s32 mpwatchMenuCanGoLeft(void)
{
#ifdef GE_MODDED_CHEATS
    MPWATCH_CLASS c = mpwatchGetClass();
    if (c == MPWATCH_CLASS_COOP)
        return g_CurrentPlayer->mpmenumode != MENU_OBJECTIVES;
    if (c == MPWATCH_CLASS_GAMEOVER_MULTI || c == MPWATCH_CLASS_GAMEOVER_COOP)
        return g_CurrentPlayer->mpmenumode == MENU_LOSSES || g_CurrentPlayer->mpmenumode == MENU_KILLS || g_CurrentPlayer->mpmenumode == MENU_SCORES;
#endif
    switch(g_CurrentPlayer->mpmenumode)
    {
        case MENU_KILLS:
        case MENU_SCORES:
        case MENU_PAUSE:
        case MENU_EXIT:
            return 1;
        case MENU_GOWOC:
        case MENU_EXIT_CONFIRM:
        case MENU_FINISHED:
            return 0;
        case MENU_LOSSES:
#ifdef GE_MODDED_CHEATS
            return 1;
#else
            return g_gameOverFlag ? 1 : 0;
#endif
        default:
            return 0;
    }
}

s32 mpwatchIsPlayerPressingRight(s32 player)
{
    s32 iVar3 = joyGetStickXInRange(player, -2, 1);

    if ((joyGetButtonsPressedThisFrame(player, R_JPAD|R_CBUTTONS)) || ((iVar3 >= 1  && (g_CurrentPlayer->mpjoywascentre)))) 
    {
        return 1;
    }

    return 0;
}

s32 mpwatchIsPlayerPressingLeft(s32 player)
{
    s32 iVar3 = joyGetStickXInRange(player, -2, 1);

    if ((joyGetButtonsPressedThisFrame(player, L_JPAD|L_CBUTTONS)) || ((iVar3 < -1 && (g_CurrentPlayer->mpjoywascentre)))) 
    {
        return 1;
    }

    return 0;
}


void mpwatchPlayBeep(void)
{
    sndPlaySfx(g_musicSfxBufferPtr, CAMERA_BEEP1_SFX, 0);
}


void mpwatchUnpauseGame(void)
{
    g_stopPlayFlag = 0;
    g_gameOverFlag = 0;
    g_pausedFlag = 0;
}


/**
 * Returns the index (0-3) of the player with the highest value.
 */
s32 mpFindMaxInt(s32 numplayers, s32 value0, s32 value1, s32 value2, s32 value3)
{
    s32 aux;
    s32 result;
 
    if ((value0 < value1) || ((value1 == value0 && ((randomGetNext() & 1))))) 
    {
        result = 1;
        aux = value1;
    }
    else 
    {
        result = 0;
        aux = value0;
    }
 
    if (numplayers >= 3) 
    {
 
        if ((aux < value2) || ((value2 == aux && ((randomGetNext() & 1))))) 
        {
            result = 2;
            aux = value2;
        }
 
        if (numplayers >= 4) 
        {
            if ((aux < value3) || ((value3 == aux && ((randomGetNext() & 1))))) {
                result = 3;
            }
        }
    }
 
    return result;
}


/**
 * Returns the index (0-3) of the player with the lowest value.
 */
s32 mpFindMinInt(s32 numplayers, s32 value0, s32 value1, s32 value2, s32 value3)
{
    s32 aux;
    s32 result;
 
    if ((value1 < value0) || ((value1 == value0 && ((randomGetNext() & 1))))) 
    {
        result = 1;
        aux = value1;
    }
    else 
    {
        result = 0;
        aux = value0;
    }
 
    if (numplayers >= 3) 
    {
        if ((value2 < aux) || ((value2 == aux && ((randomGetNext() & 1)))))
        {
            result = 2;
            aux = value2;
        }
 
        if (numplayers >= 4) 
        {
            if ((value3 < aux) || ((value3 == aux && ((randomGetNext() & 1))))) 
            {
                result = 3;
            }
        }
    }
 
    return result;
}


/**
 * Returns the index of the player with the highest value.
 * 
 * @bug: aux is s32 so each time it's set to one of the values, the decimal portion gets truncated.
 * The first comparison is safe from this since it doesn't use aux, but the rest are impacted.
 */
s32 mpFindMaxFloat(s32 numplayers, f32 value0, f32 value1, f32 value2, f32 value3)
{
    s32 aux;
    s32 result;
 
    if ((value0 < value1) || ((value1 == value0 && ((randomGetNext() & 1))))) 
    {
        aux = (s32) value1;
        result = 1;
    }
    else 
    {
        aux = (s32) value0;
        result = 0;
    }
 
    if (numplayers >= 3)
    {
        if ((aux < value2) || ((value2 == aux && ((randomGetNext() & 1))))) 
        {
            aux = (s32) value2;
            result = 2;
        }
 
        if (numplayers >= 4)
        {
            if ((aux < value3) || ((value3 == aux && ((randomGetNext() & 1))))) 
            {
                result = 3;
            }
        }
    }
 
    return result;
}


/**
 * Returns the index of the player with the lowest value.
 * 
 * Also suffers from the same bug as the above function.
 */
s32 mpFindMinFloat(s32 numplayers, f32 value0, f32 value1, f32 value2, f32 value3)
{
    s32 aux;
    s32 result;
 
    if ((value1 < value0) || ((value1 == value0 && ((randomGetNext() & 1))))) 
    {
        aux = (s32) value1;
        result = 1;
    }
    else 
    {
        aux = (s32) value0;
        result = 0;
    }
 
    if (numplayers >= 3)
    {
        if ((value2 < aux) || ((value2 == aux && ((randomGetNext() & 1))))) 
        {
            aux = (s32) value2;
            result = 2;
        }
 
        if (numplayers >= 4)
        {
            if ((value3 < aux) || ((value3 == aux && ((randomGetNext() & 1))))) 
            {
                result = 3;
            }
        }
    }
 
    return result;
}


void pauseAndLockControls(void) 
{
    lvlSetControlsLockedFlag(1);
    g_pausedFlag = TRUE;
}


bool disablePlayerActionsWhenPausedOrInMpMenu(void)
{
    if (getPlayerCount() == 1)
    {
        return TRUE;
    }

    if (g_stopPlayFlag)
    {
        return FALSE;
    }

    if (g_CurrentPlayer->mpmenuon)
    {
        return FALSE;
    }

    return TRUE;
}


void mpwatchSetStopPlayFlag(void)
{
    g_stopPlayFlag = TRUE;
}



void mpCalculateAwards(bool gameoverdelay)
{
    s32 player_count;
    s32 i;
    s32 j;
    s32 weapon_choice_2;
    s32 weapon_choice_1;
    s32 prev_player_num;
    s32 duration;

    struct AwardMetrics metrics[4];

    player_count = getPlayerCount();
    duration = getMissiontimer();

    sndDeactivateAllSfxByFlag_1();
    set_missionstate(MISSION_STATE_0);

    musicTrack1ApplySeqpVol(sub_GAME_7F0C0BF0());
    g_musicXTrack1Fade = 0;
    musicTrack1Play(M_INTROSWOOSH);

    pauseAndLockControls();

    if (gameoverdelay != 0)
    {
        g_gameOverFlag = (PAL ? 250 : 300);
    }
    else
    {
        g_gameOverFlag = 1;
    }

    alt_gameover_msg = 1;

    // Possible copy/paste error: these are the timer values for the chevron glow
    alt_gameover_msg_timer =  (PAL ? 16 : 20);

    chevron_glow = 0;
    chevron_glow_timer = 0;

    prev_player_num = get_cur_playernum();

    for (i = 0; i < player_count; i++)
    {

        set_cur_player(i);

        g_CurrentPlayer->mpmenuon = TRUE;
        g_CurrentPlayer->mpmenumode = MENU_SCORES;
        g_CurrentPlayer->ptr_text_first_mp_award = 0;
        g_CurrentPlayer->ptr_text_second_mp_award = 0;

        bondinvGetWeaponOfChoice(&weapon_choice_1, &weapon_choice_2);
        store_favorite_weapon_current_player((u32) weapon_choice_1, (u32) weapon_choice_2);

        metrics[i].num_shots = get_curplayer_shot_register(SHOT_REGISTER_TOTAL);
        metrics[i].num_headshots = get_curplayer_shot_register(SHOT_REGISTER_HEAD);
        metrics[i].num_kills = 0;
        metrics[i].num_deaths = 0;
        metrics[i].num_suicides = 0;

        for (j = 0; j < get_selected_num_players(); j++)
        {
            metrics[i].num_deaths += g_playerPlayerData[j].kill_counts[i];
            if (i == j)
            {
                metrics[i].num_suicides += g_playerPlayerData[i].kill_counts[j];
            }
            else
            {
                metrics[i].num_kills += g_playerPlayerData[i].kill_counts[j];
            }
        }

        metrics[i].ks_ratio = metrics[i].num_kills * 100.0f / (metrics[i].num_shots + 1.0f);
        metrics[i].kd_ratio = metrics[i].num_kills * 100.0f / (metrics[i].num_deaths + 1.0f);
        metrics[i].damage_to_backside = g_playerPlayerData[i].damage_to_backside;
        metrics[i].time_other_players_on_screen = g_playerPlayerData[i].time_other_players_on_screen;
        metrics[i].avg_km_per_hour = g_playerPlayerData[i].distance_traveled / 100000.0f / ((duration + 1) / (3600.0f * 60.0f));
        metrics[i].body_armor_pickups = g_playerPlayerData[i].body_armor_pickups;
        metrics[i].awards = 0;
        metrics[i].longest_inning = g_playerPlayerData[i].longest_inning;
        metrics[i].shortest_inning = g_playerPlayerData[i].shortest_inning;
    }

    set_cur_player(prev_player_num);

    // Choose which players are eligible for which awards
    i = mpFindMaxInt(player_count, metrics[0].num_suicides, metrics[1].num_suicides, metrics[2].num_suicides, metrics[3].num_suicides);

    if (metrics[i].num_suicides > 0)
    {
        metrics[i].awards |= AWARD_MOSTSUICIDAL;
    }

    i = mpFindMinInt(player_count, metrics[0].num_shots, metrics[1].num_shots, metrics[2].num_shots, metrics[3].num_shots);

    if (metrics[i].num_shots < 100)
    {
        metrics[i].awards |= AWARD_WHONEEDSAMMO;
    }

    i = mpFindMinFloat(player_count, metrics[0].body_armor_pickups, metrics[1].body_armor_pickups, metrics[2].body_armor_pickups, metrics[3].body_armor_pickups);

    if (metrics[i].body_armor_pickups <= 2.0f)
    {
        metrics[i].awards |= AWARD_WHERESTHEARMOUR;
    }

    i = mpFindMaxFloat(player_count, metrics[0].body_armor_pickups, metrics[1].body_armor_pickups, metrics[2].body_armor_pickups, metrics[3].body_armor_pickups);

    if (metrics[i].body_armor_pickups > 6.0f)
    {
        metrics[i].awards |= AWARD_ACNEGATIVE10;
    }

    i = mpFindMaxInt(player_count, metrics[0].num_headshots, metrics[1].num_headshots, metrics[2].num_headshots, metrics[3].num_headshots);

    if (metrics[i].num_headshots > 0)
    {
        metrics[i].awards |= AWARD_MARKSMANSHIP;
    }

    i = mpFindMaxFloat(player_count, metrics[0].ks_ratio, metrics[1].ks_ratio, metrics[2].ks_ratio, metrics[3].ks_ratio);

    if (metrics[i].ks_ratio > 0.0f)
    {
        metrics[i].awards |= AWARD_MOSTPROFESSIONAL;
    }

    i = mpFindMaxFloat(player_count, metrics[0].kd_ratio, metrics[1].kd_ratio, metrics[2].kd_ratio, metrics[3].kd_ratio);

    if (metrics[i].kd_ratio > 0.0f)
    {
        metrics[i].awards |= AWARD_MOSTDEADLY;
    }

    i = mpFindMinFloat(player_count, metrics[0].kd_ratio, metrics[1].kd_ratio, metrics[2].kd_ratio, metrics[3].kd_ratio);
    metrics[i].awards |= AWARD_MOSTHARMLESS;

    i = mpFindMinInt(player_count, metrics[0].time_other_players_on_screen, metrics[1].time_other_players_on_screen, metrics[2].time_other_players_on_screen, metrics[3].time_other_players_on_screen);
    metrics[i].awards |= AWARD_MOSTCOWARDLY;

    i = mpFindMaxFloat(player_count, metrics[0].avg_km_per_hour, metrics[1].avg_km_per_hour, metrics[2].avg_km_per_hour, metrics[3].avg_km_per_hour);

    if (metrics[i].avg_km_per_hour > 10.0f)
    {
        metrics[i].awards |= AWARD_MOSTFRANTIC;
    }

    i = mpFindMinInt(player_count, metrics[0].damage_to_backside, metrics[1].damage_to_backside, metrics[2].damage_to_backside, metrics[3].damage_to_backside);
    metrics[i].awards |= AWARD_MOSTHONORABLE;

    i = mpFindMaxInt(player_count, metrics[0].damage_to_backside, metrics[1].damage_to_backside, metrics[2].damage_to_backside, metrics[3].damage_to_backside);

    if (metrics[i].damage_to_backside > 0 && (metrics[i].awards & AWARD_MOSTHONORABLE) == 0)
    {
        metrics[i].awards |= AWARD_MOSTDISHONORABLE;
    }

    i = mpFindMaxInt(player_count, metrics[0].longest_inning, metrics[1].longest_inning, metrics[2].longest_inning, metrics[3].longest_inning);

    if (metrics[i].longest_inning > 0)
    {
        metrics[i].awards |= AWARD_LONGESTINNINGS;
    }

    i = mpFindMinInt(player_count, metrics[0].shortest_inning, metrics[1].shortest_inning, metrics[2].shortest_inning, metrics[3].shortest_inning);

    if (metrics[i].shortest_inning > 0)
    {
        metrics[i].awards |= AWARD_SHORTESTINNINGS;
    }

    for (i = 0; i < player_count; i++)
    {
        if (g_playerPlayerData[i].most_killed_one_time == 4)
        {
            metrics[i].awards |= AWARD_QUADKILL;
        }
        else if (g_playerPlayerData[i].most_killed_one_time == 3)
        {
            metrics[i].awards |= AWARD_TRIPLEKILL;
        }
        else if (g_playerPlayerData[i].most_killed_one_time == 2)
        {
            metrics[i].awards |= AWARD_DOUBLEKILL;
        }
    }

    // For each player, choose which two awards to actually give them.
    // Note that the first award checked is quad kill, but after that the awards
    // are checked randomly. So if a player has quad kill they'll definitely see
    // it on the endscreen, but this is not the case for triple kill or any
    // other awards.
    for (i = 0; i < player_count; i++)
    {
        s32 numdone = 0;
        s32 awardindex = 16;

        while (numdone == 0)
        {
            if (metrics[i].awards & (1 << awardindex))
            {
                metrics[i].awards &= ~(1 << awardindex);
                g_playerPointers[i]->ptr_text_first_mp_award = langGet(g_AwardNames[awardindex]);
                numdone = 1;
            }

            if (metrics[i].awards == 0)
            {
                numdone = 1;
            }

            awardindex = randomGetNext() % 17;
        }

        while (numdone < 2)
        {
            awardindex = randomGetNext() % 17;

            if (metrics[i].awards & (1 << awardindex))
            {
                metrics[i].awards &= ~(1 << awardindex);
                g_playerPointers[i]->ptr_text_second_mp_award = langGet(g_AwardNames[awardindex]);
                numdone = 2;
            }

            if (metrics[i].awards == 0)
            {
                numdone = 2;
            }
        }
    }
}

#ifdef GE_MODDED_CHEATS
/*
 * R21 campaign Co-Op terminal failure.
 *
 * The competitive award path does a substantial amount of unrelated work:
 * it kills flagged SFX, starts the multiplayer end jingle, calculates awards
 * and rewrites competitive statistics.  Campaign Co-Op only needs the normal
 * multiplayer GAME OVER watch state and its START TO EXIT lifecycle.
 *
 * The caller is responsible for waiting until all participating players have
 * completed their death sequence and for stopping ordinary stage music.
 */
void mpStartCoopGameOver(void)
{
    s32 i;
    s32 weapon_choice_1;
    s32 weapon_choice_2;

    if (g_gameOverFlag)
    {
        return;
    }

    /* R21 RC9: terminal campaign Game Over uses the same world freeze/controls
     * lock boundary as retail multiplayer.  Permanent explosions may continue
     * during the players' death sequences, but once terminal GAME OVER begins
     * the simulation must stop behind the report overlay. */
    musicStopSlot(-1);
    set_missionstate(MISSION_STATE_0);
    musicTrack1ApplySeqpVol(sub_GAME_7F0C0BF0());
    g_musicXTrack1Fade = 0;
    musicTrack1Play(M_INTROSWOOSH);
    pauseAndLockControls();
    g_gameOverFlag = 1;
    alt_gameover_msg = 1;
    alt_gameover_msg_timer = (PAL ? 16 : 20);
    chevron_glow = 0;
    chevron_glow_timer = 0;

    for (i = 0; i < getPlayerCount(); i++)
    {
        set_cur_player(i);
        g_CurrentPlayer->mpmenuon = TRUE;
        g_CurrentPlayer->mpmenumode = MENU_SCORES;
        g_CurrentPlayer->mpjoywascentre = TRUE;
        g_CurrentPlayer->ptr_text_first_mp_award = 0;
        g_CurrentPlayer->ptr_text_second_mp_award = 0;

        /* Preserve Weapon of Choice for the live terminal page and for the
         * later mission-report snapshot. */
        bondinvGetWeaponOfChoice(&weapon_choice_1, &weapon_choice_2);
        store_favorite_weapon_current_player((u32)weapon_choice_1, (u32)weapon_choice_2);
    }

    set_cur_player(0);
}
#endif


void mpwatchMenuTick(void)
{
    s32 player_num;
    s32 player_count;
    s32 x_centered;
    s32 menu_count;
    s32 i;

    player_num = get_cur_playernum();
    player_count = getPlayerCount();
    x_centered = joyGetStickXInRange(player_num, -2, 1);

    // The player in shuffled position 0 drives down g_gameOverFlag which is both a flag and a timer.
    if (!get_player_position_in_shuffled(player_num) && (g_gameOverFlag >= 2))
    {
        g_gameOverFlag -= speedgraphframes;

        if (g_gameOverFlag <= 0) 
        { 
            g_gameOverFlag = 1;
        }
    }

    if (player_count != 1)
    {
        // If a player has their pause menu up when they die and the game isn't over, turn their menu off. 
        if ((g_CurrentPlayer->bonddead) && (!g_gameOverFlag))
        {
            g_CurrentPlayer->mpmenuon = FALSE;
            g_CurrentPlayer->healthdisplaytime = 0;
            return;
        }

        if (g_gameOverFlag < 2)
        {
            if (get_player_position_in_shuffled(player_num) == 0)
            {
                chevron_glow_timer += speedgraphframes;
                alt_gameover_msg_timer += speedgraphframes;

                // Toggle chevron glow
                if (chevron_glow_timer >= (PAL ? 16 : 20))
                {
                    chevron_glow_timer -= (PAL ? 16 : 20);
                    chevron_glow = !chevron_glow;
                }

                // Flip between "GAME OVER" and "START TO EXIT" text.
                if (alt_gameover_msg_timer >= (PAL ? 100 : 120))
                {
                    alt_gameover_msg_timer -= (PAL ? 100: 120);
                    alt_gameover_msg = !alt_gameover_msg;
                }
            }

            if (g_playerPerm->most_killed_one_life < g_CurrentPlayer->kills_this_life)
            {
                g_playerPerm->most_killed_one_life = g_CurrentPlayer->kills_this_life;
            }

            if (g_playerPerm->longest_inning < (getMissiontimer() - g_CurrentPlayer->lifestarttime60))
            {
                g_playerPerm->longest_inning = getMissiontimer() - g_CurrentPlayer->lifestarttime60;
            }

            if (g_CurrentPlayer->mpmenuon != FALSE)
            {
#ifdef GE_MODDED_CHEATS
                if (mpwatchIsCampaignCoop() && g_gameOverFlag
                    && joyGetButtonsPressedThisFrame(player_num, A_BUTTON | START_BUTTON))
                {
                    mpwatchPlayBeep();
                    frontStoreCoopMissionReportStats();
                    bossSetLoadedStage(LEVELID_TITLE);
                    return;
                }
#endif
#ifdef GE_MODDED_CHEATS
                if (g_CurrentPlayer->mpmenumode == MENU_OBJECTIVES && mpwatchConfigHandleInput(player_num))
                {
                }
                else
#endif
                if (mpwatchIsPlayerPressingRight(player_num) && mpwatchMenuCanGoRight())
                {
                    mpwatchPlayBeep();
#ifdef GE_MODDED_CHEATS
                    mpwatchMenuGoRight();
#else
                    g_CurrentPlayer->mpmenumode++;
#endif
                }
                else if (mpwatchIsPlayerPressingLeft(player_num) && mpwatchMenuCanGoLeft())
                {
                    mpwatchPlayBeep();
#ifdef GE_MODDED_CHEATS
                    mpwatchMenuGoLeft();
#else
                    g_CurrentPlayer->mpmenumode--;
#endif
                }
                else if (mpwatchIsPlayerPressingRight(player_num) && (g_CurrentPlayer->mpmenumode == MENU_EXIT_CONFIRM))
                {
                    mpwatchPlayBeep();
                    g_CurrentPlayer->mpquitconfirm = 1;
                }
                else if (mpwatchIsPlayerPressingLeft(player_num) && (g_CurrentPlayer->mpmenumode == MENU_EXIT_CONFIRM))
                {
                    mpwatchPlayBeep();
                    g_CurrentPlayer->mpquitconfirm = 0;
                }
                else if (joyGetButtonsPressedThisFrame(player_num, A_BUTTON)
                    && (
#ifdef GE_MODDED_CHEATS
                        (mpwatchUseCoopPauseCarousel() && g_CurrentPlayer->mpmenumode == MENU_SCORES)
                        || (!mpwatchUseCoopPauseCarousel() && g_CurrentPlayer->mpmenumode == MENU_PAUSE)
#else
                        g_CurrentPlayer->mpmenumode == MENU_PAUSE
#endif
                    )
                )
                {
                    mpwatchPlayBeep();
                    if (!g_pausedFlag)
                    {
                        g_pausedFlag = 1;
                        who_paused = get_cur_playernum();
                        lvlSetControlsLockedFlag(1);
                    }
                    else if (get_cur_playernum() == who_paused)
                    {
                        g_pausedFlag = 0;
                        lvlSetControlsLockedFlag(0);
                    }
                }
                else if (g_CurrentPlayer->mpmenumode == MENU_FINISHED)
                {
                    if (joyGetButtonsPressedThisFrame(player_num, B_BUTTON))
                    {
                        mpwatchPlayBeep();
                        g_CurrentPlayer->mpmenuon = TRUE;
                        g_CurrentPlayer->mpmenumode = MENU_SCORES;
                    }
                }
                else if (((joyGetButtonsPressedThisFrame(player_num, A_BUTTON | START_BUTTON)) && ((((g_CurrentPlayer->mpmenumode != MENU_EXIT)) && (g_CurrentPlayer->mpmenumode != MENU_EXIT_CONFIRM)) || ((g_CurrentPlayer->mpmenumode == MENU_EXIT_CONFIRM) && (g_CurrentPlayer->mpquitconfirm != 1)))) || (joyGetButtonsPressedThisFrame(player_num, B_BUTTON)))
                {
                    mpwatchPlayBeep();

                    if (g_gameOverFlag)
                    {
                        menu_count = 0;
                        g_CurrentPlayer->mpmenumode = MENU_FINISHED;

                        for (i = 0; i < player_count; i++)
                        {
                            if (g_playerPointers[i]->mpmenumode == MENU_FINISHED)
                            {
                                menu_count++;
                            }
                        }

                        if (menu_count == player_count)
                        {
                            bossSetLoadedStage(LEVELID_TITLE);
                        }
                    }
                    else
                    {
                        g_CurrentPlayer->mpmenuon = FALSE;
                        g_CurrentPlayer->healthdisplaytime = (PAL ? 50 : 60);

                        if (get_cur_playernum() == who_paused)
                        {
                            g_pausedFlag = 0;
                            lvlSetControlsLockedFlag(0);
                        }
                    }
                }
                else if ((joyGetButtonsPressedThisFrame(player_num, A_BUTTON | START_BUTTON)) && (g_CurrentPlayer->mpmenumode == MENU_EXIT))
                {
                    mpwatchPlayBeep();
                    g_CurrentPlayer->mpmenumode = MENU_EXIT_CONFIRM;
                    g_CurrentPlayer->mpquitconfirm = 0;
                }
                else if (joyGetButtonsPressedThisFrame(player_num, A_BUTTON | START_BUTTON))
                {
                    if ((g_CurrentPlayer->mpmenumode == MENU_EXIT_CONFIRM) && (g_CurrentPlayer->mpquitconfirm == 1))
                    {
                        mpwatchPlayBeep();
                        g_CurrentPlayer->mpmenuon = FALSE;
                        g_CurrentPlayer->healthdisplaytime = 0;
                        mpCalculateAwards(FALSE);
                    }
                }

                if ((x_centered == 0) || (x_centered == -1))
                {
                    g_CurrentPlayer->mpjoywascentre = 1;
                    return;
                }

                g_CurrentPlayer->mpjoywascentre = 0;
                return;
            }

            if (joyGetButtonsPressedThisFrame(player_num, START_BUTTON))
            {
                mpwatchPlayBeep();
                g_CurrentPlayer->mpmenuon = TRUE;
                g_CurrentPlayer->mpmenumode = MENU_SCORES;
#ifdef GE_MODDED_CHEATS
                mpcfg_mode[player_num] = 0;
                mpcfg_row[player_num] = 0;
                mpcfg_modal[player_num] = 0;
                mpcfg_yready[player_num] = 1;
#endif
                g_CurrentPlayer->mpjoywascentre = 1;
                g_CurrentPlayer->apparenthealth = g_CurrentPlayer->bondhealth;
                g_CurrentPlayer->apparentarmour = g_CurrentPlayer->bondarmour;
            }
        }
    }
}


Gfx *display_text_for_playerdata_on_MP_menu(Gfx *gdl, s32 x, s32 y, s32 points, TEXTCOLORS text_color) {

    s32 textX;
    s32 textY;
    s32 textwidth;
    s32 textheight;
    s32 unused;
    u16 *text;
    s16 viX;
    s32 viY;

    sprintf(&text, "%d", points);

    textMeasure(&textheight, &textwidth, &text, ptrFontBankGothicChars, ptrFontBankGothic, 0);

    textX = x - (textwidth >> 1);
    textY = y;

    switch (text_color) 
    {
        case GREEN_NORMAL:
            viX = viGetX();
            viY = viGetY();
            gdl = textRender(gdl, &textX, &textY, &text, ptrFontBankGothicChars, ptrFontBankGothic, 0xFF00B0, viX, viY, 0, 0);
            break;

        case GREEN_HIGHLIGHT:
            viX = viGetX();
            viY = viGetY();
            gdl = textRenderOutlined(gdl, &textX, &textY, &text, ptrFontBankGothicChars, ptrFontBankGothic, 0xA0FFA0F0, 0x7000A0, viX, viY, 0, 0);
            break;

        case RED_NORMAL:
            viX = viGetX();
            viY = viGetY();
            gdl = textRender(gdl, &textX, &textY, &text, ptrFontBankGothicChars, ptrFontBankGothic, 0xFF4040B0, viX, viY, 0, 0);
            break;

        case RED_HIGHLIGHT:
            viX = viGetX();
            viY = viGetY();
            gdl = textRenderOutlined(gdl, &textX, &textY, &text, ptrFontBankGothicChars, ptrFontBankGothic, 0xFFA0A0F0, 0x700000A0, viX, viY, 0, 0);
            break;

        case BLUE_NORMAL:
            viX = viGetX();
            viY = viGetY();
            gdl = textRender(gdl, &textX, &textY, &text, ptrFontBankGothicChars, ptrFontBankGothic, 0x4040FFB0, viX, viY, 0, 0);
            break;

        case BLUE_HIGHLIGHT:
            viX = viGetX();
            viY = viGetY();
            gdl = textRenderOutlined(gdl, &textX, &textY, &text, ptrFontBankGothicChars, ptrFontBankGothic, 0xA0A0FFF0, 0x70A0, viX, viY, 0, 0);
            break;
    }

    return gdl;
}


//rodata
/*8005BC20*/
const char ascii_MP_watch_menu_BLANK[] = "";
const char ascii_MP_watch_menu_left_chevron[] = "<";
const char ascii_MP_watch_menu_right_chevron[] = ">";
const char ascii_pnum_KILLS[] = "%s%d %s";
const char ascii_pnum_LOSSES[] = "%s%d %s";


s32 get_points_for_mp_player(s32 playernum)
{
    s32 team_or_token;
    s32 player_count;
    s32 i;
    s32 j;
    s32 points;

    // The have_token_or_goldengun field is also treated as a team identifier.
    team_or_token = g_playerPlayerData[playernum].have_token_or_goldengun;

    player_count = getPlayerCount();

    points = 0;

    switch (get_scenario())
    {
        case SCENARIO_NORMAL:
        case SCENARIO_MWTGG:
        case SCENARIO_LTK:
            for (i = 0; i < player_count; i++)
            {
                if (i != playernum)
                {
                    points += g_playerPlayerData[playernum].kill_counts[i];
                }
                else
                {
                    points -= g_playerPlayerData[i].kill_counts[playernum];
                }
            }

            points += g_playerPlayerData[playernum].killed_gg_owner_count * (player_count - 2);
            break;

        case SCENARIO_YOLT:
            points = MAX_PLAYER_COUNT - g_playerPlayerData[playernum].order_out_in_yolt;
            break;

        case SCENARIO_TLD:
            points = g_playerPlayerData[playernum].flag_counter;
            break;
            
        case SCENARIO_2v2:
        case SCENARIO_3v1:
        case SCENARIO_2v1:
            for (i = 0; i < player_count; i++)
            {
                if (g_playerPlayerData[i].have_token_or_goldengun == team_or_token)
                {
                    for (j = 0; j < player_count; j++)
                    {
                        if (g_playerPlayerData[j].have_token_or_goldengun != team_or_token)
                        {
                            points += g_playerPlayerData[i].kill_counts[j];
                        }
                        else
                        {
                            points -= g_playerPlayerData[i].kill_counts[j];
                        }
                    }
                }
            }
            break;

        default:
            break;
    }

    return points;
}


void write_playerrank_to_buffer(char *buffer, s32 playernum)
{
    s32 scenario;
    s32 count;
    s32 scores[4];
    s32 players[4];
    s32 tmp;
    s32 i;
    s32 j;

    scenario = get_scenario();
    count = getPlayerCount();

    for (i = 0; i < count; i++)
    {
        scores[i] = get_points_for_mp_player(i);
        players[i] = i;
    }

    for (j = 0; j < count; j++)
    {
        for (i = 0; i < (count - 1); i++)
        {
            if (scores[i] < scores[i + 1])
            {
                tmp = scores[i + 1];
                scores[i + 1] = scores[i];
                scores[i] = tmp;
                tmp = players[i + 1];
                players[i + 1] = players[i];
                players[i] = tmp;
            }
        }

    }

    for (i = 0; i < count; i++)
    {
        if (playernum == players[i])
        {
            break;
        }
    }

    for (j = 0; j <= i; j++)
    {
        if (scores[j] == scores[i])
        {
            break;
        }
    }

    switch (j)
    {
        case 0:
            sprintf(buffer, langGet(getStringID(LMPMENU, MPMENU_STR_11_RANK1ST))); /* Rank: 1st */
            break;
        case 1:
            sprintf(buffer, langGet(getStringID(LMPMENU, MPMENU_STR_12_RANK2ND))); /* Rank: 2nd */
            break;
        case 2:
            if ((scenario != SCENARIO_2v2) && (scenario != SCENARIO_2v1))
            {
                sprintf(buffer, langGet(getStringID(LMPMENU, MPMENU_STR_13_RANK3RD))); /* Rank: 3rd */
            }
            else
            {
                sprintf(buffer, langGet(getStringID(LMPMENU, MPMENU_STR_12_RANK2ND))); /* Rank: 2nd */
            }
            break;
        case 3:
            if (scenario != SCENARIO_3v1)
            {
                sprintf(buffer, langGet(getStringID(LMPMENU, MPMENU_STR_14_RANK4TH))); /* Rank: 4th */
            }
            else
            {
                sprintf(buffer, langGet(getStringID(LMPMENU, MPMENU_STR_12_RANK2ND))); /* Rank: 2nd */
            }
            break;
    }

}


s32 mpwatchShouldDisplayRank(s32 param_1)
{
    switch(get_scenario())
    {
        case SCENARIO_NORMAL:
        case SCENARIO_TLD:
        case SCENARIO_MWTGG:
        case SCENARIO_LTK:
        case SCENARIO_2v2:
        case SCENARIO_3v1:
        case SCENARIO_2v1:
            return 1;
#ifdef GE_MODDED_CHEATS
        case SCENARIO_COOP:
            return 0;
#endif
        case SCENARIO_YOLT:
            return param_1 ? 0 : 1;
        default:
#ifdef DEBUG
            osSyncPrintf("Invalid scenario %d!", get_scenario());
#endif
        do {} while (1);
    }
}

s32 mpwatchShouldDisplayScore(s32 param_1)
{
    switch(get_scenario())
    {
        case SCENARIO_NORMAL:
        case SCENARIO_MWTGG:
        case SCENARIO_LTK:
        case SCENARIO_2v2:
        case SCENARIO_3v1:
        case SCENARIO_2v1:
            return 1;
        break;
        case SCENARIO_YOLT:
        case SCENARIO_TLD:
            return 0;
        break;
        default:
#ifdef DEBUG
            osSyncPrintf("Invalid scenario %d!", get_scenario());
#endif
            do {} while (1);
    }
}


/**
 * Draws the text for the multiplayer pause menu, post-game screens, and the "press start" prompt shown after death.
 * 
 * When the match is running it works through a horizontal carousel of pages starting at the "Score" page.
 * Losses << Kills << Score >> Pause >> Exit
 * 
 * During campaign Co-Op R21 replaces the competitive score/pause cards:
 * Deaths << Kills << Objectives/Pause << Exit
 *
 * When the game is over it displays these screens:
 * WOC/Awards << Losses << Kills << Score
 *
 * With the menu closed, nothing is drawn unless the player is dead and their
 * death animation has finished, in which case the continue prompt is centred
 * in the viewport. That's suppressed in YOLT once two of the player's lives are
 * gone since there is nothing to continue to.
 *
 * @param gdl display list to append to
 * @return the advanced display list pointer
 */
Gfx *mp_watch_menu_display(Gfx *gdl)
{
    s32 curplayernum;
    s32 player_count;
    s32 x;
    s32 y;
    s32 k;
    s32 textwidth;
    s32 textheight;
    s32 m;
    s32 h1;
    s32 h2;
    char rankbuffer[4];
    s32 two_player_x_offset;
    char *text;
    s32 scores[4];
    s32 i;
    TEXTCOLORS current_colour;
    TEXTCOLORS same_team_colour;
    TEXTCOLORS other_team_colour;
    MPSCENARIOS scenario;
    s32 fav_textheight;
    s32 fav_textwidth;
    s32 fav_x_offset;
    s32 x3;
    s32 y3;
    s32 textwidth3;
    s32 textheight3;
    char *text3;
    s32 self_paused;
    s32 total_kills_against_current;
    s16 x2;
    s16 viewleft;
    s32 colour;
    s32 q;
    s32 can_go_left;
    s32 can_go_right;
#ifdef GE_MODDED_CHEATS
    char objectivebuffer[32];
    const char *objectivestatustext;
    s32 objectiveindex;
    s32 visibleobjective;
    OBJECTIVESTATUS objectivestatus;
#endif
 
    curplayernum = get_cur_playernum();
    player_count = getPlayerCount();
    self_paused = 0;
 
    if (player_count == 1)
    {
        return gdl;
    }
 
    if (g_CurrentPlayer->mpmenuon)
    {
        gdl = microcode_constructor(gdl);
    
        if (player_count == 2)
        {
            two_player_x_offset = 80;
        }
        else
        {
            two_player_x_offset = 0;
        }

        switch (g_CurrentPlayer->mpmenumode)
        {
            case MENU_GOWOC:
            case MENU_LOSSES:
            case MENU_KILLS:
            case MENU_SCORES:
#ifdef GE_MODDED_CHEATS
                if (mpwatchUseCoopPauseCarousel() && g_CurrentPlayer->mpmenumode == MENU_SCORES)
                {
                    /* R21 RC4: getStringID does not parenthesize TEXTSLOT.
                     * Parenthesize this ternary explicitly so LMPMENU bank bits
                     * are always retained (DUMP(7): langGet(0x18) -> badvaddr 0x60). */
                    text = (char *) langGet(getStringID(LMPMENU,
                        (g_pausedFlag ? MPMENU_STR_18_PAUSED : MPMENU_STR_19_PAUSE)));
                    if (g_pausedFlag && get_cur_playernum() == who_paused)
                    {
                        self_paused = 1;
                    }
                    break;
                }
#endif
                if (!g_gameOverFlag)
                {
                    text = (char *) langGet(getStringID(LMPMENU, MPMENU_STR_15_PLAY)); /* PLAY */
                }
                else
                {
                    /* Keep GAME OVER visible continuously. alt_gameover_msg now
                     * only selects its two-state opacity, using the same timer
                     * cadence as the old GAME OVER/START TO EXIT alternation. */
                    text = (char *) langGet(getStringID(LMPMENU, MPMENU_STR_16_GAMEOVER)); /* GAME OVER */
                }
                break;
#ifdef GE_MODDED_CHEATS
            case MENU_OBJECTIVES:
                /* Player-settings hub/submenus own this inserted watch card.
                 * Use the stock watch header path so there is only one title. */
                text = mpcfg_mode[curplayernum] == 2 ? "SPECIAL"
                    : mpcfg_mode[curplayernum] == 3 ? "CHEATS" : "OPTIONS";
                break;
#endif
            case MENU_FINISHED:
                text = (char *) ascii_MP_watch_menu_BLANK;
                break;
            case MENU_PAUSE:
                if (g_pausedFlag)
                {
                    text = (char *) langGet(getStringID(LMPMENU, MPMENU_STR_18_PAUSED)); /* PAUSED */
                    if (get_cur_playernum() == who_paused)
                    {
                        self_paused = 1;
                    }
                }
                else
                {
                    text = (char *) langGet(getStringID(LMPMENU, MPMENU_STR_19_PAUSE)); /* PAUSE */
                }
                break;
            case MENU_EXIT:
            case MENU_EXIT_CONFIRM:
                text = (char *) langGet(getStringID(LMPMENU, MPMENU_STR_1A_EXIT)); /* EXIT */
                x = (viGetViewLeft() + two_player_x_offset) + 65;
                break;
        }
 
        textMeasure(&textheight, &textwidth, text, ptrFontBankGothicChars, ptrFontBankGothic, 0);
        x = ((viGetViewLeft() + two_player_x_offset) - (textwidth >> 1)) + 80;
        y = (viGetViewTop() - (textheight >> 1)) + (22 + MPMENU_YOFF);
 
        if (self_paused)
        {
            viewleft = viGetX(); 
            h1 = viGetY();
            gdl = textRenderOutlined(gdl, &x, &y, text, ptrFontBankGothicChars, ptrFontBankGothic, 0xa0ffa0f0, 0x007000a0, viewleft, h1, 0, 0);
        }
        else
        {
            viewleft = viGetX(); 
            h1 = viGetY();
            colour = 0x00ff00b0;
            if (g_gameOverFlag && !alt_gameover_msg)
            {
                /* Instant two-state dimming: no fade/pulse and no text swap. */
                colour = 0x00ff0070;
            }
            gdl = textRender(gdl, &x, &y, text, ptrFontBankGothicChars, ptrFontBankGothic, colour, viewleft, h1, 0, 0);
        }
 
        if (g_CurrentPlayer->mpmenumode != MENU_EXIT_CONFIRM
            && g_CurrentPlayer->mpmenumode != MENU_FINISHED)
        {
            can_go_left = mpwatchMenuCanGoLeft();
            viewleft = viGetViewLeft();
    
            /**
             * Colour is reused to store a pixel offset here.
             */
            colour = g_gameOverFlag ? 10 : 0, x = ((viewleft + two_player_x_offset) - colour) + 40;

            if (g_gameOverFlag)
            {
                x -= 8;
            }

            y = viGetViewTop() + (22 + MPMENU_YOFF);
            viewleft = viGetX();
            h1 = viGetY();

            if (!can_go_left || !chevron_glow)
            {
                /* At the left edge the chevron remains visible but stops blinking. */
                gdl = textRender(gdl, &x, &y, (char *) ascii_MP_watch_menu_left_chevron, ptrFontBankGothicChars, ptrFontBankGothic, 0x00ff00b0, viewleft, h1, 0, 0);
            }
            else
            {
                gdl = textRenderOutlined(gdl, &x, &y, (char *) ascii_MP_watch_menu_left_chevron, ptrFontBankGothicChars, ptrFontBankGothic, 0xa0ffa0f0, 0x007000a0, viewleft, h1, 0, 0);
            }

            {
                can_go_right = mpwatchMenuCanGoRight();
                viewleft = viGetViewLeft();
                colour = g_gameOverFlag ? 10 : 0, x = ((colour + 112) + viewleft) + two_player_x_offset;

                if (g_gameOverFlag)
                {
                    x += 8;
                }

                y = viGetViewTop() + (22 + MPMENU_YOFF);
                viewleft = viGetX();
                h1 = viGetY();

                if (!can_go_right || !chevron_glow)
                {
                    /* At the right edge the chevron remains visible but stops blinking. */
                    gdl = textRender(gdl, &x, &y, (char *) ascii_MP_watch_menu_right_chevron, ptrFontBankGothicChars, ptrFontBankGothic, 0x00ff00b0, viewleft, h1, 0, 0);
                }
                else
                {
                    gdl = textRenderOutlined(gdl, &x, &y, (char *) ascii_MP_watch_menu_right_chevron, ptrFontBankGothicChars, ptrFontBankGothic, 0xa0ffa0f0, 0x007000a0, viewleft, h1, 0, 0);
                }
            }
        }

        if ((g_CurrentPlayer->mpmenumode == MENU_SCORES
#ifdef GE_MODDED_CHEATS
            && !mpwatchIsCampaignCoop()
#endif
            ) || (g_CurrentPlayer->mpmenumode == MENU_PAUSE))
        {
            if (player_count > 0)
            {
                i = 0;
 
                do
                {
                    scores[i] = get_points_for_mp_player(i);
                    i++;
                }
                while (i != player_count);
            }
 
            fav_x_offset = (g_gameOverFlag == 0); 
 
            if (fav_x_offset) 
            { 
                fav_x_offset = (g_stopPlayFlag == 0); 
            }
 
            if (mpwatchShouldDisplayRank(fav_x_offset))
            {
                write_playerrank_to_buffer(rankbuffer, curplayernum);
                textMeasure(&textheight, &textwidth, rankbuffer, ptrFontBankGothicChars, ptrFontBankGothic, 0);
 
                x = ((viGetViewLeft() + two_player_x_offset) - (textwidth >> 1)) + 80;
                y = (viGetViewTop() - (textheight >> 1)) + (37 + MPMENU_YOFF);
                viewleft = viGetX(); 
                h1 = viGetY();
                gdl = textRender(gdl, &x, &y, rankbuffer, ptrFontBankGothicChars, ptrFontBankGothic, 0x00ff00b0, viewleft, h1, 0, 0);
            }

            fav_x_offset = (g_gameOverFlag == 0); 
 
            if (fav_x_offset) 
            { 
                fav_x_offset = (g_stopPlayFlag == 0); 
            }
 
            if (mpwatchShouldDisplayScore(fav_x_offset))
            {
                scenario = get_scenario();
                text = (char *) langGet(getStringID(LMPMENU, MPMENU_STR_1B_SCORES)); /* SCORES */
                textMeasure(&textheight, &textwidth, text, ptrFontBankGothicChars, ptrFontBankGothic, 0);
                x = ((viGetViewLeft() + two_player_x_offset) - (textwidth >> 1)) + 80;
                /* Keep the normal multiplayer SCORES heading aligned a little
                 * higher, matching the visual placement of Kills/Losses. */
                y = (viGetViewTop() - (textheight >> 1)) + (50 + MPMENU_YOFF)
                    + (g_gameOverFlag && !mpwatchIsCampaignCoop());
                viewleft = viGetX(); 
                h1 = viGetY();
                gdl = textRender(gdl, &x, &y, text, ptrFontBankGothicChars, ptrFontBankGothic, 0x00ff00b0, viewleft, h1, 0, 0);
 
                if (((((scenario == SCENARIO_2v2) || (scenario == SCENARIO_3v1)) || (scenario == SCENARIO_2v1)) || (scenario == SCENARIO_TLD)) || (scenario == SCENARIO_MWTGG))
                {
                    if (g_playerPlayerData[curplayernum].have_token_or_goldengun == 0)
                    {
                        current_colour = RED_HIGHLIGHT;
                        same_team_colour = RED_NORMAL;
                        other_team_colour = BLUE_NORMAL;
                    }
                    else
                    {
                        current_colour = BLUE_HIGHLIGHT;
                        same_team_colour = BLUE_NORMAL;
                        other_team_colour = RED_NORMAL;
                    }
                }
                else
                {
                    current_colour = GREEN_HIGHLIGHT;
                    same_team_colour = GREEN_NORMAL;
                    other_team_colour = GREEN_NORMAL;
                }
 
                if (player_count == 2)
                {
                    viewleft = viGetViewLeft();
                    x2 = viGetViewTop();

                    if (curplayernum == 0)
                    {
                        colour = current_colour;
                    }
                    else
                    {
                        q = g_playerPlayerData[0].have_token_or_goldengun == g_playerPlayerData[curplayernum].have_token_or_goldengun ? same_team_colour : other_team_colour;
                        colour = q;
                    }
 
                    gdl = display_text_for_playerdata_on_MP_menu(gdl, (viewleft + two_player_x_offset) + 80, x2 + (70 + MPMENU_YOFF) - ((g_gameOverFlag && !mpwatchIsCampaignCoop()) ? 2 : 0), scores[0], colour);
                    viewleft = viGetViewLeft();
                    x2 = viGetViewTop();

                    curplayernum == 1 ? (colour = current_colour) : (q = g_playerPlayerData[1].have_token_or_goldengun == g_playerPlayerData[curplayernum].have_token_or_goldengun ? same_team_colour : other_team_colour, colour = q);

                    gdl = display_text_for_playerdata_on_MP_menu(gdl, (viewleft + two_player_x_offset) + 80, x2 + (86 + MPMENU_YOFF) - ((g_gameOverFlag && !mpwatchIsCampaignCoop()) ? 2 : 0), scores[1], colour);
                }
                else
                {
                    viewleft = viGetViewLeft();
                    x2 = viGetViewTop();
 
                    if (curplayernum == 0)
                    {
                        colour = current_colour;
                    }
                    else
                    {
                        q = g_playerPlayerData[0].have_token_or_goldengun == g_playerPlayerData[curplayernum].have_token_or_goldengun ? same_team_colour : other_team_colour;
                        colour = q;
                    }
 
                    gdl = display_text_for_playerdata_on_MP_menu(gdl, (viewleft + two_player_x_offset) + 64, x2 + (70 + MPMENU_YOFF) - ((g_gameOverFlag && !mpwatchIsCampaignCoop()) ? 2 : 0), scores[0], colour);
                    viewleft = viGetViewLeft();
                    x2 = viGetViewTop();
 
                    if (curplayernum == 1)
                    {
                        colour = current_colour;
                    }
                    else
                    {
                        q = g_playerPlayerData[1].have_token_or_goldengun == g_playerPlayerData[curplayernum].have_token_or_goldengun ? same_team_colour : other_team_colour;
                        colour = q;
                    }
 
                    gdl = display_text_for_playerdata_on_MP_menu(gdl, (viewleft + two_player_x_offset) + 96, x2 + (70 + MPMENU_YOFF) - ((g_gameOverFlag && !mpwatchIsCampaignCoop()) ? 2 : 0), scores[1], colour);
                    viewleft = viGetViewLeft();
                    x2 = viGetViewTop();
 
                    if (curplayernum == 2)
                    {
                        colour = current_colour;
                    }
                    else
                    {
                        q = g_playerPlayerData[2].have_token_or_goldengun == g_playerPlayerData[curplayernum].have_token_or_goldengun ? same_team_colour : other_team_colour;
                        colour = q;
                    }
 
                    gdl = display_text_for_playerdata_on_MP_menu(gdl, (viewleft + two_player_x_offset) + 64, x2 + (86 + MPMENU_YOFF) - ((g_gameOverFlag && !mpwatchIsCampaignCoop()) ? 2 : 0), scores[2], colour);
 
                    if (player_count == 4)
                    {
                        viewleft = viGetViewLeft();
                        x2 = viGetViewTop();

                        curplayernum == 3 ? (colour = current_colour) : (q = g_playerPlayerData[3].have_token_or_goldengun == g_playerPlayerData[curplayernum].have_token_or_goldengun ? same_team_colour : other_team_colour, colour = q);
                        gdl = display_text_for_playerdata_on_MP_menu(gdl, (viewleft + two_player_x_offset) + 96, x2 + (86 + MPMENU_YOFF) - ((g_gameOverFlag && !mpwatchIsCampaignCoop()) ? 2 : 0), scores[3], colour);
                    }
                }
            }

            // Keep colour live through this branch to reproduce the original register allocation.
            if (colour);
        }
#ifdef GE_MODDED_CHEATS
        else if (mpwatchUseCoopPauseCarousel() && g_CurrentPlayer->mpmenumode == MENU_SCORES)
        {
            /* R21 combined Objectives/Pause card.  Objective state is the
             * mission-global table, shared by all Co-Op players. */
            text = "OBJECTIVES";
            textMeasure(&textheight, &textwidth, text, ptrFontBankGothicChars, ptrFontBankGothic, 0);
            x = ((viGetViewLeft() + two_player_x_offset) - (textwidth >> 1)) + 80;
            y = (viGetViewTop() - (textheight >> 1)) + (37 + MPMENU_YOFF);
            viewleft = viGetX();
            h1 = viGetY();
            gdl = textRender(gdl, &x, &y, text, ptrFontBankGothicChars, ptrFontBankGothic,
                    0x00ff00b0, viewleft, h1, 0, 0);

            visibleobjective = 0;

            for (objectiveindex = 0; objectiveindex < objectiveGetCount(); objectiveindex++)
            {
                if (get_difficulty_for_objective(objectiveindex) <= lvlGetSelectedDifficulty())
                {
                    objectivestatus = get_status_of_objective(objectiveindex);

                    switch (objectivestatus)
                    {
                        case OBJECTIVESTATUS_COMPLETE:
                            objectivestatustext = "Complete";
                            colour = 0xa0ffa0f0;
                            break;
                        case OBJECTIVESTATUS_FAILED:
                            objectivestatustext = "Failed";
                            colour = 0xff4040d0;
                            break;
                        case OBJECTIVESTATUS_INCOMPLETE:
                        default:
                            objectivestatustext = "Incomplete";
                            colour = 0x00ff00b0;
                            break;
                    }

                    sprintf(objectivebuffer, "Obj. %c: %s", 'A' + visibleobjective, objectivestatustext);
                    textMeasure(&textheight, &textwidth, objectivebuffer, ptrFontBankGothicChars, ptrFontBankGothic, 0);
                    x = ((viGetViewLeft() + two_player_x_offset) - (textwidth >> 1)) + 80;
                    y = viGetViewTop() + (51 + MPMENU_YOFF) + visibleobjective * 7;
                    viewleft = viGetX();
                    h1 = viGetY();
                    gdl = textRender(gdl, &x, &y, objectivebuffer, ptrFontBankGothicChars, ptrFontBankGothic,
                            colour, viewleft, h1, 0, 0);
                    visibleobjective++;
                }
            }

            if (visibleobjective == 0)
            {
                text = "No objectives";
                textMeasure(&textheight, &textwidth, text, ptrFontBankGothicChars, ptrFontBankGothic, 0);
                x = ((viGetViewLeft() + two_player_x_offset) - (textwidth >> 1)) + 80;
                y = viGetViewTop() + (55 + MPMENU_YOFF);
                viewleft = viGetX();
                h1 = viGetY();
                gdl = textRender(gdl, &x, &y, text, ptrFontBankGothicChars, ptrFontBankGothic,
                        0x00ff00b0, viewleft, h1, 0, 0);
            }

            if (g_gameOverFlag)
            {
                /* RC6: terminal Co-Op keeps final objectives and Weapon of
                 * Choice together on the GAME OVER page. */
                text = (char *)langGet(getStringID(LMPMENU, MPMENU_STR_1F_WEAPONOFCHOICE));
                textMeasure(&textheight, &textwidth, text, ptrFontBankGothicChars, ptrFontBankGothic, 0);
                x = ((viGetViewLeft() + two_player_x_offset) - (textwidth >> 1)) + 80;
                y = viGetViewTop() + (87 + MPMENU_YOFF);
                viewleft = viGetX(); h1 = viGetY();
                gdl = textRender(gdl, &x, &y, text, ptrFontBankGothicChars, ptrFontBankGothic,
                        0x00ff00b0, viewleft, h1, 0, 0);

                text = frontGetPlayersFavoriteWeaponInHand(curplayernum, 0);
                textMeasure(&textheight, &textwidth, text, ptrFontBankGothicChars, ptrFontBankGothic, 0);
                x = ((viGetViewLeft() + two_player_x_offset) - (textwidth >> 1)) + 80;
                y = viGetViewTop() + (96 + MPMENU_YOFF);
                gdl = textRender(gdl, &x, &y, text, ptrFontBankGothicChars, ptrFontBankGothic,
                        0x00ff00b0, viGetX(), viGetY(), 0, 0);
            }
            /* Multiplayer Co-Op pause watch intentionally has no A: PAUSE /
             * A: UNPAUSE prompt.  The watch itself already communicates the
             * paused state and the extra footer crowded the viewport. */
        }
#endif
#ifdef GE_MODDED_CHEATS
        else if (g_CurrentPlayer->mpmenumode == MENU_OBJECTIVES)
        {
            s32 mode = mpcfg_mode[curplayernum];
            s32 row = mpcfg_row[curplayernum];
            s32 value = 0;
            char *label;
            char *vtext;
            CHEAT_ID cheat;

            if (mode == 0)
            {
                static char *hub[3] = {"Options","Special Options","In-Game Cheats"};
                for (i=0;i<3;i++)
                {
                    text=hub[i]; textMeasure(&textheight,&textwidth,text,ptrFontBankGothicChars,ptrFontBankGothic,0);
                    x=((viGetViewLeft()+two_player_x_offset)-(textwidth>>1))+80; y=viGetViewTop()+48+i*16+MPMENU_YOFF;
                    gdl=textRender(gdl,&x,&y,text,ptrFontBankGothicChars,ptrFontBankGothic,i==row?0xa0ffa0f0:0x00ff00b0,viGetX(),viGetY(),0,0);
                }
            }
            else
            {
                if (mode == 1)
                {
                    static const u8 optionindex[6]={PLAYER_OPTION_LOOK,PLAYER_OPTION_AUTOAIM,PLAYER_OPTION_AIM,PLAYER_OPTION_SIGHT,PLAYER_OPTION_LOOKAHEAD,PLAYER_OPTION_AMMODISPLAY};
                    value=mpwatchConfigOptionBit(curplayernum,row);
                    if (row == 0) { label=(char *)langGet(getStringID(LOPTIONS, OPTION_STR_32_CONTROLSTYLE_LF)); vtext=frontModGetOptionLabel(21+value); }
                    else if (row == 7) { label=frontModGetOptionLabel(8); vtext=(char *)langGet(game_options_entries[PLAYER_OPTION_AUTOAIM].text[value+1]); }
                    else { label=(char *)langGet(game_options_entries[optionindex[row-1]].text[0]); vtext=(char *)langGet(game_options_entries[optionindex[row-1]].text[value+1]); }
                }
                else if (mode == 2)
                {
                    if (row < 4) label=frontModGetOptionLabel(12+row);
                    else if (row == 4) label=frontModGetOptionLabel(30);
                    else if (row == 5) label=frontModGetOptionLabel(31);
                    else if (row == 6) label=frontModGetOptionLabel(16);
                    else if (row == 7) label=frontModGetOptionLabel(8);
                    else label=frontModGetOptionLabel(29);
                    value=mpwatchConfigSpecialValue(curplayernum,row);
                    vtext=(char *)langGet(game_options_entries[PLAYER_OPTION_AUTOAIM].text[value+1]);
                }
                else
                {
                    cheat=(CHEAT_ID)(row+1); label=(char *)frontModGetCheatMenuText(cheat);
                    if (mpwatchCheatUnlocked(cheat)) vtext=(char *)langGet(game_options_entries[PLAYER_OPTION_AUTOAIM].text[cheatIsActive(cheat)+1]); else vtext="Locked";
                }

                textMeasure(&textheight,&textwidth,label,ptrFontBankGothicChars,ptrFontBankGothic,0);
                x=((viGetViewLeft()+two_player_x_offset)-(textwidth>>1))+80; y=viGetViewTop()+57+MPMENU_YOFF;
                gdl=textRender(gdl,&x,&y,label,ptrFontBankGothicChars,ptrFontBankGothic,0xa0ffa0f0,viGetX(),viGetY(),0,0);
                textMeasure(&textheight,&textwidth,vtext,ptrFontBankGothicChars,ptrFontBankGothic,0);
                x=((viGetViewLeft()+two_player_x_offset)-(textwidth>>1))+80; y=viGetViewTop()+75+MPMENU_YOFF;
                gdl=textRender(gdl,&x,&y,vtext,ptrFontBankGothicChars,ptrFontBankGothic,0x00ff00b0,viGetX(),viGetY(),0,0);

                if (mode == 1 && mpcfg_modal[curplayernum])
                {
                    if (row == 0)
                    {
                        text=frontModGetOptionLabel(21+(mpcfg_choice[curplayernum]&3));
                        textMeasure(&textheight,&textwidth,text,ptrFontBankGothicChars,ptrFontBankGothic,0);
                        x=((viGetViewLeft()+two_player_x_offset)-(textwidth>>1))+80; y=viGetViewTop()+94+MPMENU_YOFF;
                        gdl=textRender(gdl,&x,&y,text,ptrFontBankGothicChars,ptrFontBankGothic,0xa0ffa0f0,viGetX(),viGetY(),0,0);
                    }
                    else
                    {
                        s32 opt = row == 7 ? PLAYER_OPTION_AUTOAIM : row - 1;
                        char *a=(char *)langGet(game_options_entries[opt].text[1]); char *b=(char *)langGet(game_options_entries[opt].text[2]);
                        text=a; textMeasure(&textheight,&textwidth,text,ptrFontBankGothicChars,ptrFontBankGothic,0); x=viGetViewLeft()+two_player_x_offset+48-(textwidth>>1); y=viGetViewTop()+94+MPMENU_YOFF;
                        gdl=textRender(gdl,&x,&y,text,ptrFontBankGothicChars,ptrFontBankGothic,mpcfg_choice[curplayernum]?0x00ff00b0:0xa0ffa0f0,viGetX(),viGetY(),0,0);
                        text=b; textMeasure(&textheight,&textwidth,text,ptrFontBankGothicChars,ptrFontBankGothic,0); x=viGetViewLeft()+two_player_x_offset+112-(textwidth>>1); y=viGetViewTop()+94+MPMENU_YOFF;
                        gdl=textRender(gdl,&x,&y,text,ptrFontBankGothicChars,ptrFontBankGothic,mpcfg_choice[curplayernum]?0xa0ffa0f0:0x00ff00b0,viGetX(),viGetY(),0,0);
                    }
                }
            }
        }
#endif
        else if (g_CurrentPlayer->mpmenumode == MENU_KILLS)
        {
            fav_x_offset = (g_gameOverFlag == 0); 

            if (fav_x_offset) 
            { 
                fav_x_offset = (g_stopPlayFlag == 0); 
            } 

            if (mpwatchShouldDisplayRank(fav_x_offset))
            {
                write_playerrank_to_buffer(rankbuffer, curplayernum);
                textMeasure(&textheight, &textwidth, rankbuffer, ptrFontBankGothicChars, ptrFontBankGothic, 0);
                x = ((viGetViewLeft() + two_player_x_offset) - (textwidth >> 1)) + 80;
                y = (viGetViewTop() - (textheight >> 1)) + (37 + MPMENU_YOFF);
                viewleft = viGetX(); h1 = viGetY();
                gdl = textRender(gdl, &x, &y, rankbuffer, ptrFontBankGothicChars, ptrFontBankGothic, 0x00ff00b0, viewleft, h1, 0, 0);
            }
 
#ifdef GE_MODDED_CHEATS
            if (mpwatchIsCampaignCoop())
            {
                text = (char *) langGet(getStringID(LMPMENU, MPMENU_STR_1D_KILLS));
            }
            else
#endif
            {
                q = (s32) langGet(getStringID(LMPMENU, MPMENU_STR_1C_P)); /* P */
                h2 = (s32) langGet(getStringID(LMPMENU, MPMENU_STR_1D_KILLS)); /* KILLS */
                sprintf(rankbuffer, ascii_pnum_KILLS, (char *) q, curplayernum + 1, (char *) h2);
                text = rankbuffer;
            }
 
            textMeasure(&textheight, &textwidth, text, ptrFontBankGothicChars, ptrFontBankGothic, 0);
            x = ((viGetViewLeft() + two_player_x_offset) - (textwidth >> 1)) + 80;
            y = (viGetViewTop() - (textheight >> 1)) + (49 + MPMENU_YOFF)
                + ((!g_gameOverFlag && !mpwatchIsCampaignCoop()) ? 1 : 0)
                + ((g_gameOverFlag && !mpwatchIsCampaignCoop()) ? 2 : 0);
            viewleft = viGetX(); 
            h1 = viGetY();
            gdl = textRender(gdl, &x, &y, text, ptrFontBankGothicChars, ptrFontBankGothic, 0x00ff00b0, viewleft, h1, 0, 0);
 
            /* RC5: always render all four player slots in campaign Co-Op so
             * the owner's slot is not an unexplained blank.  The owner's cell
             * is outlined to make it obvious whose watch is open.  Keep the
             * retail competitive behaviour (self slot omitted) unchanged. */
            for (i = 0; i < player_count; i++)
            {
#ifdef GE_MODDED_CHEATS
                if (mpwatchIsCampaignCoop() || curplayernum != i)
#else
                if (curplayernum != i)
#endif
                {
                    viewleft = viGetViewLeft();
                    h1 = viGetViewTop();
                    x2 = player_count == 2 ? 80 : 64 + ((i & 1) << 5);
                    y = 70 + ((player_count == 2 ? i : i >> 1) << 4) + MPMENU_YOFF;
#ifdef GE_MODDED_CHEATS
                    /* Keep Co-Op KILLS values on the same vertical grid as
                     * normal multiplayer Game Over values.  Co-Op pause was
                     * already two pixels higher; Game Over now matches it. */
                    if (g_gameOverFlag || mpwatchIsCampaignCoop()) y -= 2;
                    current_colour = mpwatchIsCampaignCoop() && i == curplayernum ? GREEN_HIGHLIGHT : GREEN_NORMAL;
#else
                    current_colour = GREEN_NORMAL;
#endif
                    gdl = display_text_for_playerdata_on_MP_menu(gdl,
                            (viewleft + two_player_x_offset) + x2, h1 + y,
#ifdef GE_MODDED_CHEATS
                            mpwatchIsCampaignCoop() ? g_playerPlayerData[i].kill_count :
#endif
                            g_playerPlayerData[curplayernum].kill_counts[i], current_colour);
                }
            }
        }
        else if (g_CurrentPlayer->mpmenumode == MENU_LOSSES)
        {
            fav_x_offset = (g_gameOverFlag == 0); 

            if (fav_x_offset) 
            { 
                fav_x_offset = (g_stopPlayFlag == 0); 
            } 

            if (mpwatchShouldDisplayRank(fav_x_offset))
            {
                write_playerrank_to_buffer(rankbuffer, curplayernum);
                textMeasure(&textheight, &textwidth, rankbuffer, ptrFontBankGothicChars, ptrFontBankGothic, 0);
                x = ((viGetViewLeft() + two_player_x_offset) - (textwidth >> 1)) + 80;
                y = (viGetViewTop() - (textheight >> 1)) + (37 + MPMENU_YOFF);
                viewleft = viGetX(); 
                h1 = viGetY();
                gdl = textRender(gdl, &x, &y, rankbuffer, ptrFontBankGothicChars, ptrFontBankGothic, 0x00ff00b0, viewleft, h1, 0, 0);
            }
 
#ifdef GE_MODDED_CHEATS
            if (mpwatchIsCampaignCoop())
            {
                text = "DEATHS";
            }
            else
#endif
            {
                q = (s32) langGet(getStringID(LMPMENU, MPMENU_STR_1C_P)); /* P */
                h2 = (s32) langGet(getStringID(LMPMENU, MPMENU_STR_1E_LOSSES)); /* LOSSES */
                sprintf(rankbuffer, ascii_pnum_LOSSES, (char *) q, curplayernum + 1, (char *) h2);
                text = rankbuffer;
            }
 
            textMeasure(&textheight, &textwidth, text, ptrFontBankGothicChars, ptrFontBankGothic, 0);
            x = ((viGetViewLeft() + two_player_x_offset) - (textwidth >> 1)) + 80;
            y = (viGetViewTop() - (textheight >> 1)) + (49 + MPMENU_YOFF)
                + ((!g_gameOverFlag && !mpwatchIsCampaignCoop()) ? 1 : 0)
                + ((g_gameOverFlag && !mpwatchIsCampaignCoop()) ? 2 : 0);
            viewleft = viGetX(); 
            h1 = viGetY();
            gdl = textRender(gdl, &x, &y, text, ptrFontBankGothicChars, ptrFontBankGothic, 0xff4040b0, viewleft, h1, 0, 0);
 
            /* RC5: Co-Op DEATHS mirrors the KILLS ownership cue.  Retail
             * competitive mode still only shows the diagonal when it records
             * an actual self-kill; Co-Op always shows it (including zero). */
            for (i = 0; i < player_count; i++)
            {
                viewleft = viGetViewLeft();
                h1 = viGetViewTop();
                x2 = player_count == 2 ? 80 : 64 + ((i & 1) << 5);
                y = 70 + ((player_count == 2 ? i : i >> 1) << 4) + MPMENU_YOFF;

#ifdef GE_MODDED_CHEATS
                /* Match Co-Op DEATHS values to the normal multiplayer
                 * Game Over value row positions. */
                if (g_gameOverFlag || mpwatchIsCampaignCoop()) y -= 2;
                if (mpwatchIsCampaignCoop())
                {
                    current_colour = i == curplayernum ? GREEN_HIGHLIGHT : GREEN_NORMAL;
                    gdl = display_text_for_playerdata_on_MP_menu(gdl,
                            (viewleft + two_player_x_offset) + x2, h1 + y,
                            g_playerPointers[i]->deathcount, current_colour);
                }
                else
#endif
                if (curplayernum != i)
                {
                    gdl = display_text_for_playerdata_on_MP_menu(gdl,
                            (viewleft + two_player_x_offset) + x2, h1 + y,
                            g_playerPlayerData[i].kill_counts[curplayernum], GREEN_NORMAL);
                }
                else if (g_playerPlayerData[i].kill_counts[i] > 0)
                {
                    gdl = display_text_for_playerdata_on_MP_menu(gdl,
                            (viewleft + two_player_x_offset) + x2, h1 + y,
                            g_playerPlayerData[i].kill_counts[curplayernum], RED_HIGHLIGHT);
                }
            }
        }
#ifdef GE_MODDED_CHEATS
        else if (mpwatchIsCampaignCoop() && g_CurrentPlayer->mpmenumode == MENU_GOWOC)
        {
            /* RC6: Friendly Kills is the player-vs-player-only page. The
             * diagonal (suicides/environment) is deliberately excluded. */
            text = "FRIENDLY KILLS";
            textMeasure(&textheight, &textwidth, text, ptrFontBankGothicChars, ptrFontBankGothic, 0);
            x = ((viGetViewLeft() + two_player_x_offset) - (textwidth >> 1)) + 80;
            y = (viGetViewTop() - (textheight >> 1)) + (49 + MPMENU_YOFF);
            gdl = textRender(gdl, &x, &y, text, ptrFontBankGothicChars, ptrFontBankGothic,
                    0x00ff00b0, viGetX(), viGetY(), 0, 0);

            for (i = 0; i < player_count; i++)
            {
                s32 friendly = 0;
                for (m = 0; m < player_count; m++)
                {
                    if (m != i) friendly += g_playerPlayerData[i].kill_counts[m];
                }
                viewleft = viGetViewLeft();
                h1 = viGetViewTop();
                x2 = player_count == 2 ? 80 : 64 + ((i & 1) << 5);
                y = 70 + ((player_count == 2 ? i : i >> 1) << 4) + MPMENU_YOFF;
                /* Friendly Kills uses the same two-pixel-up value grid in
                 * both Co-Op pause and Co-Op Game Over. */
                y -= 2;
                current_colour = i == curplayernum ? GREEN_HIGHLIGHT : GREEN_NORMAL;
                gdl = display_text_for_playerdata_on_MP_menu(gdl,
                        (viewleft + two_player_x_offset) + x2, h1 + y, friendly, current_colour);
            }
        }
#endif
        else if (g_CurrentPlayer->mpmenumode == MENU_GOWOC)
        {
            fav_x_offset = two_player_x_offset;

            if (player_count >= 3)
            {
                if (curplayernum & 1)
                {
                    fav_x_offset = two_player_x_offset - 7;
                }
                else
                {
                    fav_x_offset = two_player_x_offset + 7;
                }
            }
 
            text = (char *) langGet(getStringID(LMPMENU, MPMENU_STR_1F_WEAPONOFCHOICE)); /* Weapon of choice: */
            textMeasure(&fav_textheight, &fav_textwidth, text, ptrFontBankGothicChars, ptrFontBankGothic, 0);
            x = ((viGetViewLeft() + fav_x_offset) - (fav_textwidth >> 1)) + 80;
            y = (viGetViewTop() - (fav_textheight >> 1)) + (37 + MPMENU_YOFF);
            viewleft = viGetX(); h1 = viGetY();
            gdl = textRender(gdl, &x, &y, text, ptrFontBankGothicChars, ptrFontBankGothic, 0x00ff00b0, viewleft, h1, 0, 0);
            text = frontGetPlayersFavoriteWeaponInHand(curplayernum, 0);
            textMeasure(&fav_textheight, &fav_textwidth, text, ptrFontBankGothicChars, ptrFontBankGothic, 0);
            x = ((viGetViewLeft() + fav_x_offset) - (fav_textwidth >> 1)) + 80;
            x2 = viGetViewTop();
 
            if (j_text_trigger) 
            { 
                i = 4; 
            } 
            else 
            { 
                i = 0; 
            } 

            y = ((i + (u32) x2) - (fav_textheight >> 1)) + (53 + MPMENU_YOFF);
 
            viewleft = viGetX(); 
            h1 = viGetY();
            gdl = textRender(gdl, &x, &y, text, ptrFontBankGothicChars, ptrFontBankGothic, 0x00ff00b0, viewleft, h1, 0, 0);
 
            if (g_CurrentPlayer->ptr_text_first_mp_award)
            {
                text = (char *) g_CurrentPlayer->ptr_text_first_mp_award;
                textMeasure(&fav_textheight, &fav_textwidth, text, ptrFontBankGothicChars, ptrFontBankGothic, 0);
                x = ((viGetViewLeft() + fav_x_offset) - (fav_textwidth >> 1)) + 80;
                y = (viGetViewTop() - (fav_textheight >> 1)) + (75 + MPMENU_YOFF);
                viewleft = viGetX(); 
                h1 = viGetY();
                gdl = textRender(gdl, &x, &y, text, ptrFontBankGothicChars, ptrFontBankGothic, 0x00ff00b0, viewleft, h1, 0, 0);
            }

            if (g_CurrentPlayer->ptr_text_second_mp_award)
            {
                text = (char *) g_CurrentPlayer->ptr_text_second_mp_award;
                textMeasure(&fav_textheight, &fav_textwidth, text, ptrFontBankGothicChars, ptrFontBankGothic, 0);
                x = ((viGetViewLeft() + fav_x_offset) - (fav_textwidth >> 1)) + 80;
                y = (viGetViewTop() - (fav_textheight >> 1)) + (88 + MPMENU_YOFF);
                viewleft = viGetX(); 
                h1 = viGetY();
                gdl = textRender(gdl, &x, &y, text, ptrFontBankGothicChars, ptrFontBankGothic, 0x00ff00b0, viewleft, h1, 0, 0);
            }
        }

        if (g_CurrentPlayer->mpmenumode == MENU_EXIT_CONFIRM)
        {
            text = (char *) langGet(getStringID(LMPMENU, MPMENU_STR_20_CANCEL)); /* cancel */
            textMeasure(&textheight, &textwidth, text, ptrFontBankGothicChars, ptrFontBankGothic, 0);
            x = ((viGetViewLeft() + two_player_x_offset) - (textwidth >> 1)) + 54;
            y = (viGetViewTop() - (textheight >> 1)) + (54 + MPMENU_YOFF);
 
            if (g_CurrentPlayer->mpquitconfirm == 0)
            {
                viewleft = viGetX(); 
                h1 = viGetY();
                gdl = textRenderOutlined(gdl, &x, &y, text, ptrFontBankGothicChars, ptrFontBankGothic, 0xa0ffa0f0, 0x007000a0, viewleft, h1, 0, 0);
            }
            else
            {
                viewleft = viGetX(); 
                h1 = viGetY();
                gdl = textRender(gdl, &x, &y, text, ptrFontBankGothicChars, ptrFontBankGothic, 0x00ff00b0, viewleft, h1, 0, 0);
            }
 
            text = (char *) langGet(getStringID(LMPMENU, MPMENU_STR_21_CONFIRM)); /* confirm */
            textMeasure(&textheight, &textwidth, text, ptrFontBankGothicChars, ptrFontBankGothic, 0);
            x = ((viGetViewLeft() + two_player_x_offset) - (textwidth >> 1)) + 104;
            y = (viGetViewTop() - (textheight >> 1)) + (54 + MPMENU_YOFF);
 
            if (g_CurrentPlayer->mpquitconfirm == 1)
            {
                viewleft = viGetX(); 
                h1 = viGetY();
                gdl = textRenderOutlined(gdl, &x, &y, text, ptrFontBankGothicChars, ptrFontBankGothic, 0xa0ffa0f0, 0x007000a0, viewleft, h1, 0, 0);
            }
            else
            {
                viewleft = viGetX(); 
                h1 = viGetY();
                gdl = textRender(gdl, &x, &y, text, ptrFontBankGothicChars, ptrFontBankGothic, 0x00ff00b0, viewleft, h1, 0, 0);
            }
        }
#ifdef GE_MODDED_CHEATS
        if (g_gameOverFlag)
        {
            text = (char *)langGet(getStringID(LMPMENU, MPMENU_STR_17_STARTTTOEXIT));
            textMeasure(&textheight, &textwidth, text, ptrFontBankGothicChars, ptrFontBankGothic, 0);
            x = viGetViewLeft() + (viGetViewWidth() >> 1) - (textwidth >> 1);
            y = viGetViewTop() + viGetViewHeight() - textheight - 14;
            gdl = textRender(gdl, &x, &y, text, ptrFontBankGothicChars, ptrFontBankGothic,
                    0x00ff00b0, viGetX(), viGetY(), 0, 0);
        }
#endif
        gdl = combiner_bayer_lod_perspective(gdl);
    }
    else if (((((g_CurrentPlayer->bonddead) && (g_CurrentPlayer->deathanimfinished)) && (g_CurrentPlayer->redbloodfinished)) && (!g_stopPlayFlag)) && (!g_gameOverFlag)
#ifdef GE_MODDED_CHEATS
        && !lvlIsCoopPermanentExplosionFailure()
#endif
    )
    {
        total_kills_against_current = 0;
 
        for (m = 0; m < player_count; m++)
        {
            total_kills_against_current += g_playerPlayerData[m].kill_counts[curplayernum];
        }
 
        if ((get_scenario() != SCENARIO_YOLT) || (total_kills_against_current < 2))
        {
            gdl = bgScissorCurrentPlayerViewDefault(gdl);
            gdl = microcode_constructor(gdl);
            text3 = (char *) langGet(getStringID(LMPMENU, MPMENU_STR_22_PRESSSTART_LF)); /* press start */
            textMeasure(&textheight3, &textwidth3, text3, ptrFontBankGothicChars, ptrFontBankGothic, 0);
            x2 = viGetViewLeft();
            x3 = (x2 + (viGetViewWidth() >> 1)) - (textwidth3 >> 1);
            x2 = viGetViewTop();
            y3 = (x2 + (viGetViewHeight() >> 1)) - (textheight3 >> 1);
#ifndef VERSION_US
            gdl = microcode_constructor_related_to_menus(gdl, x3 - 1, y3 - 1, x3 + textwidth3 + 1, y3 + textheight3 + 1, 0);
#endif
            viewleft = viGetX(); 
            h1 = viGetY();
            gdl = textRender(gdl, &x3, &y3, text3, ptrFontBankGothicChars, ptrFontBankGothic, 0x00ff00b0, viewleft, h1, 0, 0);
            gdl = combiner_bayer_lod_perspective(gdl);
        }
    }
 
    return gdl;
}


s32 mpwatchShouldDisplayGauges(void)
{
    return g_gameOverFlag ? FALSE : (g_CurrentPlayer->mpmenuon | (g_CurrentPlayer->healthdisplaytime > 0));
}


s32 checkGamePaused(void) 
{
    return g_pausedFlag;
}
