#!/usr/bin/env python3
from pathlib import Path
import re
import sys

R = Path(__file__).resolve().parents[1]
EX = (R/"src/game/explosion.c").read_text(errors="replace")
MK = (R/"Makefile").read_text(errors="replace")

checks = []

def ck(name, cond):
    ok = bool(cond)
    checks.append((name, ok))
    print(("[PASS] " if ok else "[FAIL] ") + name)

def func(name):
    pat = re.compile(r"\b" + re.escape(name) + r"\s*\(")

    for m in pat.finditer(EX):
        p = EX.find("(", m.start())
        depth = 0
        close = -1

        for i in range(p, len(EX)):
            if EX[i] == "(":
                depth += 1
            elif EX[i] == ")":
                depth -= 1
                if depth == 0:
                    close = i
                    break

        if close < 0:
            continue

        q = close + 1
        while q < len(EX) and EX[q].isspace():
            q += 1

        if q >= len(EX) or EX[q] != "{":
            continue

        depth = 0
        for i in range(q, len(EX)):
            if EX[i] == "{":
                depth += 1
            elif EX[i] == "}":
                depth -= 1
                if depth == 0:
                    return EX[m.start():i+1]

    return ""

fast = func("explosionRenderPartFast")
normal = func("explosionRenderPart")
smoke = func("explosionSmokeRenderPart")
particles = func("explosionRenderFlyingParticles")
prop = func("explosionRenderPropExplosion")

ck("exact audit parser resolves distinct flare functions",
   fast and normal and
   "Vtx base = g_ExplosionRenderPartDefaultVertex;" in fast and
   "Vtx spA0;" in normal)

ck("Unlimited scratch guard is option scoped",
   "if (!g_ModUnlimitedExplosionsEnabled)" in EX)

ck("16 KiB VTX/Mtx safety reserve is enforced",
   "#define R27O_EXP_RENDER_VTX_RESERVE 0x4000" in EX and
   "dynGetFreeVtx()" in EX)

ck("512-command GFX safety reserve is enforced",
   "#define R27O_EXP_RENDER_GFX_RESERVE 512" in EX and
   "dynGetFreeGfx(gdl)" in EX)

ck("fast flare reserves fifth vertex in Unlimited mode",
   "g_ModUnlimitedExplosionsEnabled" in fast and
   "dynAllocateVertices(5)" in fast and
   "dynAllocateVertices(4)" in fast and
   "vertices[4] = base;" in fast)

ck("normal flare reserves fifth vertex in Unlimited mode",
   "g_ModUnlimitedExplosionsEnabled" in normal and
   "dynAllocateVertices(5)" in normal and
   "dynAllocateVertices(4)" in normal and
   "vertices[4] = spA0;" in normal)

ck("smoke checks scratch before dynamic vertex allocation",
   "gdl, 4 * sizeof(Vtx), 2" in smoke and
   smoke.find("gdl, 4 * sizeof(Vtx), 2") < smoke.find("dynAllocateVertices(4)"))

ck("flying debris checks scratch before dynamic matrix allocation",
   "gdl, sizeof(Mtx), 3" in particles and
   particles.find("gdl, sizeof(Mtx), 3") < particles.find("dynAllocateMatrix()"))

ck("explosion prop checks GFX reserve",
   "explosionUnlimitedRenderScratchAvailable(gdl, 0, 24)" in prop)

ck("simulation remains expanded rather than re-capped",
   "g_ModUnlimitedExplosionsEnabled" in EX and
   "EXPLOSION_BUFFER_LEN_RETAIL" in EX and
   "var_v0 < explosion_limit" in EX)

prereq = next((x for x in MK.splitlines() if x.startswith("prerequisites:")), "")
ck("R27O R4 audit is mandatory",
   "r27o-r4-explosion-render-scratch-audit:" in MK and
   "r27o-r4-explosion-render-scratch-audit" in prereq)

bad = [name for name, ok in checks if not ok]

print()
if bad:
    print(f"R27O R4 EXPLOSION RENDER SCRATCH AUDIT: FAIL ({len(checks)-len(bad)}/{len(checks)})")
    for name in bad:
        print(" - " + name)
    sys.exit(1)

print(f"R27O R4 EXPLOSION RENDER SCRATCH AUDIT: PASS ({len(checks)}/{len(checks)})")
