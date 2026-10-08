#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
MODEL = (ROOT / "src/game/model.c").read_text(errors="replace")
MATRIX = (ROOT / "src/game/matrixmath.c").read_text(errors="replace")
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

fast = fn(MODEL, "static void modelMatrixMultiplyHomogeneousUnrolled(")
retail = fn(MATRIX, "void matrix_4x4_multiply_homogeneous(Mtxf *lhs, Mtxf *rhs, Mtxf *result)")

ck("retail homogeneous multiply remains untouched in matrixmath.c",
   "for (i = 0; i < 3; i++)" in retail and
   "for (j = 0; j < 4; j++)" in retail and
   "if (j == 3)" in retail)

# The helper intentionally uses the standard macro wrapper
# `do { ... } while (0)`. That is not a runtime matrix loop. Remove only
# that exact macro idiom before checking for actual runtime loops.
fast_without_macro_wrappers = fast.replace("while (0)", "")

ck("R27N fast helper contains no runtime matrix loops",
   "for (" not in fast_without_macro_wrappers and
   "while (" not in fast_without_macro_wrappers)

ck("R27N uses same three multiply/add stages per output",
   "value = lhs->m[0][I] * rhs->m[J][0];" in fast and
   "value += lhs->m[1][I] * rhs->m[J][1];" in fast and
   "value += lhs->m[2][I] * rhs->m[J][2];" in fast and
   "result->m[J][I] = value;" in fast)

ck("all twelve 3-term dot products are explicitly expanded",
   all(f"MODEL_MUL_DOT3({i}, {j});" in fast
       for i in range(3) for j in range(4)))

ck("translation is still a separate final add for each axis",
   "result->m[3][0] += lhs->m[3][0];" in fast and
   "result->m[3][1] += lhs->m[3][1];" in fast and
   "result->m[3][2] += lhs->m[3][2];" in fast)

ck("homogeneous fourth column remains exact",
   "result->m[0][3] = 0.0f;" in fast and
   "result->m[1][3] = 0.0f;" in fast and
   "result->m[2][3] = 0.0f;" in fast and
   "result->m[3][3] = 1.0f;" in fast)

ck("runtime gate reads the Micro-Optimizations bit directly",
   "g_ModGameplayOptions3 & MODOPT3_ENABLE_MICROOPT" in MODEL)

ck("Micro-Optimizations Off still calls retail helper",
   "matrix_4x4_multiply_homogeneous((LHS), (RHS), (RESULT));" in MODEL)

site_count = MODEL.count("MODEL_MATRIX_MUL_HOMO(") - 2
ck("all known model homogeneous-multiply sites are routed", site_count >= 11)
print(f"[INFO] routed model homogeneous multiply sites: {site_count}")

ck("matrix/joint callback behavior remains after matrix construction",
   "g_ModelJointPositionedFunc(matrix0, matrix0_mtx);" in MODEL or
   "g_ModelJointPositionedFunc(matrix0, matrix0_mtx)" in MODEL)

ck("R27I/J/K/L model optimizations remain present",
   "modelLoadAnimationFrameCached" in MODEL and
   "modelAnimRead3x12AsU16AngleFast" in MODEL and
   "R27K:" in MODEL and
   "modelSinCosExactCached" in MODEL)

prereq = next((line for line in MAKE.splitlines() if line.startswith("prerequisites:")), "")
ck("R27N audit is a mandatory build prerequisite",
   "r27n-exact-unrolled-model-matrix-multiply-audit:" in MAKE and
   "r27n-exact-unrolled-model-matrix-multiply-audit" in prereq)

failed = [name for name, ok in checks if not ok]
print()
if failed:
    print(f"R27N EXACT UNROLLED MODEL MATRIX MULTIPLY AUDIT: FAIL ({len(checks)-len(failed)}/{len(checks)})")
    for name in failed:
        print(" - " + name)
    sys.exit(1)

print(f"R27N EXACT UNROLLED MODEL MATRIX MULTIPLY AUDIT: PASS ({len(checks)}/{len(checks)})")
