#!/usr/bin/env python3
from pathlib import Path
import sys

R = Path(__file__).resolve().parents[1]
F2 = (R / "src/game/file2.c").read_text(errors="replace")
O = (R / "src/game/options.c").read_text(errors="replace")
OH = (R / "src/game/options.h").read_text(errors="replace")
FR = (R / "src/game/front.c").read_text(errors="replace")
SP = (R / "src/game/spectrum.c").read_text(errors="replace")
MP = (R / "src/game/mpmenu.c").read_text(errors="replace")
BV = (R / "src/game/bondview2.c").read_text(errors="replace")
BVC = (R / "src/game/bondview.c").read_text(errors="replace")
CA = (R / "src/game/chraction.c").read_text(errors="replace")
PO = (R / "src/game/propobj.c").read_text(errors="replace")
BC = (R / "src/bondconstants.h").read_text(errors="replace")
MK = (R / "Makefile").read_text(errors="replace")

checks = []
def ck(name, cond):
    cond = bool(cond)
    checks.append((name, cond))
    print(("[PASS] " if cond else "[FAIL] ") + name)

ck("jump animation implementation is preserved but presentation is disabled",
   "#define MOD_EXPERIMENTAL_JUMP_ANIMATIONS_ENABLED 0" in BV
   and "R27U R10: presentation-only Experimental Jump animation" in BV
   and "ANIM_surface_vent_jump" in BV and "ANIM_dancing" in BV
   and "MOD_EXPERIMENTAL_JUMP_ANIMATIONS_ENABLED" in BV)

ck("jump physics and R9 collision remain active",
   "#define MOD_EXPERIMENTAL_JUMP_HEIGHT 128.0f" in BV
   and "R27U R7 R1: compact upward head sweep against real BG" in BV
   and "R27U R9 R5 R2: airborne upper-wall BG sweep" in BV)

ck("Enhancements runtime toggle defaults Off",
   "u8 g_ModAdditionalPlayerDeathAnimationsEnabled;" in O
   and "extern u8 g_ModAdditionalPlayerDeathAnimationsEnabled;" in OH
   and "g_ModAdditionalPlayerDeathAnimationsEnabled = FALSE;" in F2)

ck("Enhancements persists per folder in the four free R27D tail bytes",
   "GE_SRAM_EXT_TAIL_ENH_BASE (MAX_FOLDER_COUNT * GE_SRAM_EXT_TAIL_SP_STRIDE)" in F2
   and "GE_SRAM_EXT_TAIL_ENH_ADDITIONAL_PLAYER_DEATHS 0x01" in F2
   and "ge_sram_ext_enh_tail_must_fit" in F2
   and "GE_SRAM_EXT_TAIL_ENH_BASE + folder" in F2
   and "reserved_tail[enh] = enhflags" in F2)

ck("packed labels are exact and scratch grew only in BSS",
   "static u32 text[4][10];" in SP
   and "index == 76" in SP and "0x456E6861" in SP
   and "index == 77" in SP and "0x41646469" in SP
   and "0x65720000" in SP)

ck("main Special Options renders Enhancements after Experimental",
   "page2labels[10] = {12,13,14,15,30,16,8,29,74,76}" in FR
   and "s32 count = g_ModOptionsPage ? 10 : 11;" in FR
   and "s32 maxrow = g_ModOptionsPage ? 9 : 10;" in FR
   and "MENU_ENHANCEMENTS_OPTIONS" in BC
   and "row == 9" in FR
   and "frontChangeMenu(MENU_ENHANCEMENTS_OPTIONS, FALSE);" in FR
   and "MENU_ADDITIONAL_DEATH_ANIMATIONS" in BC
   and "frontChangeMenu(MENU_ADDITIONAL_DEATH_ANIMATIONS, FALSE);" in FR
   and "interface_menu_additional_death_animations" in FR
   and '"Additional Death Animations For Player >"' in FR)

_FRWS = " ".join(FR.split())
ck("Experimental, Enhancements and Additional Death Animations share compact wrapper-free frontend init",
   "void init_menu_experimental_options(void)" not in FR
   and "init_menu_experimental_options();" not in FR
   and "case MENU_EXPERIMENTAL_OPTIONS: tab_prev_highlight = FALSE; load_walletbond(); break;" in _FRWS
   and "case MENU_ENHANCEMENTS_OPTIONS: tab_prev_highlight = FALSE; load_walletbond(); break;" in _FRWS
   and "case MENU_ADDITIONAL_DEATH_ANIMATIONS: tab_prev_highlight = FALSE; load_walletbond(); break;" in _FRWS
   and FR.count("tab_prev_highlight = FALSE; load_walletbond();") == 3)

ck("SP Watch Enhancements is three deferred-save rows before Cheats",
   "#define MODWATCH_OPTION_ROWS 18" in O
   and "#define MODWATCH_MODE_ENHANCEMENTS 0x700" in O
   and "#define MODWATCH_ENHANCEMENTS_ROWS 3" in O
   and "MODWATCH_STATE = MODWATCH_MODE_ENHANCEMENTS;" in O
   and "g_ModAlwaysShowCrosshairEnabled" in O
   and "g_ModDisableBodyArmorEnabled" in O
   and "g_ModWatchSettingsDirty = TRUE;" in O)

ck("SP Watch parent order is Debug 14 / Experimental 15 / Enhancements 16 / Cheats 17",
   "#define MODWATCH_OPTION_ROWS 18" in O
   and "row == 14" in O
   and "MODWATCH_STATE = MODWATCH_MODE_DEBUG;" in O
   and "row == 15" in O
   and "MODWATCH_STATE = MODWATCH_MODE_EXPERIMENTAL;" in O
   and "row == 16" in O
   and "MODWATCH_STATE = MODWATCH_MODE_ENHANCEMENTS;" in O
   and "MODWATCH_STATE = MODWATCH_MODE_CHEATS;" in O)

ck("MP/Co-op Watch Enhancements is three-row mode 9 before Cheats",
   "if (mode == 2) return 19;" in MP
   and "if (mode == 9) return 3;" in MP
   and "mpcfg_mode[player]=9" in MP
   and "mpcfg_row[player]=17" in MP
   and 'mpcfg_mode[curplayernum] == 9 ? "ENHANCEMENTS"' in MP
   and "g_ModAlwaysShowCrosshairEnabled" in MP
   and "g_ModDisableBodyArmorEnabled" in MP
   and "mpcfg_globals_dirty = TRUE;" in MP)

_h0 = CA.find("bool handles_shot_actors(ChrRecord *self, s32 hitpart, coord3d *vector, s32 weaponid, bool isPlayer)")
_h1 = CA.find("bool chrCanSeeBond(", _h0)
_HIT = CA[_h0:_h1] if _h0 >= 0 and _h1 > _h0 else ""

_wdamage = _HIT.find("record_damage_kills(damageToCause * 0.125f, vector->x, vector->z, playerNum, 1);")
_wmarker = _HIT.find("R27V: use the exact guard wall-stagger qualification for Bond")
_wsetcur = _HIT.find("set_cur_player(playerNum);", _wmarker)

ck("direct player gunshots mark wall-death context before damage",
   "g_ModPlayerGunshotDeathContext =" in _HIT
   and _HIT.find("g_ModPlayerGunshotDeathContext =")
       < _HIT.find("record_damage_kills(damageToCause * 0.125f")
   and "g_ModPlayerGunshotDeathContext = FALSE;" in _HIT
   and "R27V: use the exact guard wall-stagger qualification for Bond" not in _HIT)

_g0 = CA.find("void chrlvUpdateShotbondsum(ChrRecord *self, s32 *arg1, s32 *arg2, ITEM_IDS item)")
_g1 = CA.find("s32 sub_GAME_7F02D630(", _g0)
_GUARDSHOT = CA[_g0:_g1] if _g0 >= 0 and _g1 > _g0 else ""
_gdamage = _GUARDSHOT.find("bondviewCallRecordDamageKills(t2, subroty, -1, 1);")
_gmarker = _GUARDSHOT.find("R27V R12: ordinary guard gunfire wall-stagger death")
_greset = _GUARDSHOT.find("self->shotbondsum = 0.0f;", _gmarker)

ck("ordinary guard gunfire marks context before damage and clears it after",
   "R27V R14: mark only front-half ordinary guard gunfire" in _GUARDSHOT
   and _GUARDSHOT.find("g_ModPlayerGunshotDeathContext =")
       < _GUARDSHOT.find("bondviewCallRecordDamageKills(t2, subroty, -1, 1);")
   and _GUARDSHOT.find("g_ModPlayerGunshotDeathContext = FALSE;")
       > _GUARDSHOT.find("bondviewCallRecordDamageKills(t2, subroty, -1, 1);")
   and "R27V R12: ordinary guard gunfire wall-stagger death" not in _GUARDSHOT)

ck("wall death preserves enemy stagger's random mirror flip",
   "R27V R14: pre-kill wall-stagger qualification" in BV
   and "padding = 2 + (randomGetNext() & 1);" in BV
   and "g_CurrentPlayer->startnewbonddie = padding;" in BV
   and "#define MOD_PLAYER_DEATH_WALL_FLIP0 2" in BV
   and "#define MOD_PLAYER_DEATH_WALL_FLIP1 3" in BV
   and "PTR_ANIM_death_stagger_back_to_wall" in BV)

ck("Facility toxic gas selects Death Neck only on the lethal gas tick",
   "void handle_gas_damage(void)" in PO
   and "r27v_player_was_alive = g_CurrentPlayer->bonddead == FALSE;" in PO
   and "bossGetStageNum() == LEVELID_FACILITY" in PO
   and "g_CurrentPlayer->bonddead != FALSE" in PO
   and "g_CurrentPlayer->startnewbonddie = 4;" in PO
   and "#define MOD_PLAYER_DEATH_GAS_NECK   4" in BV
   and "PTR_ANIM_death_neck" in BV)

_d0 = BVC.find("s32 g_bondviewBondDeathAnimations[]")
_d1 = BVC.find("s32 g_bondviewBondDeathAnimationsCount", _d0)
_DEATHPOOL = BVC[_d0:_d1] if _d0 >= 0 and _d1 > _d0 else ""

ck("special player deaths do not enter Bond's ordinary random pool",
   _DEATHPOOL
   and "PTR_ANIM_death_stagger_back_to_wall" not in _DEATHPOOL
   and "PTR_ANIM_death_neck" not in _DEATHPOOL)

ck("authoritative special deaths are mirrored by visible TP/MP bodies",
   "these two contextual deaths are deliberately not members" in BV
   and "PTR_ANIM_death_stagger_back_to_wall + (s32)ptr_animation_table" in BV
   and "PTR_ANIM_death_neck + (s32)ptr_animation_table" in BV
   and "modelGetAnimSpeed((Model *)&ppointers[index]->model)" in BV)


_r0 = BV.find("void record_damage_kills(f32 damage_amount")
_r1 = BV.find("void bondviewCallRecordDamageKills(", _r0)
_RDAMAGE = BV[_r0:_r1] if _r0 >= 0 and _r1 > _r0 else ""
_rprobe = _RDAMAGE.find("R27V R14: pre-kill wall-stagger qualification")
_rkill = _RDAMAGE.find("bondviewKillCurrentPlayer();")

ck("wall geometry is tested before Bond enters the dead state",
   _rprobe >= 0 and _rkill > _rprobe
   and _RDAMAGE[_rprobe:_rkill].count(
       "chrlvPathingCollisionRelated7F0264B0(") == 2
   and "(randomGetNext() % (u32)0x14) == 0" in _RDAMAGE[_rprobe:_rkill]
   and "150.0f" in _RDAMAGE[_rprobe:_rkill]
   and "10.0f" in _RDAMAGE[_rprobe:_rkill])

ck("Death Stagger Back To Wall remains the exact BHEAD clip with natural end",
   "PTR_ANIM_death_stagger_back_to_wall" in BV
   and "g_CurrentPlayer->startnewbonddie - MOD_PLAYER_DEATH_WALL_FLIP0" in BV
   and "modelSetAnimEndFrame(&g_CurrentPlayer->model, 67.0f);" not in BV
   and "R27V R13: FP-safe wall death" not in BV)

ck("Always Show Crosshair is a persisted Enhancements bit defaulting Off",
   "u8 g_ModAlwaysShowCrosshairEnabled;" in O
   and "GE_SRAM_EXT_TAIL_ENH_ALWAYS_SHOW_CROSSHAIR    0x02" in F2
   and "g_ModAlwaysShowCrosshairEnabled = FALSE;" in F2
   and "GE_SRAM_EXT_TAIL_ENH_ALWAYS_SHOW_CROSSHAIR" in F2)

ck("Always Show Crosshair remains the second Enhancements row",
   "index == 78" in SP
   and "g_ModAlwaysShowCrosshairEnabled" in FR
   and "g_ModAlwaysShowCrosshairEnabled" in O
   and "g_ModAlwaysShowCrosshairEnabled" in MP
   and "#define MODWATCH_ENHANCEMENTS_ROWS 3" in O
   and "if (mode == 9) return 3;" in MP)

ck("Always Show Crosshair bypasses only the damage sight-reason bit",
   "gunSetSightVisible(GUNSIGHTREASON_DAMAGE," in BV
   and "g_ModAlwaysShowCrosshairEnabled" in BV
   and "GUNSIGHTREASON_NOTAIMING" in BV)

ck("R27V audit is mandatory",
   "r27v-additional-player-deaths-audit:" in MK
   and "r27v-additional-player-deaths-audit" in
      next((x for x in MK.splitlines() if x.startswith("prerequisites:")), ""))

bad = [name for name, ok in checks if not ok]
if "Experimental, Enhancements and Additional Death Animations share compact wrapper-free frontend init" in bad:
    print("[DEBUG] compact init phrase count:",
          FR.count("tab_prev_highlight = FALSE; load_walletbond();"))
    print("[DEBUG] normalized Experimental init:",
          "case MENU_EXPERIMENTAL_OPTIONS: tab_prev_highlight = FALSE; load_walletbond(); break;" in _FRWS)
    print("[DEBUG] normalized Enhancements init:",
          "case MENU_ENHANCEMENTS_OPTIONS: tab_prev_highlight = FALSE; load_walletbond(); break;" in _FRWS)
    print("[DEBUG] normalized Additional Death Animations init:",
          "case MENU_ADDITIONAL_DEATH_ANIMATIONS: tab_prev_highlight = FALSE; load_walletbond(); break;" in _FRWS)
print()
print("R27V R14 ADDITIONAL PLAYER DEATHS / ENHANCEMENTS AUDIT: %s (%d/%d)" %
      ("PASS" if not bad else "FAIL", len(checks) - len(bad), len(checks)))
if bad:
    for name in bad:
        print(" - " + name)
    sys.exit(1)
