#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
MODEL = (ROOT / "src/game/model.c").read_text(errors="replace")
LD = (ROOT / "ge007.ld").read_text(errors="replace")
ANI = (ROOT / "src/game/initanitable.c").read_text(errors="replace")
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

cache = fn(MODEL, "static void modelSinCosExactCached(f32 angle, f32 *sine, f32 *cosine)")
mat = fn(MODEL, "static void modelSetPositionRotationXYZExactCached(")
quat = fn(MODEL, "static void modelQuaternionSetRotationXYZExactCached(")
group = fn(MODEL, "void modelBuildGroupMatrices(Mtxf **parentMtx, Model *model, ModelGroupMtxBuildArg *mgm, coord3d *rot)")
proc = fn(MODEL, "void process_02_position(ModelRenderData *arg0, Model *model, ModelNode *node)")
init = fn(MODEL, "void modelInitTrigCache(void)")

ck("trig cache follows R27I cache and stays inside 8 MiB",
   "_modelTrigCacheStart = ALIGN(_animFrameCacheEnd, 16);" in LD and
   "_modelTrigCacheEnd = _modelTrigCacheStart + 0x1100;" in LD and
   'ASSERT(_modelTrigCacheEnd <= 0x80800000' in LD)

ck("cache is 256-entry 2-way and uses no retail lower-BSS arrays",
   "#define MOD_MODEL_TRIG_SLOT_COUNT 256" in MODEL and
   "#define MOD_MODEL_TRIG_WAYS 2" in MODEL and
   "(ModModelTrigEntry *)_modelTrigCacheStart" in MODEL and
   "static ModModelTrigEntry" not in MODEL)

ck("cache key is exact f32 bit pattern",
   "bits.f = angle;" in cache and
   "entries[slot].key == bits.u" in cache)

ck("cache misses call the same retail cosf then sinf functions",
   "*cosine = cosf(angle);" in cache and
   "*sine = sinf(angle);" in cache)

ck("cache publishes only after retail trig values exist",
   cache.find("entries[slot].sine = *sine;") >
       cache.find("*sine = sinf(angle);") and
   cache.find("entries[slot].valid = TRUE;") >
       cache.find("entries[slot].cosine = *cosine;"))

ck("cache initializes deterministically with animation system",
   "modelInitTrigCache();" in ANI and
   "entries[i].valid = FALSE;" in init and
   "nextway[i] = 0;" in init)

ck("ordinary XYZ matrix formula preserves retail operation ordering",
   all(x in mat for x in [
       "sin_x_sin_z = sin_x * sin_z;",
       "cos_x_sin_z = cos_x * sin_z;",
       "sin_x_cos_z = sin_x * cos_z;",
       "cos_x_cos_z = cos_x * cos_z;",
       "matrix->m[1][0] = ((sin_x_cos_z * sin_y) - cos_x_sin_z);",
       "matrix->m[2][2] = cos_x * cos_y;"
   ]))

ck("ordinary animated joints use cache only under Micro-Optimizations",
   group.count("modMicroOptimizationsEnabled()") >= 2 and
   "modelSetPositionRotationXYZExactCached" in group and
   "matrix_4x4_set_position_and_rotation_around_xyz" in group)

ck("blended quaternion formula preserves retail operation ordering",
   all(x in quat for x in [
       "half_x = angles->f[0] * 0.5f;",
       "cos_x_cos_y = cos_x * cos_y;",
       "cos_x_sin_y = cos_x * sin_y;",
       "sin_x_cos_y = sin_x * cos_y;",
       "sin_x_sin_y = sin_x * sin_y;",
       "q[0] = (cos_x_cos_y * cos_z) + (sin_x_sin_y * sin_z);",
       "q[3] = (cos_x_cos_y * sin_z) - (sin_x_sin_y * cos_z);"
   ]))

ck("blend path retains retail quaternion and slerp fallbacks",
   proc.count("modMicroOptimizationsEnabled()") >= 2 and
   "modelQuaternionSetRotationXYZExactCached(&rot1, q1);" in proc and
   "modelQuaternionSetRotationXYZExactCached(&rot3, q2);" in proc and
   "quaternion_set_rotation_around_xyzf(&rot1, q1);" in proc and
   "quaternion_set_rotation_around_xyzf(&rot3, q2);" in proc and
   "quaternion_slerp(q1, q2, model->unk84, result);" in proc)

# R27N replaces the literal retail homogeneous-multiply call with an exact
# Micro-Optimizations wrapper.  R27L only needs to prove that the same
# parent/local transform is still multiplied and the joint callback remains.
parent_multiply_present = (
    "matrix_4x4_multiply_homogeneous(parent, &tmp, matrix0_mtx);" in group
    or "MODEL_MATRIX_MUL_HOMO(parent, &tmp, matrix0_mtx);" in group
)

ck("joint callbacks and parent matrix multiplication remain intact",
   parent_multiply_present and
   "g_ModelJointPositionedFunc(matrix0, matrix0_mtx);" in group)

prereq = next((line for line in MAKE.splitlines() if line.startswith("prerequisites:")), "")
ck("R27L audit is a mandatory build prerequisite",
   "r27l-exact-model-trig-cache-audit:" in MAKE and
   "r27l-exact-model-trig-cache-audit" in prereq)

failed = [name for name, ok in checks if not ok]
print()
if failed:
    print(f"R27L EXACT MODEL TRIG CACHE AUDIT: FAIL ({len(checks)-len(failed)}/{len(checks)})")
    for name in failed:
        print(" - " + name)
    sys.exit(1)

print(f"R27L EXACT MODEL TRIG CACHE AUDIT: PASS ({len(checks)}/{len(checks)})")
