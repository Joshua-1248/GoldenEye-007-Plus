#include <ultra64.h>
#include <math.h>
#include <os_extension.h>
#include <PR/libaudio.h>
#include <assets/font_dl.h>
#include <deb.h>
#include <memp.h>
#include <music.h>
#include <tlb_manage.h>
#include <fr.h>
#include <snd.h>
#include <ramrom.h>
#include <random.h>
#include <joy.h>
#include <token.h>
#include "debugmenu_handler.h"
#include "lv.h"
#include "language.h"
#include "initcheattext.h"
#include "front.h"
#include "bondinv.h"
#include "player.h"
#include "propobj.h"
#include "cleanup_objects.h"
#include "explosion.h"
#include "chrai.h"
#include "mp_music.h"
#include "initunk_005520.h"
#include "initmttex.h"
#include "glass.h"
#include "image_bank.h"
#include "textrelated.h"
#include "initmenus.h"
#include "cheat.h"
#include "bg.h"
#include "objective.h"
#include "mpmenu.h"
#include "vtxstore.h"
#include "initunk_005450.h"
#include "initobjects.h"
#include "initguards.h"
#include "prop.h"
#include "initexplosioncasing.h"
#include "alloc_window_pieces.h"
#include "initunk_007290.h"
#include "initcheattext.h"
#include "initpathtablelinks.h"
#include "ejectedcartridges.h"
#include "inititemslots.h"
#include "initBondDATA.h"
#include "bondview.h"
#include "bondview_r.h"
#include "initBondDATAdefaults.h"
#include "viewport.h"
#include "stan.h"
#include "gun.h"
#include "debug_camera.h"
#include "mp_music.h"
#include "bgroomtrans.h"
#include "unk_092E50.h"
#include "frametiming.h"
#include "chr.h"

/* Used by the Co-Op stage bootstrap before the first gameplay tick. */
void bondviewUpdatePlayerCollisionBounds(void);

// bss
//CODE.bss:8008C260
u32 *ptr_font_DL;
//CODE.bss:8008C264
s32 dword_CODE_bss_8008C264;
s32 dword_CODE_bss_8008C268;
s32 dword_CODE_bss_8008C26C;
//CODE.bss:8008C270
char ramrom_data_target[0x380];
//CODE.bss:8008C5F0
s32 record_slot_num;
//CODE.bss:8008C5F4
u8 * address_demo_loaded;
//CODE.bss:8008C5F8
s32 dword_CODE_bss_8008C5F8;


// data
//D:80048360
s32 lvl_c_debug_notice_list = 0;
//D:80048364
s32 g_CurrentStageToLoad = 0;
//D:80048368
f32 D_80048368 = 1.0;
//D:8004836C
s32 musictrack1_playing = 0;
//D:80048370
s32 g_ControlsLockedFlag = 0;
//D:80048374
s32 g_ClockTimer = 0;


#if defined (BUGFIX_R1)
// addresses updated, per build\ge007.j.map
// 800483a8
f32 g_JP_GlobalTimerDelta = 0;
// 800483ac
s32 g_GlobalTimer = 0;
// 800483b0
s32 D_80048380 = 0;
// 800483b4
f32 g_GlobalTimerDelta = 0;
#else
//D:80048378
f32 g_GlobalTimerDelta = 0;
//D:8004837C
s32 g_GlobalTimer = 0;
//D:80048380
s32 D_80048380 = 0;
//D:80048384
#endif
/*
* Selected difficulty mode.
* 0x80048384
*/
s32 g_SelectedDifficulty = DIFFICULTY_AGENT;

//D:80048388
s32 D_80048388 = 0;
//D:8004838C
s32 D_8004838C = 0;
//D:80048390
s32 D_80048390 = 0;
//D:80048394
s32 D_80048394 = 0;

/**
 * Address 0x80048398.
 */
s32 g_MpTime = 0x8CA0;

/**
 * Address 0x8004839C.
 */
s32 g_MpPoint = 0xA;

/**
 * Address 0x800483A0.
 */
ALSoundState * g_MpSoundStateRelated = NULL;

/**
 * Address 0x800483A4.
 */
f32 g_CurrentMultiPlayerSec = 0.0;

//D:800483A8
s32 D_800483A8 = 0;

/**
 * Address 0x800483AC.
 */
f32 g_CurrentMultiPlayerMin = 0.0;

//D:800483B0
s32 D_800483B0 = 0;

/**
 * Address 0x800483B4.
 */
f32 g_StageTimeSec = 0;

//D:800483B8
s32 D_800483B8 = 0;

/**
 * Power on time in seconds.
 * Address 0x800483BC.
 */
f32 g_PowerOnTimeSec = 0;

/**
 * Debug variable, seems to track whether user input has changed since
 * the last time the method was entered.
 *
 * Addres 0x800483C0.
 */
s32 D_800483C0 = 1;

//D:800483C4
s32 D_800483C4 = 0xFFFFFFFF;

//D:800483C8
struct LvlMpUnknown *D_800483C8 = NULL;

/**
* Debug variable, something to do with portals.
* Address 0x800483CC.
*/
s32 g_DebugPortalIndex = 0;

/**
 * Input buffer used in debug portal method.
 * Address 0x800483D0.
 */
s32 g_DebugPortalsInputBuffer = 0;
s32 g_DebugPortalsInputBuffer1 = 0;
s32 g_DebugPortalsInputBuffer2 = 0;
s32 g_DebugPortalsInputBuffer3 = 0;
s32 g_DebugPortalsInputBuffer4 = 0;

extern s32 g_DebugPortalsInputBufferSource1;
extern s32 g_DebugPortalsInputBufferSource2;
extern s32 g_DebugPortalsInputBufferSource3;
extern s32 g_DebugPortalsInputBufferSource4;

#pragma weak g_DebugPortalsInputBufferSource1 = g_DebugPortalsInputBuffer1
#pragma weak g_DebugPortalsInputBufferSource2 = g_DebugPortalsInputBuffer2
#pragma weak g_DebugPortalsInputBufferSource3 = g_DebugPortalsInputBuffer3
#pragma weak g_DebugPortalsInputBufferSource4 = g_DebugPortalsInputBuffer4

/**
 * Something debug related in the MP manage method.
 * Index to play sound effect.
 * Address 0x800483E4.
 */
s16 g_DebugMpGameSoundFxIndex = 0;

// unused address padding
s16 D_800483E6 = 0;


extern u8* _fontdlSegmentRomStart;
extern u8* _fontdlSegmentRomEnd;


// forward declarations

Gfx * lvlPortalDebug7F0BDF10(Gfx * arg0);

// end forward declarations

s32 sub_GAME_7F0BD8F0(void)
{
    return D_800483C0;
}

void sub_GAME_7F0BD8FC(s32 arg0)
{
    D_800483C0 = arg0;
}

void lvInit(void)
{
    s32 size;

    debTryAdd(&lvl_c_debug_notice_list, "lv_c_debug");
    size = (s32)&_fontdlSegmentRomEnd - (s32)&_fontdlSegmentRomStart;
    lvl_c_debug_notice_list = 1;
    ptr_font_DL = mempAllocBytesInBank(size, MEMPOOL_PERMANENT);
    romCopy(ptr_font_DL, &_fontdlSegmentRomStart, size);
}

/**
 * Unreferenced.
 */
void lvlPlayRandomMusicTrack1(void)
{
    musictrack1_playing = randomGetNext() % 0x3dU + M_INTRO;
    musicTrack1Play(musictrack1_playing);
}

void lvlPlayMusicTrack1(MUSIC_TRACKS track)
{
    musictrack1_playing = track;
    musicTrack1Play(musictrack1_playing);
}

void lvlMusicAppendPlaySoloDeathShort(void)
{
    musictrack1_playing = (musictrack1_playing + M_SHORT_SOLO_DEATH) % NUM_MUSIC_TRACKS;

    if (musictrack1_playing == M_NONE)
    {
        musictrack1_playing = M_SHORT_SOLO_DEATH;
    }

    musicTrack1Play(musictrack1_playing);
}

void lvlMusicAppendPlayEndTheme(void)
{
    musictrack1_playing = (musictrack1_playing + M_END_SOMETHING) % NUM_MUSIC_TRACKS;

    if (musictrack1_playing == M_NONE)
    {
        musictrack1_playing = M_END_SOMETHING;
    }

    musicTrack1Play(musictrack1_playing);
}

/**
 * Unreferenced.
 */
void lvlMusicPlayStageTrackOrRandom(void)
{
    lvlPlayMusicTrack1(getmusictrack_or_randomtrack(g_CurrentStageToLoad));
}

/**
 * Stage load method.
 * Title screen is handled as a special case.
 * First half of method resets stage and player values (including mutliplayer values) to defaults.
 * Second part loads stage data (init guards, init guard heads, etc).
 *
 * NTSC Address: 0x7F0BDAB0.
 * NTSC-J Address: 7F0BE660.
 * PAL Address: 7F0BCE60.
 **/
void lvlStageLoad(s32 stage)
{
    s32 i;
    struct player_data *player_data;

    g_CurrentStageToLoad = stage;

    // this if block pushes where g_CurrentStageToLoad gets loaded to the
    // top of the method. Maybe a debug log about which level is loaded.
    if(0)
    {
        #ifdef DEBUG
        // removed
        #endif
    }

    D_800483C0 = 1;
    g_ControlsLockedFlag = 0;
    g_ClockTimer = 1;

#ifdef VERSION_US
    g_GlobalTimerDelta = 1.0f;
#endif
#if defined(VERSION_JP) || defined(VERSION_EU)
    g_JP_GlobalTimerDelta = 1.0f;
#endif

    D_80048380 = 0;
    g_GlobalTimer = 0;

#if defined(VERSION_JP)
    g_GlobalTimerDelta = 1.f;
#endif
#if defined(VERSION_EU)
    g_GlobalTimerDelta = 1.20000004768f;
#endif

    D_80048388 = 0;
    D_8004838C = 0;
    D_80048390 = 0;
    D_80048394 = 0;
    g_CurrentMultiPlayerSec = 0.0f;
    D_800483B0 = 0;
    g_StageTimeSec = 0.0f;
    g_MpSoundStateRelated = 0;

    sndSetScalerApplyVolumeAllSfxSlot(1.0f);
    musicTrack1ApplySeqpVol(VOLUME_MAX);
    musicTrack2ApplySeqpVol(VOLUME_MAX);
    musicTrack3ApplySeqpVol(VOLUME_MAX);
    sub_GAME_7F0C1364();
    modelmgrSetLevelResetting(TRUE);
    set_mt_tex_alloc();
#ifdef VERSION_EU
    bullet_moving_sparks_reset();
#else
    bullet_sparks_reset_all();
#endif
    texReset();
    load_font_tables();

    /* If title screen, initialize screen and folder setup.
    * Otherwise:
    * - enable cheats for player
    * - init watch
    * - reset some player values, like view height
    * - reset multiplayer stats
    */
    if (stage == LEVELID_TITLE)
    {
        init_menus_or_reset();
    }
    else
    {
        g_NewCheatUnlocked = 0;

        if ((g_CurrentStageToLoad != LEVELID_TITLE) && (D_80048394 == 0) && (g_ClockTimer > 0))
        {
            if (g_AppendCheatSinglePlayer != 0)
            {
                s32 s0 = 1;

                for (s0 = 1; s0 != CHEAT_INVALID; s0++)
                {
                    if (g_CheatActivated[s0] && cheatIsEnemyRockets(s0))
                    {
                        cheatButtonTurnOnCheatForPlayers(s0);
                    }
                }
            }
        }

        load_bg_file(g_CurrentStageToLoad);
        skySetStageNum(g_CurrentStageToLoad);

        // HACK: This method call is wrong. The function takes one argument, but the asm calls it without
        // any arguments here.
        init_watch_at_start_of_stage();

        sub_GAME_7F0C11FC(stage);

        for (i=0; i<4; i++)
        {
            s32 s3;
            player_data = (struct player_data *)&g_playerPlayerData[i];

            if (getPlayerCount() == 1
#ifdef GE_MODDED_CHEATS
                && gamemode != GAMEMODE_MULTI
#endif
            )
            {
                // s4 variable
                player_data->autoaim = 0;
                player_data->sight = 0;
                player_data->handicap = 1.0f;
                player_data->player_perspective_height = 1.0f;
            }
            else
            {
#ifdef GE_PHYSICAL_FASTPATHS
                /* Retail repeats this whole four-player setup once for every
                 * outer player (16 handicap/height writes plus four copies of
                 * the same MP limits/aim settings).  All operations are pure
                 * table reads/assignments, so perform the identical setup once. */
                if (i == 0)
                {
#endif
                    for (s3 = 0; s3 < 4; s3++)
                    {
                        if (get_scenario() == SCENARIO_LTK)
                        {
                            g_playerPlayerData[s3].handicap = 200.0f;
                        }
                        else
                        {
                            g_playerPlayerData[s3].handicap = get_player_mp_handicap(s3);
                        }

                        g_playerPlayerData[s3].player_perspective_height = get_player_mp_char_height(s3);
                    }

                    lvlSetMpTime(get_mp_timelimit());
                    lvlSetMpPoint(get_mp_pointlimit());
                    copy_aim_settings_to_playerdata();
#ifdef GE_PHYSICAL_FASTPATHS
                }
#endif
            }

            // g_playerPlayerData s4 variable
            player_data->time_other_players_on_screen = 0;
            player_data->damage_to_backside = 0;
            player_data->min_time_between_kills = S32_MAX;
            player_data->max_time_between_kills = 0;
            player_data->most_killed_one_life = 0;
            player_data->most_killed_one_time = 0;
            player_data->longest_inning = 0;
            player_data->shortest_inning = S32_MAX;
            player_data->order_out_in_yolt = 0;
            player_data->flag_counter = 0;
            player_data->distance_traveled = 0.0f; // one kind of float zero
            player_data->body_armor_pickups = 0.f; // a different kind of float zero

            // g_playerPlayerData s2, different than above
            for (s3 = 0; s3 < 4; s3++)
            {
                player_data->kill_counts[s3] = 0;
            }
        }
    }

    something_with_stage_objectives();
#ifdef GE_MODDED_CHEATS
    bondinvResetCoopSharedProps();
#endif
    mpwatchUnpauseGame();
    sub_GAME_7F09B820();
    initModelHitEntryFreeList();
    modelmgrResetSlotCounts();
    init_load_objpos_table();
    reinit_between_menus();
    init_sound_effects_registers();
    init_guards();
    bodiesReset(stage);
    proplvreset2(stage);
    alloc_explosion_smoke_casing_scorch_impact_buffers();
    alloc_shattered_window_pieces();
    sub_GAME_7F007290();
    initCheatTextBuffer();

    if (g_CurrentStageToLoad == LEVELID_TITLE)
    {
        disableOnscreenCheatText();
    }
    else
    {
        s32 s0;

        init_path_table_links();
        init_ejected_cartridges();

        for (s0 = 0; s0 < getPlayerCount(); s0++)
        {
            set_cur_player(s0);
            reinit_gunheld_totaltime();
            init_player_BONDdata_stats();
            init_player_BONDdata();
            bondviewLoadSetupIntroSection();
            bondviewPlayerBeginLife();
            sets_a_bunch_of_BONDdata_values_to_default();
            disableOnscreenCheatText();
        }

#ifdef GE_MODDED_CHEATS
        /*
         * Campaign Co-Op creates more than one player from a setup which was
         * authored for a single Bond.  Retail normally gets away with leaving
         * collision_bounds untouched here because the single player computes
         * them during the first movement tick before another player can query
         * them.  With two or more Co-Op players, one player's first collision
         * query can inspect another player's still-uninitialised rectangle.
         *
         * A Project64 R8 fault dump caught exactly that state: mainThread took
         * an FP invalid-operation exception in stanPointProjectsOntoEdge while
         * player 1's collision_bounds still contained allocator/debug fill.
         * Build all Co-Op player rectangles once, after every player has a
         * valid field_488 spawn position and before gameplay is allowed to tick.
         */
        if (gamemode == GAMEMODE_MULTI && get_scenario() == SCENARIO_COOP)
        {
            for (s0 = 0; s0 < getPlayerCount(); s0++)
            {
                set_cur_player(s0);
                bondviewUpdatePlayerCollisionBounds();
            }
        }
#endif

        set_cur_player(0);
#ifdef GE_PHYSICAL_FASTPATHS
        /* Hide first-person texture DMA/decompression inside stage loading so
         * weapon switches do not pay that cost during active gameplay. */
        gunPrewarmFirstPersonTextureCache();
#endif
    }

    /**
     * Leave stage load allocation mode.
     * From this point on, model creation should try to reuse existing slots.
     */
    modelmgrSetLevelResetting(FALSE);
    
    zbufDeallocate();
    viSetVideoMode(MD_NORMAL);
    D_80048368 = 1.0f;
    lvlSetControlsLockedFlag(0);
}



s32 lvlGetCurrentStageToLoad(void)
{
    return g_CurrentStageToLoad;
}

#ifdef GE_MODDED_CHEATS
/*
 * R15: campaign Co-Op exit cinematics use a single Player 1 presentation.
 * CAMERAMODE_POSEND is the scripted third-person camera mode used by the
 * retail exit-camera commands (look-at, switch and orbit).  Keeping this
 * predicate central lets the view tick, viewport sizing and renderer agree on
 * exactly when split-screen must collapse.
 */
s32 lvlIsCoopEndCutscene(void)
{
    static s32 sharedViewportLatched = FALSE;

    if (gamemode != GAMEMODE_MULTI || get_scenario() != SCENARIO_COOP || stop_time_flag == 0)
    {
        sharedViewportLatched = FALSE;
        return FALSE;
    }

    /* R18: camera_switch enters POSEND a few yields after the retail exit flag
     * is armed.  Once that actual end camera begins, keep the shared P1
     * viewport latched until the exit sequence clears stop_time_flag.  R15
     * recomputed this from the instantaneous camera mode on every call, which
     * could make one frame alternate between split and shared viewport rules
     * while the camera/fade state advanced. */
    if (bondviewGetCameraMode() == CAMERAMODE_POSEND)
    {
        sharedViewportLatched = TRUE;
    }

    return sharedViewportLatched;
}

/*
 * R20: Silo and Bunker II deliberately arm the retail permanent-explosion
 * hazard when their terminal failure condition is reached.  In campaign
 * Co-Op that must become a one-way failure state: dead players may not
 * respawn into the still-active explosions.
 */
s32 lvlIsCoopPermanentExplosionFailure(void)
{
    return gamemode == GAMEMODE_MULTI
        && get_scenario() == SCENARIO_COOP
        && g_SurroundBondWithExplosionsFlag
        && (g_CurrentStageToLoad == LEVELID_SILO || g_CurrentStageToLoad == LEVELID_BUNKER2);
}
#endif


 /**
 * Address: 7F0BDF10
 * 
 * Debug method. Something to do with portals. Button press
 * on controller 1 and 2 are used for control flow.
 */
Gfx *lvlPortalDebug7F0BDF10(Gfx *gdl)
{
    s32 temp_v1;
    bool portalDebugStateChanged; // This value has no observable use in the final assembly but the variable definitely existed in the orignal code.
    s32 *selectedPortal = &g_DebugPortalIndex;

    portalDebugStateChanged = FALSE;

    if (gdl)
    {
        gdl = bgDebugRemoved7F0B9DE4(gdl, g_DebugPortalIndex, -1);

        g_DebugPortalsInputBuffer = g_DebugPortalsInputBufferSource1;
        g_DebugPortalsInputBuffer1 = g_DebugPortalsInputBufferSource2;
        temp_v1 = g_DebugPortalsInputBufferSource4;
        g_DebugPortalsInputBuffer2 = g_DebugPortalsInputBufferSource3;
        g_DebugPortalsInputBuffer3 = temp_v1;

        temp_v1 = joyGetButtons(PLAYER_1, A_BUTTON) | joyGetButtons(PLAYER_2, A_BUTTON);

        if ((temp_v1 == g_DebugPortalsInputBuffer3) == FALSE)
        {
            D_800483C0 ^= 1;
        }

        if ((g_DebugPortalsInputBuffer1 == g_DebugPortalsInputBuffer) == FALSE)
        {
            D_800483C0 ^= 1;
        }

        g_DebugPortalsInputBuffer4 = temp_v1;

        bgRemoved7F0B9DF4(temp_v1 ? g_DebugPortalIndex : -1);

        return gdl;
    }

    if (joyGetButtonsPressedThisFrame(PLAYER_1, L_JPAD) |
        joyGetButtonsPressedThisFrame(PLAYER_2, L_JPAD))
    {
        g_DebugPortalIndex = *selectedPortal - 1;

        if (portalDebugStateChanged = TRUE, *selectedPortal < 0)
        {
            g_DebugPortalIndex = 0;
        }
    }

    if (joyGetButtonsPressedThisFrame(PLAYER_1, R_JPAD) |
        joyGetButtonsPressedThisFrame(PLAYER_2, R_JPAD))
    {
        g_DebugPortalIndex = *selectedPortal + 1;
        portalDebugStateChanged = TRUE;
    }

    if ((joyGetButtons(PLAYER_1, R_TRIG) | joyGetButtons(PLAYER_2, R_TRIG)) &&
        (joyGetButtons(PLAYER_1, L_TRIG) | joyGetButtons(PLAYER_2, L_TRIG)))
    {
        if (joyGetButtonsPressedThisFrame(PLAYER_1, D_JPAD))
        {
            bgSwapConnectedRooms(*selectedPortal);
        }
    }
    else if (joyGetButtons(PLAYER_1, R_TRIG) | joyGetButtons(PLAYER_2, R_TRIG))
    {
        if ((joyGetButtonsPressedThisFrame(PLAYER_1, D_JPAD) |
             joyGetButtonsPressedThisFrame(PLAYER_2, D_JPAD)) &&
            (bgGetDataPortalsControlBytes1Bit1(g_DebugPortalIndex) == 0))
        {
            bgToggleDataPortalsContrlBytes1Bit1(g_DebugPortalIndex, 0);
            portalDebugStateChanged = TRUE;
        }

        if ((joyGetButtonsPressedThisFrame(PLAYER_1, U_JPAD) |
             joyGetButtonsPressedThisFrame(PLAYER_2, U_JPAD)) &&
            (bgGetDataPortalsControlBytes1Bit1(g_DebugPortalIndex) != 0))
        {
            bgToggleDataPortalsContrlBytes1Bit1(g_DebugPortalIndex, 1);
            portalDebugStateChanged = TRUE;
        }
    }
    else if (joyGetButtons(PLAYER_1, L_TRIG) | joyGetButtons(PLAYER_2, L_TRIG))
    {
        if ((joyGetButtonsPressedThisFrame(PLAYER_1, D_JPAD) |
             joyGetButtonsPressedThisFrame(PLAYER_2, D_JPAD)) &&
            bgGetDataPortalsControlBytes1Bit2(g_DebugPortalIndex))
        {
            bgClearDataPortalsControlBytes1Low2Bits(g_DebugPortalIndex);
            portalDebugStateChanged = TRUE;
        }

        if ((joyGetButtonsPressedThisFrame(PLAYER_1, U_JPAD) |
             joyGetButtonsPressedThisFrame(PLAYER_2, U_JPAD)) &&
            (bgGetDataPortalsControlBytes1Bit2(g_DebugPortalIndex) == 0))
        {
            bgSetDataPortalsControlBytes1Bit2(g_DebugPortalIndex);
            portalDebugStateChanged = TRUE;
        }
    }
    else
    {
        if (joyGetButtonsPressedThisFrame(PLAYER_1, D_JPAD) |
            joyGetButtonsPressedThisFrame(PLAYER_2, D_JPAD))
        {
            sub_GAME_7F0B9A7C(g_DebugPortalIndex);
            portalDebugStateChanged = TRUE;
        }

        if (joyGetButtonsPressedThisFrame(PLAYER_1, U_JPAD) |
            joyGetButtonsPressedThisFrame(PLAYER_2, U_JPAD))
        {
            sub_GAME_7F0B9A2C(g_DebugPortalIndex);
            portalDebugStateChanged = TRUE;
        }
    }

    if (portalDebugStateChanged)
    {
        // Removed
    }

    return NULL;
}


/**
 * Graphics render method.
 * Also sets player max ammo if infinite ammo cheat is enabled.
 *
 * Address 0x7F0BE30C (VERSION_US).
 */

Gfx* lvlRender(Gfx* DL)
{
    gSPSegment(DL++, SPSEGMENT_PHYSICAL, NULL);
    gSPSegment(DL++, SPSEGMENT_UNKNOWN, osVirtualToPhysical(ptr_font_DL));

    gSPDisplayList(DL++, &dlFastPipelineSetup);
    gSPDisplayList(DL++, &dlZBufferGeometry);

    if (g_CurrentStageToLoad == LEVELID_TITLE)
    {
        DL = viClearZBufCurrentPlayer(DL);
        DL = viSetupCurrentPlayerView(DL);
        gDPSetScissor(DL++, G_SC_NON_INTERLACE, 0, 0, (s16)viGetX(), (s16)viGetY());
        DL = menu_jump_constructor_handler(DL);
    }
    else
    {
        s32 i;
        s32 pcount;

#ifdef GE_MODDED_CHEATS
        pcount = lvlIsCoopEndCutscene() ? 1 : getPlayerCount();

        if (lvlIsCoopEndCutscene())
        {
            /* Collapse from split-screen only after first restoring a full
             * framebuffer scissor and clearing every old player region. */
            gDPSetScissor(DL++, G_SC_NON_INTERLACE, 0, 0, (s16)viGetX(), (s16)viGetY());
            DL = viSetFillColor(DL, 0, 0, 0);
            DL = viFillScreen(DL);
        }
#else
        pcount = getPlayerCount();
#endif

        gSPClipRatio(DL++, FRUSTRATIO_2);

        for(i = 0; i < pcount; i++)
        {
#ifdef GE_MODDED_CHEATS
            if (lvlIsCoopEndCutscene())
            {
                set_cur_player(0);
            }
            else
#endif
            {
                set_cur_player(get_nth_player_from_shuffled(i));
            }

            viSetViewSize(g_CurrentPlayer->viewx, g_CurrentPlayer->viewy);
            viSetViewPosition(g_CurrentPlayer->viewleft, g_CurrentPlayer->viewtop);
            viSetFovY(g_CurrentPlayer->fovy);
            viSetAspect(g_CurrentPlayer->aspect);

            DL = viClearZBufCurrentPlayer(DL);
            DL = viSetupCurrentPlayerView(DL);

            if (get_debug_render_raster() == DEB_MOVE_VIEW)
            {
                DL = sub_GAME_7F091580(DL);
            }

            if (get_debug_render_raster() == DEB_STAN_VIEW)
            {
                DL = stanRenderDebugStanView(DL);
            }

            if (get_debug_render_raster() == DEB_BOND_VIEW)
            {
                DL = bondviewRenderDebugBondView(DL);
            }

#ifdef GE_MODDED_CHEATS
            if (!lvlIsCoopEndCutscene())
#endif
            {
                DL = viSetupScreensForNumPlayers(DL);
            }
            DL = skyRender(DL);

#ifdef GE_MODDED_CHEATS
            /*
             * R12 renderer-state isolation. Perfect Dark rebuilds the core
             * render state inside each player-view pass. GoldenEye historically
             * relied on the previous pass leaving compatible RSP/RDP state.
             * Campaign Co-Op makes that assumption much easier to violate, but
             * the same corruption can also occur with one player at particular
             * sky/background transitions. Restore the canonical textured,
             * Z-buffered pipeline after the sky before rendering rooms/props.
             * This is intentionally player-count agnostic.
             */
            gSPDisplayList(DL++, &dlFastPipelineSetup);
            gSPDisplayList(DL++, &dlZBufferGeometry);
#endif

            bgRoomVisibilityRelated();
            propsTick();
            chraiUpdateOnscreenPropCount();
            chrpropUpdateAutoaimTarget();
            chraiCheckUseHeldItems();

            if (bond_pressed_reload_activate() && bond_interact_object())
            {
                attempt_reload_item_in_hand(GUNRIGHT);
                attempt_reload_item_in_hand(GUNLEFT);
            }

            propsTickPlayer();
            DL = bgLevelRender(DL);

            if (get_debug_portal_flag())
            {
                DL = lvlPortalDebug7F0BDF10(DL);
            }

            if (get_debug_stan_problems_flag())
            {
                DL = sub_GAME_7F0B303C(DL);
            }

            if (get_debug_stanhit_flag())
            {
                DL = sub_GAME_7F0B3034(DL);
                DL = write_stan_tiles_in_yellow(DL);
            }

            if (get_debug_stanregion_flag())
            {
                DL = sub_GAME_7F0B3034(DL);
                DL = sub_GAME_7F0B312C(DL, -0x7FC0);
            }

            if (tokenFind(1, "-stanshow_"))
            {
                StandTilePoint *tile1 = stanMatchTileName(tokenFind(1, "-stanshow_"));
                if (tile1)
                {
                    DL = sub_GAME_7F0B3034(DL);
                    DL = sub_GAME_7F0B3024(DL, tile1, 0xFF0000FF);
                }
            }

            if (tokenFind(2, "-stanshow_"))
            {
                StandTilePoint *tile2 = stanMatchTileName(tokenFind(2, "-stanshow_"));
                if (tile2)
                {
                    DL = sub_GAME_7F0B3034(DL);
                    DL = sub_GAME_7F0B3024(DL, tile2, 0xFF00FF);
                }
            }

            if (tokenFind(3, "-stanshow_"))
            {
                StandTilePoint *tile3 = stanMatchTileName(tokenFind(3, "-stanshow_"));
                if (tile3)
                {
                    DL = sub_GAME_7F0B3034(DL);
                    DL = sub_GAME_7F0B3024(DL, tile3, 0xFFFF);
                }
            }

            setanimationdebugflag(getDebugMode() == DEB_SELANIM);
            DL = weaponRenderTracers(DL);

#if defined(VERSION_EU)
            bullet_moving_sparks_update(&DL, ZBUF_SURFACE);
#else /* VERSION_US, VERSION_JP, unspecified */
            bullet_sparks_render_all(&DL, ZBUF_SURFACE);
#endif
            DL = glassRenderShards(DL);
            DL = explosionRenderFlyingParticles(DL);

            if (

#if defined(BUGFIX_R1)
                cheatIsActive(CHEAT_INFINITE_AMMO) != 0
                && (
                    (getCurrentPlayerWeaponId(GUNRIGHT) != ITEM_WATCHLASER)
                    || (g_CurrentPlayer->trigger_down == 0)
                )
#else /* VERSION_US */
                cheatIsActive(CHEAT_INFINITE_AMMO) != 0
#endif
                )
            {
                set_max_ammo_for_cur_player();
            }

            if (get_debug_render_raster() == DEB_BOND_VIEW)
            {
                DL = maybe_mp_interface(DL);
            }
            else
            {
                DL = bondviewRemoved7F08BCB8(DL);
            }

            DL = mp_watch_menu_display(DL);
        }
    }

    gDPSetScissor(DL++, G_SC_NON_INTERLACE, 0, 0, viGetX(), viGetY());
#ifdef GE_MODDED_CHEATS
    if (getPlayerCount() > 1 && get_scenario() == SCENARIO_COOP)
    {
        DL = countdownTimerRender(DL);
    }
#endif

    return DL;
}


/**
 * Sets the modifier values for the level being loaded.
 * This covers the enemy accuracy, reaction speed, and similar values.
 *
 * address 0x7F0BE8D0
 */
void lvlSetMultipliersForDifficulty(void)
{
    if (g_SelectedDifficulty == DIFFICULTY_AGENT)
    {
        f32 armorDiff = currentPlayerGetHealth() + currentPlayerGetArmor();
        f32 damageMultiplier = 1.0f;

        if (armorDiff <= 0.125f)
        {
            damageMultiplier = 0.5f;
        }
        else if (armorDiff <= 0.6f)
        {
            damageMultiplier = (((armorDiff - 0.125f) * 0.5f) / 0.47500002f) + 0.5f;
        }

        F_80030B14 = 2.0f;
        F_80030B18 = 2.0f;
        g_AutogunPendingDamageTick = (0.5f * damageMultiplier);
        g_AutogunDamageScalar = (0.5f * damageMultiplier);
        F_80030B24 = 2.0f;
        g_AiAccuracyModifier = DEFAULT_AGENT_AI_ACCURACY_MODIFIER;
        g_AiDamageModifier = (DEFAULT_AGENT_AI_DAMAGE_MODIFIER * damageMultiplier);
        g_AiHealthModifier = 2.0f;
        g_SpExplosionDamageMult = (f32) (0.25f * damageMultiplier);
        difficulty = 1.5f;
        g_SoloAmmoMultiplier = DEFAULT_AGENT_SOLO_AMMO_MULTIPLIER;
        g_AiReactionSpeed = DEFAULT_AGENT_AI_REACTION_SPEED;
    }
    else if (g_SelectedDifficulty == DIFFICULTY_SECRET)
    {
        F_80030B14 = 1.0f;
        F_80030B18 = 1.0f;
        g_AutogunPendingDamageTick = 0.75f;
        g_AutogunDamageScalar = 0.75f;
        F_80030B24 = 1.0f;
        g_AiAccuracyModifier = DEFAULT_SECRET_AGENT_AI_ACCURACY_MODIFIER;
        g_AiDamageModifier = DEFAULT_SECRET_AGENT_AI_DAMAGE_MODIFIER;
        g_AiHealthModifier = 1.0f;
        g_SpExplosionDamageMult = 0.75f;

#if defined(BUGFIX_R1)
        if (j_text_trigger)
        {
            difficulty = 1.1f;
        }
        else
        {
            difficulty = 0.75f;
        }
#else
        // VERSION_US
        difficulty = 0.75f;
#endif

        g_SoloAmmoMultiplier = DEFAULT_SECRET_AGENT_SOLO_AMMO_MULTIPLIER;
        g_AiReactionSpeed = DEFAULT_SECRET_AGENT_AI_REACTION_SPEED;
    }
    else if (g_SelectedDifficulty == DIFFICULTY_00)
    {
        F_80030B14 = 1.0f;
        F_80030B18 = 1.0f;
        g_AutogunPendingDamageTick = 1.0f;
        g_AutogunDamageScalar = 1.0f;
        F_80030B24 = 1.0f;
        g_AiAccuracyModifier = DEFAULT_00_AGENT_AI_ACCURACY_MODIFIER;
        g_AiDamageModifier = DEFAULT_00_AGENT_AI_DAMAGE_MODIFIER;
        g_AiHealthModifier = 1.0f;
        g_SpExplosionDamageMult = 1.0f;

#if defined(BUGFIX_R1)
        if (j_text_trigger)
        {
            difficulty = 0.75f;
        }
        else
        {
            difficulty = 0.2f;
        }
#else
        // VERSION_US
        difficulty = 0.2f;
#endif

        g_SoloAmmoMultiplier = DEFAULT_00_AGENT_SOLO_AMMO_MULTIPLIER;
        g_AiReactionSpeed = DEFAULT_00_AGENT_AI_REACTION_SPEED;
    }
    else if (g_SelectedDifficulty == DIFFICULTY_007)
    {
        F_80030B14 = 1.0f;
        F_80030B18 = 1.0f;
        g_AutogunPendingDamageTick = 1.0f;
        g_AutogunDamageScalar = 1.0f;
        F_80030B24 = 1.0f;
        g_AiAccuracyModifier = DEFAULT_007_AI_ACCURACY_MODIFIER;
        g_AiDamageModifier = DEFAULT_007_AI_DAMAGE_MODIFIER;
        g_AiHealthModifier = 1.0f;
        g_SpExplosionDamageMult = 1.0f;
        difficulty = 1.0f;
        g_SoloAmmoMultiplier = DEFAULT_007_SOLO_AMMO_MULTIPLIER;
        g_AiReactionSpeed = DEFAULT_007_AI_REACTION_SPEED;
    }
}


/**
 * Multiplayer method. Manages a lot of stuff.
 * Tracks you-only-live-twice kills/deaths.
 * Lots of debug code.
 *
 * Address: 0x7F0BEB88 (NTSC).
 * Address: 0x7F0BF7AC (NTSC-J).
 * Address: 0x7F0BDFAC (PAL).
 */
void lvlManageMpGame(void)
{
    tlbmanageResetCurrentEntriesCount();

    if (g_ControlsLockedFlag != 0)
    {
        g_ClockTimer = 0;
    }
    else if (checkGamePaused() != 0)
    {
        g_ClockTimer = 0;
    }
    else
    {
        g_ClockTimer = speedgraphframes;
        D_80048380 += 1;
    }

#ifdef VERSION_US
    g_GlobalTimerDelta = (f32) g_ClockTimer;
#else
    g_JP_GlobalTimerDelta = (f32) g_ClockTimer;
#ifdef VERSION_EU
    g_GlobalTimerDelta = g_JP_GlobalTimerDelta * 1.2f;
#else
    g_GlobalTimerDelta = g_JP_GlobalTimerDelta;
#endif
#endif
    g_GlobalTimer += g_ClockTimer;
    if ((g_CurrentStageToLoad != LEVELID_TITLE) && (D_80048394 == 0) && (g_ClockTimer > 0))
    {
        if (g_AppendCheatSinglePlayer != 0
#ifdef GE_MODDED_CHEATS
            || g_CheatActivated[CHEAT_NO_RADAR_MP]
#endif
        )
        {
            s32 i;
            for (i = 1; i != CHEAT_INVALID; i++)
            {
                if (g_CheatActivated[i] && !cheatIsEnemyRockets(i))
                {
                    cheatButtonTurnOnCheatForPlayers(i);
                }
            }
        }
    }

    if ((getPlayerCount() >= 2) && (g_CurrentStageToLoad != LEVELID_TITLE))
    {
        if (get_mission_state() == MISSION_STATE_6)
        {
            s32 i;
            s32 mp_alive_count;
            s32 mp_player_field424_count;

            mp_alive_count = 0;
            mp_player_field424_count = 0;

            for (i=0; i<getPlayerCount(); i++)
            {
                if (g_playerPointers[i]->bonddead != FALSE)
                {
                    mp_alive_count++;
                    if (g_playerPointers[i]->redbloodfinished)
                    {
                        mp_player_field424_count++;
                    }
                }
            }

            if ((mp_alive_count > 0) && (mp_alive_count == mp_player_field424_count))
            {
#ifdef GE_MODDED_CHEATS
                /* RC5: the Silo/Bunker II terminal explosion failure owns the
                 * death-music state until every participating player has
                 * finished the complete death/fade lifecycle.  Returning to
                 * MISSION_STATE_1 here would briefly restart ordinary stage
                 * music between staggered Co-Op deaths and would also allow
                 * the campaign dynamic-music slot scheduler to fight the
                 * death state. */
                if (!lvlIsCoopPermanentExplosionFailure())
#endif
                {
                    set_missionstate(MISSION_STATE_1);
                }
            }
        }

#ifdef GE_MODDED_CHEATS
        /* R21: clear AI references only after a player's actual death/fade
         * lifecycle is complete, matching PD's Co-Op semantics.  Repeating
         * this check is harmless; released hostile targets can then acquire a
         * living player on their next normal AI update. */
        if (gamemode == GAMEMODE_MULTI && get_scenario() == SCENARIO_COOP)
        {
            s32 i;

            for (i = 0; i < getPlayerCount(); i++)
            {
                if (g_playerPointers[i]->bonddead
                    && g_playerPointers[i]->redbloodfinished
                    && g_playerPointers[i]->deathanimfinished
                    && g_playerPointers[i]->colourfadetimemax60 < 0.0f)
                {
                    chrCoopClearRefsToPlayer(i);
                }
            }
        }

        /* Permanent explosions in Silo/Bunker II are terminal in campaign
         * Co-Op.  Wait until every player's complete death sequence is over,
         * then stop only the ordinary level music and enter the game's real
         * multiplayer-style GAME OVER / START TO EXIT state. */
        if (!g_gameOverFlag && lvlIsCoopPermanentExplosionFailure())
        {
            s32 i;
            s32 fullydead = 0;

            for (i = 0; i < getPlayerCount(); i++)
            {
                if (g_playerPointers[i]->bonddead
                    && g_playerPointers[i]->redbloodfinished
                    && g_playerPointers[i]->deathanimfinished
                    && g_playerPointers[i]->colourfadetimemax60 < 0.0f)
                {
                    fullydead++;
                }
            }

            if (fullydead == getPlayerCount())
            {
                mission_failed_or_aborted = TRUE;
                g_isBondKIA = TRUE;
                /* RC6: snapshot the campaign result before the shared Game
                 * Over lifecycle can transition back to the frontend. The
                 * Game Over initializer owns the audio handoff itself. */
                frontStoreCoopMissionReportStats();
                mpStartCoopGameOver();
            }
        }
#endif

        if (g_MpTime > 0)
        {
            s32 current_time;
            s32 sp180;
            s32 i;
            current_time = D_80048394;
            sp180 = g_ClockTimer + D_80048394;

#ifdef VERSION_EU
            if ((D_80048394 < (g_MpTime - 0xBB8)) && (sp180 >= (g_MpTime - 0xBB8)))
#else
            if ((D_80048394 < (g_MpTime - 0xE10)) && (sp180 >= (g_MpTime - 0xE10)))
#endif
            {
                for (i = 0; i < getPlayerCount(); i++)
                {
                    set_cur_player(i);
#ifdef VERSION_US
                    HUDMESSAGEBOTTOM("One minute left");
#else
                    HUDMESSAGEBOTTOM(langGet(0xB044));
#endif
                }
            }

            // sound alarm when game is about to end (10 seconds before end)
#ifdef VERSION_EU
            if ((sp180 >= (g_MpTime - 0x1F4)) && (g_MpSoundStateRelated == 0) && (lvlGetControlsLockedFlag() == 0))
#else
            if ((sp180 >= (g_MpTime - 0x258)) && (g_MpSoundStateRelated == 0) && (lvlGetControlsLockedFlag() == 0))
#endif
            {
                sndPlaySfx(g_musicSfxBufferPtr, ALARM1_SFX, &g_MpSoundStateRelated);
            }

            // stop alarm
            if (lvlGetControlsLockedFlag() != 0)
            {
                if ((g_MpSoundStateRelated != NULL) && (sndGetPlayingState(g_MpSoundStateRelated) != 0))
                {
                    sndDeactivate(g_MpSoundStateRelated);
                }
            }

            if ((current_time < g_MpTime) && (sp180 >= g_MpTime))
            {
                mpCalculateAwards(FALSE);
            }
        }

        // when playing with a kill limit, g_MpPoint is not zero
        if ((g_MpPoint > 0) && (g_ClockTimer != 0))
        {
            s32 var_player_count1;
            s32 i;
            s32 mp_player_currently_in_dying_animation;
            s32 mp_players_over_point_limit;

            var_player_count1 = getPlayerCount();
            mp_player_currently_in_dying_animation = 0;
            mp_players_over_point_limit = 0;

            for (i = 0; i < var_player_count1; i++)
            {
                if (g_playerPointers[i]->bonddead != FALSE &&
                    (g_playerPointers[i]->redbloodfinished == FALSE || g_playerPointers[i]->deathanimfinished == FALSE || g_playerPointers[i]->colourfadetimemax60 >= 0.0f))
                {
                    mp_player_currently_in_dying_animation++;
                }

                if (get_points_for_mp_player(i) >= g_MpPoint)
                {
                    // counts players over kill limit
                    mp_players_over_point_limit++;
                }
            }

            if (mp_players_over_point_limit > 0)
            {
                if (mp_player_currently_in_dying_animation == 0)
                {
                    // end game after dying players are finished dying
                    mpCalculateAwards(FALSE);
                }
                else
                {
                    // this will cause the game to freeze players, to stop them from moving once game ended
                    mpwatchSetStopPlayFlag();
                }
            }
        }


        // YOLT scenario: end-of-game tracking.
        if ((get_scenario() == SCENARIO_YOLT) && (g_ClockTimer != 0))
        {
            s32 player_count;
            s32 killed_count;
            s32 not_dead_count;
            s32 fully_dead_total;
            s32 killed_total;
            s32 i;
            s32 j;

            player_count = getPlayerCount();
            killed_total = 0;
            fully_dead_total = 0;

            for (i = 0; i < player_count; i++)
            {
                killed_count = 0;
                not_dead_count = 0;

                for (j = 0; j < player_count; j++)
                {
                    if (g_playerPointers[j]->bonddead == 0)
                    {
                        not_dead_count++;
                    }
                    killed_count += g_playerPlayerData[j].kill_counts[i];
                }

                if (killed_count >= 2)
                {
                    if (g_playerPlayerData[i].order_out_in_yolt == 0)
                    {
                        g_playerPlayerData[i].order_out_in_yolt = (u8) (not_dead_count + 1);
                    }

                    killed_total++;

                    if (g_playerPointers[i]->redbloodfinished
                        && g_playerPointers[i]->deathanimfinished
                        && g_playerPointers[i]->colourfadetimemax60 < 0.0f)
                    {
                        fully_dead_total++;
                    }
                }
            }

            if (fully_dead_total >= player_count - 1)
            {
                mpCalculateAwards(FALSE);
            }
            else if (killed_total >= player_count - 1)
            {
                mpwatchSetStopPlayFlag();
            }
        } // end YOLT

        if (0)
        {
            char debug_buf[268];
            sprintf(debug_buf, "setdetail %d %d %d %d %d %d %d %d %d", 0, 0, 0, 0, 0, 0, 0, 0, 0);
        }
    }

    D_80048394 = D_80048394 + g_ClockTimer;
#ifdef VERSION_EU
    g_CurrentMultiPlayerSec = (f32) (D_80048394) / 50.0f;
#else
    g_CurrentMultiPlayerSec = (f32) (D_80048394) / 60.0f;
#endif
    D_800483A8 = D_800483A8 + g_ClockTimer;
#ifdef VERSION_EU
    g_CurrentMultiPlayerMin = (f32) (D_800483A8) / 50.0f;
#else
    g_CurrentMultiPlayerMin = (f32) (D_800483A8) / 60.0f;
#endif

    if (joyGetButtonsPressedThisFrame(PLAYER_1, ANY_BUTTON))
    {
        D_80048388 = 0;
        D_80048390 = 0;
    }
    else
    {
        D_80048390 = D_80048390 + g_ClockTimer;

#ifdef VERSION_EU
        if (D_80048390 >= 0x5DC)
#else
        if (D_80048390 >= 0x708)
#endif
        {
            D_80048388 = 1;
        }
    }

    if (D_80048388 != 0)
    {
        D_8004838C += g_ClockTimer;
    }
    else
    {
        D_800483B0 = D_800483B0 + g_ClockTimer;
#ifdef VERSION_EU
        g_StageTimeSec = (f32) (D_800483B0) / 50.0f;
#else
        g_StageTimeSec = (f32) (D_800483B0) / 60.0f;
#endif
        D_800483B8 = D_800483B8 + g_ClockTimer;
#ifdef VERSION_EU
        g_PowerOnTimeSec = (f32) (D_800483B8) / 50.0f;
#else
        g_PowerOnTimeSec = (f32) (D_800483B8) / 60.0f;
#endif
    }

    viSetUseZBuf(1);

    if (g_CurrentStageToLoad == LEVELID_TITLE)
    {
        cheat_buttons_mp_related();
        menu_init();
        langTick();
    }
    else
    {
        sub_GAME_7F09BBBC();
        lvlSetMultipliersForDifficulty();
        updateRoomStatusFlags();
        sub_GAME_7F092E50();
        skyTick();
#ifdef VERSION_EU
        bullet_moving_spark_create();
#else
        bullet_sparks_update_all();
#endif
        update_bullet_casings();
        update_broken_windows();
        explosionUpdateFlyingParticles();
        chrpropTick();
        reset_all_music_slots();
        langTick();

        if ((get_debug_joy2detailedit_flag() != 0) && (D_800483C8 == 0))
        {
            s32 i;
            D_800483C8 = (struct LvlMpUnknown *)mempAllocBytesInBank(0x3000, MEMPOOL_STAGE);
            if (D_800483C8 != 0)
            {
                for (i=0; i<3000; i++)
                {
                    D_800483C8[i].unk_0 = 0xff;
                    D_800483C8[i].unk_1 = (D_800483C8[i].unk_1 & 0xFF1F) | 0x20;
                    D_800483C8[i].unk_1 = (D_800483C8[i].unk_1 & 0xFFE3) | 4;
                }
            }
        }

        if (get_debug_portal_flag() != 0)
        {
            lvlPortalDebug7F0BDF10(0);
        }

        switch (getDebugMode())
        {
            case 4:
            {
                if (joyGetButtonsPressedThisFrame(PLAYER_1, L_CBUTTONS))
                {
                    sub_GAME_7F0AF630(-1);
                    debugStanView(0, 0, 0);
                }

                if (joyGetButtonsPressedThisFrame(PLAYER_1, R_CBUTTONS))
                {
                    sub_GAME_7F0AF630(1);
                    debugStanView(0, 0, 0);
                }
            }
            break;

            case 8:
            {
                if (joyGetButtonsPressedThisFrame(PLAYER_1, L_CBUTTONS))
                {
                    chrDecrementAnimationTablePointerCount();
                }

                if (joyGetButtonsPressedThisFrame(PLAYER_1, R_CBUTTONS))
                {
                    chrIncrementAnimationTablePointerCount();
                }

                if (joyGetButtonsPressedThisFrame(PLAYER_1, L_TRIG))
                {
                    chrToggleD_8002C90C();
                }

                sub_GAME_7F022EE0(joyGetButtons(PLAYER_1, R_TRIG) != 0);
            }
            break;
        }

    }
    {
        struct ALBank * sfx;
        s16 sound_index;
        s16 *sound_index_ptr;
        switch (getDebugMode())
        {
            case 0x38:
            {
                s32 sp30;
                s32 sp2C;
                sp30 = viGetHorizontalOffset();
                sp2C = viGet800232A0();
                if (joyGetButtons(PLAYER_1, D_CBUTTONS))
                {
                    sp2C += 1;
                }
                if (joyGetButtons(PLAYER_1, U_CBUTTONS))
                {
                    sp2C += -1;
                }
                if (joyGetButtons(PLAYER_1, R_CBUTTONS))
                {
                    sp30 += 1;
                }
                if (joyGetButtons(PLAYER_1, L_CBUTTONS))
                {
                    sp30 += -1;
                }
                viSetHorizontalOffset(sp30);
                viSet800232A0(sp2C);
            }
            break;

            case 0xc:
            {
                if (joyGetButtonsPressedThisFrame(PLAYER_1, (L_JPAD | L_CBUTTONS)))
                {
                    lvlMusicAppendPlayEndTheme();
                }

                if (joyGetButtonsPressedThisFrame(PLAYER_1, (R_JPAD | R_CBUTTONS)))
                {
                    lvlMusicAppendPlaySoloDeathShort();
                }

                if (joyGetButtonsPressedThisFrame(PLAYER_1, D_JPAD))
                {
                    musicTrack1Stop();
                }

                if (joyGetButtonsPressedThisFrame(PLAYER_1, B_BUTTON))
                {
                    musicTrack1SaveCurrentVolumeAsTrackDefault();
                }
            }
            break;
            case 0xd:
            {
#ifdef VERSION_US
                sound_index_ptr = &g_DebugMpGameSoundFxIndex;

                if (joyGetButtonsPressedThisFrame(PLAYER_1, (D_JPAD | L_JPAD | L_TRIG | L_CBUTTONS)))
                {
                    sound_index = g_DebugMpGameSoundFxIndex - 1;\
                    sfx = g_musicSfxBufferPtr;\
                    *sound_index_ptr = sound_index;
                    sndPlaySfx(sfx, sound_index, NULL);
                }

                if (joyGetButtonsPressedThisFrame(PLAYER_1, (U_JPAD | R_JPAD | R_TRIG | R_CBUTTONS)))
                {
                    sound_index = g_DebugMpGameSoundFxIndex + 1;\
                    sfx = g_musicSfxBufferPtr;\
                    *sound_index_ptr = sound_index;
                    sndPlaySfx(sfx, sound_index, NULL);
                }

                if (joyGetButtonsPressedThisFrame(PLAYER_1, D_CBUTTONS))
                {
                    sndDeactivateAllSfxByFlag_1();
                }

                if (joyGetButtonsPressedThisFrame(PLAYER_1, U_CBUTTONS))
                {
                    sound_index = g_DebugMpGameSoundFxIndex;\
                    sfx = g_musicSfxBufferPtr;
                    sndPlaySfx(sfx, sound_index, NULL);
                }
#else
                sound_index_ptr = &g_DebugMpGameSoundFxIndex;

                if (joyGetButtonsPressedThisFrame(PLAYER_1, (D_JPAD | L_JPAD | L_TRIG | L_CBUTTONS)))
                {
                    sound_index = g_DebugMpGameSoundFxIndex - 1;\
                    sfx = g_musicSfxBufferPtr;\
                    *sound_index_ptr = sound_index;
                    sndPlaySfx(sfx, sound_index, NULL);
                }

                if (joyGetButtonsPressedThisFrame(PLAYER_1, (U_JPAD | R_JPAD | R_TRIG | R_CBUTTONS)))
                {
                    sound_index = g_DebugMpGameSoundFxIndex + 1;\
                    sfx = g_musicSfxBufferPtr;\
                    *sound_index_ptr = sound_index;
                    sndPlaySfx(sfx, sound_index, NULL);
                }

                if (joyGetButtonsPressedThisFrame(PLAYER_1, D_CBUTTONS))
                {
                    sndDeactivateAllSfxByFlag_1();
                }

                if (joyGetButtonsPressedThisFrame(PLAYER_1, U_CBUTTONS))
                {
                    sound_index = g_DebugMpGameSoundFxIndex;\
                    sfx = g_musicSfxBufferPtr;
                    sndPlaySfx(sfx, sound_index, NULL);
                }
#endif
            }
            break;

            default:
            break;
        }
    }
}



/**
 * Assumes a debug mode is present, and handles debug edit intro, debug stan edit, debug bond "view."
 * By default, the DEB_BOND_VIEW path is chosen without debug info.
 * This updates the player viewport(s), and handles player movement.
 *
 * Multiplayer:
 * Updates distance_traveled and possibly (depending on scenario) have_token_or_goldengun.
 *
 * US Address 0x7F0BF800.
 * EU address 7F0BEC44.
 */
void lvlViewMoveTick(void)
{
    s8 local_player_number;
    s32 padding;
    f32 temp_f0;
    f32 temp_f2;

    local_player_number = get_cur_playernum();
    cheat_buttons_mp_related();

    switch (get_debug_freeze_processing())
    {
        case 0:
        {
            if ((getDebugMode() == DEB_MOVE_VIEW) || ((getDebugMode() == DEB_INTRO_EDIT) && (D_80036ABC < 0)))
            {
                debugFreeCamera(joyGetStickX(local_player_number), joyGetStickY(local_player_number), joyGetButtons(local_player_number, ANY_BUTTON));
            }
            else
            {
                debugFreeCamera(joyGetStickX(local_player_number), joyGetStickY(local_player_number), 0);
            }
        }
        break;

        case 1:
        {
            if (getDebugMode() == DEB_STAN_VIEW)
            {
                debugStanView(joyGetStickX(local_player_number), joyGetStickY(local_player_number), joyGetButtons(local_player_number, ANY_BUTTON));
            }
            else
            {
                debugStanView(joyGetStickX(local_player_number), joyGetStickY(local_player_number), 0);
            }
        }
        break;

        case 2:
        {
            if (getDebugMode() == DEB_BOND_VIEW)
            {
                bondviewMovePlayerUpdateViewport(joyGetStickX(local_player_number), joyGetStickY(local_player_number), joyGetButtons(local_player_number, ANY_BUTTON));
            }
            else
            {
                bondviewMovePlayerUpdateViewport(joyGetStickX(local_player_number), joyGetStickY(local_player_number), 0);
            }

            mpwatchMenuTick();
        }
        break;
    }

    temp_f0 = g_CurrentPlayer->prop->pos.x - g_CurrentPlayer->bondprevpos.x;
    temp_f2 = g_CurrentPlayer->prop->pos.z - g_CurrentPlayer->bondprevpos.z;

    g_playerPerm->distance_traveled += sqrtf((temp_f0 * temp_f0) + (temp_f2 * temp_f2));

    if (get_scenario() == SCENARIO_TLD)
    {
        if (bondinvIsAliveWithFlag())
        {
            if (getCurrentPlayerWeaponId(GUNRIGHT) != ITEM_TOKEN)
            {
                currentPlayerEquipWeaponWrapper(GUNRIGHT, ITEM_TOKEN);

                if(1);

                if (g_CurrentPlayer->hands[GUNRIGHT].weapon_action_state == GUN_ANIM_STATE_FIRE)
                {
                    g_CurrentPlayer->hands[GUNRIGHT].weapon_action_state = GUN_ANIM_STATE_SWITCH_LOWER;
                }
            }

            g_playerPerm->flag_counter += g_ClockTimer;
            g_playerPerm->have_token_or_goldengun = 1;
        }
        else
        {
            g_playerPerm->have_token_or_goldengun = 0;
        }

        return;
    }

    if (get_scenario() == SCENARIO_MWTGG)
    {
        if (bondinvHasGoldenGun())
        {
            g_playerPerm->have_token_or_goldengun = 1;
        }
        else
        {
            g_playerPerm->have_token_or_goldengun = 0;
        }

        return;
    }
}





void lvlUnloadStageTextData(void)
{
    if (g_MpSoundStateRelated != NULL)
    {
        if (sndGetPlayingState(g_MpSoundStateRelated) != AL_STOPPED)
        {
            sndDeactivate(g_MpSoundStateRelated);
        }
    }

    if (g_CurrentStageToLoad != LEVELID_TITLE)
    {
        langClearBank(langGetLangBankIndexFromStagenum(g_CurrentStageToLoad));
        set_favorite_weapon_for_every_player();
#ifdef GE_MODDED_CHEATS
        frontStoreCoopMissionReportStats();
#endif
    }

    cheatDisableAllCheats();
    cleanupGuardData();
    cleanupObjectSounds();
    cleanupExplosions();
    cleanup_window_pieces();
    cleanup_REMOVED_();
    cleanupAlarms();
    cleanupObjects(g_CurrentStageToLoad);
    cleanupObjectives();
    cleanupSFXRelated();
    cleanupplayersoundrelated();
    set_missionstate_zero();
    cleanup_rooms();
}


void lvlSetControlsLockedFlag(s32 arg0)
{
    #if defined(BUGFIX_R1)
    if ((arg0 != 0) && (g_ControlsLockedFlag == 0))
    {
        joyRumblePakStop();
    }
    #endif

    g_ControlsLockedFlag = arg0;
}


s32 lvlGetControlsLockedFlag(void)
{
    return g_ControlsLockedFlag;
}


DIFFICULTY lvlGetSelectedDifficulty(void)
{
    return g_SelectedDifficulty;
}


void lvlSetSelectedDifficulty(DIFFICULTY arg0)
{
    g_SelectedDifficulty = arg0;
}

void lvlSetMpTime(s32 arg0)
{
    g_MpTime = arg0;
}


void lvlSetMpPoint(s32 arg0)
{
    g_MpPoint = arg0;
}


f32 lvlGetCurrentMultiPlayerSec(void)
{
    return g_CurrentMultiPlayerSec;
}


f32 lvlGetCurrentMultiPlayerMin(void)
{
    return g_CurrentMultiPlayerMin;
}


/**
 * Unreferenced.
 */
f32 lvlGetStageTimeSec(void)
{
    return g_StageTimeSec;
}


/**
 * Unreferenced.
 */
f32 lvlGetPowerOnTimeSec(void)
{
    return g_PowerOnTimeSec;
}


