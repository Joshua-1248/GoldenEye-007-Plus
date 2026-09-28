#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
MAKE = (ROOT/'Makefile').read_text(errors='replace')
FILE2 = (ROOT/'src/game/file2.c').read_text(errors='replace')
JOY = (ROOT/'src/joy.c').read_text(errors='replace')
PROBE = (ROOT/'src/libultrare/io/conteepprobe.c').read_text(errors='replace')
READ = (ROOT/'src/libultrare/io/conteepread.c').read_text(errors='replace')
WRITE = (ROOT/'src/libultrare/io/conteepwrite.c').read_text(errors='replace')
LREAD = (ROOT/'src/libultrare/io/conteeplongread.c').read_text(errors='replace')
LWRITE = (ROOT/'src/libultrare/io/conteeplongwrite.c').read_text(errors='replace')
FRONT = (ROOT/'src/game/front.c').read_text(errors='replace')
MPMENU = (ROOT/'src/game/mpmenu.c').read_text(errors='replace')

checks=[]
def ck(name, cond):
    checks.append((name, bool(cond)))
    print(('[PASS] ' if cond else '[FAIL] ') + name)

ck('Plus defaults to 16 Kbit EEPROM', 'SAVE_EEPROM16K ?= $(MODDED_CHEATS)' in MAKE)
ck('SRAM is retained but opt-in', 'SAVE_SRAM ?= NO' in MAKE and '-DGE_SAVE_SRAM' in MAKE)
ck('SRAM and EEPROM16 are mutually exclusive', 'SAVE_EEPROM16K=YES and SAVE_SRAM=YES are mutually exclusive' in MAKE)
ck('EEPROM16 compile define is emitted', '-DGE_SAVE_EEPROM16K' in MAKE)
ck('EEPROM16 build has separate output identity', 'OUTCODE := $(EEPROM16_BASE_OUTCODE)-eep16' in MAKE)
ck('2.0I-style probe distinguishes 4K and 16K', 'CONT_EEPROM | CONT_EEP16K' in PROBE and 'EEPROM_TYPE_16K' in PROBE)
ck('single-block EEPROM read accepts 16K device', ('type != (CONT_EEPROM | CONT_EEP16K)' in READ or 'case CONT_EEPROM | CONT_EEP16K:' in READ) and 'EEPROM_MAXBLOCKS' in READ)
ck('single-block EEPROM write accepts 16K device', ('type != (CONT_EEPROM | CONT_EEP16K)' in WRITE or 'case CONT_EEPROM | CONT_EEP16K:' in WRITE) and 'EEPROM_MAXBLOCKS' in WRITE)
ck('long EEPROM read permits 256-block address space', 'EEP16K_MAXBLOCKS' in LREAD and '#ifdef GE_SAVE_EEPROM16K' in LREAD)
ck('long EEPROM write permits 256-block address space', 'EEP16K_MAXBLOCKS' in LWRITE and '#ifdef GE_SAVE_EEPROM16K' in LWRITE)
ck('extension journal occupies EEPROM bytes 0x200..0x27f', 'GE_SRAM_EXT_BANK_A_OFFSET   0x0200u' in FILE2 and 'GE_SRAM_EXT_BANK_B_OFFSET   0x0240u' in FILE2)
ck('EEPROM extension transport maps byte offsets to 8-byte blocks', 'block = offset / EEPROM_BLOCK_SIZE;' in FILE2 and 'joyGamePakLongRead((u8)block' in FILE2 and 'joyGamePakLongWrite((u8)block' in FILE2)
ck('EEPROM16 build requires a real 16K probe result', 'joyGamePakProbe() == EEPROM_TYPE_16K' in FILE2)
ck('extended settings are active for either SRAM or EEPROM16', '#if defined(GE_SAVE_SRAM) || defined(GE_SAVE_EEPROM16K)' in FILE2 and '#if defined(GE_SAVE_SRAM) || defined(GE_SAVE_EEPROM16K)' in FRONT and '#if defined(GE_SAVE_SRAM) || defined(GE_SAVE_EEPROM16K)' in MPMENU)
ck('legacy 512-byte save layout remains intact', 'save_data_size_must_remain_0x60' in FILE2 and 'GE_SRAM_LEGACY_SIZE   0x00000200u' in FILE2)
ck('versioned extension journal remains in use', 'GE_SRAM_EXT_VERSION_LEGACY  1u' in FILE2 and 'GE_SRAM_EXT_VERSION_V2      2u' in FILE2 and 'GE_SRAM_EXT_VERSION         3u' in FILE2)
ck('V46 audit is mandatory build prerequisite', 'eeprom16-backend-v46-audit:' in MAKE and 'eeprom16-backend-v46-audit' in next((x for x in MAKE.splitlines() if x.startswith('prerequisites:')), ''))

bad=[n for n,ok in checks if not ok]
print()
print(f"EEPROM16 BACKEND V46 AUDIT: {'PASS' if not bad else 'FAIL'} ({len(checks)-len(bad)}/{len(checks)})")
if bad:
    for n in bad: print(' - ' + n)
    sys.exit(1)
