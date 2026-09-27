#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
FRONT = (ROOT / 'src/game/front.c').read_text()
FILE2 = (ROOT / 'src/game/file2.c').read_text()
FILE2H = (ROOT / 'src/game/file2.h').read_text()
OPTIONS = (ROOT / 'src/game/options.c').read_text()
SPEC = (ROOT / 'src/game/spectrum.c').read_text()
MAKE = (ROOT / 'Makefile').read_text()

checks = []
def check(name, cond):
    checks.append((name, bool(cond)))
    print(('[PASS] ' if cond else '[FAIL] ') + name)

check('public extended-settings load API exists',
      's32 fileLoadExtendedSettings(save_data *save);' in FILE2H and
      's32 fileLoadExtendedSettings(save_data *save)' in FILE2)
check('frontend loads authoritative extended-settings journal before rendering Special Options',
      '#if defined(GE_SAVE_SRAM) || defined(GE_SAVE_EEPROM16K)' in FRONT and 'if (!fileLoadExtendedSettings(save))' in FRONT)
check('frontend falls back to legacy mirror when extension is absent',
      'g_ModGameplayOptions2 = save->mod_options2;' in FRONT and
      'fileLoadThirdPersonCameraSettings(save);' in FRONT)
check('frontend exit commits live GE+ state to extension',
      'fileWriteSave(save);' in FRONT and 'fileStoreExtendedSettings(save);' in FRONT)
check('frontend extended-settings commit remains deferred to menu exit',
      FRONT.find('fileStoreExtendedSettings(save);') > FRONT.find('if (g_ModOptionsDirty)'))
check('Directional Shoulder compact label is encoded',
      '/* Directional Shoulder */' in SPEC and
      '0x6C646572; buf[5]=0;' in SPEC and
      'Directional Shoulder View Toggle */' not in SPEC)
check('main-menu Special Options preserve capitalized On/Off',
      'valueptr = value ? "On" : "Off";' in FRONT)
check('in-game Special Options base rows use lower-case on/off',
      'value = enabled ? "on" : "off";' in OPTIONS)
check('in-game GE+ Special Options use lower-case on/off',
      OPTIONS.count('? "on" : "off"') >= 4)
check('ordinary options page retains its existing capitalization policy',
      '(opt&OPTION_AUTOAIM)?"On":"Off"' in FRONT)
check('V39 audit is mandatory build prerequisite',
      'sram-frontend-special-v39-audit' in MAKE and
      'scripts/audit_sram_frontend_special_options_v39.py' in MAKE)

failed=[n for n,ok in checks if not ok]
print(f"\nSRAM FRONTEND/SPECIAL OPTIONS V39 AUDIT: {'PASS' if not failed else 'FAIL'} ({len(checks)-len(failed)}/{len(checks)})")
if failed:
    for n in failed: print(' - ' + n)
    raise SystemExit(1)
