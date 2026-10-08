#!/usr/bin/env python3
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
MODEL = (ROOT / "src/game/model.c").read_text(errors="replace")
ANI = (ROOT / "src/game/initanitable.c").read_text(errors="replace")
LD = (ROOT / "ge007.ld").read_text(errors="replace")
MAKE = (ROOT / "Makefile").read_text(errors="replace")
ASSET = (ROOT / "assets/animationtable_data.c").read_text(errors="replace")

checks = []

def ck(name, cond):
    ok = bool(cond)
    checks.append((name, ok))
    print(("[PASS] " if ok else "[FAIL] ") + name)

def fn(text, sig):
    s = text.find(sig)
    if s < 0:
        return ""
    b = text.find("{", s)
    if b < 0:
        return ""
    d = 0
    for i in range(b, len(text)):
        if text[i] == "{":
            d += 1
        elif text[i] == "}":
            d -= 1
            if d == 0:
                return text[s:i+1]
    return ""

load = fn(MODEL, "s32 loadAnimationFrame(ModelAnimation* anim, s32 frame, ModelSkeleton* unused)")
subcalc = fn(MODEL, "void subcalcmatrices(ModelRenderData *arg0, struct Model *arg1)")
cacheload = fn(MODEL, "static s32 modelLoadAnimationFrameCached(u32 source, u32 size, u32 sourceoffset)")
cacheinit = fn(MODEL, "void modelInitAnimationFrameCache(void)")

ck("cache is reserved after TP body cache, outside retail lower BSS",
   "_animFrameCacheStart = ALIGN(_thirdPersonBodyCacheEnd, 16);" in LD and
   "_animFrameCacheEnd = _animFrameCacheStart + 0x1700;" in LD)

ck("cache has an 8 MiB hard bound",
   'ASSERT(_animFrameCacheEnd <= 0x80800000' in LD)

ck("R27G2 fast scratch remains a separate reservation",
   "_gameFastBssEnd" in LD and
   "_animFrameCacheStart = ALIGN(_thirdPersonBodyCacheEnd, 16);" in LD)

ck("64 slots x 80 bytes and 2-way lookup are encoded",
   "#define MOD_ANIM_CACHE_SLOT_COUNT 64" in MODEL and
   "#define MOD_ANIM_CACHE_WAYS 2" in MODEL and
   "#define MOD_ANIM_CACHE_SLOT_SIZE 80" in MODEL)

ck("metadata lives in linker-reserved cache, not static lower BSS arrays",
   "_animFrameCacheStart + MOD_ANIM_CACHE_META_OFFSET" in MODEL and
   "static ModAnimFrameCacheMeta" not in MODEL)

ck("cache initialization is deterministic at animation startup",
   "modelInitAnimationFrameCache();" in ANI and
   "meta[i].valid = FALSE;" in cacheinit and
   "nextway[i] = 0;" in cacheinit)

ck("cache key validates exact ROM source and DMA size",
   "meta[slot].source == source" in cacheload and
   "meta[slot].size == size" in cacheload)

ck("cache publishes tag only after blocking ROM DMA",
   cacheload.find("romCopy(dest, (void *)source, size);") >= 0 and
   cacheload.find("meta[slot].source = source;") >
       cacheload.find("romCopy(dest, (void *)source, size);"))

ck("Micro-Optimizations runtime gate controls cache use",
   "if (modMicroOptimizationsEnabled() && size <= MOD_ANIM_CACHE_SLOT_SIZE)" in load)

ck("RAM-backed animations retain original direct-pointer path",
   "if (anim->address & 0x80000000)" in load and
   "ret = anim->address + (frame * frameSize);" in load)

ck("odd ROM source alignment semantics are preserved",
   "if (source & 1)" in load and
   "source--;" in load and
   "frameSize++;" in load and
   "sourceoffset = 1;" in load)

ck("oversized/future frames retain retail ROM-copy fallback",
   "romCopy((void* ) dest, (void* ) source, size);" in load)

ck("retail scratch allocator progression is preserved on cache path",
   load.count("D_80036414->uselessPointer += 1;") >= 2 and
   load.count("D_80036414->animBufferPtr2 = dest + size;") >= 2)

ck("skeleton decoder/interpolation still requests the same 2-4 frames",
   "arg1->unk34 = loadAnimationFrame(arg1->anim, arg1->framea" in subcalc and
   "arg1->unk38 = loadAnimationFrame(arg1->anim, arg1->frameb" in subcalc and
   "arg1->unk64 = loadAnimationFrame(arg1->anim2, arg1->frame2a" in subcalc and
   "arg1->unk68 = loadAnimationFrame(arg1->anim2, arg1->frame2b" in subcalc and
   "instcalcmatrices(arg0, arg1);" in subcalc)

sizes = []
pattern = re.compile(r"u32\s+ANIM_DATA_[A-Za-z0-9_]+\[\]\s*=\s*\{\s*([^}]+)\}", re.S)
for m in pattern.finditer(ASSET):
    vals = [x.strip() for x in m.group(1).split(",") if x.strip()]
    if len(vals) >= 4 and re.fullmatch(r"0x[0-9a-fA-F]+", vals[3]):
        word3 = int(vals[3], 16)
        framesize = (word3 & 0xffff) >> 3
        aligned = (framesize + 15) & ~15
        sizes.append(aligned)

maxsize = max(sizes) if sizes else -1
ck("current authored animations fit the 80-byte fast slot",
   bool(sizes) and maxsize <= 80)
print(f"[INFO] parsed animation entries: {len(sizes)}, maximum aligned DMA frame: {maxsize} bytes")

prereq = next((line for line in MAKE.splitlines() if line.startswith("prerequisites:")), "")
ck("R27I audit is a mandatory build prerequisite",
   "r27i-character-animation-frame-cache-audit:" in MAKE and
   "r27i-character-animation-frame-cache-audit" in prereq)

failed = [name for name, ok in checks if not ok]
print()
if failed:
    print(f"R27I CHARACTER ANIMATION FRAME CACHE AUDIT: FAIL ({len(checks)-len(failed)}/{len(checks)})")
    for name in failed:
        print(" - " + name)
    sys.exit(1)

print(f"R27I CHARACTER ANIMATION FRAME CACHE AUDIT: PASS ({len(checks)}/{len(checks)})")
