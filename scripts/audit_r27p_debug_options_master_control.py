#!/usr/bin/env python3
from pathlib import Path
import re
import sys

R = Path(__file__).resolve().parents[1]
BC=(R/"src/bondconstants.h").read_text(errors="replace")
FR=(R/"src/game/front.c").read_text(errors="replace")
O=(R/"src/game/options.c").read_text(errors="replace")
OH=(R/"src/game/options.h").read_text(errors="replace")
MP=(R/"src/game/mpmenu.c").read_text(errors="replace")
F2=(R/"src/game/file2.c").read_text(errors="replace")
SPEC=(R/"src/game/spectrum.c").read_text(errors="replace")
BOSS=(R/"src/boss.c").read_text(errors="replace")
DBG=(R/"src/game/debugmenu_handler.c").read_text(errors="replace")
DMENU=(R/"src/debugmenu.c").read_text(errors="replace")
MK=(R/"Makefile").read_text(errors="replace")

checks=[]

def ck(name, cond):
    ok=bool(cond)
    checks.append((name,ok))
    print(("[PASS] " if ok else "[FAIL] ")+name)

ck("Debug frontend menu ID is appended after SP Character",
   BC.find("MENU_SINGLE_PLAYER_CHARACTER,") <
   BC.find("MENU_DEBUG_OPTIONS,") <
   BC.find("MENU_MAX"))

ck("runtime toggle defaults Off and is exported",
   "u8 g_ModMasterControlDebugMenuEnabled = FALSE;" in O and
   "extern u8 g_ModMasterControlDebugMenuEnabled;" in OH)

ck("Debug and Master Control Debug Menu labels are packed",
   "index == 71" in SPEC and "index == 72" in SPEC and
   "Master Control Debug Menu" in SPEC)

ck("main Special Options draws Debug after Single-Player Character",
   FR.find("frontModGetOptionLabel(66)") <
   FR.find("frontModGetOptionLabel(71)"))

ck("main Debug submenu is one save-backed On/Off row",
   "void init_menu_debug_options(void)" in FR and
   "void interface_menu_debug_options(void)" in FR and
   "Gfx *constructor_menu_debug_options" in FR and
   "g_ModMasterControlDebugMenuEnabled ^= 1;" in FR and
   "g_ModOptionsDirty = TRUE;" in FR and
   "frontModCommitDeferredSubmenuSettings();" in FR)

ck("main Debug submenu owns blank Special Options folder state",
   "R27P R15: Debug uses the same blank folder presentation as Special Options" in FR and
   "disable_all_switches(walletinst[0]);" in FR and
   "set_item_visibility_in_objinstance(walletinst[0], SW_TABS, 1);" in FR and
   "set_item_visibility_in_objinstance(walletinst[0], SW_BLANK, 1);" in FR and
   "R27P R15: clear child tab state before returning to Special Options" in FR and
   "frontChangeMenu(MENU_MOD_OPTIONS, FALSE);" in FR)

ck("Cheat Page 2 B returns to Page 1 while Page 1 B exits",
   "R27P R15: B on Cheat Page 2 returns to Page 1" in FR and
   "if (tab_start_selected != 0)" in FR and
   "frontModBuildCheatPage(0);" in FR and
   "tab_prev_selected = 1;" in FR)

ck("main Debug update/init/interface dispatchers all terminate with break",
   re.search(r"case\s+MENU_DEBUG_OPTIONS\s*:\s*update_menu_debug_options\(\)\s*;\s*break\s*;", FR) is not None and
   re.search(r"case\s+MENU_DEBUG_OPTIONS\s*:\s*init_menu_debug_options\(\)\s*;\s*break\s*;", FR) is not None and
   re.search(r"case\s+MENU_DEBUG_OPTIONS\s*:\s*interface_menu_debug_options\(\)\s*;\s*break\s*;", FR) is not None)

ck("main Debug menu has full update/init/input/render dispatch",
   FR.count("MENU_DEBUG_OPTIONS") >= 5 and
   "update_menu_debug_options();" in FR and
   "init_menu_debug_options();" in FR and
   "interface_menu_debug_options();" in FR and
   "constructor_menu_debug_options(DL);" in FR)

ck("SP Watch keeps Debug row 14 before Experimental, Enhancements and Cheats",
   "#define MODWATCH_MODE_DEBUG 0x500" in O and
   "#define MODWATCH_DEBUG_ROWS 1" in O and
   "#define MODWATCH_OPTION_ROWS 18" in O and
   "MODWATCH_STATE = MODWATCH_MODE_DEBUG;" in O and
   "row == 14" in O and
   "row == 15" in O and
   "row == 16" in O and
   "frontModGetOptionLabel(71)" in O and
   "frontModGetOptionLabel(72)" in O and
   "frontModGetOptionLabel(74)" in O and
   "frontModGetOptionLabel(76)" in O)

ck("SP Watch Debug toggle is deferred-save",
   "static void modWatchToggleDebugOption" in O and
   "g_ModMasterControlDebugMenuEnabled ^= 1;" in O and
   "g_ModWatchSettingsDirty = TRUE;" in O and
   "if (mode == MODWATCH_MODE_DEBUG)" in O)

ck("MP/Co-op Watch has Debug mode 7 and one row",
   ("if (mode == 7) return 1;" in MP or "if (mode == 7 || mode == 8) return 1;" in MP) and
   "mpcfg_mode[player]=7" in MP and
   "mpcfg_row[player]=15" in MP and
   'mpcfg_mode[curplayernum] == 7 ? "DEBUG"' in MP)

ck("MP Debug toggle uses shared deferred dirty latch",
   "static void mpwatchConfigToggleDebug" in MP and
   "g_ModMasterControlDebugMenuEnabled ^= 1;" in MP and
   "mpcfg_globals_dirty = TRUE;" in MP and
   "mpwatchConfigDebugValue" in MP)

ck("new option uses independent extension reserved bit 0x10",
   "#define GE_SRAM_EXT_RESERVED_MASTER_CONTROL_DEBUG 0x10" in F2 and
   "record->reserved & GE_SRAM_EXT_RESERVED_MASTER_CONTROL_DEBUG" in F2 and
   "g_ModMasterControlDebugMenuEnabled ? GE_SRAM_EXT_RESERVED_MASTER_CONTROL_DEBUG" in F2)

ck("old/no-extension saves default Master Control Debug Menu Off",
   "g_ModMasterControlDebugMenuEnabled = FALSE;" in F2)

combo = "L_TRIG | R_TRIG | U_JPAD | U_CBUTTONS"
ck("in-game MCM combo is exactly L+R+D-Pad Up+C-Up",
   combo in BOSS and
   "joyGetButtons(0, R27P_MCM_COMBO) == R27P_MCM_COMBO" in BOSS and
   "joyGetButtonsPressedThisFrame(0, R27P_MCM_COMBO) != 0" in BOSS)

ck("GE+ MCM entry goes through retail debug-menu initialization",
   "R27P R10: alternate GE+ MCM opener reuses retail init path" in DBG and
   "button_held & (L_TRIG | R_TRIG | U_JPAD | U_CBUTTONS)" in DBG and
   "button_pressed & (L_TRIG | R_TRIG | U_JPAD | U_CBUTTONS)" in DBG and
   "show_debug_menu_flag = varv0;" in DBG and
   "stop_recording_ramrom(button_held);" in DBG)

ck("R7 direct-open shortcut and latch are gone",
   "R27P R7: direct GE+ MCM entry" not in BOSS and
   "g_R27pMcmComboLatched" not in BOSS and
   "show_debug_menu_flag = TRUE;" not in BOSS and
   "g_BossIsDebugMenuOpen = TRUE;" not in BOSS)

ck("MCM combo is gated by persisted option and cannot open on title frontend",
   "g_ModMasterControlDebugMenuEnabled" in BOSS and
   "g_StageNum != LEVELID_TITLE" in BOSS)

ck("GE+ MCM entry is compiled in standard non-DEBUGMENU builds",
   BOSS.find("#define R27P_MCM_COMBO (L_TRIG | R_TRIG | U_JPAD | U_CBUTTONS)") <
       BOSS.find("#ifdef DEBUGMENU", BOSS.find("#define R27P_MCM_COMBO")) and
   "#ifndef DEBUGMENU\n                            if (g_BossIsDebugMenuOpen" in BOSS and
   "g_ModMasterControlDebugMenuEnabled" in
       BOSS[BOSS.find("#ifndef DEBUGMENU"):BOSS.find("#undef R27P_MCM_COMBO")] and
   "joyGetButtons(0, R27P_MCM_COMBO) == R27P_MCM_COMBO" in
       BOSS[BOSS.find("#ifndef DEBUGMENU"):BOSS.find("#undef R27P_MCM_COMBO")] and
   "joyGetButtonsPressedThisFrame(0, R27P_MCM_COMBO) != 0" in
       BOSS[BOSS.find("#ifndef DEBUGMENU"):BOSS.find("#undef R27P_MCM_COMBO")])

ck("P1 remains the authoritative MCM navigation controller",
   "joyGetStickX(0)" in BOSS and
   "joyGetStickY(0)" in BOSS and
   "joyGetButtons(0, ANY_BUTTON)" in BOSS and
   "joyGetButtonsPressedThisFrame(0, ANY_BUTTON)" in BOSS)

ck("legacy C-Up+C-Down gesture remains only in non-modded fallback",
   "#else" in BOSS and
   "|| joyGetButtons(0, U_CBUTTONS | D_CBUTTONS) == (U_CBUTTONS | D_CBUTTONS)" in BOSS and
   "#else\n        varv0 = (button_held & U_CBUTTONS) && (button_held & D_CBUTTONS);" in DBG)

ck("turning access Off closes both boss and handler MCM state",
   "!g_ModMasterControlDebugMenuEnabled && g_BossIsDebugMenuOpen" in BOSS and
   "show_debug_menu_flag = FALSE;" in BOSS and
   "g_BossIsDebugMenuOpen = FALSE;" in BOSS)

ck("GE+ MCM uses D-pad navigation, A activation, Start exit",
   all(x in DBG for x in [
       "button_pressed & L_JPAD",
       "button_pressed & R_JPAD",
       "button_pressed & U_JPAD",
       "button_pressed & D_JPAD",
       "if (button_pressed & A_BUTTON)",
       "if (button_pressed & START_BUTTON)",
   ]) and
   "R27P: GE+ uses A to activate" in DBG)

ck("MCM Start exit restores normal Bond View gameplay controls",
   "R27P R12: restore normal gameplay control state after MCM exit" in DBG and
   "g_DebugMode = DEB_BOND_VIEW;" in DBG and
   "debug_render_raster = DEB_BOND_VIEW;" in DBG and
   "debug_freeze_processing = DEB_BOND_VIEW;" in DBG and
   DBG.find("g_DebugMode = DEB_BOND_VIEW;", DBG.find("if (button_pressed & START_BUTTON)")) >
       DBG.find("if (button_pressed & START_BUTTON)") and
   DBG.find("show_debug_menu_flag = 0;", DBG.find("if (button_pressed & START_BUTTON)")) >
       DBG.find("g_DebugMode = DEB_BOND_VIEW;", DBG.find("if (button_pressed & START_BUTTON)")))


ck("R27H quick preset keeps Master Control Debug Menu Off",
   "frontModApplyQuickSettingsPreset" not in FR or
   "g_ModMasterControlDebugMenuEnabled = FALSE;" in FR)

prereq=next((x for x in MK.splitlines() if x.startswith("prerequisites:")),"")
ck("GE+ MCM text uses command-correct GFX budget without normal random dropout",
   "R27P R13: use Gfx-command units consistently." in DMENU and
   "available = dynGetFreeGfx(gdl) - 256;" in DMENU and
   "needed = gdl2 - gdl;" in DMENU and
   "R27P R13: 128 Gfx commands == 1 KiB." in DMENU and
   "if (dynGetFreeGfx(gdl) >= 128)" in DMENU and
   "randomGetNext() & 0xFF" in DMENU)

ck("R27P audit is mandatory",
   "r27p-debug-options-master-control-audit:" in MK and
   "r27p-debug-options-master-control-audit" in prereq)

bad=[name for name,ok in checks if not ok]
print()
if bad:
    print(f"R27P DEBUG OPTIONS / MASTER CONTROL AUDIT: FAIL ({len(checks)-len(bad)}/{len(checks)})")
    for name in bad:
        print(" - "+name)
    sys.exit(1)

print(f"R27P DEBUG OPTIONS / MASTER CONTROL AUDIT: PASS ({len(checks)}/{len(checks)})")
