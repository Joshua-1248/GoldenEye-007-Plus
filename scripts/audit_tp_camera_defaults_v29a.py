#!/usr/bin/env python3
from pathlib import Path
import re, sys
ROOT=Path(__file__).resolve().parents[1]
h=(ROOT/'src/game/options.h').read_text(errors='replace')
hroot=(ROOT/'options.h').read_text(errors='replace')
o=(ROOT/'src/game/options.c').read_text(errors='replace')
b=(ROOT/'src/game/bondview2.c').read_text(errors='replace')
f=(ROOT/'src/game/file2.c').read_text(errors='replace')
checks=[
 ('distance default 310', '#define TP_CAM_DISTANCE_DEFAULT 310' in h and '#define TP_CAM_DISTANCE_DEFAULT 310' in hroot),
 ('height default -12', '#define TP_CAM_HEIGHT_DEFAULT (-12)' in h and '#define TP_CAM_HEIGHT_DEFAULT (-12)' in hroot),
 ('horizontal default -24', '#define TP_CAM_HORIZONTAL_DEFAULT (-24)' in h and '#define TP_CAM_HORIZONTAL_DEFAULT (-24)' in hroot),
 ('down-frame default 24', '#define TP_CAM_DOWN_FRAME_DEFAULT 24' in h and '#define TP_CAM_DOWN_FRAME_DEFAULT 24' in hroot),
 ('UI uses distance default constant', 'TP_CAM_DISTANCE_DEFAULT + g_ModThirdPersonCameraDistanceAdjust' in o),
 ('UI uses height default constant', 'TP_CAM_HEIGHT_DEFAULT + g_ModThirdPersonCameraHeightAdjust' in o),
 ('UI uses horizontal default constant', 'TP_CAM_HORIZONTAL_DEFAULT + g_ModThirdPersonCameraHorizontalAdjust' in o),
 ('UI uses down-frame default constant', 'TP_CAM_DOWN_FRAME_DEFAULT + g_ModThirdPersonCameraDownFrameAdjust' in o),
 ('EEPROM migration restores zero adjustments', all(x in f for x in ['g_ModThirdPersonCameraDistanceAdjust = 0;','g_ModThirdPersonCameraHeightAdjust = 0;','g_ModThirdPersonCameraHorizontalAdjust = 0;','g_ModThirdPersonCameraDownFrameAdjust = 0;'])),
]
for name,ok in checks:
 print(('[PASS] ' if ok else '[FAIL] ')+name)
if not all(ok for _,ok in checks): sys.exit(1)
print('TP CAMERA DEFAULTS V29A AUDIT: PASS (%d/%d)' % (len(checks),len(checks)))
