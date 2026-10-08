#!/usr/bin/env python3
from pathlib import Path
import sys

R = Path(__file__).resolve().parents[1]
FR = (R/'src/game/front.c').read_text(errors='replace')
O = (R/'src/game/options.c').read_text(errors='replace')
MP = (R/'src/game/mpmenu.c').read_text(errors='replace')
MK = (R/'Makefile').read_text(errors='replace')

checks=[]
def ck(name, cond):
    cond=bool(cond); checks.append((name,cond)); print(('[PASS] ' if cond else '[FAIL] ')+name)

def between(src, start, end):
    a=src.find(start)
    if a < 0: return ''
    b=src.find(end,a+len(start))
    return src[a:] if b < 0 else src[a:b]

front_patch = between(FR, 'static void frontPatchesChange', 'static void frontThirdPersonOptionsChange')
front_tp = between(FR, 'static void frontThirdPersonOptionsChange', 'void init_menu_patches')
front_patch_ui = between(FR, 'void init_menu_patches', 'Gfx *constructor_menu_patches')
front_tp_ui = between(FR, 'void init_menu_third_person_options', 'Gfx *constructor_menu_third_person_options')
front_spchar = between(FR, 'void init_menu_single_player_character', 'Gfx *constructor_menu_single_player_character')

ck('frontend Patches edits mark dirty without writing immediately',
   'g_ModOptionsDirty = TRUE;' in front_patch and 'frontModStoreCurrentSettings();' not in front_patch)
ck('frontend Third-Person edits mark dirty without writing immediately',
   'g_ModOptionsDirty = TRUE;' in front_tp and 'frontModStoreCurrentSettings();' not in front_tp)
ck('frontend SP Character edits mark dirty without writing immediately',
   'g_ModOptionsDirty = TRUE;' in front_spchar and
   'frontModStoreCurrentSettings();' not in front_spchar)
ck('frontend deferred helper performs the one physical menu-exit commit',
   'static void frontModCommitDeferredSubmenuSettings(void)' in FR and
   'frontModStoreCurrentSettings();\n    g_ModOptionsDirty = FALSE;' in FR)
ck('frontend Patches exit commits dirty settings',
   'frontModCommitDeferredSubmenuSettings();' in front_patch_ui and
   'frontChangeMenu(MENU_MOD_OPTIONS, FALSE);' in front_patch_ui)
ck('frontend Third-Person exit commits dirty settings',
   'frontModCommitDeferredSubmenuSettings();' in front_tp_ui and
   'frontChangeMenu(MENU_ENHANCEMENTS_OPTIONS, FALSE);' in front_tp_ui)
ck('frontend SP Character exit commits dirty settings',
   'frontModCommitDeferredSubmenuSettings();' in front_spchar and
   'frontChangeMenu(MENU_MOD_OPTIONS, FALSE);' in front_spchar)
ck('frontend submenu Previous-tab exits use the same commit path',
   front_patch_ui.count('frontCheckCursorOnPreviousTab()') == 1 and
   front_tp_ui.count('frontCheckCursorOnPreviousTab()') == 1 and
   'frontCheckCursorOnPreviousTab()' in front_spchar)

sp_nav = between(O, 'void watch_special_options_navigation(void)', '// WATCH_INDEX_GAME_OPTIONS')
ck('SP Watch submenu changes remain RAM-only until a deferred commit',
   'g_ModWatchSettingsDirty = TRUE;' in O and
   'static void modWatchCommitDeferredSettings(void)' in O)
ck('SP Watch commits parent Special Options when entering another page',
   sp_nav.count('modWatchCommitDeferredSettings();') >= 6 and
   'MODWATCH_STATE = MODWATCH_MODE_PATCHES;' in sp_nav and
   'MODWATCH_STATE = MODWATCH_MODE_TP_OPTIONS;' in sp_nav)
ck('SP Watch Patches and TP Options commit once when backing out',
   'if (mode == MODWATCH_MODE_PATCHES)\n        {\n            modWatchCommitDeferredSettings();' in sp_nav and
   'if (mode == MODWATCH_MODE_TP_OPTIONS)\n        {\n            modWatchCommitDeferredSettings();' in sp_nav)

mp_store = between(MP, 'static void mpwatchConfigStoreGlobals', 'static void mpwatchConfigToggleSpecial')
mp_toggle = between(MP, 'static void mpwatchConfigToggleSpecial', 'static s32 mpwatchConfigSpecialValue')
mp_patch = between(MP, 'static void mpwatchConfigTogglePatch', 'static s32 mpwatchConfigPatchValue')
mp_tp = between(MP, 'static void mpwatchConfigToggleTpOption', 'static s32 mpwatchConfigTpOptionValue')
mp_adjust = between(MP, 'static void mpwatchConfigAdjustTpCamera', 'static void mpwatchConfigTpValueText')
mp_input = between(MP, 'static s32 mpwatchConfigHandleInput', 'typedef enum MPWATCH_CLASS')

ck('MP Watch has one shared dirty latch for save-backed Special Options',
   'static u8 mpcfg_globals_dirty;' in MP)
ck('MP Watch store is dirty-gated and clears after the commit',
   'if (!mpcfg_globals_dirty)' in mp_store and
   'fileWriteSave(save);' in mp_store and
   'mpcfg_globals_dirty = FALSE;' in mp_store)
ck('MP base Special toggles no longer write EEPROM immediately',
   'mpcfg_globals_dirty = TRUE;' in mp_toggle and 'mpwatchConfigStoreGlobals();' not in mp_toggle)
ck('MP Patches toggles no longer write EEPROM immediately',
   'mpcfg_globals_dirty = TRUE;' in mp_patch and 'mpwatchConfigStoreGlobals();' not in mp_patch)
ck('MP Third-Person toggles no longer write EEPROM immediately',
   'mpcfg_globals_dirty = TRUE;' in mp_tp and 'mpwatchConfigStoreGlobals();' not in mp_tp)
ck('MP Third-Person camera adjustments no longer write EEPROM per step',
   'mpcfg_globals_dirty = TRUE;' in mp_adjust and 'mpwatchConfigStoreGlobals();' not in mp_adjust)
ck('MP Watch commits on exits from Special/Patches/TP Options',
   mp_input.count('mpwatchConfigStoreGlobals();') >= 4 and
   'if (mode == 5)' in mp_input and 'if (mode == 6)' in mp_input and 'if (mode == 2)' in mp_input)
ck('MP store serializes extension state before the physical file write',
   mp_store.find('fileStoreExtendedSettings(save);') < mp_store.find('fileWriteSave(save);'))

ck('R27E audit is mandatory build prerequisite',
   'r27e-deferred-special-options-save-audit:' in MK and
   'r27e-deferred-special-options-save-audit' in next((x for x in MK.splitlines() if x.startswith('prerequisites:')),''))

bad=[n for n,c in checks if not c]
print()
print(f"R27E DEFERRED SPECIAL-OPTIONS SAVE AUDIT: {'PASS' if not bad else 'FAIL'} ({len(checks)-len(bad)}/{len(checks)})")
if bad:
    for n in bad: print(' - '+n)
    sys.exit(1)
