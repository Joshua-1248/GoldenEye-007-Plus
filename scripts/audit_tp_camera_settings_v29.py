#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
FILEH = (ROOT/'src/game/file.h').read_text(errors='replace')
FILE2 = (ROOT/'src/game/file2.c').read_text(errors='replace')
FILE2H = (ROOT/'src/game/file2.h').read_text(errors='replace')
OPTIONS = (ROOT/'src/game/options.c').read_text(errors='replace')
BONDINV = (ROOT/'src/game/bondinv.c').read_text(errors='replace')
BONDVIEW = (ROOT/'src/game/bondview2.c').read_text(errors='replace')
SPECTRUM = (ROOT/'src/game/spectrum.c').read_text(errors='replace')
MAKE = (ROOT/'Makefile').read_text(errors='replace')

checks = []
def check(name, cond):
    checks.append((name, bool(cond)))
    print(('[PASS] ' if cond else '[FAIL] ') + name)

check('save_data keeps explicit camera tail inside fixed EEPROM record',
      'u8 mod_camera_tail;' in FILEH and 'save_data_size_must_remain_0x60' in FILE2)
check('camera pack has signature and mixed-radix EEPROM encoding',
      'MOD_CAMERA_PACK_SIGNATURE' in FILE2H and 'data = data * 61' in FILE2 and 'data = data * 81' in FILE2 and 'data = data * 97' in FILE2)
check('camera pack uses only retail-spare storage',
      'unlocked_cheats_3 & 0x0f' in FILE2 and 'flag_007 & 0x01' in FILE2 and 'times[(SP_LEVEL_MAX - 1) * 4 - 1]' in FILE2 and 'mod_camera_tail' in FILE2)
check('camera settings reload from selected EEPROM folder',
      FILE2.count('fileLoadThirdPersonCameraSettings(save);') >= 1)
check('watch camera adjustment defers packed EEPROM write until leaving/unpausing',
      'g_ModWatchSettingsDirty = TRUE;' in OPTIONS and 'modWatchCommitDeferredSettings();' in OPTIONS and 'fileStoreThirdPersonCameraSettings(save);' in OPTIONS)
check('unlock-everything preserves camera bits in cheat byte',
      '(save->unlocked_cheats_3 & 0xf0) | 0x0f' in FILE2 and '(save->unlocked_cheats_3 & 0xf0) | 0x0f' in SPECTRUM)
check('B then fresh L/R selects a shoulder only in live Third Person presentation',
      'bondviewThirdPersonPresentationActive(abChordPlayer)' in BONDVIEW and '(oldbuttons & B_BUTTON)' in BONDVIEW and '(L_TRIG | R_TRIG)' in BONDVIEW and 'tpShoulderCommand = -1' in BONDVIEW and 'tpShoulderCommand = 1' in BONDVIEW)
check('directional shoulder selection changes runtime TP offset without rewriting saved value',
      'g_ModThirdPersonShoulderSwapped[player] = leftshoulder ? TRUE : FALSE' in BONDVIEW and 'horizontaloffset = ((f32)TP_CAM_HORIZONTAL_DEFAULT + (f32)g_ModThirdPersonCameraHorizontalAdjust)' in BONDVIEW and '* g_ModThirdPersonShoulderBlend[shoulderplayer]' in BONDVIEW)
check('shoulder transition is smoothed rather than snapped',
      'shoulderstep = 0.12f * g_GlobalTimerDelta' in BONDVIEW and 'g_ModThirdPersonShoulderBlend[shoulderplayer] += shoulderstep' in BONDVIEW)
check('settled gadget A press gets a browse window instead of immediately lowering invisible item',
      'bondinvMissionItemModeActive()' in BONDVIEW and 'abBrowseSource' in BONDVIEW and 'moveData.weaponForwardOffset = 0' in BONDVIEW)
check('A release without B still exits gadget mode through normal forward cycle',
      'abReleaseWeaponAdvance = TRUE' in BONDVIEW and 'moveData.weaponForwardOffset = 1' in BONDVIEW)
check('settled mission item to item switch remains immediate',
      'item-to-item browsing stays immediate and cheap' in BONDINV and 'currentPlayerUnEquipWeaponWrapper(GUNRIGHT, item);' in BONDINV)
check('Micro-optimizations UI label uses lowercase o',
      '0x6F2D6F70' in SPECTRUM and 'Micro-optimizations' in SPECTRUM)
check('V29 audit is mandatory build prerequisite',
      'tp-camera-settings-v29-audit:' in MAKE and 'tp-camera-settings-v29-audit' in MAKE.split('prerequisites:',1)[1].split('\n',1)[0])

failed = [n for n,ok in checks if not ok]
if failed:
    print(f'\nTP CAMERA/INPUT V29 AUDIT: FAIL ({len(checks)-len(failed)}/{len(checks)})', file=sys.stderr)
    raise SystemExit(1)
print(f'\nTP CAMERA/INPUT V29 AUDIT: PASS ({len(checks)}/{len(checks)})')
