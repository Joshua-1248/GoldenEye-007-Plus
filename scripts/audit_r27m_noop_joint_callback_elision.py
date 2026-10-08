#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
CHR = (ROOT / "src/game/chr.c").read_text(errors="replace")
MAKE = (ROOT / "Makefile").read_text(errors="replace")

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

need = fn(CHR, "static s32 chrNeedsJointPositionedCallback(ChrRecord *chr)")
tick = fn(CHR, "s32 chrTick(PropRecord *prop)")
callback = fn(CHR, "void chrHandleJointPositioned(enum CHR_RENDER_PART bodypart, Mtxf *matrix)")

ck("predicate covers all four live aim offsets",
   all(x in need for x in [
       "chr->aimuplshoulder != 0.0f",
       "chr->aimuprshoulder != 0.0f",
       "chr->aimupback != 0.0f",
       "chr->aimsideback != 0.0f",
   ]))

ck("predicate keeps callback for active flinch",
   "chr->flinchcnt >= 0" in need)

ck("predicate keeps callback for DK scaling",
   "CHEAT_DK_MODE" in need and
   "g_CheatPlayerTextRelated[CHEAT_DK_MODE]" in need and
   "chrCanUseDKModeScaling" in need)

ck("callback selection is Micro-Optimizations gated",
   "if (modMicroOptimizationsEnabled())" in tick and
   "chrNeedsJointPositionedCallback(g_CurModelChr)" in tick)

ck("Micro-Optimizations Off retains unconditional callback",
   "else\n        {\n            g_ModelJointPositionedFunc = chrHandleJointPositioned;" in tick)

ck("callback decision occurs after flinch advancement and expiry",
   tick.find("g_CurModelChr->flinchcnt += g_ClockTimer;") >= 0 and
   tick.find("chrNeedsJointPositionedCallback(g_CurModelChr)") >
       tick.find("g_CurModelChr->flinchcnt += g_ClockTimer;"))

ck("callback decision occurs immediately before body matrix build",
   tick.find("chrNeedsJointPositionedCallback(g_CurModelChr)") <
       tick.find("subcalcmatrices(&renderdata, model);") and
   tick.find("g_ModelJointPositionedFunc = NULL;") >
       tick.find("subcalcmatrices(&renderdata, model);"))

ck("g_CurModelChr remains established before flinch and matrix work",
   tick.find("g_CurModelChr = chr;") <
       tick.find("g_CurModelChr->flinchcnt >= 0") and
   tick.find("g_CurModelChr = chr;") <
       tick.find("subcalcmatrices(&renderdata, model);"))

ck("joint callback still preserves arm torso head behavior",
   all(x in callback for x in [
       "CHR_RENDERPART_LEFT_ARM",
       "CHR_RENDERPART_RIGHT_ARM",
       "CHR_RENDERPART_TORSO",
       "CHR_RENDERPART_HEAD",
       "chrGetFlinchAmount(chr)",
       "matrix_4x4_multiply_homogeneous_in_place",
   ]))

ck("body skeleton build itself is never skipped",
   "subcalcmatrices(&renderdata, model);" in tick)

prereq = next((line for line in MAKE.splitlines() if line.startswith("prerequisites:")), "")
ck("R27M audit is a mandatory build prerequisite",
   "r27m-noop-joint-callback-elision-audit:" in MAKE and
   "r27m-noop-joint-callback-elision-audit" in prereq)

failed = [name for name, ok in checks if not ok]
print()
if failed:
    print(f"R27M NO-OP JOINT CALLBACK ELISION AUDIT: FAIL ({len(checks)-len(failed)}/{len(checks)})")
    for name in failed:
        print(" - " + name)
    sys.exit(1)

print(f"R27M NO-OP JOINT CALLBACK ELISION AUDIT: PASS ({len(checks)}/{len(checks)})")
