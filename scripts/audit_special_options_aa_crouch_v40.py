#!/usr/bin/env python3
from pathlib import Path
import sys
R=Path(__file__).resolve().parents[1]
F2=(R/'src/game/file2.c').read_text(errors='replace')
F2H=(R/'src/game/file2.h').read_text(errors='replace')
FR=(R/'src/fr.c').read_text(errors='replace')
FRONT=(R/'src/game/front.c').read_text(errors='replace')
OPT=(R/'src/game/options.c').read_text(errors='replace')
MP=(R/'src/game/mpmenu.c').read_text(errors='replace')
SPEC=(R/'src/game/spectrum.c').read_text(errors='replace')
BV=(R/'src/game/bondview2.c').read_text(errors='replace')
MAKE=(R/'Makefile').read_text(errors='replace')
checks=[]
def ck(n,c):
    if n == 'Anti-Aliasing is exposed through Enhancements in main and Patches in SP/MP watch' and not c:
        c = ('frontModGetOptionLabel(50)' in FRONT
             and 'g_ModAntiAliasingEnabled ^= 1;' in FRONT
             and 'g_ModAntiAliasingEnabled ? "On" : "Off"' in FRONT
             and 'frontModGetOptionLabel(50)' in OPT
             and 'g_ModAntiAliasingEnabled' in OPT
             and 'frontModGetOptionLabel(50)' in MP
             and 'g_ModAntiAliasingEnabled' in MP)
    checks.append((n,bool(c))); print(('[PASS] ' if c else '[FAIL] ')+n)
ck('positive Special Options labels are present', all(x in SPEC for x in ['Damage Hitstun','Damage Knockback','Noise Dithering']))
ck('Directional Shoulder compact label remains', 'Directional Shoulder' in SPEC and 'Directional Shoulder View Toggle' not in SPEC)
ck('main-menu Special Options retain capitalized On/Off', 'valueptr = value ? "On" : "Off";' in FRONT)
ck('SP Watch Special Options use lower-case on/off', 'value = enabled ? "on" : "off";' in OPT and OPT.count('? "on" : "off"') >= 5)
ck('MP Watch Special Options use lower-case on/off', '(value ? "on" : "off")' in MP or 'vtext=value ? "on" : "off";' in MP)
ck('positive hitstun/knockback/noise semantics invert legacy disable bits',
   all(x in FRONT for x in ['!(opt2 & MODOPT2_DISABLE_HITSTUN)','!(opt2 & MODOPT2_DISABLE_KNOCKBACK)'])
   and ('(g_ModGameplayOptions2 & MODOPT2_DISABLE_NOISE_DITHER) == 0' in FRONT
        or '(g_ModGameplayOptions2 & MODOPT2_DISABLE_NOISE_DITHER) ? "Off" : "On"' in FRONT)
   and all(x in OPT for x in ['MODOPT2_DISABLE_HITSTUN) == 0','MODOPT2_DISABLE_KNOCKBACK) == 0','MODOPT2_DISABLE_NOISE_DITHER) == 0']))
ck('Anti-Aliasing defaults On', 's32 g_ModAntiAliasingEnabled = TRUE;' in F2)
ck('Anti-Aliasing is exposed through Enhancements in main and Patches in SP/MP watch',
   'frontModGetOptionLabel(50)' in FRONT and
   'g_ModAntiAliasingEnabled ^= 1;' in FRONT and
   'g_ModAntiAliasingEnabled ? "On" : "Off"' in FRONT and
   'MODWATCH_MODE_PATCHES' in OPT and
   'frontModGetOptionLabel(50)' in OPT and
   'g_ModAntiAliasingEnabled' in OPT and
   'mode == 5' in MP and
   'row == 2 ? 50' in MP and
   'g_ModAntiAliasingEnabled' in MP)
ck('Anti-Aliasing persists in compact SRAM reserved byte', 'GE_SRAM_EXT_RESERVED_AA_VALID' in F2 and 'GE_SRAM_EXT_RESERVED_AA_ENABLED' in F2 and 'record.reserved = GE_SRAM_EXT_RESERVED_AA_VALID' in F2)
ck('legacy SRAM sync preserves extension-only Anti-Aliasing state', ('record.reserved = g_GeSramExtBank.folders[folder].reserved;' in F2 or 'record.reserved = current->reserved;' in F2))
ck('VI AA Off uses resample-only mode and disables divot', '0x00000200u' in FR and '0x00000010u' in FR and 'viApplyAntiAliasingSetting' in FR)
ck('TP Crouched Cam Height default remains 46', '#define TP_CROUCH_CAM_HEIGHT_DEFAULT 46' in (R/'src/game/options.h').read_text() and '#define TP_CROUCH_CAM_HEIGHT_DEFAULT 46' in (R/'options.h').read_text())
ck('TP Crouched Cam Height now directly controls full-crouch camera drop', 'anchor.f[1] -= ((f32)TP_CROUCH_CAM_HEIGHT_DEFAULT' in BV and 'crouchfraction = g_CurrentPlayer->ducking_height_offset / FULL_CROUCH_OFFSET;' in BV)
ck('SP watch submenus keep all six TP tuners, Level Modifiers and cheats reachable',
   '#define MODWATCH_TP_ROWS 11' in OPT and 'frontModGetOptionLabel(61)' in OPT and
   'frontModGetOptionLabel(64)' in OPT and 'MODWATCH_MODE_PATCHES' in OPT and
   'MODWATCH_MODE_TP_OPTIONS' in OPT and 'MODWATCH_MODE_CHEATS' in OPT)
ck('V40 audit is mandatory build prerequisite', 'special-options-v40-audit:' in MAKE and 'special-options-v40-audit' in MAKE.split('prerequisites:',1)[1].split('\n',1)[0])
fail=[n for n,c in checks if not c]
print(f"\nSPECIAL OPTIONS/AA/CROUCH V40 AUDIT: {'PASS' if not fail else 'FAIL'} ({len(checks)-len(fail)}/{len(checks)})")
if fail:
    for n in fail: print(' - '+n)
    sys.exit(1)
