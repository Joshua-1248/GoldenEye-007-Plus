#!/usr/bin/env python3
from pathlib import Path
import sys

R = Path(__file__).resolve().parents[1]
LD = (R/"ge007.ld").read_text(errors="replace")
EH = (R/"src/game/explosion.h").read_text(errors="replace")
INIT = (R/"src/game/initexplosioncasing.c").read_text(errors="replace")
EX = (R/"src/game/explosion.c").read_text(errors="replace")
MK = (R/"Makefile").read_text(errors="replace")

checks = []

def ck(name, cond):
    ok = bool(cond)
    checks.append((name, ok))
    print(("[PASS] " if ok else "[FAIL] ") + name)

ck("expanded explosion pool is a dedicated high-memory reservation",
   "_unlimitedExplosionPoolStart" in LD and
   "_unlimitedExplosionPoolEnd = _unlimitedExplosionPoolStart + 0xF800;" in LD and
   "_unlimitedExplosionPoolEnd <= 0x80800000" in LD)

ck("high-memory reservation follows existing Plus cache space",
   "ALIGN(_modelTrigCacheEnd, 16)" in LD or
   "ALIGN(_thirdPersonBodyCacheEnd, 16)" in LD)

ck("64 entries are restricted to physical-fastpath Plus builds",
   "#if defined(GE_MODDED_CHEATS) && defined(GE_PHYSICAL_FASTPATHS)" in EH and
   "#define EXPLOSION_BUFFER_LEN 64" in EH and
   "#define EXPLOSION_BUFFER_LEN EXPLOSION_BUFFER_LEN_RETAIL" in EH)

ck("retail capacity remains exactly six",
   "#define EXPLOSION_BUFFER_LEN_RETAIL 6" in EH)

ck("Explosion ABI is guarded at 0x3E0",
   "r27o_explosion_size_must_be_0x3e0" in INIT and
   "sizeof(struct Explosion) == 0x3E0" in INIT)

ck("physical Plus uses the dedicated pool directly",
   "g_ExplosionBuffer = (struct Explosion *)_unlimitedExplosionPoolStart;" in INIT)

ck("nonphysical/retail fallback still uses MEMPOOL_STAGE",
   "mempAllocBytesInBank(" in INIT and
   "EXPLOSION_BUFFER_LEN * sizeof(struct Explosion), MEMPOOL_STAGE" in INIT)

ck("Off still limits new explosion creation to six",
   "g_ModUnlimitedExplosionsEnabled" in EX and
   ": EXPLOSION_BUFFER_LEN_RETAIL;" in EX and
   "var_v0 < explosion_limit" in EX)

ck("On still exposes the complete allocated pool",
   "? EXPLOSION_BUFFER_LEN" in EX)

prereq = next((x for x in MK.splitlines() if x.startswith("prerequisites:")), "")
ck("R27O R3 audit is mandatory",
   "r27o-r3-highmem-explosion-pool-audit:" in MK and
   "r27o-r3-highmem-explosion-pool-audit" in prereq)

bad = [name for name, ok in checks if not ok]
print()
if bad:
    print(f"R27O R3 HIGHMEM EXPLOSION POOL AUDIT: FAIL ({len(checks)-len(bad)}/{len(checks)})")
    for name in bad:
        print(" - " + name)
    sys.exit(1)

print(f"R27O R3 HIGHMEM EXPLOSION POOL AUDIT: PASS ({len(checks)}/{len(checks)})")
