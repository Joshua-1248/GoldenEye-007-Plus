#!/usr/bin/env python3
"""Convert a GoldenEye 4 Kbit EEPROM save image into GE+ 32 KiB SRAM.

The V35 SRAM backend mirrors the complete 512-byte retail EEPROM address space
at SRAM offsets 0x0000..0x01ff.  The rest of SRAM is initialized to 0xff and
reserved for future GoldenEye Plus data.

Many emulators store a 4 Kbit EEPROM save in a 2048-byte .eep container.  Only
the first 512 bytes belong to the 4 Kbit device and are imported.
"""
from pathlib import Path
import argparse
import sys

EEP4K_BYTES = 512
SRAM_BYTES = 32768


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("input", type=Path, help="GoldenEye .eep file (512 or padded 2048 bytes)")
    ap.add_argument("output", type=Path, help="output .sra file")
    ap.add_argument("--force", action="store_true", help="overwrite output if it exists")
    args = ap.parse_args()

    raw = args.input.read_bytes()
    if len(raw) < EEP4K_BYTES:
        print(f"error: input is only {len(raw)} bytes; need at least {EEP4K_BYTES}", file=sys.stderr)
        return 2
    if len(raw) not in (EEP4K_BYTES, 2048):
        print(f"warning: input is {len(raw)} bytes; importing the first 512 bytes as 4 Kbit EEPROM", file=sys.stderr)
    if args.output.exists() and not args.force:
        print(f"error: {args.output} exists (use --force to overwrite)", file=sys.stderr)
        return 2

    out = bytearray([0xFF]) * SRAM_BYTES
    out[:EEP4K_BYTES] = raw[:EEP4K_BYTES]
    args.output.write_bytes(out)
    print(f"Imported {EEP4K_BYTES} legacy EEPROM bytes into {SRAM_BYTES}-byte SRAM image: {args.output}")
    print("SRAM expansion area 0x0200..0x7fff initialized to 0xff.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
