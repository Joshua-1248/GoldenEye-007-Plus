#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
BV = (ROOT/'src/game/bondview2.c').read_text(errors='replace')
OPT = (ROOT/'src/game/options.c').read_text(errors='replace')
FRONT = (ROOT/'src/game/front.c').read_text(errors='replace')
MP = (ROOT/'src/game/mpmenu.c').read_text(errors='replace')
FILE2H = (ROOT/'src/game/file2.h').read_text(errors='replace')
PROP = (ROOT/'src/game/propobj.c').read_text(errors='replace')
SPEC = (ROOT/'src/game/spectrum.c').read_text(errors='replace')
SEV = (ROOT/'assets/obseg/setup/UsetupsevxbZ.c').read_text(errors='replace')
MAKE = (ROOT/'Makefile').read_text(errors='replace')

checks=[]
def check(name, cond):
    checks.append((name, bool(cond)))
    print(('[PASS] ' if cond else '[FAIL] ') + name)

# Shoulder selection (V33 may add a symmetric-toggle mode, but the V32 directional path must remain)
check('B+L and B+R directional shoulder commands remain available',
      'tpShoulderCommand = -1' in BV and 'tpShoulderCommand = 1' in BV and '(L_TRIG | R_TRIG)' in BV)
check('shoulder command is gated by live TP and B-held-first',
      'bondviewThirdPersonPresentationActive(abChordPlayer)' in BV and '(oldbuttons & B_BUTTON)' in BV)
check('left/right command resolves through smoothed shoulder state',
      'bondviewSetThirdPersonShoulder(abChordPlayer, tpShoulderCommand < 0)' in BV and
      'g_ModThirdPersonShoulderSwapped[player] = leftshoulder ? TRUE : FALSE' in BV and
      'shoulderstep = 0.12f * g_GlobalTimerDelta' in BV)

# Gadget entry from unarmed
check('A+B gadget entry no longer requires a weapon-switch when unarmed',
      'bondinvWeaponSwitchInProgress()\n            || getCurrentPlayerWeaponId(GUNRIGHT) == ITEM_UNARMED' in BV)
check('unarmed gadget bypass covers fist/slappers state',
      'getCurrentPlayerWeaponId(GUNRIGHT) == ITEM_FIST' in BV)
check('gadget chord still consumes detonation while active',
      'moveData.detonating = 0;' in BV and 'bondinvCycleMissionItem();' in BV)

# Surface 2 Remote Mine
check('Surface 2 script really tests Remote Mine item 0x1d for stationary state',
      'if_item_is_stationary_within_level(0x1d' in SEV and 'if_item_is_attached_to_object(0x1d' in SEV)
check('thrown-item finder ignores player-owned presentation/inventory props',
      ('weaponPropOwnedByPlayer' in PROP and
       'prop->type == PROP_TYPE_VIEWER || prop->type == PROP_TYPE_PLAYER' in PROP and
       'if (weaponPropOwnedByPlayer(prop))' in PROP and 'continue;' in PROP) or
      'prop->parent != NULL && prop->parent->type == PROP_TYPE_VIEWER' in PROP)
check('thrown-item finder still accepts real stationary world items',
      'check_if_entry_is_collectable(KeyID, prop)' in PROP and 'RUNTIMEBITFLAG_HASPROJECTILE' in PROP)

# Deferred watch writes
check('SP watch uses a deferred settings dirty flag',
      'g_ModWatchSettingsDirty' in OPT and 'modWatchCommitDeferredSettings' in OPT)
check('camera steps no longer write EEPROM inside adjust function',
      'static void modWatchAdjustThirdPersonCamera' in OPT and
      OPT[OPT.index('static void modWatchAdjustThirdPersonCamera'):OPT.index('static void modWatchToggleCheat')].count('fileWriteSave') == 0)
check('deferred watch settings commit on page exit and unpause',
      OPT.count('modWatchCommitDeferredSettings();') >= 3)

# TP Crouch Cam persistence + surfaces
check('TP Crouch Cam owns save-backed bit 4 without growing save_data',
      'MODOPT3_TP_CROUCH_CAM' in FILE2H and '0x10' in FILE2H)
check('TP Crouch Cam follows smooth crouch offset proportionally',
      ('g_CurrentPlayer->ducking_height_offset * g_playerPerm->player_perspective_height' in BV or 'crouchfraction = g_CurrentPlayer->ducking_height_offset / FULL_CROUCH_OFFSET;' in BV))
check('main-menu Special Options exposes TP Crouch Cam',
      ('page2labels[12]' in FRONT or 'page2labels[13]' in FRONT or 'page2labels[14]' in FRONT or 'page2labels[15]' in FRONT) and '48' in FRONT and 'MODOPT3_TP_CROUCH_CAM' in FRONT)
check('SP watch exposes TP Crouch Cam',
      'MODWATCH_OPTION_ROWS' in OPT and 'frontModGetOptionLabel(48)' in OPT and 'MODOPT3_TP_CROUCH_CAM' in OPT)
check('MP watch exposes TP Crouch Cam',
      ('mode == 2 ? 14' in MP or 'mode == 2 ? 15' in MP or 'mode == 2 ? 16' in MP or 'mode == 2 ? 17' in MP) and 'frontModGetOptionLabel(48)' in MP and 'MODOPT3_TP_CROUCH_CAM' in MP)
check('TP Crouch Cam label is exact',
      'TP Crouch Cam' in SPEC and '0x54502043' in SPEC and '0x726F7563' in SPEC)
check('V32 audit is mandatory build prerequisite',
      'tp-controls-watch-surface-v32-audit:' in MAKE and 'tp-controls-watch-surface-v32-audit' in MAKE.split('prerequisites:',1)[1].split('\n',1)[0])

failed=[n for n,ok in checks if not ok]
if failed:
    print(f'\nTP CONTROLS/WATCH/SURFACE V32 AUDIT: FAIL ({len(checks)-len(failed)}/{len(checks)})', file=sys.stderr)
    for n in failed: print(' - '+n, file=sys.stderr)
    raise SystemExit(1)
print(f'\nTP CONTROLS/WATCH/SURFACE V32 AUDIT: PASS ({len(checks)}/{len(checks)})')
