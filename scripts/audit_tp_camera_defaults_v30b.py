#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
OPT = (ROOT / "src/game/options.h").read_text(errors="replace")
F2H = (ROOT / "src/game/file2.h").read_text(errors="replace")
F2 = (ROOT / "src/game/file2.c").read_text(errors="replace")
MAKE = (ROOT / "Makefile").read_text(errors="replace")

def check(name, ok):
    print(("[PASS] " if ok else "[FAIL] ") + name)
    if not ok:
        print("TP CAMERA DEFAULT RESET V30B AUDIT: FAIL:", name, file=sys.stderr)
        raise SystemExit(1)

checks = [
    ("Distance default is 300", "#define TP_CAM_DISTANCE_DEFAULT 300" in OPT),
    ("Height default is -12", "#define TP_CAM_HEIGHT_DEFAULT (-12)" in OPT),
    ("Horizontal default is -24", "#define TP_CAM_HORIZONTAL_DEFAULT (-24)" in OPT),
    ("Down Frame default is 24", "#define TP_CAM_DOWN_FRAME_DEFAULT 24" in OPT),
    ("V30 legacy signature is recognized", "MOD_CAMERA_PACK_SIGNATURE3_LEGACY_V30  0x05" in F2H),
    ("V30B signature is retained as a migration source", "MOD_CAMERA_PACK_SIGNATURE3_LEGACY_V30B 0x06" in F2H),
    ("new persistence signature is distinct", "MOD_CAMERA_PACK_SIGNATURE3             0x07" in F2H),
    ("legacy V30 save resets all TP camera adjustments", F2.count("g_ModThirdPersonCameraDistanceAdjust = 0;") >= 2 and F2.count("g_ModThirdPersonCameraHorizontalAdjust = 0;") >= 2),
    ("legacy V30 migration preserves Stay In TP On Death", "MOD_CAMERA_PACK_SIGNATURE3_LEGACY_V30" in F2 and "g_ModStayInTpOnDeathDefault = (packed & MOD_CAMERA_STAY_TP_DEATH_BIT) != 0;" in F2),
    ("migration immediately rewrites save", "fileStoreThirdPersonCameraSettings(save);" in F2 and "fileWriteSave(save);" in F2),
    ("audit is mandatory build prerequisite", "tp-camera-default-reset-v30b-audit" in MAKE),
]
for name, ok in checks:
    check(name, ok)
print(f"TP CAMERA DEFAULT RESET V30B AUDIT: PASS ({len(checks)}/{len(checks)})")
