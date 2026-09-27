#!/usr/bin/env python3
from pathlib import Path
import re
import sys
ROOT=Path(__file__).resolve().parents[1]
OPTH=(ROOT/'src/game/options.h').read_text(errors='replace')
root_opth_path=ROOT/'options.h'
ROOTOPTH=root_opth_path.read_text(errors='replace') if root_opth_path.exists() else OPTH
OPT=(ROOT/'src/game/options.c').read_text(errors='replace')
V32=(ROOT/'scripts/audit_tp_controls_watch_surface_v32.py').read_text(errors='replace')
PROP=(ROOT/'src/game/propobj.c').read_text(errors='replace')
MAKE=(ROOT/'Makefile').read_text(errors='replace')
checks=[]
def check(name, cond):
    checks.append((name,bool(cond))); print(('[PASS] ' if cond else '[FAIL] ')+name)
def get_crouch_default(text):
    match = re.search(r'^#define\s+TP_CROUCH_CAM_HEIGHT_DEFAULT\s+(-?\d+)\s*$', text, re.MULTILINE)
    return int(match.group(1)) if match else None

GAME_CROUCH_DEFAULT = get_crouch_default(OPTH)
ROOT_CROUCH_DEFAULT = get_crouch_default(ROOTOPTH)
check('TP Crouched Cam Height authored default agrees across both headers and remains in range',
      GAME_CROUCH_DEFAULT is not None and
      GAME_CROUCH_DEFAULT == ROOT_CROUCH_DEFAULT and
      -60 <= GAME_CROUCH_DEFAULT <= 60)
check('UI still displays authored base plus signed adjustment', 'TP_CROUCH_CAM_HEIGHT_DEFAULT + g_ModThirdPersonCrouchCameraHeightAdjust' in OPT)
check('V32 audit recognizes semantic V38 ownership helper', 'weaponPropOwnedByPlayer' in V32 and 'PROP_TYPE_PLAYER' in V32)
check('V38 ownership helper remains present', 'static bool weaponPropOwnedByPlayer' in PROP and 'prop = prop->parent;' in PROP)
check('R1 audit is mandatory build prerequisite', 'v38r1-defaults-compat-audit:' in MAKE and 'v38r1-defaults-compat-audit' in MAKE.split('prerequisites:',1)[1].split('\n',1)[0])
failed=[n for n,ok in checks if not ok]
if failed:
    print(f'\nV38 R1 DEFAULTS/COMPAT AUDIT: FAIL ({len(checks)-len(failed)}/{len(checks)})', file=sys.stderr)
    for n in failed: print(' - '+n, file=sys.stderr)
    raise SystemExit(1)
print(f'\nV38 R1 DEFAULTS/COMPAT AUDIT: PASS ({len(checks)}/{len(checks)})')
