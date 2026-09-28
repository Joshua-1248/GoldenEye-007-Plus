#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
MAKE = (ROOT / 'Makefile').read_text(errors='replace')
HEADER = (ROOT / 'src/rom_header.s').read_text(errors='replace')
PROBE = (ROOT / 'src/libultrare/io/conteepprobe.c').read_text(errors='replace')
READ = (ROOT / 'src/libultrare/io/conteepread.c').read_text(errors='replace')
WRITE = (ROOT / 'src/libultrare/io/conteepwrite.c').read_text(errors='replace')
LREAD = (ROOT / 'src/libultrare/io/conteeplongread.c').read_text(errors='replace')
LWRITE = (ROOT / 'src/libultrare/io/conteeplongwrite.c').read_text(errors='replace')
DOC = (ROOT / 'docs/FLASHCART_COMPATIBILITY.md').read_text(errors='replace')

checks = []
def ck(name, cond):
    checks.append((name, bool(cond)))
    print(('[PASS] ' if cond else '[FAIL] ') + name)

ck('GoldenEye Plus still defaults to EEPROM16K', 'SAVE_EEPROM16K ?= $(MODDED_CHEATS)' in MAKE)
ck('modded ROM uses ED homebrew flashcart ID', '.ifdef GE_MODDED_CHEATS' in HEADER and '.ascii "ED"' in HEADER)
ck('retail/non-modded ROM keeps GE cartridge ID', '.ascii "GE"       # retail GoldenEye cartridge ID' in HEADER)
ck('EEPROM16 homebrew header config is 0x20', '.ifdef GE_SAVE_EEPROM16K' in HEADER and '.byte  0x20' in HEADER)
ck('SRAM opt-in homebrew header config is 0x30', '.ifdef GE_SAVE_SRAM' in HEADER and '.byte  0x30' in HEADER)
ck('EEPROM probe distinguishes 4K and 16K', 'CONT_EEPROM | CONT_EEP16K' in PROBE and 'EEPROM_TYPE_16K' in PROBE)
ck('EEPROM16 read follows later libultra transaction sequence',
   'case CONT_EEPROM | CONT_EEP16K:' in READ and
   'ret = __osSiRawStartDma(OS_READ, &__osEepPifRam);' in READ and
   'Later libultra builds only populate the active Joybus command.' in READ)
ck('EEPROM16 write follows later libultra transaction sequence',
   'case CONT_EEPROM | CONT_EEP16K:' in WRITE and
   'Keep the write command/response transaction intact' in WRITE)
ck('EEPROM status marks status request as END in EEPROM16 build',
   '#ifdef GE_SAVE_EEPROM16K' in WRITE and '__osContLastCmd = CONT_CMD_END;' in WRITE)
ck('EEPROM16 long reads do not impose EEPROM write-cycle delay',
   '#ifndef GE_SAVE_EEPROM16K' in LREAD and 'osSetTimer' in LREAD)
ck('EEPROM long writes retain write-cycle delay', 'osSetTimer' in LWRITE and 'osRecvMesg' in LWRITE)
ck('flashcart compatibility document exists', 'SummerCart64' in DOC and 'EverDrive-64' in DOC and '16 Kbit' in DOC and '2 KiB' in DOC)
ck('V84 audit is a mandatory prerequisite',
   'flashcart-eeprom16-v84-audit:' in MAKE and
   'flashcart-eeprom16-v84-audit' in next((x for x in MAKE.splitlines() if x.startswith('prerequisites:')), ''))

bad = [name for name, ok in checks if not ok]
print()
print(f"FLASHCART / EEPROM16 V84 AUDIT: {'PASS' if not bad else 'FAIL'} ({len(checks)-len(bad)}/{len(checks)})")
if bad:
    for name in bad:
        print(' - ' + name)
    sys.exit(1)
