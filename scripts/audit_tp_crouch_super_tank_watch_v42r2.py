#!/usr/bin/env python3
from pathlib import Path
import sys
R=Path(__file__).resolve().parents[1]
F=(R/'src/game/file2.c').read_text(errors='replace')
O=(R/'src/game/options.c').read_text(errors='replace')
M=(R/'src/game/mpmenu.c').read_text(errors='replace')
B=(R/'src/game/bondview2.c').read_text(errors='replace')
H=(R/'src/game/options.h').read_text(errors='replace')
MK=(R/'Makefile').read_text(errors='replace')
checks=[]
def ck(n,c): checks.append((n,bool(c))); print(('[PASS] ' if c else '[FAIL] ')+n)
ck('authored TP crouched height is 46', '#define TP_CROUCH_CAM_HEIGHT_DEFAULT 46' in H)
ck('current SRAM schema stores canonical absolute crouch height', '#define GE_SRAM_EXT_VERSION         3u' in F and 'record.crouch_camera_height_adjust = TP_CROUCH_CAM_HEIGHT_DEFAULT' in F)
ck('legacy pre-absolute SRAM is migrated once at schema upgrade', 'GE_SRAM_EXT_V1_CROUCH_ABSOLUTE' in F and 'fileSramExtEnsureCurrentVersion' in F)
ck('ordinary SRAM saves persist exact displayed height', 'record.crouch_camera_height_adjust = TP_CROUCH_CAM_HEIGHT_DEFAULT' in F and '+ g_ModThirdPersonCrouchCameraHeightAdjust;' in F)
ck('load converts exact stored height back to runtime adjustment', 'record->crouch_camera_height_adjust - TP_CROUCH_CAM_HEIGHT_DEFAULT' in F)
ck('watch adjustment clamps actual height, not a free-running trim', 's32 height = TP_CROUCH_CAM_HEIGHT_DEFAULT' in O and 'if (height < 0) height = 0;' in O and 'if (height > 96) height = 96;' in O)
ck('runtime and watch display consume the same base-plus-adjust value', 'TP_CROUCH_CAM_HEIGHT_DEFAULT + g_ModThirdPersonCrouchCameraHeightAdjust' in O and 'TP_CROUCH_CAM_HEIGHT_DEFAULT' in B and 'g_ModThirdPersonCrouchCameraHeightAdjust' in B)
ck('SP in-game cheat ordering remains complete with stable IDs',
   'if (row < CHEAT_KINETIC_EXPLOSIONS)' in O
   and 'return (CHEAT_ID)(row + 1);' in O
   and 'return CHEAT_ULTRA_KINETICS;' in O
   and 'return (CHEAT_ID)row;' in O)
ck('MP in-game cheat ordering remains complete with stable IDs',
   'if (row < CHEAT_KINETIC_EXPLOSIONS)' in M
   and 'return (CHEAT_ID)(row + 1);' in M
   and 'return CHEAT_ULTRA_KINETICS;' in M
   and 'return (CHEAT_ID)row;' in M)
C=(R/'src/bondconstants.h').read_text(errors='replace')
ck('Super Tank remains before Mirrored Levels and CHEAT_INVALID', C.find('CHEAT_SUPER_TANK') < C.find('CHEAT_MIRRORED_LEVELS') < C.find('CHEAT_INVALID'))
ck('V42 R2 audit is mandatory prerequisite', 'tp-crouch-super-tank-watch-v42r2-audit:' in MK and 'tp-crouch-super-tank-watch-v42r2-audit' in MK.split('prerequisites:',1)[1].split('\n',1)[0])
failed=[n for n,c in checks if not c]
print(f"\nTP CROUCH/SUPER TANK WATCH V42 R2 AUDIT: {'PASS' if not failed else 'FAIL'} ({len(checks)-len(failed)}/{len(checks)})")
if failed:
    for n in failed: print(' - '+n)
    raise SystemExit(1)
