#!/usr/bin/env python3
from pathlib import Path
import re
import sys

root = Path(__file__).resolve().parents[1]
joy = (root / 'src/joy.c').read_text(errors='replace')
file2 = (root / 'src/game/file2.c').read_text(errors='replace')
make = (root / 'Makefile').read_text(errors='replace')
conv = (root / 'scripts/convert_eep_to_sram.py').read_text(errors='replace')
checks = [
    ('SRAM retained but disabled by default', 'SAVE_SRAM ?= NO' in make),
    ('16 Kbit EEPROM defaults on for Plus builds', 'SAVE_EEPROM16K ?= $(MODDED_CHEATS)' in make),
    ('SRAM compile define exists', '-DGE_SAVE_SRAM' in make),
    ('SRAM build has separate output identity', 'OUTCODE := $(SRAM_BASE_OUTCODE)-sram' in make),
    ('EEPROM backend retained', 'osEepromLongRead' in joy),
    ('SRAM transport stays out of fixed resident joy.c', 'GE_SRAM_BASE' not in joy and 'fileSramTransfer' in file2),
    ('standard SRAM base', '#define GE_SRAM_BASE          0x08000000u' in file2),
    ('32 KiB SRAM size', '#define GE_SRAM_SIZE          0x00008000u' in file2),
    ('legacy EEPROM window is 512 bytes', '#define GE_SRAM_LEGACY_SIZE   0x00000200u' in file2),
    ('legacy EEPROM block addresses map x8', '(u32)address * EEPROM_BLOCK_SIZE' in file2),
    ('SRAM PI domain 2 latency', 'PI_BSD_DOM2_LAT_REG, 0x05' in file2),
    ('SRAM PI domain 2 pulse', 'PI_BSD_DOM2_PWD_REG, 0x0c' in file2),
    ('SRAM PI domain 2 page', 'PI_BSD_DOM2_PGS_REG, 0x0d' in file2),
    ('SRAM PI domain 2 release', 'PI_BSD_DOM2_RLS_REG, 0x02' in file2),
    ('PI timing restored after access', 'fileSramRestorePiTiming' in file2),
    ('DMA uses aligned bounce buffer', 'g_GeSramBounce' in file2),
    ('converter creates 32768-byte SRAM', 'SRAM_BYTES = 32768' in conv),
    ('converter imports first 512 bytes', 'out[:EEP4K_BYTES] = raw[:EEP4K_BYTES]' in conv),
    ('SRAM probe reserves final 16 bytes', '#define GE_SRAM_PROBE_OFFSET  (GE_SRAM_SIZE - 0x10u)' in file2 and '#define GE_SRAM_PROBE_SIZE    0x10u' in file2),
    ('SRAM probe is cached per boot', 'g_GeSramProbeState = -1' in file2 and 'if (g_GeSramProbeState >= 0)' in file2),
    ('SRAM probe performs read-write-readback', 'fileSramTransfer(GE_SRAM_PROBE_OFFSET, saved' in file2 and 'fileSramTransfer(GE_SRAM_PROBE_OFFSET, (void *)probeA' in file2 and 'fileSramTransfer(GE_SRAM_PROBE_OFFSET, verify' in file2),
    ('SRAM probe verifies exact pattern', 'verify[i] != probeA[i]' in file2),
    ('SRAM probe restores reserved bytes', 'fileSramTransfer(GE_SRAM_PROBE_OFFSET, saved, GE_SRAM_PROBE_SIZE, OS_WRITE)' in file2),
    ('SRAM transport no longer calls osPiStartDma', 'ret = osPiStartDma(' not in file2 and 'osPiStartDma(&g_GeSramIoMesg' not in file2),
    ('SRAM transport takes PI access lock', '__osPiGetAccess();' in file2 and '__osPiRelAccess();' in file2),
    ('SRAM programs Domain-2 cart address directly', 'PI_CART_ADDR_REG, GE_SRAM_BASE + offset' in file2),
    ('SRAM read uses PI WR length register', 'PI_WR_LEN_REG, amount - 1' in file2),
    ('SRAM write uses PI RD length register', 'PI_RD_LEN_REG, amount - 1' in file2),
    ('save probe uses SRAM readback result', 'return fileSramProbe();' in file2),
]
failed = 0
for name, ok in checks:
    print(('[PASS] ' if ok else '[FAIL] ') + name)
    failed += not ok
print(f"SRAM BACKEND V35 R4 AUDIT: {'PASS' if not failed else 'FAIL'} ({len(checks)-failed}/{len(checks)})")
sys.exit(1 if failed else 0)
