#!/usr/bin/env python3
from pathlib import Path
import math
import re
import sys

R = Path(__file__).resolve().parents[1]

F2H = (R/"src/game/file2.h").read_text(errors="replace")
F2  = (R/"src/game/file2.c").read_text(errors="replace")
SP  = (R/"src/game/spectrum.c").read_text(errors="replace")
FR  = (R/"src/game/front.c").read_text(errors="replace")
O   = (R/"src/game/options.c").read_text(errors="replace")
MP  = (R/"src/game/mpmenu.c").read_text(errors="replace")
BV  = (R/"src/game/bondview2.c").read_text(errors="replace")
BC  = (R/"src/bondconstants.h").read_text(errors="replace")
MK  = (R/"Makefile").read_text(errors="replace")

checks=[]

def ck(name, cond):
    cond=bool(cond)
    checks.append((name,cond))
    print(("[PASS] " if cond else "[FAIL] ")+name)

ck("Experimental Jumping runtime option is independent and defaults Off",
   "#define MODOPT3_EXPERIMENTAL_JUMP       0x02" in F2H
   and "DEFAULT_MOD_OPTIONS3" in F2H
   and "MODOPT3_EXPERIMENTAL_JUMP" not in
       next((x for x in F2H.splitlines() if x.startswith("#define DEFAULT_MOD_OPTIONS3")), ""))

ck("runtime API preserves MODOPT3 signature",
   "s32 modExperimentalJumpEnabled(void)" in F2
   and "void modSetExperimentalJumpEnabled(s32 enabled)" in F2
   and "MODOPT3_SIGNATURE_MASK" in F2)

ck("EEPROM/SRAM journal uses free flags bit 0x40",
   "#define GE_SRAM_EXT_FLAG_EXPERIMENTAL_JUMP 0x40" in F2
   and "record->flags & GE_SRAM_EXT_FLAG_EXPERIMENTAL_JUMP" in F2
   and "record.flags |= GE_SRAM_EXT_FLAG_EXPERIMENTAL_JUMP" in F2)

ck("generic legacy mirror writes preserve authoritative Jump flag",
   "current->flags & GE_SRAM_EXT_FLAG_EXPERIMENTAL_JUMP" in F2
   and "record.flags &= ~GE_SRAM_EXT_FLAG_EXPERIMENTAL_JUMP" in F2)

ck("old/no-extension paths force Jumping Off instead of reading camera signature bit 1",
   "g_ModGameplayOptions3 &= ~MODOPT3_EXPERIMENTAL_JUMP;" in F2
   and "g_ModGameplayOptions3 &= ~MODOPT3_EXPERIMENTAL_JUMP;" in FR)

ck("packed labels expose Experimental and Jumping (Press L)",
   "index == 74" in SP and "index == 75" in SP
   and "0x45787065" in SP and "0x4A756D70" in SP)

ck("main Special Options appends Experimental submenu row",
   "page2labels[10] = {12,13,14,15,30,16,8,29,74,76}" in FR
   and "MENU_EXPERIMENTAL_OPTIONS" in BC
   and "frontChangeMenu(MENU_EXPERIMENTAL_OPTIONS, FALSE);" in FR)

ck("frontend Experimental submenu is one save-backed On/Off row",
   "void interface_menu_experimental_options(void)" in FR
   and "Gfx *constructor_menu_experimental_options" in FR
   and "modSetExperimentalJumpEnabled(!modExperimentalJumpEnabled());" in FR
   and "g_ModOptionsDirty = TRUE;" in FR)

ck("SP Watch appends Experimental after Debug and before Cheats",
   "#define MODWATCH_OPTION_ROWS 18" in O
   and "#define MODWATCH_MODE_EXPERIMENTAL 0x600" in O
   and "MODWATCH_STATE = MODWATCH_MODE_EXPERIMENTAL;" in O
   and "row == 15" in O
   and "frontModGetOptionLabel(74)" in O
   and "frontModGetOptionLabel(75)" in O)

ck("SP Experimental toggle follows deferred-save policy",
   "else if (mode == MODWATCH_MODE_EXPERIMENTAL)" in O
   and "modSetExperimentalJumpEnabled(!modExperimentalJumpEnabled());" in O
   and "g_ModWatchSettingsDirty = TRUE;" in O
   and "modWatchCommitDeferredSettings();" in O
   and "modWatchToggleExperimentalOption" not in O)

ck("MP/Co-op Watch appends Experimental mode 8 after Debug",
   "if (mode == 2) return 19;" in MP
   and "if (mode == 7 || mode == 8) return 1;" in MP
   and "mpcfg_mode[player]=8" in MP
   and "mpcfg_row[player]=16" in MP
   and 'mpcfg_mode[curplayernum] == 8 ? "EXPERIMENTAL"' in MP)

ck("MP Experimental toggle follows deferred-save policy",
   "else if (mode == 8)" in MP
   and "modSetExperimentalJumpEnabled(!modExperimentalJumpEnabled());" in MP
   and "mpcfg_globals_dirty = TRUE;" in MP
   and "mpwatchConfigStoreGlobals();" in MP
   and "mpwatchConfigToggleExperimental" not in MP
   and "mpwatchConfigExperimentalValue" not in MP)

ck("fresh L queues a per-player jump request",
   "g_ModExperimentalJumpRequestMask |= 1u << abChordPlayer;" in BV
   and "((buttons & ~oldbuttons) & L_TRIG)" in BV)

ck("Jumping steals L from manual-aim/inventory mapping only while enabled",
   "stack_padding_sp9C = modExperimentalJumpEnabled();" in BV
   and "if (stack_padding_sp9C)" in BV
   and "aimButtons &= ~L_TRIG;" in BV
   and "invButtons &= ~L_TRIG;" in BV)

ck("Jumping steals L from TP B+L shoulder handling while enabled",
   BV.count("freshshoulder &= ~L_TRIG;") >= 2)

# Scope ordering checks to the actual vertical-update function. bondview2.c
# contains other Map Maker/Fly references earlier in the file, so whole-file
# BV.find() ordering is not meaningful here.
_y0 = BV.find("void bondviewUpdatePlayerY(s32 use_stanHeight, f32 stanHeight_offset)")
_y1 = BV.find("void bondviewUpdatePlayerCollisionPositionFields(void)", _y0)
_Y = BV[_y0:_y1] if _y0 >= 0 and _y1 > _y0 else ""

_jump_consume = _Y.find("g_ModExperimentalJumpRequestMask &=")
_mapmaker_early = _Y.find("mapmakerNativeTestActive()")
_fly_early = _Y.find("CHEAT_FLY_MODE")
_noclip_early = _Y.find("CHEAT_NO_CLIPPING")

ck("jump request is consumed before Fly/NoClip/MapMaker vertical early returns",
   _jump_consume >= 0
   and _mapmaker_early > _jump_consume
   and _fly_early > _jump_consume
   and _noclip_early > _jump_consume)

ck("jump requires grounded live player with a valid STAN and not tank",
   "g_CurrentPlayer->field_7C == 0.0f" in BV
   and "g_CurrentPlayer->field_488.current_tile_ptr != NULL" in BV
   and "g_PlayerIsInTank == 0" in BV
   and "g_CurrentPlayer->bonddead == FALSE" in BV)

ck("jump height requirement is encoded as 128.0f",
   "#define MOD_EXPERIMENTAL_JUMP_HEIGHT 128.0f" in BV
   and "#define MOD_EXPERIMENTAL_JUMP_NORMAL_TICKS 30.0f" in BV
   and "#define MOD_EXPERIMENTAL_JUMP_FAST_TICKS 14.0f" in BV
   and "MOD_EXPERIMENTAL_JUMP_VELOCITY_NORMAL" in BV
   and "MOD_EXPERIMENTAL_JUMP_VELOCITY_FAST" in BV)

# Independently verify the two sampled apex formulas.
okphysics=True
for gravity,ticks in ((0.27777779,30),(1.388889,14)):
    v=(128.0 + 0.5*gravity*ticks*ticks)/ticks
    y=0.0
    maxy=0.0
    for _ in range(ticks+5):
        v2=v-gravity
        y+=(v+v2)*0.5
        v=v2
        maxy=max(maxy,y)
    okphysics = okphysics and abs(maxy-128.0) < 0.001

ck("GoldenEye trapezoid integration peaks at 128.000 in normal and Fast-Bond gravity",
   okphysics)

# R27U R4 R1: IDO/C89 requires local declarations before executable statements.
_load0 = F2.find("s32 fileLoadExtendedSettings(save_data *save)")
_load1 = F2.find("void fileStoreExtendedSettings(save_data *save)", _load0)
_LOAD = F2[_load0:_load1] if _load0 >= 0 and _load1 > _load0 else ""
_decl_pos = _LOAD.find("u32 folder;")
_default_pos = _LOAD.find("modSetExperimentalJumpEnabled(FALSE);")

ck("extended-settings loader keeps C89 declarations before Experimental default",
   _decl_pos >= 0 and _default_pos > _decl_pos)

ck("single-use Experimental menu helpers are compacted",
   "void init_menu_experimental_options(void)" not in FR
   and "void update_menu_experimental_options(void)" not in FR
   and "update_menu_debug_options();" in FR
   and "init_menu_experimental_options();" not in FR
   and "modWatchToggleExperimentalOption" not in O
   and "mpwatchConfigToggleExperimental" not in MP
   and "mpwatchConfigExperimentalValue" not in MP)

# R27U R7 R1: compact ceiling collision for Experimental Jumping.
_y0 = BV.find("void bondviewUpdatePlayerY(s32 use_stanHeight, f32 stanHeight_offset)")
_y1 = BV.find("void bondviewUpdatePlayerCollisionPositionFields(void)", _y0)
_Y = BV[_y0:_y1] if _y0 >= 0 and _y1 > _y0 else ""

ck("jump ceiling collision uses rendered BG and cancels ascent",
   "R27U R7 R1: compact upward head sweep against real BG" in _Y
   and "modExperimentalJumpEnabled()" in _Y
   and "g_CurrentPlayer->field_7C > 0.0f" in _Y
   and "new_field_70 > g_CurrentPlayer->field_70" in _Y
   and "bondviewThirdPersonFindBackgroundHitFraction(" in _Y
   and "bondviewGetPlayerDuckingHeightRelated(g_CurrentPlayer) + 10.0f" in _Y
   and "new_field_7c = 0.0f;" in _Y)

ck("jump ceiling sweep is before landing correction",
   _Y.find("R27U R7 R1: compact upward head sweep against real BG") >= 0
   and _Y.find("R27U R7 R1: compact upward head sweep against real BG")
       < _Y.find("if (new_field_70 < g_CurrentPlayer->stanHeight)"))

# R27U R8 R1: N64 hot-path and fixed-slot compaction checks.
_r8_snapshot = "stack_padding_sp9C = modExperimentalJumpEnabled();"
_r8_queue = "g_ModExperimentalJumpRequestMask |= 1u << abChordPlayer;"
_r8_consume = ("i = 1u << get_cur_playernum();\n"
               "    modExperimentalJumpRequest = g_ModExperimentalJumpRequestMask & i;\n"
               "    g_ModExperimentalJumpRequestMask &= ~i;")
_r8_fresh_gate = ("if ((((buttons & ~oldbuttons) & L_TRIG) != 0)\n"
                  "            && stack_padding_sp9C\n"
                  "            && lvlGetControlsLockedFlag() == 0\n"
                  "            && disablePlayerActionsWhenPausedOrInMpMenu())")
_r8_ceiling_gate = ("if (g_CurrentPlayer->field_7C > 0.0f\n"
                    "                && new_field_70 > g_CurrentPlayer->field_70\n"
                    "                && modExperimentalJumpEnabled())")

ck("N64 jump hot paths avoid redundant option/player queries",
   BV.count(_r8_snapshot) == 1
   and BV.count(_r8_queue) == 1
   and BV.count(_r8_consume) == 1
   and "g_ModExperimentalJumpRequestMask |= 1u << get_cur_playernum();" not in BV
   and "g_ModExperimentalJumpRequestMask &= ~(1u << get_cur_playernum());" not in BV)

ck("fresh L short-circuits jump-only controls queries",
   BV.count(_r8_fresh_gate) == 1)

ck("ceiling path rejects falling before Jumping getter",
   BV.count(_r8_ceiling_gate) == 1
   and "R27U R7 R1: compact upward head sweep against real BG" in BV)

_FRWS = " ".join(FR.split())
ck("Experimental wrapper stays removed and all three compact submenu inits are inline",
   "void init_menu_experimental_options(void)" not in FR
   and "init_menu_experimental_options();" not in FR
   and "case MENU_EXPERIMENTAL_OPTIONS: tab_prev_highlight = FALSE; load_walletbond(); break;" in _FRWS
   and "case MENU_ENHANCEMENTS_OPTIONS: tab_prev_highlight = FALSE; load_walletbond(); break;" in _FRWS
   and "case MENU_ADDITIONAL_DEATH_ANIMATIONS: tab_prev_highlight = FALSE; load_walletbond(); break;" in _FRWS
   and FR.count("tab_prev_highlight = FALSE; load_walletbond();") == 3)

# R27U R9 R5: compact airborne upper-wall collision.
_c0 = BV.find("void bondviewCalcUpdatePlayerCollision(struct coord3d *offset, s32 allow_scoot)")
_c1 = BV.find("void bondviewUpdatePlayerY(s32 use_stanHeight, f32 stanHeight_offset)", _c0)
_C = BV[_c0:_c1] if _c0 >= 0 and _c1 > _c0 else ""

_y0 = BV.find("void bondviewUpdatePlayerY(s32 use_stanHeight, f32 stanHeight_offset)")
_y1 = BV.find("void bondviewUpdatePlayerCollisionPositionFields(void)", _y0)
_Y = BV[_y0:_y1] if _y0 >= 0 and _y1 > _y0 else ""

_w0 = _C.find("R27U R9 R5 R2: airborne upper-wall BG sweep")
_w1 = _C.find("Recover the player's floor tile if movement leaves the quick current-tile", _w0)
_W = _C[_w0:_w1] if _w0 >= 0 and _w1 > _w0 else ""

ck("airborne upper-wall collision lives before R27S destination recovery",
   _w0 >= 0 and _w1 > _w0
   and "R27S R9 R1: full-polygon authored STAN validation" in _C)

ck("airborne wall supplement is Jumping-only and ballistic-only",
   "g_CurrentPlayer->field_7C != 0.0f" in _W
   and "g_PlayerIsInTank == 0" in _W
   and "(g_ModGameplayOptions3 & MODOPT3_EXPERIMENTAL_JUMP)" in _W)

ck("airborne wall supplement reuses existing rendered-BG scratch",
   "temp_f2 = bondviewGetPlayerDuckingHeightRelated(g_CurrentPlayer) + 10.0f;" in _W
   and "collision3_pt0 = g_CurrentPlayer->bondprevpos;" in _W
   and "collision3_pt0.f[1] = g_CurrentPlayer->field_70 + temp_f2;" in _W
   and "collision3_pt1 = g_CurrentPlayer->field_488.collision_position;" in _W
   and "collision3_pt1.f[1] = g_CurrentPlayer->field_70 + temp_f2;" in _W
   and _W.count("bondviewThirdPersonFindBackgroundHitFraction(") == 1
   and "&temp_f2" in _W)

ck("airborne wall hit rejects only X/Z and restores pre-move STAN",
   "collision_position.f[0]" in _W
   and "bondprevpos.f[0]" in _W
   and "collision_position.f[2]" in _W
   and "bondprevpos.f[2]" in _W
   and "current_tile_ptr = r27s_prev_stan;" in _W
   and "field_7C =" not in _W
   and "stanHeight =" not in _W)

ck("Map Maker and No-Clipping keep earlier horizontal return paths",
   _C.find("mapmakerNativeTestActive()") >= 0
   and _C.find("mapmakerNativeTestActive()") < _w0
   and _C.find("g_CheatActivated[CHEAT_NO_CLIPPING]") >= 0
   and _C.find("g_CheatActivated[CHEAT_NO_CLIPPING]") < _w0)

ck("first-frame launch sentinel removes separate launch-only local",
   "modExperimentalJumpLaunch" not in _Y
   and "modExperimentalJumpRequest = -1;" in _Y
   and "|| modExperimentalJumpRequest < 0" in _Y)

# R27U R10: presentation-only takeoff/fall/landing animation.
_AT = (R / "assets/animationtable_data.c").read_text(errors="replace")
_p0 = BV.find("s32 playerTick(PropRecord *prop)")
_p1 = BV.find("Gfx * bondviewRemoved7F08BCB8(Gfx *arg0)", _p0)
_PT = BV[_p0:_p1] if _p0 >= 0 and _p1 > _p0 else ""

ck("R10 playerTick audit scope resolves the real full function",
   _p0 >= 0 and _p1 > _p0
   and "R27U R10: presentation-only Experimental Jump animation" in _PT
   and "tailret = chrTick(prop);" in _PT)

_a0 = _PT.find("R27U R10: presentation-only Experimental Jump animation")
_a1 = _PT.find("join_768:", _a0)
_JA = _PT[_a0:_a1] if _a0 >= 0 and _a1 > _a0 else ""

_s0 = _AT.find("u32 ANIM_DATA_surface_vent_jump[]")
_s1 = _AT.find("};", _s0)
_SURF = _AT[_s0:_s1] if _s0 >= 0 and _s1 > _s0 else ""

_d0 = _AT.find("u32 ANIM_DATA_dancing[]")
_d1 = _AT.find("};", _d0)
_DANCE = _AT[_d0:_d1] if _d0 >= 0 and _d1 > _d0 else ""

ck("jump animation frame contract is exact 00-28 / 29-56 / landing 1A-00",
   "#define MOD_EXPERIMENTAL_JUMP_ANIM_TAKEOFF_END 0x28" in BV
   and "#define MOD_EXPERIMENTAL_JUMP_ANIM_FALL_START 0x29" in BV
   and "#define MOD_EXPERIMENTAL_JUMP_ANIM_FALL_END 0x56" in BV
   and "#define MOD_EXPERIMENTAL_JUMP_ANIM_LAND_START 0x1a" in BV)

ck("B2 Surface Vent Jump authored last frame is exactly 0x56 and non-looping",
   "0x00570c00" in _SURF)

ck("AA Dancing contains requested reverse landing range",
   "0x00ef0c00" in _DANCE)

ck("airborne body uses Surface Vent Jump and landing uses AA Dancing",
   "animation_table_ptrs1[ANIM_surface_vent_jump]" in _JA
   and "animation_table_ptrs1[ANIM_dancing]" in _JA
   and "modelGetAnimFrame(ppointers[index]->bodyModel) > 0.0f" in _JA)

ck("Surface Vent Jump keeps normal/Fast-Bond presentation cadence",
   "#define MOD_EXPERIMENTAL_JUMP_ANIM_SPEED_NORMAL 1.5f" in BV
   and "#define MOD_EXPERIMENTAL_JUMP_ANIM_SPEED_FAST 3.125f" in BV
   and "ppointers[index]->field_7C > 12.0f" in _JA
   and "ppointers[index]->field_1288" in _JA)

ck("jump animation never writes authoritative player XYZ",
   "collision_position.f[0] =" not in _JA
   and "collision_position.f[1] =" not in _JA
   and "collision_position.f[2] =" not in _JA
   and "field_70 =" not in _JA
   and "stanHeight =" not in _JA)

ck("jump animation preserves authored pose while blocking world root XYZ",
   "fwd = ppointers[index]->bodyModel->anim_translation_scale;" not in _JA
   and "ppointers[index]->bodyModel->anim_translation_scale = 0.0f;" not in _JA
   and "ppointers[index]->bodyModel->anim2 = NULL;" not in _JA
   and "tpdeathanim = -1;" in _JA
   and "CHRFLAG_IGNORE_ANIM_TRANSLATION" in _PT)

_tick = _PT.find("tailret = chrTick(prop);")
_flag_save = _PT.rfind(
    "found = chr->chrflags & CHRFLAG_IGNORE_ANIM_TRANSLATION;", 0, _tick)
_flag_set = _PT.rfind(
    "chr->chrflags |= CHRFLAG_IGNORE_ANIM_TRANSLATION;", 0, _tick)
_flag_restore = _PT.find(
    "chr->chrflags &= ~CHRFLAG_IGNORE_ANIM_TRANSLATION;", _tick)
ck("native ignore-translation flag wraps only the custom presentation chrTick",
   _flag_save >= 0
   and _flag_set > _flag_save
   and _flag_set < _tick
   and _flag_restore > _tick
   and "if (tpdeathanim == -1 && found == 0)" in _PT)

ck("jump animation transitions smoothly without restarting takeoff-to-fall",
   "#define MOD_EXPERIMENTAL_JUMP_ANIM_MERGE 8.0f" in BV
   and "tpdeathanim == -1 ? MOD_EXPERIMENTAL_JUMP_ANIM_MERGE : 16.0f" in _PT
   and _JA.count("animation_table_ptrs1[ANIM_surface_vent_jump]") >= 2
   and "MOD_EXPERIMENTAL_JUMP_ANIM_FALL_START" not in _JA)

ck("landing begins at 0x1A, reverses to zero, and is not endframe-clamped",
   "if (tpdeathanim == -1 && angle < 0.0f)" in _PT
   and "startframe = MOD_EXPERIMENTAL_JUMP_ANIM_LAND_START;" in _PT
   and "angle = -1.0f;" in _JA
   and "frame = -1.0f;" in _JA
   and "local90 = -1.0f;" in _JA
   and "local90 = MOD_EXPERIMENTAL_JUMP_ANIM_LAND_START;" not in _JA)

ck("R10 jump presentation is preserved but temporarily disabled",
   "#define MOD_EXPERIMENTAL_JUMP_ANIMATIONS_ENABLED 0" in BV
   and "MOD_EXPERIMENTAL_JUMP_ANIMATIONS_ENABLED" in _JA)

pr=next((x for x in MK.splitlines() if x.startswith("prerequisites:")),"")
ck("R27U audit is mandatory",
   "r27u-experimental-jumping-audit:" in MK
   and "r27u-experimental-jumping-audit" in pr)

bad=[name for name,ok in checks if not ok]
print()
print("R27U EXPERIMENTAL JUMPING AUDIT: "
      + ("PASS" if not bad else "FAIL")
      + f" ({len(checks)-len(bad)}/{len(checks)})")
if bad:
    for name in bad:
        print(" - "+name)
    sys.exit(1)
