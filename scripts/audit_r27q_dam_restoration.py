# R27S_R3_R2_R27Q_AUDIT_SUPERSESSION
# R27Q source-shape checks superseded by mandatory R27R drivable-speedboat audit.
#!/usr/bin/env python3
from pathlib import Path
import re
import sys

R = Path(__file__).resolve().parents[1]

files = {
    "lm": R / "src/game/levelmodifiers.c",
    "lh": R / "src/game/levelmodifiers.h",
    "prop": R / "src/game/prop.c",
    "proph": R / "src/game/prop.h",
    "cleanup": R / "src/game/cleanup_objects.c",
    "front": R / "src/game/front.c",
    "options": R / "src/game/options.c",
    "mp": R / "src/game/mpmenu.c",
    "make": R / "Makefile",
}

for name, path in files.items():
    if not path.exists():
        print(f"[FAIL] Missing {path}")
        sys.exit(1)

T = {k: p.read_text(errors="replace") for k, p in files.items()}
checks = []

def ck(name, cond):
    ok = bool(cond)
    checks.append((name, ok))
    print(("[PASS] " if ok else "[FAIL] ") + name)

LM = T["lm"]
LH = T["lh"]
P = T["prop"]
PH = T["proph"]
C = T["cleanup"]
F = T["front"]
O = T["options"]
MP = T["mp"]
MK = T["make"]

ck("R27Q C comment syntax is valid",
   all(re.search(r"^\s*\*\((?:R27Q_DAM_DOCK_RESTORATION|-{5,}\))",
                 T[k], re.M) is None
       for k in ("lm", "lh", "prop", "proph")))


ck("Dam is implemented in the frontend Level Modifiers catalog",
   "index == 0) /* Dam */" in LM and
   "levelModifiersLevelImplemented" in F)

ck("Dam current-stage page exposes all three modifier rows",
   re.search(r'g_LevelModifierCurrentStage\s*==\s*LEVELID_DAM\)\s*\n\s*return\s+g_LevelModifierDamRuntimeAvailable\s*\?\s*3\s*:\s*0\s*;', LM) is not None)

ck("requested Dam modifier labels are exact",
   '"Restore Doors Near Dam Docks"' in LM and
   '"Restore Speedboat"' in LM)

ck("both Dam modifiers declare reversible policy",
   LM.count("return LEVELMOD_POLICY_REVERSIBLE;") >= 3 and
   "levelModifiersGetDamDoorsPolicy" in LM and
   "levelModifiersGetDamSpeedboatPolicy" in LM)




ck("old speedboat absolute-address hacks are not reproduced",
   all(x not in P and x not in LM for x in
       ["0x801E1F54", "0x801DB374", "0x801E1FAD",
        "801E1F54", "801DB374", "801E1FAD"]))






ck("four extra Dam model slots are reserved for restorations and drivable boat",
   "return 4; /* two restored doors + static speedboat + drivable speedboat */" in LM and
   "numObjects += levelModifiersGetReservedObjectCount(stageId);" in P)

ck("Dam templates are captured after pad/STAN resolution and before mirror transform",
   "levelModifiersOnSetupReady(stageId);" in P and
   P.find("levelModifiersOnSetupReady(stageId);") <
   P.find("mirrorLevelsApplySetupIfNeeded();"))

ck("preloaded Dam restorations spawn after ordinary setup objects",
   "levelModifiersOnPropsLoaded(stageId);" in P)


ck("frontend detail menu supports multiple rows and generic toggling",
   "g_LevelModifiersDetailChoice" in F and
   "levelModifiersGetFrontendModifierCount" in F and
   "levelModifiersToggleFrontendModifier" in F and
   "frontCheckCursorOnPreviousTab" in F)

ck("SP Watch uses generic current-stage modifier dispatch",
   "levelModifiersToggleCurrentStageModifier(row)" in O and
   "levelModifiersGetCurrentStageModifierName(row)" in O and
   "levelModifiersGetCurrentStageModifierValue(row)" in O)

ck("MP/Co-Op Watch uses generic current-stage modifier dispatch",
   "levelModifiersToggleCurrentStageModifier(row)" in MP and
   "levelModifiersGetCurrentStageModifierName(row)" in MP and
   "levelModifiersGetCurrentStageModifierValue(row)" in MP)

ck("Silo remains latched while Dam/Citadel reversible behavior is retained",
   "LEVELMOD_POLICY_LATCHED" in LM and
   "g_LevelModifierSiloBetaVentActive ? \"ACTIVE\" : \"ACTIVATE\"" in LM and
   "levelModifiersSetCitadelWater(!g_LevelModifierCitadelWaterActive)" in LM)

prereq = next((line for line in MK.splitlines() if line.startswith("prerequisites:")), "")
ck("R27Q audit is a mandatory build prerequisite",
   "r27q-dam-restoration-audit:" in MK and
   "r27q-dam-restoration-audit" in prereq)

bad = [name for name, ok in checks if not ok]
print()
print(f"R27Q DAM DOCK RESTORATION AUDIT: {'PASS' if not bad else 'FAIL'} "
      f"({len(checks) - len(bad)}/{len(checks)})")
if bad:
    for name in bad:
        print(" - " + name)
    sys.exit(1)
