#!/usr/bin/env python3
from pathlib import Path
import sys

R = Path(__file__).resolve().parents[1]
O = (R/"src/game/options.c").read_text(errors="replace")
OH = (R/"src/game/options.h").read_text(errors="replace")
F2 = (R/"src/game/file2.c").read_text(errors="replace")
SP = (R/"src/game/spectrum.c").read_text(errors="replace")
FR = (R/"src/game/front.c").read_text(errors="replace")
MP = (R/"src/game/mpmenu.c").read_text(errors="replace")
PO = (R/"src/game/prop.c").read_text(errors="replace")
CP = (R/"src/game/chrprop.c").read_text(errors="replace")
POBJ = (R/"src/game/propobj.c").read_text(errors="replace")
BV2 = (R/"src/game/bondview2.c").read_text(errors="replace")
MW = (R/"src/game/mp_weapon.c").read_text(errors="replace")
MU = (R/"src/music.c").read_text(errors="replace")
MK = (R/"Makefile").read_text(errors="replace")

checks = []
def ck(name, cond):
    ok = bool(cond)
    checks.append((name, ok))
    print(("[PASS] " if ok else "[FAIL] ") + name)

ck("three R27W runtime flags exist and default through BSS/save fallback",
   all(x in O for x in [
       "u8 g_ModDisableBodyArmorEnabled;",
       "u8 g_ModSiloXMusicLoopFixEnabled;",
       "u8 g_ModAr33PropFixMpEnabled;",
   ])
   and all(x in F2 for x in [
       "g_ModDisableBodyArmorEnabled = FALSE;",
       "g_ModSiloXMusicLoopFixEnabled = FALSE;",
       "g_ModAr33PropFixMpEnabled = FALSE;",
   ]))

ck("R27W uses free bits in the existing Enhancements byte",
   "GE_SRAM_EXT_TAIL_ENH_DISABLE_BODY_ARMOR        0x04" in F2
   and "GE_SRAM_EXT_TAIL_ENH_SILOX_LOOP_FIX            0x08" in F2
   and "GE_SRAM_EXT_TAIL_ENH_AR33_PROP_FIX_MP          0x10" in F2
   and "GE_SRAM_EXT_TAIL_ENH_BASE + folder" in F2)

ck("packed R27W labels are present",
   "index == 79" in SP
   and "index == 80" in SP
   and "index == 81" in SP)

ck("Disable Body Armor is appended to Enhancements everywhere",
   "#define MODWATCH_ENHANCEMENTS_ROWS 3" in O
   and "if (mode == 9) return 3;" in MP
   and "g_ModDisableBodyArmorEnabled" in FR
   and "g_ModDisableBodyArmorEnabled" in O
   and "g_ModDisableBodyArmorEnabled" in MP)

ck("Always Show Crosshair is preserved when it was present",
   ("g_ModAlwaysShowCrosshairEnabled" in O and "index == 78" in SP))

ck("Main Patches has three rows while Watch Patches keeps Silo X and AR33 in seven rows",
   "for (i = 0; i < 3; i++)" in FR
   and "frontModGetOptionLabel(i == 0 ? 55 : i == 1 ? 80 : 81)" in FR
   and "#define MODWATCH_PATCH_ROWS 7" in O
   and "frontModGetOptionLabel(80)" in O
   and "frontModGetOptionLabel(81)" in O
   and "if (mode == 5) return 7;" in MP
   and "row == 4 ? 70 : row == 5 ? 80 : 81" in MP)

ck("Disable Body Armor hides live pickups and blocks collection",
   "void modSyncBodyArmorPickups(void)" in PO
   and "chrpropDisable(obj->prop);" in PO
   and "g_ModDisableBodyArmorEnabled && obj->type == PROPDEF_ARMOUR" in POBJ
   and "return TICKOP_NONE;" in POBJ)

ck("Disable Body Armor restore respects collection and MP regen state",
   "obj->prop != NULL" in PO
   and "else if (obj->prop->timetoregen <= 0)" in PO
   and "obj->type != PROPDEF_ARMOUR || !g_ModDisableBodyArmorEnabled" in CP
   and "((BodyArmourRecord *) obj)->amount" in CP
   and "r27w_body_armor_applied" in CP)

ck("Facility gas Death Neck uses the shortened enemy duration",
   "MOD_PLAYER_DEATH_GAS_NECK" in BV2
   and "PTR_ANIM_death_neck" in BV2
   and "modelSetAnimEndFrame(&g_CurrentPlayer->model, 241.0f);" in BV2
   and "modelSetAnimEndFrame(ppointers[index]->bodyModel, 241.0f);" in BV2)

ck("Silo X loop fix targets Track 2 only after it is stopped",
   "g_musicXTrack2CurrentTrackNum == M_SILOX" in MU
   and "mission_state == MISSION_STATE_2 || mission_state == MISSION_STATE_5" in MU
   and "alCSPGetState(g_musicXTrack2SeqPlayer) == AL_STOPPED" in MU
   and ("musicTrack2Play(M_SILOX);" in MU
        or ("alCSeqNew(&g_musicXTrack2Seq, g_musicXTrack2SeqData);" in MU
            and "alCSPPlay(g_musicXTrack2SeqPlayer);" in MU)))

ck("AR33 prop fix is limited to Remote Mines and Timed Mines",
   "set == mp_weapon_set_remote_m || set == mp_weapon_set_timed_m" in MW
   and "g_ModAr33PropFixMpEnabled ? PROP_CHRM16 : PROP_CHRKALASH" in MW
   and "set[4].propID = prop;" in MW and "set[5].propID = prop;" in MW)

ck("R27W audit is mandatory",
   "r27w-gameplay-patches-audit" in MK)

failed = [name for name, ok in checks if not ok]
print()
if failed:
    print("R27W GAMEPLAY/PATCH OPTIONS AUDIT: FAIL (%d/%d)" %
          (len(checks) - len(failed), len(checks)))
    for name in failed:
        print(" - " + name)
    sys.exit(1)

print("R27W GAMEPLAY/PATCH OPTIONS AUDIT: PASS (%d/%d)" %
      (len(checks), len(checks)))
