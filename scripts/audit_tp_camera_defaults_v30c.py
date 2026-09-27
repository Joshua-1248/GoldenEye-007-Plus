#!/usr/bin/env python3
from pathlib import Path
import sys
ROOT = Path(__file__).resolve().parents[1]
F2=(ROOT/'src/game/file2.c').read_text(errors='replace')
F2H=(ROOT/'src/game/file2.h').read_text(errors='replace')
OPTH=(ROOT/'src/game/options.h').read_text(errors='replace')
MK=(ROOT/'Makefile').read_text(errors='replace')
checks=[
 ('defaults are 300/-12/-24/24', all(x in OPTH for x in ['#define TP_CAM_DISTANCE_DEFAULT 300','#define TP_CAM_HEIGHT_DEFAULT (-12)','#define TP_CAM_HORIZONTAL_DEFAULT (-24)','#define TP_CAM_DOWN_FRAME_DEFAULT 24'])),
 ('V30 and V30B signatures are both migration sources', 'MOD_CAMERA_PACK_SIGNATURE3_LEGACY_V30  0x05' in F2H and 'MOD_CAMERA_PACK_SIGNATURE3_LEGACY_V30B 0x06' in F2H),
 ('V30C current signature is 7', 'MOD_CAMERA_PACK_SIGNATURE3             0x07' in F2H),
 ('both legacy formats reset camera adjustments', '== MOD_CAMERA_PACK_SIGNATURE3_LEGACY_V30' in F2 and '== MOD_CAMERA_PACK_SIGNATURE3_LEGACY_V30B' in F2 and F2.count('g_ModThirdPersonCameraHeightAdjust = 0;') >= 2),
 ('Stay In TP default survives reset migration', 'g_ModStayInTpOnDeathDefault = (packed & MOD_CAMERA_STAY_TP_DEATH_BIT) != 0;' in F2),
 ('generic settings save cannot repack TP camera before folder load', 'fileSaveSettingsForFolder(save_data *save)' in F2 and 'V30C: Do not repack TP camera values from this generic settings-save' in F2),
 ('camera values still persist at explicit edit sites', F2.count('fileStoreThirdPersonCameraSettings(save);') >= 2),
 ('V30C audit is mandatory prerequisite', 'tp-camera-default-reset-v30c-audit' in MK and 'scripts/audit_tp_camera_defaults_v30c.py' in MK),
]
for name,ok in checks: print(('[PASS] ' if ok else '[FAIL] ')+name)
passed=sum(ok for _,ok in checks)
print(f'TP CAMERA DEFAULT RESET V30C AUDIT: {"PASS" if passed==len(checks) else "FAIL"} ({passed}/{len(checks)})')
if passed!=len(checks): sys.exit(1)
