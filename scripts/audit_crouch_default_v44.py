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
ck('schema v3 is current while v1/v2 remain readable for migration', '#define GE_SRAM_EXT_VERSION_LEGACY  1u' in F and '#define GE_SRAM_EXT_VERSION_V2      2u' in F and '#define GE_SRAM_EXT_VERSION         3u' in F)
ck('legacy absolute baseline 36 migrates to authored 46', 'record->crouch_camera_height_adjust == 36' in F and 'record->crouch_camera_height_adjust = TP_CROUCH_CAM_HEIGHT_DEFAULT;' in F)
ck('current records do not use crouch marker bits', 'GE_SRAM_EXT_RESERVED_CROUCH36' not in F and 'GE_SRAM_EXT_RESERVED_CROUCH46' not in F)
ck('V44 compatibility audit remains mandatory', 'crouch-default-v44-audit:' in M and 'crouch-default-v44-audit' in M.split('prerequisites:',1)[1].split('\n',1)[0])
failed=[n for n,c in checks if not c]
print(f"\nCROUCH DEFAULT V44 COMPAT AUDIT: {'PASS' if not failed else 'FAIL'} ({len(checks)-len(failed)}/{len(checks)})")
if failed: sys.exit(1)
