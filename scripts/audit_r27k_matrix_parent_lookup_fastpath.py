#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
MODEL = (ROOT / "src/game/model.c").read_text(errors="replace")
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

idxfn = fn(MODEL, "s32 modelFindNodeMtxIndex(ModelNode *node, s32 arg1)")
mtxfn = fn(MODEL, "Mtxf *modelFindNodeMtx(struct Model *model, struct ModelNode *node, s32 arg2)")

ck("matrix-index fast path is Micro-Optimizations gated and slot-0 only",
   "modMicroOptimizationsEnabled() && arg1 == 0" in idxfn)

ck("matrix-pointer fast path is Micro-Optimizations gated and slot-0 only",
   "modMicroOptimizationsEnabled() && arg2 == 0" in mtxfn)

ck("HEADER maps to the exact authored MatrixIndex",
   "node->Data->Header.MatrixIndex" in idxfn and
   "model->render_pos[node->Data->Header.MatrixIndex].pos" in mtxfn)

ck("GROUP and OP03 map to exact MatrixID0",
   idxfn.count("MODELNODE_OPCODE_GROUP") >= 2 and
   "return node->Data->Group.MatrixID0;" in idxfn and
   "return &model->render_pos[node->Data->Group.MatrixID0].pos;" in mtxfn)

ck("GROUPSIMPLE maps to exact Group1",
   "return node->Data->GroupSimple.Group1;" in idxfn and
   "return &model->render_pos[node->Data->GroupSimple.Group1].pos;" in mtxfn)

ck("fast path walks the same Parent chain and preserves not-found result",
   "node = node->Parent;" in idxfn and
   "return -1;" in idxfn and
   "node = node->Parent;" in mtxfn and
   "return NULL;" in mtxfn)

ck("generic 0x100/0x200 selector remains intact for nonzero slots",
   "MatrixIDs[arg1 == 0x200 ? 2 : (arg1 == 0x100 ? 1 : 0)]" in idxfn)

ck("modelFindNodeMtx retains generic helper fallback",
   "s32 index = modelFindNodeMtxIndex(node, arg2);" in mtxfn and
   "return &model->render_pos[index].pos;" in mtxfn)

ck("no matrices, joints, animation decoders or render relations are skipped",
   "modelUpdateMatrices(ModelRenderData *arg0, Model *model)" in MODEL and
   "process_02_position(arg0, model, node);" in MODEL and
   "modelUpdateDistanceRelations(model, node);" in MODEL and
   "modelUpdateReorderRelations(model, node);" in MODEL)

prereq = next((line for line in MAKE.splitlines() if line.startswith("prerequisites:")), "")
ck("R27K audit is a mandatory build prerequisite",
   "r27k-matrix-parent-lookup-fastpath-audit:" in MAKE and
   "r27k-matrix-parent-lookup-fastpath-audit" in prereq)

failed = [name for name, ok in checks if not ok]
print()
if failed:
    print(f"R27K MATRIX PARENT LOOKUP FASTPATH AUDIT: FAIL ({len(checks)-len(failed)}/{len(checks)})")
    for name in failed:
        print(" - " + name)
    sys.exit(1)

print(f"R27K MATRIX PARENT LOOKUP FASTPATH AUDIT: PASS ({len(checks)}/{len(checks)})")
