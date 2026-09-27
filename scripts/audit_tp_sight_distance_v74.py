#!/usr/bin/env python3
from pathlib import Path
import sys
R=Path(__file__).resolve().parents[1]
H=(R/'src/game/options.h').read_text(errors='replace')
O=(R/'src/game/options.c').read_text(errors='replace')
F2=(R/'src/game/file2.c').read_text(errors='replace')
F2H=(R/'src/game/file2.h').read_text(errors='replace')
FR=(R/'src/game/front.c').read_text(errors='replace')
MP=(R/'src/game/mpmenu.c').read_text(errors='replace')
BV=(R/'src/game/bondview2.c').read_text(errors='replace')
MK=(R/'Makefile').read_text(errors='replace')
checks=[]
def ck(n,c):
    checks.append((n,bool(c)))
    print(('[PASS] ' if c else '[FAIL] ')+n)

ck('TP Sight Translucency no longer aliases mod_options3 camera/signature bits',
   'MODOPT3_DISABLE_TP_SIGHT_TRANSLUCENCY' not in F2H+F2+FR+MP+O+BV)
ck('TP Sight Translucency has independent runtime state, default On',
   'g_ModTpSightTranslucencyEnabled = TRUE;' in O and
   'extern u8 g_ModTpSightTranslucencyEnabled;' in H)
ck('main/SP/MP surfaces all consume independent sight state',
   'g_ModTpSightTranslucencyEnabled ^= 1;' in FR and
   'g_ModTpSightTranslucencyEnabled ^= 1;' in O and
   'g_ModTpSightTranslucencyEnabled ^= 1;' in MP)
ck('extended journal independently persists disabled sight state',
   'GE_SRAM_EXT_FLAG_DISABLE_TP_SIGHT_TRANSLUCENCY 0x10' in F2 and
   'if (!g_ModTpSightTranslucencyEnabled)' in F2 and
   'g_ModTpSightTranslucencyEnabled =' in F2)
ck('v3 migration repairs V72/V73 contaminated sight state',
   '#define GE_SRAM_EXT_VERSION_V2      2u' in F2 and
   '#define GE_SRAM_EXT_VERSION         3u' in F2 and
   'record->flags &= ~GE_SRAM_EXT_FLAG_DISABLE_TP_SIGHT_TRANSLUCENCY;' in F2)
ck('TP Cam Distance authored default/range/step are 300, 100..600, step 5',
   all(x in H for x in ['#define TP_CAM_DISTANCE_DEFAULT 300','#define TP_CAM_DISTANCE_MIN 100','#define TP_CAM_DISTANCE_MAX 600','#define TP_CAM_DISTANCE_STEP 5']))
ck('TP Cam Distance runtime adjustment is widened beyond s8',
   'extern s16 g_ModThirdPersonCameraDistanceAdjust;' in H and
   's16 g_ModThirdPersonCameraDistanceAdjust;' in O)
ck('TP Cam Distance watch adjustment clamps the actual value to 100..600',
   'if (actual < TP_CAM_DISTANCE_MIN) actual = TP_CAM_DISTANCE_MIN;' in O and
   'if (actual > TP_CAM_DISTANCE_MAX) actual = TP_CAM_DISTANCE_MAX;' in O)
ck('v3 extension stores camera distance in compact 5-unit form',
   'g_ModThirdPersonCameraDistanceAdjust = (s32)record->camera_distance_adjust * 5;' in F2 and
   's32 encoded = g_ModThirdPersonCameraDistanceAdjust / 5;' in F2)
ck('legacy mirror sync cannot erase v3 distance or sight state',
   'record.camera_distance_adjust = current->camera_distance_adjust;' in F2 and
   'current->flags & GE_SRAM_EXT_FLAG_DISABLE_TP_SIGHT_TRANSLUCENCY' in F2)
ck('TP Crosshair Range authored default is 500', '#define TP_CROSSHAIR_RANGE_DEFAULT 500' in H)
ck('75-percent translucency remains 0x40 for sight and collision floor',
   'alpha = 0x40;' in BV and 'collisionalpha = 0x40;' in BV and
   'if (collisionalpha < 0x40) collisionalpha = 0x40;' in BV)
ck('V74 audit is a mandatory build prerequisite',
   'tp-sight-distance-v74-audit:' in MK and
   'tp-sight-distance-v74-audit' in next((x for x in MK.splitlines() if x.startswith('prerequisites:')),''))

bad=[n for n,c in checks if not c]
print()
print(f"TP SIGHT/DISTANCE V74 AUDIT: {'PASS' if not bad else 'FAIL'} ({len(checks)-len(bad)}/{len(checks)})")
if bad:
    for n in bad: print(' - '+n)
    sys.exit(1)
