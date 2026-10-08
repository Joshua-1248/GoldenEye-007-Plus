#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
BV = (ROOT/'src/game/bondview2.c').read_text(errors='replace')
OPT = (ROOT/'src/game/options.c').read_text(errors='replace')
OPTH = (ROOT/'src/game/options.h').read_text(errors='replace')
ROOTOPTH = (ROOT/'options.h').read_text(errors='replace')
FRONT = (ROOT/'src/game/front.c').read_text(errors='replace')
MP = (ROOT/'src/game/mpmenu.c').read_text(errors='replace')
F2 = (ROOT/'src/game/file2.c').read_text(errors='replace')
MAKE = (ROOT/'Makefile').read_text(errors='replace')

checks=[]
def check(name, cond):
    checks.append((name, bool(cond)))
    print(('[PASS] ' if cond else '[FAIL] ') + name)

check('TP Crouched Cam Height has authored default 46',
      '#define TP_CROUCH_CAM_HEIGHT_DEFAULT 46' in OPTH and
      '#define TP_CROUCH_CAM_HEIGHT_DEFAULT 46' in ROOTOPTH)
check('crouch-height adjustment has a dedicated runtime variable',
      'g_ModThirdPersonCrouchCameraHeightAdjust' in OPT and
      'extern s8 g_ModThirdPersonCrouchCameraHeightAdjust;' in OPTH and
      'extern s8 g_ModThirdPersonCrouchCameraHeightAdjust;' in ROOTOPTH)
check('SP Third-Person Options exposes the crouched-height tuner',
      '#define MODWATCH_TP_ROWS 11' in OPT and
      'frontModGetOptionLabel(60)' in OPT and 'MODWATCH_MODE_TP_OPTIONS' in OPT)
check('tuner is exposed in main, SP and MP Third-Person Options',
      'labelindex = i == 0 ? 32 : i <= 3 ? 46 + i : i == 4 ? 51 : 51 + i' in FRONT and
      'frontModGetOptionLabel(60)' in OPT and '52 + row' in MP)
check('new tuner is placed after the four existing TP camera tuners',
      OPT.find('frontModGetOptionLabel(56)') < OPT.find('frontModGetOptionLabel(57)') <
      OPT.find('frontModGetOptionLabel(58)') < OPT.find('frontModGetOptionLabel(59)') <
      OPT.find('frontModGetOptionLabel(60)'))
check('In-Game Cheats remains reachable after the new Special Options submenus',
      'MODWATCH_MODE_CHEATS' in OPT and 'frontModGetOptionLabel(54)' in OPT and
      'frontModGetOptionLabel(65)' in OPT)
check('adjustment uses the same two-unit camera tuning cadence',
      'g_ModThirdPersonCrouchCameraHeightAdjust + delta' in OPT)
check('adjustment is clamped to the authored 0..96 crouch-height range',
      'if (height < 0) height = 0;' in OPT and
      'if (height > 96) height = 96;' in OPT)
check('TP Crouch Cam gate still owns crouch-camera behavior',
      ('if (g_ModGameplayOptions3 & MODOPT3_TP_CROUCH_CAM)' in BV or
       'if (!tankcamera && (g_ModGameplayOptions3 & MODOPT3_TP_CROUCH_CAM))' in BV))
check('crouched height is proportional to live smooth crouch fraction',
      'crouchfraction = g_CurrentPlayer->ducking_height_offset / FULL_CROUCH_OFFSET;' in BV and
      'g_ModThirdPersonCrouchCameraHeightAdjust) * crouchfraction' in BV)
check('crouch fraction is clamped so standing cannot be displaced',
      'if (crouchfraction < 0.0f) crouchfraction = 0.0f;' in BV and
      'if (crouchfraction > 1.0f) crouchfraction = 1.0f;' in BV)
check('configured crouch height drives chase pivot',
      ('anchor.f[1] += g_CurrentPlayer->ducking_height_offset * g_playerPerm->player_perspective_height;' in BV) or
      ('anchor.f[1] -= ((f32)TP_CROUCH_CAM_HEIGHT_DEFAULT' in BV))
pack = F2.split('static u32 filePackThirdPersonCameraSettings(void)',1)[1].split('void fileStoreThirdPersonCameraSettings',1)[0]
check('legacy packed EEPROM camera format still excludes crouch-height tuner',
      'g_ModThirdPersonCrouchCameraHeightAdjust' not in pack)
check('V34 audit is mandatory build prerequisite',
      'tp-crouched-cam-height-v34-audit:' in MAKE and
      'tp-crouched-cam-height-v34-audit' in MAKE.split('prerequisites:',1)[1].split('\n',1)[0])

failed=[n for n,ok in checks if not ok]
if failed:
    print(f'\nTP CROUCHED CAM HEIGHT V34 AUDIT: FAIL ({len(checks)-len(failed)}/{len(checks)})', file=sys.stderr)
    for n in failed: print(' - '+n, file=sys.stderr)
    raise SystemExit(1)
print(f'\nTP CROUCHED CAM HEIGHT V34 AUDIT: PASS ({len(checks)}/{len(checks)})')
