#!/usr/bin/env python3
from pathlib import Path
import re, sys

root = Path(__file__).resolve().parents[1]
file2 = (root/'src/game/file2.c').read_text(errors='replace')
file2h = (root/'src/game/file2.h').read_text(errors='replace')
options = (root/'src/game/options.c').read_text(errors='replace')
makefile = (root/'Makefile').read_text(errors='replace')

checks = []
def check(name, cond):
    checks.append((name, bool(cond)))
    print(f"[{'PASS' if cond else 'FAIL'}] {name}")

check('legacy 512-byte compatibility window remains unchanged', '#define GE_SRAM_LEGACY_SIZE   0x00000200u' in file2)
check('extension starts immediately after legacy window', '#define GE_SRAM_EXT_BANK_A_OFFSET   0x0200u' in file2)
check('extension uses second 64-byte journal bank', '#define GE_SRAM_EXT_BANK_B_OFFSET   0x0240u' in file2 and '#define GE_SRAM_EXT_BANK_SIZE       0x40u' in file2)
check('extension has GEPS magic and explicit v1/v2/v3 schema', 'GE_SRAM_EXT_MAGIC           0x47455053u' in file2 and 'GE_SRAM_EXT_VERSION_LEGACY  1u' in file2 and 'GE_SRAM_EXT_VERSION_V2      2u' in file2 and 'GE_SRAM_EXT_VERSION         3u' in file2)
check('folder record stays compact at eight bytes', 'GE_SRAM_EXT_RECORD_SIZE     8u' in file2 and 'ge_sram_ext_record_must_be_8' in file2)
check('bank stays exactly 64 bytes', 'ge_sram_ext_bank_must_be_64' in file2)
check('bank has checksum validation', 'fileSramExtChecksum' in file2 and 'fileSramExtBankValid' in file2 and '0xedb88320u' in file2)
check('two-bank generation selection exists', 'generation' in file2 and '(s16)(b.generation - a.generation) > 0' in file2)
check('writes target inactive bank and read back verify', 'g_GeSramExtActiveBank == 0 ? 1 : 0' in file2 and 'fileSramExtBankValid(&verify)' in file2)
check('per-folder record contains all five TP camera tuners', all(x in file2 for x in ['camera_distance_adjust','camera_height_adjust','camera_horizontal_adjust','camera_downframe_adjust','crouch_camera_height_adjust']))
check('per-folder record stores gameplay option byte', 'u8 mod_options2;' in file2)
check('record stores GE+ option flags explicitly', all(x in file2 for x in ['GE_SRAM_EXT_FLAG_STAY_TP_DEATH','GE_SRAM_EXT_FLAG_MICROOPT','GE_SRAM_EXT_FLAG_TP_CROUCH_CAM','GE_SRAM_EXT_FLAG_DIRECTIONAL']))
check('extension is authoritative when a valid record exists', 'if (!fileSramExtLoadFolder(folder, save))' in file2)
check('legacy fields remain compatibility mirror', 'fileStoreThirdPersonCameraSettings(save);' in file2 and 'fileSramExtSyncLegacySave(save);' in file2)
check('legacy save lazily migrates into extension', 'fileStoreExtendedSettings(save);' in file2)
check('TP crouched cam height now participates in deferred save', 'g_ModWatchSettingsDirty = TRUE;' in options and 'if (row != 15)' not in options)
check('deferred commit explicitly stores extension settings', 'fileStoreExtendedSettings(save);\n        fileWriteSave(save);' in options)
check('folder deletion invalidates extension record', 'fileSramExtInvalidateFolder(foldernum);' in file2)
check('plain 4 Kbit EEPROM build retains no-op extension API', '#if !defined(GE_SAVE_SRAM) && !defined(GE_SAVE_EEPROM16K)' in file2 and 'void fileStoreExtendedSettings(save_data *save) { (void)save; }' in file2)
check('V36 audit is mandatory build prerequisite', 'sram-extended-settings-v36-audit:' in makefile and 'sram-extended-settings-v36-audit' in next((line for line in makefile.splitlines() if line.startswith('prerequisites:')), ''))

failed = [n for n,c in checks if not c]
print()
if failed:
    print(f"SRAM EXTENDED SETTINGS V36 AUDIT: FAIL ({len(checks)-len(failed)}/{len(checks)})")
    for n in failed: print('  -',n)
    sys.exit(1)
print(f"SRAM EXTENDED SETTINGS V36 AUDIT: PASS ({len(checks)}/{len(checks)})")
