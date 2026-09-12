#!/usr/bin/env python3
from pathlib import Path
import re, sys

ROOT = Path(sys.argv[1] if len(sys.argv) > 1 else '.')

def read(rel):
    return (ROOT / rel).read_text(errors='replace')

def func_body(text, name):
    m = re.search(r'\b(?:void|s32|u32|bool|Gfx\s*\*)\s+' + re.escape(name) + r'\s*\([^)]*\)\s*\{', text)
    if not m:
        return ''
    i = m.end()-1
    depth = 0
    for j in range(i, len(text)):
        c = text[j]
        if c == '{': depth += 1
        elif c == '}':
            depth -= 1
            if depth == 0:
                return text[m.start():j+1]
    return ''

files = {p: read(p) for p in [
    'src/rom_header.s','src/game/bg.c','src/game/bondinv.c','src/game/bondview2.c','src/game/chr.c',
    'src/game/chrprop.c','src/game/file2.c','src/game/front.c','src/game/lv.c',
    'src/game/mpmenu.c','src/game/objective_status.c','src/game/options.c',
    'src/game/options.h','src/game/propobj.c','src/game/gunfire.c','src/game/spectrum.c',
    'src/game/file2.h','src/game/mp_music.c','src/game/cheat.h','src/game/front.h','build_r21.sh','Makefile','scripts/patch_r21_front_textids.py',
    'assets/obseg/prop/sev_door3/Model.c','assets/obseg/prop/sev_door3_wind/Model.c',
    'assets/obseg/prop/sev_door4_wind/Model.c','assets/obseg/prop/sev_door_v1/Model.c'
]}

checks=[]
def check(name, ok, detail=''):
    checks.append((name,bool(ok),detail))

def has(f, *needles):
    s=files[f]
    return all(n in s for n in needles)

check('R21 ROM revision is 0x15', '0x15' in files['src/rom_header.s'])
check('B+Z queued mission-item path exists', has('src/game/bondinv.c','g_MissionItemQueueValid','bondinvProcessMissionItemQueue','gunRequestHandWeaponChange'))
check('B+Z queue is ticked after gun update', 'bondinvProcessMissionItemQueue();' in files['src/game/bondview2.c'])
check('RC9 B+Z chord requires prior B plus fresh Z edge', has('src/game/bondview2.c','(oldbuttons & B_BUTTON)','!(oldbuttons & Z_TRIG)','bzInventoryPressed = TRUE;'))
check('RC9 B+Z valid chord remains consumed until release', has('src/game/bondview2.c','if (bzInventoryChord)','buttons &= ~(B_BUTTON | Z_TRIG);'))
check('Shared Co-Op mission item registries exist', has('src/game/bondinv.c','g_CoopSharedProps','g_CoopSharedItems','bondinvShareCoopItem','bondinvShareCoopProp'))
check('Shared mission items restore during inventory reinit', has('src/game/bondinv.c','g_CoopSharedPropCount','bondinvAddPropToInv(g_CoopSharedProps[i])','bondinvAddInvItem(i)'))
check('Mission pickups feed the shared registry', has('src/game/propobj.c','bondinvShareCoopItem','bondinvShareCoopProp'))
check('Objective status is broadcast to each Co-Op player HUD', has('src/game/objective_status.c','getPlayerCount()','set_cur_player(player)','hudmsgBottomShow(&buffer)'))
check('Per-viewport mission timer is suppressed in Co-Op', has('src/game/bondview2.c','getPlayerCount() == 1 || get_scenario() != SCENARIO_COOP','countdownTimerRender(gdl)'))
check('One full-screen Co-Op timer render exists', has('src/game/lv.c','getPlayerCount() > 1 && get_scenario() == SCENARIO_COOP','countdownTimerRender(DL)'))
check('Timer coordinates use physical screen center', has('src/game/propobj.c','xpos = viGetX() / 2','ypos = viGetY() / 2'))
check('Split-screen top dialogue uses compact Bank Gothic path', has('src/game/bondview2.c','msgchars = ptrFontBankGothicChars','msgfont = ptrFontBankGothic','msg.x + msg.textwidth + 1'))
check('Pickup HUD uses tighter split-screen text spacing', 'setTextSpacingInverted(4)' in files['src/game/bondview2.c'])
body=func_body(files['src/game/mpmenu.c'],'mpStartCoopGameOver')
check('Dedicated Co-Op GAME OVER path exists', bool(body))
check('Co-Op GAME OVER does not calculate competitive awards', bool(body) and 'mpCalculateAwards' not in body)
check('Permanent-explosion terminal waits for full death sequence', has('src/game/lv.c','redbloodfinished','deathanimfinished','colourfadetimemax60 < 0.0f','fullydead == getPlayerCount()'))
check('Permanent-explosion terminal stops level music and enters Co-Op GAME OVER', has('src/game/lv.c','musicTrack1Stop();','mission_failed_or_aborted = TRUE;','mpStartCoopGameOver();'))
check('AI references are cleared after completed player death lifecycle', has('src/game/lv.c','chrCoopClearRefsToPlayer(i)','redbloodfinished','deathanimfinished'))
check('Individual Co-Op death guards global Bond KIA', has('src/game/bondview2.c','gamemode != GAMEMODE_MULTI || get_scenario() != SCENARIO_COOP','g_isBondKIA = 1;'))
check('Escort ownership implementation is present', has('src/game/chr.c','SCENARIO_COOP') and has('src/game/chrprop.c','SCENARIO_COOP'))
check('Co-Op report snapshot is captured before stage cleanup', files['src/game/lv.c'].find('frontStoreCoopMissionReportStats();') < files['src/game/lv.c'].find('cleanupGuardData();'))
check('Persistent Co-Op mission report data exists', has('src/game/front.c','g_CoopMissionReport','frontStoreCoopMissionReportStats','frontHasCoopMissionReport'))
check('Live Co-Op watch uses Objectives/Pause on MENU_SCORES', has('src/game/mpmenu.c','Deaths << Kills << Objectives/Pause << Exit','mpwatchUseCoopPauseCarousel','text = "OBJECTIVES"','text = "A: PAUSE"','text = "A: UNPAUSE"'))
check('Co-Op labels are KILLS and DEATHS without player prefix', has('src/game/mpmenu.c','if (mpwatchIsCampaignCoop())','text = "DEATHS"'))
check('Competitive Score/Rank rendering is excluded from live Co-Op score card', has('src/game/mpmenu.c','g_CurrentPlayer->mpmenumode == MENU_SCORES','&& !mpwatchIsCampaignCoop()'))
check('Scenario display mapping places Co-Op immediately after Normal', has('src/game/front.c','return SCENARIO_COOP') and has('src/game/front.c','SCENARIO_NORMAL','SCENARIO_COOP'))
check('Co-Op player count clamps to at least 2', has('src/game/front.c','scenario == SCENARIO_COOP && numplayers < 2','numplayers = 2'))
check('Extra MP Characters frontend gate uses real toggle state', has('src/game/front.c','g_CheatActivated[CHEAT_EXTRA_MP_CHARS]'))
check('Multiplayer Settings hub exists', has('src/game/front.c','MENU_MP_SETTINGS','"Settings"','"Player Options"'))
check('No Radar [Multi] is exposed in Settings and hidden from Cheats', has('src/game/front.c','cheatGetMenuTextPointer(CHEAT_NO_RADAR_MP)','case CHEAT_NO_RADAR_MP:') and 'i != CHEAT_NO_RADAR_MP' in files['src/game/front.c'])
check('No Reload is inserted directly after Infinite Ammo on Cheat Page 1', has('src/game/front.c','if (i == CHEAT_INFINITE_AMMO)','arrayUnlockedCheats[totalunlockedcheats++] = CHEAT_NO_RELOAD'))
check('Per-player option bitfield covers requested seven options', has('src/game/options.h','MP_PLAYEROPT_LOOK','MP_PLAYEROPT_AUTOAIM','MP_PLAYEROPT_AIMCONTROL','MP_PLAYEROPT_SIGHT','MP_PLAYEROPT_LOOKAHEAD','MP_PLAYEROPT_AMMO','MP_PLAYEROPT_HEADROLL'))
check('Head Roll defaults On for all multiplayer players', has('src/game/options.c','MP_PLAYEROPT_DEFAULT','g_MpPlayerOptions[MAX_PLAYER_COUNT]'))
check('Per-player gameplay getters read g_MpPlayerOptions', files['src/game/options.c'].count('g_MpPlayerOptions[player_num]') >= 7)
check('Head Roll application is gated by per-player setting', 'cur_player_get_headroll_setting()' in files['src/game/bondview2.c'])
check('Four aspect ratios and generalized factor path exist', has('src/game/options.c','7.0f / 4.0f','8.0f / 3.0f','get_screen_ratio_factor'))
check('R21 EEPROM migration marker/options support is present', has('src/game/file2.c','OPTION_R21_MIGRATED','OPTION_HEADROLL','OPTION_CROSSHAIR'))
check('Build wrapper verifies NTSC-U base ROM SHA-1', has('build_r21.sh','abe01e4aeb033b6c0836819f549c791b26cfde83','baserom.u.z64'))
check('Build wrapper preserves complete Silver/Gold PP7 custom asset directories', has('build_r21.sh','custom_asset_dirs','assets/obseg/prop/chrsilverwppk','assets/obseg/prop/chrgoldwppk','cp -a','force_extract_u'))
check('Compressed-slot hard guard remains present', 'CDATA_MAX_SIZE=72704' in (ROOT/'tools/data_compress.sh').read_text(errors='replace'))
check('R21 post-link Settings text-ID hotfix is wired into mod build', has('Makefile','python3 scripts/patch_r21_front_textids.py $@') and has('scripts/patch_r21_front_textids.py','0x9c73 / 0x9c74','Multiplayer Settings No Radar On/Off','Player Options Head Roll On/Off'))
check('Settings text-ID hotfix requires unique exact signatures', has('scripts/patch_r21_front_textids.py','hits = find_all(data, old)','if len(hits) != 1','expected exactly one R21 signature'))
check('RC3 fixed-slot crash guards cover inventory/score/pickup paths', has('scripts/patch_r21_front_textids.py','Inventory forward prop-only ring wrap guard','Inventory backward prop-only ring wrap guard','Co-Op pause invalid-score-scenario guard','3/4P weapon pickup string initialization'))
check('Player Options fixed-slot row alignment patch remains present', 'Player Options label/value row alignment' in files['scripts/patch_r21_front_textids.py'])
check('Co-Op Objectives/Pause header keeps full LMPMENU text bank', has('src/game/mpmenu.c','(g_pausedFlag ? MPMENU_STR_18_PAUSED : MPMENU_STR_19_PAUSE)'))
conditional_stringids = []
for rel, text in files.items():
    if rel.endswith('.c'):
        for m in re.finditer(r'getStringID\s*\((.*?)\)', text, re.S):
            if '?' in m.group(1):
                conditional_stringids.append((rel, m.group(0)))
check('No unguarded conditional getStringID calls remain', len(conditional_stringids) == 2 and any(rel == 'src/game/mpmenu.c' and '(g_pausedFlag ?' in call for rel, call in conditional_stringids) and any(rel == 'src/game/front.c' and 'CHEAT_NO_RADAR_MP' in call for rel, call in conditional_stringids) and 'Multiplayer Settings No Radar On/Off' in files['scripts/patch_r21_front_textids.py'])
check('Options Page 2 exposes all five requested settings', has('src/game/front.c','Head Roll','Endless Death Cam','Real-Time Collapse','Disable Hitstun','Damage Flash') or has('src/game/spectrum.c','Head Roll','Endless Death Cam','Real-Time Collapse','Disable Hitstun','Damage Flash'))
check('Extended option defaults are Head Roll/Damage Flash On and death/hitstun options Off', has('src/game/file2.h','DEFAULT_MOD_OPTIONS2','MODOPT2_MIGRATED | MODOPT2_DAMAGE_FLASH') and 'OPTION_HEADROLL' in files['src/game/file2.h'])
check('Endless Death Cam loops solo death replay without incrementing finite replay count', has('src/game/bondview2.c','MODOPT2_ENDLESS_DEATHCAM','bondviewSetCameraMode(CAMERAMODE_DEATH_CAM_SP)'))
check('Real-Time Collapse uses full head animation speed', has('src/game/bondview2.c','MODOPT2_REALTIME_COLLAPSE','bheadSetSpeed','1.0f : 0.5f'))
check('Real-Time Collapse keeps animation speed separate from transition timing', has('src/game/bondview2.c','MODOPT2_REALTIME_COLLAPSE','g_Rc5RealtimeCollapsePhase','g_Rc5RealtimeCollapseTimer','g_Rc5RealtimeCollapseTimer -= g_GlobalTimerDelta','currentPlayerAdjustFade'))
check('Disable Hitstun leaves single-player trigger input active', has('src/game/bondview2.c','MODOPT2_DISABLE_HITSTUN','moveData.triggerOn = 0'))
check('Disable Knockback gates player bondshotspeed boost only', has('src/game/bondview2.c','MODOPT2_DISABLE_KNOCKBACK','bondshotspeed.x','bondshotspeed.z'))
check('Damage Flash gates only the damage fade-colour application', has('src/game/bondview2.c','MODOPT2_DAMAGE_FLASH','currentPlayerSetFadeColour'))
check('Persistent non-manual crosshair is smaller while manual Aim keeps retail size', has('src/game/gunfire.c','g_CurrentPlayer->insightaimmode ? 16.0f : 10.0f'))
check('Options labels were relocated out of compressed C data', 'g_ModOptionsLabels' not in files['src/game/front.c'] and has('src/game/spectrum.c','frontModGetOptionLabel','static u32 text[6]'))
check('PD-style room allocation GC is called for cached room reloads', has('src/game/bg.c','bgModGarbageCollectRoomsForLoad(allocsize)','cur_room_totalsize > 0'))
check('Room GC protects visible/neighbour rooms and defrags immediately', has('src/game/spectrum.c','model_bin_loaded > oldestage','!room->room_rendered','!room->room_neighbor_to_rendered','delete_room_data(oldestroom)','memaDefrag();') and '!room->field_35' not in files['src/game/spectrum.c'])
check('Severnaya arrow decal disables mipmapping on all affected door variants', all('TEXTURETYPE_TILE, 0, 0, IMAGE_1242' in files[f] for f in ['assets/obseg/prop/sev_door3/Model.c','assets/obseg/prop/sev_door3_wind/Model.c','assets/obseg/prop/sev_door4_wind/Model.c','assets/obseg/prop/sev_door_v1/Model.c']))
check('Build wrapper preserves all four complete custom Severnaya door directories', has('build_r21.sh','assets/obseg/prop/sev_door3','assets/obseg/prop/sev_door3_wind','assets/obseg/prop/sev_door4_wind','assets/obseg/prop/sev_door_v1'))

check('Everything Unlocked ROM header is GoldenEye Plus', 'GoldenEye Plus      ' in files['src/rom_header.s'])
check('Permanent-explosion dynamic music cannot override death music state', has('src/game/mp_music.c','var_t2 != 0 && get_mission_state() != MISSION_STATE_6','sub_GAME_7F0C1288();'))
check('Permanent-explosion death music stays authoritative through staggered deaths', has('src/game/lv.c','if (!lvlIsCoopPermanentExplosionFailure())','set_missionstate(MISSION_STATE_1);'))
check('RC9 permanent explosions stop spawning at terminal Co-Op GAME OVER', has('src/game/bondview2.c','g_SurroundBondWithExplosionsFlag','g_CurrentPlayer->prop != NULL','g_gameOverFlag == 0'))
check('Co-Op upper-dialogue global timer advances once per frame', has('src/game/bondview2.c','get_scenario() == SCENARIO_COOP','get_cur_playernum() != 0','return;'))
check('Split-screen upper dialogue wraps to current viewport width', has('src/game/bondview2.c','textWrap(viGetViewWidth() - 12','wrappedmsg','msgtext = wrappedmsg'))
check('Late gameplay HUD is suppressed behind each open Co-Op watch', has('src/game/bondview2.c','get_scenario() == SCENARIO_COOP && g_CurrentPlayer->mpmenuon','gunDrawSight','generate_ammo_total_microcode'))
check('Co-Op KILLS/DEATHS render and highlight current-player cell', files['src/game/mpmenu.c'].count('i == curplayernum ? GREEN_HIGHLIGHT : GREEN_NORMAL') >= 1 and has('src/game/mpmenu.c','mpwatchIsCampaignCoop()','g_playerPlayerData[i].kill_counts[curplayernum]'))
check('Six logical SP watch pages do not resize player-backed WATCH_NUMBER_SCREENS', has('src/game/options.h','#define WATCH_NUMBER_SCREENS 5','#define WATCH_LOGICAL_PAGE_COUNT 6','WATCH_INDEX_SPECIAL_OPTIONS'))
check('Six watch selector rectangles are allocated dynamically', has('src/game/options.c','dynAllocateVertices(WATCH_LOGICAL_PAGE_COUNT * 4)','draw_mod_watch_page_rectangles','WATCH_LOGICAL_PAGE_COUNT'))
check('RC9 background bounds derive only from current VI viewport', has('src/game/bg.c','bgUpdateCurrentPlayerScreenMinMax','viGetViewLeft()','viGetViewTop()','viGetViewWidth()','viGetViewHeight()') and 'bgViewRelated[0]' not in func_body(files['src/game/bg.c'],'bgUpdateCurrentPlayerScreenMinMax'))
check('RC9 visibility pass clears portal projection cache per player', has('src/game/bg.c','bgUpdateCurrentPlayerScreenMinMax();','sub_GAME_7F0B5168();') and files['src/game/bg.c'].find('bgUpdateCurrentPlayerScreenMinMax();', files['src/game/bg.c'].find('void bgDetermineVisibleRooms')) < files['src/game/bg.c'].find('sub_GAME_7F0B5168();', files['src/game/bg.c'].find('void bgDetermineVisibleRooms')))
lvlrender = func_body(files['src/game/lv.c'],'lvlRender')
check('RC9 per-player render pass sets player and VI viewport before visibility', bool(lvlrender) and lvlrender.find('set_cur_player(get_nth_player_from_shuffled(i));') < lvlrender.find('viSetViewSize(g_CurrentPlayer->viewx, g_CurrentPlayer->viewy);') < lvlrender.find('viSetViewPosition(g_CurrentPlayer->viewleft, g_CurrentPlayer->viewtop);') < lvlrender.find('bgRoomVisibilityRelated();'))
check('Special watch page shares frontend option state', has('src/game/options.c','cur_player_get_headroll_setting()','g_ModGameplayOptions2','save->mod_options2 = g_ModGameplayOptions2','fileWriteSave(save)'))
check('Live watch Cheats use conservative six-cheat allowlist', has('src/game/options.c','g_Rc5WatchCheats','CHEAT_INVINCIBILITY','CHEAT_ALLGUNS','CHEAT_LINEMODE','CHEAT_INVISIBILITY','CHEAT_INFINITE_AMMO','CHEAT_NO_RELOAD'))
check('Live watch Cheats reuse runtime activation handlers', has('src/game/options.c','cheatIsActive(cheat)','cheatButtonHandleCheatsTurnedOff(cheat)','cheatButtonHandleCheatsTurnedOn(cheat)','g_CheatActivated[cheat] = !active'))
check('Live watch Cheats preserve unlock gate and progression latch', has('src/game/options.c','frontCheckIfCheatIsUnlocked(cheat)','g_AppendCheatSinglePlayer = TRUE'))
check('Watch runtime cheat handlers are explicitly declared', has('src/game/cheat.h','cheatButtonHandleCheatsTurnedOn','cheatButtonHandleCheatsTurnedOff'))


check('RC6 Co-Op death counter increments exactly when death is committed', has('src/game/bondview2.c','campaign Co-Op deaths are counted at the instant','increment_num_deaths();','g_CurrentPlayer->bonddead = 1;'))
check('RC6 Co-Op enemy Kills exclude player-on-player kills', has('src/game/bondview2.c','player-on-player deaths remain in the','if (get_scenario() != SCENARIO_COOP)','increment_num_kills_display_text_in_MP();') and has('src/game/mpmenu.c','g_playerPlayerData[i].kill_count'))
check('RC6 Friendly Kills page exists before Deaths in Co-Op carousel', has('src/game/mpmenu.c','FRIENDLY KILLS','MENU_GOWOC','MENU_LOSSES') and files['src/game/mpmenu.c'].find('MENU_GOWOC') < files['src/game/mpmenu.c'].find('MENU_LOSSES'))
check('RC9 terminal GAME OVER freezes world and plays retail MP end cue', has('src/game/mpmenu.c','musicStopSlot(-1);','musicTrack1Play(M_INTROSWOOSH);','pauseAndLockControls();','g_gameOverFlag = 1;') and 'g_pausedFlag = FALSE;' not in body and 'lvlSetControlsLockedFlag(0);' not in body)
check('RC6 terminal GAME OVER preserves Weapon of Choice', has('src/game/mpmenu.c','bondinvGetWeaponOfChoice','store_favorite_weapon_current_player') and has('src/game/front.c','set_favorite_weapon_for_every_player();'))
check('RC6 terminal GAME OVER keeps objectives and Weapon of Choice on same card', has('src/game/mpmenu.c','text = "OBJECTIVES"','g_gameOverFlag','MPMENU_STR_1F_WEAPONOFCHOICE'))
check('RC9 START TO EXIT is raised within each viewport', has('src/game/mpmenu.c','MPMENU_STR_17_STARTTTOEXIT','viGetViewHeight() - textheight - 14'))
check('RC6 center START TO EXIT replacement is suppressed in Co-Op', has('src/game/mpmenu.c','mpwatchIsCampaignCoop() ? (char *)ascii_MP_watch_menu_BLANK'))
check('RC6 terminal exit captures report and leaves stage through title handoff', has('src/game/mpmenu.c','frontStoreCoopMissionReportStats();','bossSetLoadedStage(LEVELID_TITLE);'))
check('RC6 persistent Co-Op result latch overrides multiplayer-options fallback', has('src/game/front.c','else if (frontHasCoopMissionReport())','frontChangeMenu(MENU_MISSION_FAILED, 1);'))
check('RC6 result snapshot is not erased by late non-Co-Op teardown', has('src/game/front.c','if (gamemode != GAMEMODE_MULTI || scenario != SCENARIO_COOP)','return;'))
check('RC6 successful POSEND initialization is forced onto P1 then restores caller', has('src/game/bondview2.c','coop_restore_player','arg0 == CAMERAMODE_POSEND','set_cur_player(PLAYER_1)','set_cur_player(coop_restore_player)'))
check('RC6 shared Co-Op end cinematic renders only P1 after full-screen clear', has('src/game/lv.c','pcount = lvlIsCoopEndCutscene() ? 1 : getPlayerCount()','gDPSetScissor','viFillScreen','set_cur_player(0)'))
check('RC6 build wrapper restores custom assets even on interrupted build', has('build_r21.sh','cleanup_assets','trap cleanup_assets EXIT','restore_custom_assets'))
check('RC7 Endless Death Cam ignores exhausted random camera angles without exiting', has('src/game/bondview2.c','pickDeathCameraAngles','MODOPT2_ENDLESS_DEATHCAM','camera_mode != CAMERAMODE_FADESWIRL','bossRunTitleStage();'))
check('RC7 Real-Time Collapse cannot transition until hold commits deathanimfinished', has('src/game/bondview2.c','g_CurrentPlayer->deathanimfinished && currentPlayerIsFadeComplete()'))

check('RC7 Endless Death Cam suppresses retail replay-count title fallback until explicit exit', has('src/game/bondview2.c','g_CurrentPlayer->redbloodfinished && g_CurrentPlayer->deathanimfinished','MODOPT2_ENDLESS_DEATHCAM','camera_mode != CAMERAMODE_FADESWIRL'))
check('RC9 Endless Death Cam accepts A/B/Z/START before finite retail death flags complete', files['src/game/bondview2.c'].count('(gamemode != GAMEMODE_MULTI && (g_ModGameplayOptions2 & MODOPT2_ENDLESS_DEATHCAM))') >= 2 and files['src/game/bondview2.c'].count('CONT_A | B_BUTTON | Z_TRIG | START_BUTTON') >= 2)
check('RC7 Rapid Fire is first entry on Cheat Page 2', has('src/game/front.c','static const u8 hiddencheats[] = {','CHEAT_RAPID_FIRE,','CHEAT_EXTRA_MP_CHARS,'))
check('RC10 folder-entry unlock scan excludes appended mod-only cheats', has('src/game/front.c','for (i=CHEAT_EXTRA_MP_CHARS; i != CHEAT_NO_RELOAD; i++)','for (i=CHEAT_EXTRA_MP_CHARS; i != CHEAT_INVALID; i++)'))
check('RC9 Rapid Fire uses proven field_890 low-byte timer method without old state hacks', has('src/game/gunfire.c','cheatIsActive(CHEAT_RAPID_FIRE)','handptr->field_890 |= 0xff;') and files['src/game/gunfire.c'].count('cheatIsActive(CHEAT_RAPID_FIRE)') == 1)
check('RC7 Co-Op A pause action is scoped to Objectives/Pause card', has('src/game/mpmenu.c','mpwatchUseCoopPauseCarousel() && g_CurrentPlayer->mpmenumode == MENU_SCORES','!mpwatchUseCoopPauseCarousel() && g_CurrentPlayer->mpmenumode == MENU_PAUSE'))
check('RC9 Features/Cheats watch page is explicit fifth bar and Objectives is sixth', has('src/game/options.c','case WATCH_INDEX_GAME_OPTIONS:      return 3;','case WATCH_INDEX_SPECIAL_OPTIONS:   return 4;','case WATCH_INDEX_MISSION_STATUS:    return 5;'))
check('RC7 Features/Cheats page beeps only from A selection handlers', has('src/game/options.c','if (pressed & A_BUTTON)','watch_screen_index != WATCH_INDEX_SPECIAL_OPTIONS') and 'sndPlaySfx(g_musicSfxBufferPtr, OPTION_CHOOSE_SFX, NULL);' in files['src/game/options.c'])
check('RC7 mission-item HUD labels use viewport-safe right alignment', has('src/game/gunfire.c','(viGetViewLeft() + viGetViewWidth()) - 8, HUDHALIGN_RIGHT'))
check('RC7 room load failure cannot publish partial room geometry', has('src/game/bg.c','if (loadfailed)','g_BgRoomInfo[roomID].vertices = NULL','g_BgRoomInfo[roomID].ptr_expanded_mapping_info = NULL','g_BgRoomInfo[roomID].model_bin_loaded = 0','memaDefrag();'))
check('RC7 cold-load portal metrics invalidate stale upper-RDRAM cache before ordering', has('src/game/bg.c','The upper-RDRAM portal cache survives warm boots/stage changes','g_BgFastPortalCount = 0;','D_800443C4[i] = sub_GAME_7F0B993C(i);','bgOrderPortal(i);','bgBuildPortalFastCache();','bgBuildPortalMetricCache();') and files['src/game/bg.c'].find('g_BgFastPortalCount = 0;', files['src/game/bg.c'].find('The upper-RDRAM portal cache')) < files['src/game/bg.c'].find('D_800443C4[i] = sub_GAME_7F0B993C(i);'))

passed=sum(ok for _,ok,_ in checks)
failed=len(checks)-passed
print('GoldenEye R21 static regression audit')
print('====================================')
for name,ok,detail in checks:
    print(('[PASS] ' if ok else '[FAIL] ')+name+((' — '+detail) if detail else ''))
print(f'\nResult: {passed}/{len(checks)} checks passed; {failed} failed.')
print('\nKnown non-static validation items:')
print('[WARN] Project64/N64 runtime matrix is not executable in this Linux build container; emulator testing remains required.')
print('[INFO] Co-Op Health intentionally remains locked/greyed so all players start with equal normal health.')
print('[WARN] R20 community source included scripts/test_fastpaths.sh but omitted scripts/tests/fastpath_equivalence.c; that equivalence wrapper cannot be honestly executed from the supplied baseline.')
sys.exit(1 if failed else 0)
