#!/usr/bin/env python3
from pathlib import Path
import sys
R=Path(__file__).resolve().parents[1]
F=(R/'src/game/file2.c').read_text(errors='replace')
OH=(R/'src/game/options.h').read_text(errors='replace')
ROH=(R/'options.h').read_text(errors='replace')
M=(R/'Makefile').read_text(errors='replace')
checks=[]
def ck(n,c): checks.append((n,bool(c))); print(('[PASS] ' if c else '[FAIL] ')+n)
ck('authored TP Crouched Cam Height default is 46 in both headers', '#define TP_CROUCH_CAM_HEIGHT_DEFAULT 46' in OH and '#define TP_CROUCH_CAM_HEIGHT_DEFAULT 46' in ROH)
ck('SRAM extension schema has advanced through v2/v3 to v4', '#define GE_SRAM_EXT_VERSION_LEGACY  1u' in F and '#define GE_SRAM_EXT_VERSION_V2      2u' in F and '#define GE_SRAM_EXT_VERSION_V3      3u' in F and '#define GE_SRAM_EXT_VERSION         4u' in F)
ck('validator accepts v1/v2/v3 as migration sources', 'bank->version != GE_SRAM_EXT_VERSION_LEGACY' in F and 'bank->version != GE_SRAM_EXT_VERSION_V2' in F and 'bank->version != GE_SRAM_EXT_VERSION_V3' in F and 'bank->version != GE_SRAM_EXT_VERSION' in F)
ck('v1 pre-absolute records migrate to canonical 46', '!(record->reserved & GE_SRAM_EXT_V1_CROUCH_ABSOLUTE)' in F and 'record->crouch_camera_height_adjust = TP_CROUCH_CAM_HEIGHT_DEFAULT;' in F)
ck('v1 authored absolute 36 migrates to canonical 46', 'record->crouch_camera_height_adjust == 36' in F and 'record->crouch_camera_height_adjust = TP_CROUCH_CAM_HEIGHT_DEFAULT;' in F)
ck('other v1 absolute values are preserved', '|| record->crouch_camera_height_adjust == 36' in F)
ck('legacy crouch marker bits are stripped in v2', 'record->reserved &= (GE_SRAM_EXT_RESERVED_AA_VALID | GE_SRAM_EXT_RESERVED_AA_ENABLED);' in F)
ck('schema migration is journal committed by bank version', 'g_GeSramExtBank.version = GE_SRAM_EXT_VERSION;' in F and 'return fileSramExtCommit();' in F)
ck('ordinary current saves contain no crouch-generation marker bits', '#define GE_SRAM_EXT_RESERVED_V75_CAMERA_REPAIR 0x20' in F and 'record.reserved = GE_SRAM_EXT_RESERVED_AA_VALID' in F and 'GE_SRAM_EXT_RESERVED_V75_CAMERA_REPAIR' in F and 'GE_SRAM_EXT_V1_CROUCH_MASK' not in F[F.find('record.reserved = GE_SRAM_EXT_RESERVED_AA_VALID'):F.find('if (g_ModStayInTpOnDeathDefault)', F.find('record.reserved = GE_SRAM_EXT_RESERVED_AA_VALID'))])
ck('exact absolute user-facing height remains persisted', 'record.crouch_camera_height_adjust = TP_CROUCH_CAM_HEIGHT_DEFAULT' in F and '+ g_ModThirdPersonCrouchCameraHeightAdjust;' in F)
ck('canonical value 36 cannot be remigrated on normal load', 'if (!fileSramExtEnsureCurrentVersion())' in F and F.count('record->crouch_camera_height_adjust == 36') == 1)
ck('V45 audit is mandatory build prerequisite', 'crouch-default-v45-audit:' in M and 'crouch-default-v45-audit' in M.split('prerequisites:',1)[1].split('\n',1)[0])
failed=[n for n,c in checks if not c]
if failed:
    print(f'\nCROUCH DEFAULT V45 AUDIT: FAIL ({len(checks)-len(failed)}/{len(checks)})', file=sys.stderr)
    for n in failed: print(' - '+n, file=sys.stderr)
    raise SystemExit(1)
print(f'\nCROUCH DEFAULT V45 AUDIT: PASS ({len(checks)}/{len(checks)})')
