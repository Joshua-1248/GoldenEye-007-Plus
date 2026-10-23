#include <ultra64.h>
#include <music.h>
#include "music_0D2720.h"
#include "options.h"
#include "mp_music.h"
#include "lv.h"
#ifdef GE_MODDED_CHEATS
#include <memp.h>
#include "file2.h"
#endif

#ifdef VERSION_EU
#define MP_MUSIC_FRAMERATE 50
#else
#define MP_MUSIC_FRAMERATE 60
#endif

// bss
//CODE.bss:8008C600
s32 stageMusicID;
//CODE.bss:8008C604
s32 dword_CODE_bss_8008C604;

//CODE.bss:8008C608
s32 music_slot_active_0[4];

//CODE.bss:8008C618
s32 music_slot_minutes_0[4];

//CODE.bss:8008C628
s32 music_slot_seconds_0[4];


// data
//D:800484C0
s32 mission_state = MISSION_STATE_0;

#ifdef GE_MODDED_CHEATS
/* Music resident buffer is only 2 KiB; TND's decoded sequence is 5652 bytes.
 * Use an explicitly bounded, stage-owned additional buffer only while needed.
 * Do NOT enlarge the 296-KiB permanent pool or overwrite Track 3 data. */
#define MOD_WATCH_SEQ_BYTES 0x1800
#define MOD_WATCH_STAGE_MARGIN 0x1000
/* Alternate Track 1 buffer lives outside the fixed music/permanent heap.
 * Goldfinger folders decodes to 6764 bytes. Alignment and input staging
 * require > 6848 bytes; 0x1c00 provides guarded headroom. */
#define MOD_MAINMENU_SEQ_BYTES 0x1c00
#define MOD_MAINMENU_RETAIL_TRACK1_BYTES 6344
#define MOD_MAINMENU_STAGE_MARGIN 0x1000
extern s32 g_musicXTrack1CurrentTrackNum;
extern u8 *g_musicXTrack1SeqData;
extern u8 *g_musicXTrack2SeqData;
extern ALCSPlayer *g_musicXTrack1SeqPlayer;
extern ALCSPlayer *g_musicXTrack2SeqPlayer;
extern u16 g_musicTrackLength[NUM_MUSIC_TRACKS + 1];
extern u16 g_musicTrackCompressedLength[NUM_MUSIC_TRACKS];
extern s32 needmemallocation;
static u8 *g_ModWatchOriginalTrack2Buffer;
static u8 *g_ModWatchStageBuffer;
static u8 *g_ModMainMenuOriginalTrack1Buffer;
static u8 *g_ModMainMenuStageBuffer;

s32 modMainMenuThemeTrackId(void)
{
    switch (modMainMenuThemeGetSelection())
    {
        case 1: return M_FOLDERS_GF;
        case 2: return M_FOLDERS_TMD;
        case 3: return M_FOLDERS_TSWLM;
        default: return M_FOLDERS;
    }
}

void modMainMenuThemePlay(void)
{
    s32 track = modMainMenuThemeTrackId();
    s32 useStageBuffer = FALSE;

    if (track != M_FOLDERS)
    {
        u32 decoded = ((u32)g_musicTrackLength[track] + 15u) & ~15u;
        u32 encoded = ((u32)g_musicTrackCompressedLength[track] + 15u) & ~15u;
        u32 staging = (NUM_MUSIC_TRACKS + 15u) & ~15u;

        if (encoded > decoded + staging
            || decoded + staging > MOD_MAINMENU_SEQ_BYTES)
            track = M_FOLDERS;
        else if (decoded + staging > MOD_MAINMENU_RETAIL_TRACK1_BYTES)
            useStageBuffer = TRUE;
    }

    if (useStageBuffer)
    {
        if (!g_ModMainMenuStageBuffer)
        {
            /* The stage allocator can redirect overflow into permanent
             * memory. Decline the large custom track instead. */
            if (needmemallocation
                || mempGetBankSizeLeft(MEMPOOL_STAGE)
                    < MOD_MAINMENU_SEQ_BYTES + MOD_MAINMENU_STAGE_MARGIN)
                track = M_FOLDERS;
            else
                g_ModMainMenuStageBuffer = mempAllocBytesInBank(MOD_MAINMENU_SEQ_BYTES, MEMPOOL_STAGE);
        }
        if (g_ModMainMenuStageBuffer)
        {
            if (!g_ModMainMenuOriginalTrack1Buffer)
                g_ModMainMenuOriginalTrack1Buffer = g_musicXTrack1SeqData;
            g_musicXTrack1SeqData = g_ModMainMenuStageBuffer;
        }
    }

    /* TND/TSWLM fit the existing Track 1 allocation. Only Goldfinger
     * needs the stage buffer; original music keeps its retail path. */
    if ((!useStageBuffer || track == M_FOLDERS) && g_ModMainMenuOriginalTrack1Buffer)
        g_musicXTrack1SeqData = g_ModMainMenuOriginalTrack1Buffer;

    /* The old frontend explicitly restarted a naturally completed song. */
    if (g_musicXTrack1CurrentTrackNum != track
        || alCSPGetState(g_musicXTrack1SeqPlayer) == AL_STOPPED)
        musicTrack1Play(track);
}

s32 modWatchThemeTrackId(void)
{
    switch (modWatchThemeGetSelection())
    {
        case 1: return M_WATCH_GF;
        case 2: return M_WATCH_TMD;
        case 3: return M_WATCH_TSWLM;
        default: return M_WATCH;
    }
}

void modWatchThemePlay(void)
{
    s32 track = modWatchThemeTrackId();

    if (track != M_WATCH)
    {
        u32 decoded = ((u32)g_musicTrackLength[track] + 15u) & ~15u;
        u32 encoded = ((u32)g_musicTrackCompressedLength[track] + 15u) & ~15u;
        u32 staging = (NUM_MUSIC_TRACKS + 15u) & ~15u;

        /* The original decompressor stages compressed input at
         * decoded+table-staging-encoded. Both ends must remain in-bounds. */
        if (encoded > decoded + staging
            || decoded + staging > MOD_WATCH_SEQ_BYTES)
            track = M_WATCH;
    }

    if (track != M_WATCH)
    {
        if (!g_ModWatchStageBuffer)
        {
            /* Without enough stage memory, leave original music intact.
             * memp's overflow behavior is a fatal trap, not a NULL return. */
            /* A stage-allocation overflow can redirect into the PERMANENT
             * bank.  Refuse before entering that allocator fallback. */
            if (needmemallocation
                || mempGetBankSizeLeft(MEMPOOL_STAGE)
                    < MOD_WATCH_SEQ_BYTES + MOD_WATCH_STAGE_MARGIN)
                track = M_WATCH;
            else
                g_ModWatchStageBuffer = mempAllocBytesInBank(MOD_WATCH_SEQ_BYTES, MEMPOOL_STAGE);
        }
        if (g_ModWatchStageBuffer)
        {
            if (!g_ModWatchOriginalTrack2Buffer)
                g_ModWatchOriginalTrack2Buffer = g_musicXTrack2SeqData;
            g_musicXTrack2SeqData = g_ModWatchStageBuffer;
        }
    }

    if (track == M_WATCH && g_ModWatchOriginalTrack2Buffer)
        g_musicXTrack2SeqData = g_ModWatchOriginalTrack2Buffer;

    musicTrack2Play(track);
}

void modWatchThemeApplyDuringPause(void)
{
    /* A confirmed selection replaces only the active watch music. */
    if (mission_state == MISSION_STATE_3)
        modWatchThemePlay();
}

void modWatchThemeStageReset(void)
{
    /* The same stage-reset seam must retire the optional main-menu buffer
     * before the stage bank is reused. Leave unrelated Track 1 music alone. */
    if (g_ModMainMenuStageBuffer)
    {
        if (g_musicXTrack1SeqData == g_ModMainMenuStageBuffer)
        {
            musicTrack1Stop();
            while (alCSPGetState(g_musicXTrack1SeqPlayer) != AL_STOPPED)
                ;
            g_musicXTrack1SeqData = g_ModMainMenuOriginalTrack1Buffer;
        }
        g_ModMainMenuStageBuffer = NULL;
        g_ModMainMenuOriginalTrack1Buffer = NULL;
    }
    if (g_ModWatchStageBuffer)
    {
        /* Restore the resident pointer before stage pool memory is reset. */
        if (g_musicXTrack2SeqData == g_ModWatchStageBuffer)
        {
            musicTrack2Stop();
            while (alCSPGetState(g_musicXTrack2SeqPlayer) != AL_STOPPED)
                ;
            g_musicXTrack2SeqData = g_ModWatchOriginalTrack2Buffer;
        }
        g_ModWatchStageBuffer = NULL;
        g_ModWatchOriginalTrack2Buffer = NULL;
    }
}
#endif



u16 sub_GAME_7F0C0BF0(void)
{
    return get_mTrack2Vol();
}

u16 sub_GAME_7F0C0C10(void)
{
    return call_sndGetSfxSlotFirstNaturalVolume();
}


MISSION_STATE_ID get_mission_state(void)
{
    return mission_state;
}



void set_missionstate(MISSION_STATE_ID arg0)
{
    s32 old_mission_state = mission_state;

    mission_state = arg0;

    switch (old_mission_state)
    {
        case MISSION_STATE_0:
        switch (arg0)
        {
#if defined(VERSION_US)
            case MISSION_STATE_0:
                return;
            case MISSION_STATE_1:
                musicTrack1ApplySeqpVol(sub_GAME_7F0C0BF0());
                g_musicXTrack1Fade = 0;
                musicTrack1Play(getmusictrack_or_randomtrack(stageMusicID));
                return;
            case MISSION_STATE_2:
                break;
            case MISSION_STATE_3:
                break;
            case MISSION_STATE_4: // switch 2
                musicTrack1ApplySeqpVol(sub_GAME_7F0C0BF0());
                g_musicXTrack1Fade = 0;
                musicTrack1Play(getmusictrack_or_randomtrack(stageMusicID));
                musicTrack3ApplySeqpVol(sub_GAME_7F0C0C10());
                g_musicXTrack3Fade = 0;
                musicTrack3Play(musicGetBgTrackForStage(stageMusicID));
                return;
            case MISSION_STATE_5:
                break;
            case MISSION_STATE_6:
                break;
#endif

#if defined(VERSION_JP) || defined(VERSION_EU)
            case MISSION_STATE_0:
                musicTrack1Stop();
                musicTrack2Stop();
                musicTrack3Stop();
                return;
            case MISSION_STATE_1: // switch 2
                musicTrack1ApplySeqpVol(sub_GAME_7F0C0BF0());
                g_musicXTrack1Fade = 0;
                musicTrack1Play(getmusictrack_or_randomtrack(stageMusicID));
                return;
            case MISSION_STATE_2:
                break;
            case MISSION_STATE_3:
                break;
            case MISSION_STATE_4:
                musicTrack1ApplySeqpVol(sub_GAME_7F0C0BF0());
                g_musicXTrack1Fade = 0;
                musicTrack1Play(getmusictrack_or_randomtrack(stageMusicID));
                musicTrack3ApplySeqpVol(sub_GAME_7F0C0C10());
                g_musicXTrack3Fade = 0;
                musicTrack3Play(musicGetBgTrackForStage(stageMusicID));
                return;
                break;
            case MISSION_STATE_5:
                break;
            case MISSION_STATE_6:
                break;
#endif
        }

        case MISSION_STATE_1:
        switch (arg0)
        {
            case MISSION_STATE_0:
                musicTrack1Stop();
                musicTrack2Stop();
                musicTrack3Stop();
                return;
            case MISSION_STATE_1:
                break;
            case MISSION_STATE_2:
                musicTrack2ApplySeqpVol(sub_GAME_7F0C0BF0());
                g_musicXTrack2Fade = 0;
                musicTrack2Play(musicGetXTrackForStage(stageMusicID));
                musicTrack1FadeOut(0.5f);
                return;
            case MISSION_STATE_3:
                musicTrack2ApplySeqpVol(sub_GAME_7F0C0BF0());
                g_musicXTrack2Fade = 0;
                #ifdef GE_MODDED_CHEATS
                modWatchThemePlay();
#else
                musicTrack2Play(0x18);
#endif
                musicTrack1FadeOut(0.5f);
                return;
            case MISSION_STATE_5:
                break;
            case MISSION_STATE_6:
                musicTrack1FadeOut(0.02f);
                musicTrack2ApplySeqpVol(sub_GAME_7F0C0BF0());
                g_musicXTrack2Fade = 0;
                musicTrack2Play(0x3A);
                return;
        }

        case MISSION_STATE_2:
        switch (arg0)
        {
            case MISSION_STATE_0:
                musicTrack1Stop();
                musicTrack2Stop();
                musicTrack3Stop();
                return;
            case MISSION_STATE_1:
                musicTrack1FadeIn(0.5f, sub_GAME_7F0C0BF0());
                musicTrack2FadeOut(0.5f);
                return;
            case MISSION_STATE_2:
                return;
            case MISSION_STATE_3:
                musicTrack2ApplySeqpVol(sub_GAME_7F0C0BF0());
                g_musicXTrack2Fade = 0;
                #ifdef GE_MODDED_CHEATS
                modWatchThemePlay();
#else
                musicTrack2Play(0x18);
#endif
                return;
            case MISSION_STATE_4:
                break;
            case MISSION_STATE_5:
                break;
            case MISSION_STATE_6:
                break;
        }

        case MISSION_STATE_3:
        switch (arg0)
        {
            case MISSION_STATE_0:
                musicTrack1Stop();
                musicTrack2Stop();
                musicTrack3Stop();
                return;
            case MISSION_STATE_1:
                musicTrack1FadeIn(1.0f, sub_GAME_7F0C0BF0());
                musicTrack2FadeOut(1.0f);
                return;
            case MISSION_STATE_2:
                musicTrack2ApplySeqpVol(sub_GAME_7F0C0BF0());
                g_musicXTrack2Fade = 0;
                musicTrack2Play(musicGetXTrackForStage(stageMusicID));
                return;
            case MISSION_STATE_3:
                break;
            case MISSION_STATE_4:
                musicTrack1FadeIn(1.0f, sub_GAME_7F0C0BF0());
                musicTrack3FadeIn(1.0f, sub_GAME_7F0C0C10());
                musicTrack2FadeOut(1.0f);
                return;
            case MISSION_STATE_5:
                musicTrack3FadeIn(1.0f, sub_GAME_7F0C0C10());
                musicTrack2ApplySeqpVol(sub_GAME_7F0C0BF0());
                g_musicXTrack2Fade = 0;
                musicTrack2Play(musicGetXTrackForStage(stageMusicID));
                return;
            case MISSION_STATE_6:
                break;
        }

        case MISSION_STATE_4:
        switch (arg0)
        {
            case MISSION_STATE_0:
                musicTrack1Stop();
                musicTrack2Stop();
                musicTrack3Stop();
                return;
            case MISSION_STATE_1:
                break;
            case MISSION_STATE_2:
                break;
            case MISSION_STATE_3:
                musicTrack2ApplySeqpVol(sub_GAME_7F0C0BF0());
                g_musicXTrack2Fade = 0;
                #ifdef GE_MODDED_CHEATS
                modWatchThemePlay();
#else
                musicTrack2Play(0x18);
#endif
                musicTrack1FadeOut(0.5f);
                musicTrack3FadeOut(0.5f);
                return;
            case MISSION_STATE_4:
                break;
            case MISSION_STATE_5:
                musicTrack2ApplySeqpVol(sub_GAME_7F0C0BF0());
                g_musicXTrack2Fade = 0;
                musicTrack2Play(musicGetXTrackForStage(stageMusicID));
                musicTrack1FadeOut(0.5f);
                return;
            case MISSION_STATE_6:
                break;
        }

        case MISSION_STATE_5:
        switch (arg0)
        {
            case MISSION_STATE_0:
                musicTrack1Stop();
                musicTrack2Stop();
                musicTrack3Stop();
                return;
            case MISSION_STATE_1:
                break;
            case MISSION_STATE_2:
                break;
            case MISSION_STATE_3:
                musicTrack2ApplySeqpVol(sub_GAME_7F0C0BF0());
                g_musicXTrack2Fade = 0;
                #ifdef GE_MODDED_CHEATS
                modWatchThemePlay();
#else
                musicTrack2Play(0x18);
#endif
                musicTrack3FadeOut(0.5f);
                return;
            case MISSION_STATE_4:
                musicTrack1FadeIn(0.5f, sub_GAME_7F0C0BF0());
                musicTrack2FadeOut(0.5f);
                return;
            case MISSION_STATE_5:
                break;
            case MISSION_STATE_6:
                break;
        }

        case MISSION_STATE_6:
        switch (arg0)
        {
            case MISSION_STATE_0:
                musicTrack1Stop();
                musicTrack2Stop();
                musicTrack3Stop();
                return;
            case MISSION_STATE_1:
                musicTrack1FadeIn(2.0f, sub_GAME_7F0C0BF0());
                musicTrack2FadeOut(2.0f);
                return;
            case MISSION_STATE_2:
                break;
            case MISSION_STATE_3:
                break;
            case MISSION_STATE_4:
                break;
            case MISSION_STATE_5:
                break;
            case MISSION_STATE_6:
                g_musicXTrack2Fade = 0;
                musicTrack2Play(0x3A);
                return;
        }

        default:
        break;
    }
#ifdef DEBUG
    osSyncPrintf("Invalid current music state %d\n", arg0);
#endif

    while(1)
    {
        // error
    }
}




void sub_GAME_7F0C11FC(s32 stagenum)
{
    musicTrack1Stop();
    musicTrack2Stop();
    musicTrack3Stop();
    mission_state = MISSION_STATE_0;
    stageMusicID = stagenum;

    if (musicGetBgTrackForStage(stageMusicID) < 0)
    {
        set_missionstate(MISSION_STATE_1);
    }
    else
    {
        set_missionstate(MISSION_STATE_4);
    }

    return;
}

void set_missionstate_zero(void)
{
    set_missionstate(MISSION_STATE_0);
}

void sub_GAME_7F0C1288(void)
{
    if (musicGetBgTrackForStage(stageMusicID) < 0)
    {
        set_missionstate(MISSION_STATE_2);
    }
    else
    {
        set_missionstate(MISSION_STATE_5);
    }
}

void sub_GAME_7F0C12CC(void)
{
    if (musicGetBgTrackForStage(stageMusicID) < 0)
    {
        set_missionstate(MISSION_STATE_1);
    }
    else
    {
        set_missionstate(MISSION_STATE_4);
    }
}

void sub_GAME_7F0C1310(void)
{
    dword_CODE_bss_8008C604 = mission_state;
    set_missionstate(MISSION_STATE_3);
}

void sub_GAME_7F0C1340(void)
{
    set_missionstate(dword_CODE_bss_8008C604);
}

void sub_GAME_7F0C1364(void)
{
    s32 i;

    for (i=0; i<4; i++)
    {
        music_slot_active_0[i] = 0;
        music_slot_minutes_0[i] = 0;
        music_slot_seconds_0[i] = 0;
    }
}






void reset_all_music_slots(void)
{
    s32 i;
    s32 var_t2;

    var_t2 = 0;

    for (i=0; i<4; i++)
    {
        if (music_slot_active_0[i] || music_slot_minutes_0[i] > 0)
        {
            if (music_slot_minutes_0[i] >= g_ClockTimer)
            {
                music_slot_minutes_0[i] -= g_ClockTimer;
            }
            else
            {
                music_slot_minutes_0[i] = 0;
            }

            if (music_slot_seconds_0[i])
            {
                if (music_slot_seconds_0[i] >= g_ClockTimer)
                {
                    music_slot_seconds_0[i] -= g_ClockTimer;
                }
                else
                {
                    music_slot_seconds_0[i] = 0;
                }

                if (music_slot_seconds_0[i])
                {
                    if ((music_slot_active_0[i]) || (music_slot_minutes_0[i]))
                    {
                        var_t2 = 1;
                    }
                }
                else
                {
                    music_slot_active_0[i] = 0;
                }
            }
        }
    }

    if (g_ClockTimer != 0)
    {
        if ((get_mission_state() == MISSION_STATE_2) || (get_mission_state() == MISSION_STATE_5))
        {
            if (var_t2 == 0)
            {
                sub_GAME_7F0C12CC();
            }
        }
        else if (var_t2 != 0 && get_mission_state() != MISSION_STATE_6)
        {
            /* RC5: MISSION_STATE_6 owns multiplayer death music.  Campaign
             * Co-Op can still have live mission-music slots when a player
             * dies (especially during Silo/Bunker II permanent explosions).
             * Retail multiplayer never combines those two states.  Let the
             * slot timers continue to age, but do not request state 2/5 while
             * death music is active; 6 -> 2/5 is an intentionally-invalid
             * retail transition and falls into set_missionstate's hard trap. */
            sub_GAME_7F0C1288();
        }
    }

#ifdef GE_MODDED_CHEATS
    /* Game thread, not the VI/retrace scheduler: restarting a compact
     * sequence must not block or perform ROM I/O in musicFadeTick(). */
    musicSiloXLoopTick();
#endif
}





void musicPlaySlot(s32 slot, s32 min, s32 sec)
{
    if ((music_slot_active_0)[slot] == FALSE)
    {
        (music_slot_active_0)[slot] = TRUE;
        (music_slot_minutes_0)[slot] = min * MP_MUSIC_FRAMERATE;
        (music_slot_seconds_0)[slot] = sec * MP_MUSIC_FRAMERATE;
    }
}

void musicStopSlot(s32 slot)
{
    s32 i;

    if (-1 < slot)
    {
        (music_slot_active_0)[slot] = 0;
        return;
    }

    for (i=0; i<4; i++)
    {
        music_slot_active_0[i] = 0;
        music_slot_minutes_0[i] = 0;
        music_slot_seconds_0[i] = 0;
    }
}

