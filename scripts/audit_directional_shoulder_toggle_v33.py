#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
BV = (ROOT/'src/game/bondview2.c').read_text(errors='replace')
F2 = (ROOT/'src/game/file2.c').read_text(errors='replace')
F2H = (ROOT/'src/game/file2.h').read_text(errors='replace')
OPT = (ROOT/'src/game/options.c').read_text(errors='replace')
FRONT = (ROOT/'src/game/front.c').read_text(errors='replace')
MP = (ROOT/'src/game/mpmenu.c').read_text(errors='replace')
SPEC = (ROOT/'src/game/spectrum.c').read_text(errors='replace')
MAKE = (ROOT/'Makefile').read_text(errors='replace')

checks=[]
def check(name, cond):
    checks.append((name, bool(cond)))
    print(('[PASS] ' if cond else '[FAIL] ') + name)

check('Directional Shoulder has a dedicated persistent bit',
      'MODOPT3_DIRECTIONAL_SHOULDER' in F2H and '0x20' in F2H)
check('new option defaults Off',
      '#define DEFAULT_MOD_OPTIONS3            MODOPT3_SIGNATURE' in F2H)
check('V32 options marker migrates with reclaimed bit cleared',
      'MODOPT3_LEGACY_SIGNATURE_V32' in F2H and
      '(save->mod_options3 & 0x1f) | MODOPT3_SIGNATURE' in F2 and
      '(save->mod_options3 & 0x1f) | MODOPT3_SIGNATURE' in FRONT)
check('camera-pack three-bit signature is untouched',
      '#define MOD_CAMERA_PACK_SIGNATURE3             0x07' in F2H and
      '(((u32)(save->mod_options3 >> 1) & 0x07) << 27)' in F2)
check('default shoulder input treats either B+L or B+R as one toggle command',
      'tpShoulderCommand = 2;' in BV and
      'freshshoulder = (buttons & ~oldbuttons) & (L_TRIG | R_TRIG)' in BV)
check('symmetric toggle flips the current shoulder state',
      'bondviewToggleThirdPersonShoulder(abChordPlayer)' in BV and
      'bondviewSetThirdPersonShoulder(player, !g_ModThirdPersonShoulderSwapped[player])' in BV)
check('Directional On retains explicit B+L left and B+R right',
      'g_ModGameplayOptions3 & MODOPT3_DIRECTIONAL_SHOULDER' in BV and
      'tpShoulderCommand = -1' in BV and 'tpShoulderCommand = 1' in BV)
check('second-controller input path follows the same toggle/directional policy',
      'freshshoulder = (abSecondButtonsRaw & ~copy_prev_buttons_pressed) & (L_TRIG | R_TRIG)' in BV and
      BV.count('MODOPT3_DIRECTIONAL_SHOULDER') >= 2)
check('main-menu Third-Person Options exposes the setting',
      'labelindex = i == 0 ? 32 : i <= 3 ? 46 + i : i == 4 ? 51 : 51 + i' in FRONT and
      'MODOPT3_DIRECTIONAL_SHOULDER' in FRONT)
check('SP watch Third-Person Options exposes the setting',
      'MODWATCH_MODE_TP_OPTIONS' in OPT and 'frontModGetOptionLabel(49)' in OPT and
      'MODOPT3_DIRECTIONAL_SHOULDER' in OPT)
check('MP watch Third-Person Options exposes the setting',
      'if (mode == 6) return 11;' in MP and '47 + row' in MP and
      'MODOPT3_DIRECTIONAL_SHOULDER' in MP)
check('current compact label is present',
      'Directional Shoulder' in SPEC and
      '0x44697265' in SPEC and '0x6C646572; buf[5]=0;' in SPEC)
check('label scratch storage includes room for 32 chars plus terminator',
      ('static u32 text[4][9];' in SPEC or 'static u32 text[4][10];' in SPEC))
check('main-menu base row bounds reflect submenu restructuring',
      's32 maxrow = g_ModOptionsPage ? 9 : 10;' in FRONT and
      'MENU_ENHANCEMENTS_OPTIONS' in FRONT and 'MENU_THIRD_PERSON_OPTIONS' in FRONT)
check('V33 audit is mandatory build prerequisite',
      'directional-shoulder-toggle-v33-audit:' in MAKE and
      'directional-shoulder-toggle-v33-audit' in MAKE.split('prerequisites:',1)[1].split('\n',1)[0])

failed=[n for n,ok in checks if not ok]
if failed:
    print(f'\nDIRECTIONAL SHOULDER TOGGLE V33 AUDIT: FAIL ({len(checks)-len(failed)}/{len(checks)})', file=sys.stderr)
    for n in failed: print(' - '+n, file=sys.stderr)
    raise SystemExit(1)
print(f'\nDIRECTIONAL SHOULDER TOGGLE V33 AUDIT: PASS ({len(checks)}/{len(checks)})')
