#!/usr/bin/env python3
from pathlib import Path
import sys
R=Path(__file__).resolve().parents[1]
def t(p): return (R/p).read_text(errors="replace")
BV=t("src/game/bondview2.c"); CH=t("src/game/chr.c"); CA=t("src/game/chraction.c")
PO=t("src/game/propobj.c"); CP=t("src/game/chrprop.c"); BG=t("src/game/bg.c"); MK=t("Makefile")
checks=[]
def ck(n,c): checks.append((n,bool(c))); print(("[PASS] " if c else "[FAIL] ")+n)
ck("camera mode remembers previous state before assignment", "s32 previousCameraMode = g_CameraMode;" in BV and BV.find("s32 previousCameraMode = g_CameraMode;") < BV.find("g_CameraMode = arg0;"))
ck("destructive cinematic body handoff occurs only on first POSEND entry", "if (previousCameraMode != CAMERAMODE_POSEND)" in BV and BV.find("if (previousCameraMode != CAMERAMODE_POSEND)") < BV.find("bondviewPrepareThirdPersonBodyForCinematic();"))
ck("POSEND still uses stock solo_char_load", "solo_char_load();" in BV)
ck("Co-Op cinematic Bond resolves explicitly to P1", "g_playerPointers[PLAYER_1]->prop->chr->chrnum" in CA)
ck("Co-Op Bond-relative target context pins to P1", "if (lvlIsCoopEndCutscene())" in CH and "return PLAYER_1;" in CH)
ck("Co-Op character full tick pins to P1", "(coopMode && lvlIsCoopEndCutscene() && get_cur_playernum() == PLAYER_1)" in CH)
ck("Co-Op object and weapon simulation pin to P1", PO.count("lvlIsCoopEndCutscene() && get_cur_playernum() == PLAYER_1") >= 2)
ck("Co-Op character-prop housekeeping pins to P1", "lvlIsCoopEndCutscene() && get_cur_playernum() == PLAYER_1" in CP)
ck("Co-Op room housekeeping pins to P1", "lvlIsCoopEndCutscene() && get_cur_playernum() == PLAYER_1" in BG)
ck("Co-Op player-body bookkeeping pins to P1", BV.count("|| (lvlIsCoopEndCutscene() && get_cur_playernum() == PLAYER_1)") >= 2)
ck("V79 audit is mandatory", "surface2-posend-coop-v79-audit:" in MK and "surface2-posend-coop-v79-audit" in next((x for x in MK.splitlines() if x.startswith("prerequisites:")),""))
bad=[n for n,c in checks if not c]
print()
print(f"SURFACE II POSEND/CO-OP V79 AUDIT: {'PASS' if not bad else 'FAIL'} ({len(checks)-len(bad)}/{len(checks)})")
if bad:
    [print(" - "+n) for n in bad]
    sys.exit(1)
