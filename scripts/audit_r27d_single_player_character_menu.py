#!/usr/bin/env python3
from pathlib import Path
import re
import sys

R = Path(__file__).resolve().parents[1]
FR = (R/'src/game/front.c').read_text(errors='replace')
FH = (R/'src/game/front.h').read_text(errors='replace')
F2 = (R/'src/game/file2.c').read_text(errors='replace')
LV = (R/'src/game/lv.c').read_text(errors='replace')
BV = (R/'src/game/bondview2.c').read_text(errors='replace')
O = (R/'src/game/options.c').read_text(errors='replace')
OH = (R/'src/game/options.h').read_text(errors='replace')
MP = (R/'src/game/mpmenu.c').read_text(errors='replace')
BC = (R/'src/bondconstants.h').read_text(errors='replace')
SPEC = (R/'src/game/spectrum.c').read_text(errors='replace')
MK = (R/'Makefile').read_text(errors='replace')

checks=[]
def ck(name, cond):
    cond=bool(cond); checks.append((name,cond)); print(('[PASS] ' if cond else '[FAIL] ')+name)

# Count the first mp_chr_setup branch. It is 64 today; runtime code must still use sizeof.
m = re.search(r'struct MP_selectable_chars mp_chr_setup\[\]\s*=\s*\{(.*?)\n\};', FR, re.S)
entries = re.findall(r'^\s*\{getStringID\(', m.group(1), re.M) if m else []

ck('current multiplayer character table exposes all 65 entries including Josh_7774', len(entries) == 65)
ck('SP selector count follows mp_chr_setup size instead of retail unlock cutoff',
   'sizeof(mp_chr_setup) / sizeof(mp_chr_setup[0])' in FR and
   'frontGetMpCharacterCount' in FR and
   'num_chars_selectable_mp' not in FR[FR.find('s32 frontGetMpCharacterCount'):FR.find('void unlock_all_mp_chars')])
ck('Single-Player Options is an appended frontend menu', 'MENU_SINGLE_PLAYER_CHARACTER,' in BC)
_main_opts_start = FR.find('Gfx *constructor_menu_mod_options(Gfx *DL)')
_main_opts_end = FR.find('static void frontModStoreCurrentSettings(void)', _main_opts_start)
_main_opts = FR[_main_opts_start:_main_opts_end]
ck('main Special Options keeps Single-Player Options in the right-side frontend cluster',
   _main_opts_start >= 0 and _main_opts_end > _main_opts_start and
   _main_opts.find('frontModGetOptionLabel(53)') < _main_opts.find('frontModGetOptionLabel(66)') < _main_opts.find('frontModGetOptionLabel(71)'))
ck('SP options menu is main-frontend only and not added to Watch menus',
   'MENU_SINGLE_PLAYER_CHARACTER' not in MP and 'MENU_SINGLE_PLAYER_CHARACTER' not in (R/'src/game/options.c').read_text(errors='replace'))
ck('packed labels provide Single-Player Options, Character Modifier, Match View Height and Disabled',
   all(x in SPEC for x in ['index == 66', 'index == 67', 'index == 68', 'index == 69']))
ck('Character Modifier uses explicit Disabled zero state and table names for selections',
   'g_ModSinglePlayerCharacter == 0' in FR and
   'langGet(mp_chr_setup[g_ModSinglePlayerCharacter - 1].text_preset)' in FR)
ck('SP override reads body/head/pov from the selected character definition',
   all(x in FR for x in ['mp_chr_setup[index].body','mp_chr_setup[index].head','mp_chr_setup[index].pov']))
ck('Disabled leaves retail Bond outfit path untouched and override is solo-only',
   'frontGetSinglePlayerCharacterOverride(&body, &head, NULL);' in BV and
   'getPlayerCount() == 1 && gamemode != GAMEMODE_MULTI' in BV and
   'get_scenario() != SCENARIO_COOP' in BV)
ck('Match View Height reuses authored multiplayer pov and stays solo-only',
   'g_ModSinglePlayerMatchViewHeight' in LV and
   'frontGetSinglePlayerCharacterOverride(NULL, NULL, &pov)' in LV and
   'player_data->player_perspective_height = pov;' in LV and
   'gamemode != GAMEMODE_MULTI' in LV and 'get_scenario() != SCENARIO_COOP' in LV)
ck('SP modifier defaults are Disabled and Match View Height Off',
   'u16 g_ModSinglePlayerCharacter;' in O and 'u8 g_ModSinglePlayerMatchViewHeight;' in O and
   'g_ModSinglePlayerCharacter = 0;' in F2 and 'g_ModSinglePlayerMatchViewHeight = FALSE;' in F2)
ck('SP modifier persists per folder in unused extension tail without changing 8-byte records',
   'GE_SRAM_EXT_TAIL_SP_STRIDE 3u' in F2 and
   'GE_SRAM_EXT_TAIL_SP_CHAR_HI' in F2 and 'GE_SRAM_EXT_TAIL_SP_CHAR_LO' in F2 and
   'GE_SRAM_EXT_TAIL_SP_MATCH_VIEW_HEIGHT' in F2 and
   'ge_sram_ext_record_must_be_8' in F2 and 'ge_sram_ext_bank_must_be_64' in F2)
ck('SP settings update uses one journal commit after record and tail changes',
   'Keep the folder record and the R27D tail bytes in the same journal' in F2 and
   'g_GeSramExtBank.folders[folder] = record;' in F2)
ck('SP character submenu uses crosshair hit rows and standard Previous tab',
   'cursor_v_pos >= 75.0f' in FR and 'cursor_v_pos >= 103.0f' in FR and
   'frontAddPreviousTabText(DL)' in FR[FR.find('constructor_menu_single_player_character'):FR.find('//LEVEL MODIFIERS FRONTEND')])
ck('Level Modifiers category/list pages use crosshair selection and standard Previous tab',
   'cursor_v_pos >= 70.0f' in FR and
   ('cursor_v_pos >= 78.0f' in FR
    or 'cursor_v_pos >= (f32)LEVELMOD_FRONT_ROW_Y' in FR) and
   FR[FR.find('//LEVEL MODIFIERS FRONTEND'):FR.find('//BASIC MAP MAKER FRONTEND')].count('frontAddPreviousTabText(DL)') >= 3)
ck('Map Maker chooser uses crosshair selection with Basic above Advanced',
   'cursor_v_pos >= 68.0f' in FR and 'cursor_v_pos >= 132.0f' in FR and
   's32 optiony = i ? 142 : 78;' in FR)
ck('Map Maker emphasizes Basic/Advanced with Bank Gothic and keeps centered descriptors',
   'char *title = i ? "Advanced Map" : "Basic Map";' in FR and
   'char *description = i ? "Meshes / Rooms / Portals" : "3D Module Editor";' in FR and
   'ptrFontBankGothicChars, ptrFontBankGothic' in FR[FR.find('constructor_menu_map_maker'):FR.find('init_menu_map_maker_basic')] and
   'x = 220 - (w >> 1);' in FR[FR.find('constructor_menu_map_maker'):FR.find('init_menu_map_maker_basic')])
ck('Map Maker chooser uses standard Previous tab instead of custom B-back text',
   'frontAddPreviousTabText(DL)' in FR[FR.find('constructor_menu_map_maker'):FR.find('init_menu_map_maker_basic')] and
   'Basic: fast modules' not in FR[FR.find('constructor_menu_map_maker'):FR.find('init_menu_map_maker_basic')])
ck('main Special Options right-side column moved exactly four pixels left',
   'cursor_h_pos >= 246.0f' in FR and 'x = 248;' in FR and
   'microcode_constructor_related_to_menus(DL,246,65,248 + uw + 2,81,0x32)' in FR)
ck('R27D audit is mandatory build prerequisite',
   'r27d-single-player-character-menu-audit:' in MK and
   'r27d-single-player-character-menu-audit' in next((x for x in MK.splitlines() if x.startswith('prerequisites:')),''))

bad=[n for n,c in checks if not c]
print()
print(f"R27D SINGLE-PLAYER CHARACTER/MENU AUDIT: {'PASS' if not bad else 'FAIL'} ({len(checks)-len(bad)}/{len(checks)})")
if bad:
    for n in bad: print(' - '+n)
    sys.exit(1)
