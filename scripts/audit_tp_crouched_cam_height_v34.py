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
check('SP Special Options has a twentieth row for the new tuner',
      ('#define MODWATCH_OPTION_ROWS 20' in OPT or
       '#define MODWATCH_OPTION_ROWS 21' in OPT or
       '#define MODWATCH_OPTION_ROWS 22' in OPT or
       '#define MODWATCH_OPTION_ROWS 23' in OPT or
       '#define MODWATCH_OPTION_ROWS 24' in OPT) and
      'label = "TP Crouched Cam Height";' in OPT and
      ('else if (row == 19)' in OPT or 'else if (row == 20)' in OPT))
check('new tuner is in-game only',
      'TP Crouched Cam Height' not in FRONT and
      'TP Crouched Cam Height' not in MP)
check('new tuner is placed after the four existing TP camera tuners',
      ('else if (row == 18)' in OPT or 'else if (row == 19)' in OPT) and
      'TP Cam Down Frame' in OPT and
      'TP Crouched Cam Height' in OPT)
check('In-Game Cheats moves down one row cleanly',
      (('if (pressed & B_BUTTON) MODWATCH_STATE = 19;' in OPT and 'else if (row >= 14 && row <= 18) modWatchAdjustThirdPersonCamera(row - 3);' in OPT) or
       ('if (pressed & B_BUTTON) MODWATCH_STATE = 20;' in OPT and 'else if (row >= 15 && row <= 19) modWatchAdjustThirdPersonCamera(row - 4);' in OPT) or
       ('if (pressed & B_BUTTON) MODWATCH_STATE = 21;' in OPT and
        'else if (row >= 15 && row <= 20) modWatchAdjustThirdPersonCamera(row - 4);' in OPT and
        'label = "TP Crosshair Range";' in OPT) or
       ('if (pressed & B_BUTTON) MODWATCH_STATE = 22;' in OPT and
        'else if (row >= 16 && row <= 21) modWatchAdjustThirdPersonCamera(row - 5);' in OPT and
        'label = "TP Crosshair Range";' in OPT) or
       ('MODWATCH_STATE = 23;' in OPT and
        'else if (row >= 16 && row <= 21)' in OPT and
        'modWatchAdjustThirdPersonCamera(row - 5);' in OPT and
        'label = "TP Crosshair Range";' in OPT and
        'label = "Level Modifiers"; value = ">";' in OPT)))
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
