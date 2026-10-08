#!/usr/bin/env python3
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
MODEL = (ROOT / "src/game/model.c").read_text(errors="replace")
ASSET = (ROOT / "assets/animationtable_data.c").read_text(errors="replace")
MAKE = (ROOT / "Makefile").read_text(errors="replace")

checks = []

def ck(name, cond):
    ok = bool(cond)
    checks.append((name, ok))
    print(("[PASS] " if ok else "[FAIL] ") + name)

def generic(data, width, bitoffset):
    value = 0
    remaining = width
    p = bitoffset // 8
    bitoffset %= 8
    nthis = 8 - bitoffset

    while remaining >= nthis:
        remaining -= nthis
        mask = (1 << nthis) - 1
        value |= (data[p] & mask) << remaining
        value &= 0xffff
        p += 1
        nthis = 8

    if remaining > 0:
        mask = (1 << remaining) - 1
        value |= (data[p] >> (nthis - remaining)) & mask
        value &= 0xffff

    value <<= 16 - width
    return value & 0xffff

def fast_one(data, channel):
    off = channel + (channel >> 1)
    if channel & 1:
        v = ((data[off] & 0x0f) << 8) | data[off + 1]
    else:
        v = (data[off] << 4) | (data[off + 1] >> 4)
    return (v << 4) & 0xffff

def fast_three(data, channel):
    off = channel + (channel >> 1)
    b0, b1, b2, b3, b4 = data[off:off+5]
    if channel & 1:
        return [
            ((((b0 & 0x0f) << 8) | b1) << 4) & 0xffff,
            (((b2 << 4) | (b3 >> 4)) << 4) & 0xffff,
            ((((b3 & 0x0f) << 8) | b4) << 4) & 0xffff,
        ]
    return [
        (((b0 << 4) | (b1 >> 4)) << 4) & 0xffff,
        ((((b1 & 0x0f) << 8) | b2) << 4) & 0xffff,
        (((b3 << 4) | (b4 >> 4)) << 4) & 0xffff,
    ]

# Deterministic exhaustive byte-pair proof for the single 12-bit reader.
single_ok = True
for a in range(256):
    for b in range(256):
        data = bytes([a, b, 0, 0, 0, 0, 0, 0])
        if fast_one(data, 0) != generic(data, 12, 0):
            single_ok = False
            break
        # channel 1 starts at bit 12 -> byte 1 low nibble, so shift data.
        data2 = bytes([0, a, b, 0, 0, 0, 0, 0])
        if fast_one(data2, 1) != generic(data2, 12, 12):
            single_ok = False
            break
    if not single_ok:
        break

ck("single-channel 12-bit formula is exhaustive-equivalent to retail reader",
   single_ok)

# Deterministic dense sample proof for three consecutive channels, both parities.
triple_ok = True
seed = 0x13579bdf
for i in range(65536):
    buf = []
    for j in range(8):
        seed = (1664525 * seed + 1013904223) & 0xffffffff
        buf.append((seed >> 24) & 0xff)
    data = bytes(buf)

    for channel in (0, 1):
        got = fast_three(data, channel)
        ref = [
            generic(data, 12, (channel + 0) * 12),
            generic(data, 12, (channel + 1) * 12),
            generic(data, 12, (channel + 2) * 12),
        ]
        if got != ref:
            triple_ok = False
            break

    if not triple_ok:
        break

ck("XYZ 12-bit formula matches retail reader for 65,536 deterministic samples",
   triple_ok)

ck("fast helpers are physical-fast-only",
   "#ifdef GE_PHYSICAL_FASTPATHS" in MODEL and
   "R27J exact 12-bit animation decoder" in MODEL)

ck("XYZ fast path is gated by Micro-Optimizations and exact width 12",
   "if (modMicroOptimizationsEnabled() && width == 12)" in MODEL and
   "modelAnimRead3x12AsU16AngleFast(bitstream, channel, rotation);" in MODEL)

ck("single-axis fast path is gated and exact",
   "modelAnimRead12AsU16AngleFast(bitstream, channel)" in MODEL)

ck("generic bit reader remains present for all non-12-bit/custom animations",
   MODEL.count("modelAnimReadBitsAsU16Angle(bitstream, width, bitoffset)") >= 3 or
   (
       "rotation[0] = modelAnimReadBitsAsU16Angle(bitstream, width, bitoffset);" in MODEL and
       "raw = modelAnimReadBitsAsU16Angle(bitstream, width, bitOffset);" in MODEL
   ))

ck("XYZ float conversion and mirror semantics remain unchanged",
   "rot->x = (rotation[0] * M_TAU_F) / M_U16_MAX_VALUE_F;" in MODEL and
   "((0x10000 - rotation[1]) * M_TAU_F) / M_U16_MAX_VALUE_F;" in MODEL and
   "((0x10000 - rotation[2]) * M_TAU_F) / M_U16_MAX_VALUE_F;" in MODEL)

ck("single-axis float conversion remains unchanged",
   "angle = ((f32)(s32)(0x10000 - raw) * M_TAU_F) / M_U16_MAX_VALUE_F;" in MODEL and
   "angle = ((f32)raw * M_TAU_F) / M_U16_MAX_VALUE_F;" in MODEL)

# Verify authored width distribution rather than assuming it.
widths = {}
pattern = re.compile(r"u32\s+ANIM_DATA_[A-Za-z0-9_]+\[\]\s*=\s*\{\s*([^}]+)\}", re.S)
for m in pattern.finditer(ASSET):
    vals = [x.strip() for x in m.group(1).split(",") if x.strip()]
    if len(vals) >= 2 and re.fullmatch(r"0x[0-9a-fA-F]+", vals[1]):
        word1 = int(vals[1], 16)
        width = (word1 >> 8) & 0xff
        widths[width] = widths.get(width, 0) + 1

ck("all non-empty authored animations currently use 12-bit channels",
   widths.get(12, 0) > 0 and
   set(widths.keys()).issubset({0, 12}) and
   widths.get(0, 0) <= 1)

print("[INFO] authored animation width distribution:", widths)

prereq = next((line for line in MAKE.splitlines() if line.startswith("prerequisites:")), "")
ck("R27J audit is a mandatory build prerequisite",
   "r27j-exact-12bit-joint-decode-audit:" in MAKE and
   "r27j-exact-12bit-joint-decode-audit" in prereq)

failed = [name for name, ok in checks if not ok]
print()
if failed:
    print(f"R27J EXACT 12-BIT JOINT DECODE AUDIT: FAIL ({len(checks)-len(failed)}/{len(checks)})")
    for name in failed:
        print(" - " + name)
    sys.exit(1)

print(f"R27J EXACT 12-BIT JOINT DECODE AUDIT: PASS ({len(checks)}/{len(checks)})")
