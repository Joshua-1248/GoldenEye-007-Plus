#!/usr/bin/env python3
from pathlib import Path
import sys
R=Path(__file__).resolve().parents[1]
F=(R/'src/game/file2.c').read_text(errors='replace')
MK=(R/'Makefile').read_text(errors='replace')
checks=[]
def ck(name, cond):
    checks.append((name, bool(cond)))
    print(('[PASS] ' if cond else '[FAIL] ')+name)

ck('extension reserves a one-time V75 camera repair marker',
   '#define GE_SRAM_EXT_RESERVED_V75_CAMERA_REPAIR 0x20' in F)
ck('generic legacy sync preserves TP camera distance',
   'record.camera_distance_adjust = current->camera_distance_adjust;' in F)
ck('generic legacy sync preserves TP camera height',
   'record.camera_height_adjust = current->camera_height_adjust;' in F)
ck('generic legacy sync preserves TP camera horizontal',
   'record.camera_horizontal_adjust = current->camera_horizontal_adjust;' in F)
ck('generic legacy sync preserves TP camera down-frame',
   'record.camera_downframe_adjust = current->camera_downframe_adjust;' in F)
ck('generic legacy sync preserves TP crouched camera height',
   'record.crouch_camera_height_adjust = current->crouch_camera_height_adjust;' in F)
ck('V74 poison repair recognizes the exact 4/72/96 displayed signature',
   'record->camera_height_adjust == 16' in F and
   'record->camera_horizontal_adjust == 96' in F and
   'record->camera_downframe_adjust == 72' in F)
ck('known poison repair resets only those three adjustments to authored defaults',
   'record->camera_height_adjust = 0;' in F and
   'record->camera_horizontal_adjust = 0;' in F and
   'record->camera_downframe_adjust = 0;' in F)
ck('repair is one-time and persisted in the extension bank',
   'if (!(record->reserved & GE_SRAM_EXT_RESERVED_V75_CAMERA_REPAIR))' in F and
   'record->reserved |= GE_SRAM_EXT_RESERVED_V75_CAMERA_REPAIR;' in F and
   'fileSramExtCommit();' in F)
ck('normal extended-settings writes mark records as already checked',
   '| GE_SRAM_EXT_RESERVED_V75_CAMERA_REPAIR' in F)
ck('V75 audit is a mandatory build prerequisite',
   'tp-camera-legacy-sync-v75-audit:' in MK and
   'tp-camera-legacy-sync-v75-audit' in next((x for x in MK.splitlines() if x.startswith('prerequisites:')),''))

bad=[n for n,c in checks if not c]
print()
print(f"TP CAMERA LEGACY SYNC V75 AUDIT: {'PASS' if not bad else 'FAIL'} ({len(checks)-len(bad)}/{len(checks)})")
if bad:
    for n in bad: print(' - '+n)
    sys.exit(1)
