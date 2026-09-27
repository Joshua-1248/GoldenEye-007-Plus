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
ck('authored crouch default is 46 in both public headers', '#define TP_CROUCH_CAM_HEIGHT_DEFAULT 46' in OH and '#define TP_CROUCH_CAM_HEIGHT_DEFAULT 46' in ROH)
ck('legacy v1 crouch metadata remains recognizable for migration', 'GE_SRAM_EXT_V1_CROUCH_ABSOLUTE' in F)
ck('pre-absolute legacy records reset to the authored current baseline', '!(record->reserved & GE_SRAM_EXT_V1_CROUCH_ABSOLUTE)' in F and 'record->crouch_camera_height_adjust = TP_CROUCH_CAM_HEIGHT_DEFAULT;' in F)
ck('legacy migration is committed through the journaled bank writer', 'g_GeSramExtBank.version = GE_SRAM_EXT_VERSION;' in F and 'return fileSramExtCommit();' in F)
ck('ordinary current records no longer consume crouch generation bits', 'record.reserved = GE_SRAM_EXT_RESERVED_AA_VALID' in F and 'GE_SRAM_EXT_V1_CROUCH_MASK' in F)
ck('V41 R3 audit remains a mandatory compatibility prerequisite', 'crouch-default-v41r3-audit:' in M and 'crouch-default-v41r3-audit' in M.split('prerequisites:',1)[1].split('\n',1)[0])
failed=[n for n,c in checks if not c]
if failed:
    print(f'\nCROUCH DEFAULT SRAM MIGRATION V41 R3 AUDIT: FAIL ({len(checks)-len(failed)}/{len(checks)})', file=sys.stderr)
    for n in failed: print(' - '+n, file=sys.stderr)
    raise SystemExit(1)
print(f'\nCROUCH DEFAULT SRAM MIGRATION V41 R3 AUDIT: PASS ({len(checks)}/{len(checks)})')
