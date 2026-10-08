#!/usr/bin/env python3
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
MM = ROOT / "src/game/mapmaker.c"
FRONT = ROOT / "src/game/front.c"

failures = []

def require(path, needle, why):
    text = (ROOT / path).read_text(errors="replace")
    if needle not in text:
        failures.append(f"{path}: missing {why}: {needle}")

def forbid(path, needle, why):
    text = (ROOT / path).read_text(errors="replace")
    if needle in text:
        failures.append(f"{path}: {why}: {needle}")

# The editor/runtime implementation itself must never select a retail stage.
mm = MM.read_text(errors="replace")
if "selected_stage" in mm or "mission_folder_setup_entries" in mm or "multi_stage_setups" in mm:
    failures.append("mapmaker.c contains normal frontend stage-routing state")
for token in ("LEVELID_RUNWAY", "LEVELID_CITADEL", "LEVELID_DAM", "LEVELID_FACILITY"):
    if token in mm:
        failures.append(f"mapmaker.c references retail stage ID {token}")

# The frontend Native Test branch is allowed one stage assignment, and it must
# be the dedicated Map Maker stage.
front = FRONT.read_text(errors="replace")
start = front.find("void interface_menu_map_maker_basic(void)")
end = front.find("Gfx *constructor_menu_map_maker_basic", start)
if start < 0 or end < 0:
    failures.append("front.c: could not isolate Map Maker Basic interface")
else:
    block = front[start:end]
    assigns = re.findall(r"selected_stage\s*=\s*(LEVELID_[A-Z0-9_]+)", block)
    if assigns != ["LEVELID_MAP_MAKER"]:
        failures.append(f"front.c: Native Test stage assignments are {assigns!r}, expected only LEVELID_MAP_MAKER")

# Dedicated stage must have every loader-facing component of its own.
for path, needle, why in [
    ("src/bondconstants.h", "LEVELID_MAP_MAKER", "dedicated stage ID"),
    ("src/boss.c", "{ LEVELID_MAP_MAKER", "dedicated memory profile"),
    ("src/game/bg.c", '"bg/bg_mapmaker_all_p.seg"', "dedicated BG"),
    ("src/game/bg.c", '"Tbg_mapmaker_all_p_stanZ"', "dedicated STAN"),
    ("src/game/chraidata.c", '"UsetupmapmakerZ"', "dedicated setup lookup"),
    ("src/game/language.c", "case LEVELID_MAP_MAKER", "dedicated safe language mapping"),
    ("src/game/music_0D2720.c", "{ LEVELID_MAP_MAKER", "dedicated music row"),
    ("src/game/bgfog.c", "{LEVELID_MAP_MAKER", "dedicated neutral environment"),
    ("assets/obseg/bg/bg_mapmaker_all_p.c", "struct bg_header header", "private BG resource"),
    ("assets/obseg/stan/Tbg_mapmaker_all_p_stanZ.c", "Tbg_mapmaker_all_p_stanZ", "private STAN resource"),
    ("assets/obseg/setup/UsetupmapmakerZ.c", "UsetupmapmakerZ", "private setup resource"),
]:
    require(path, needle, why)

# Native Test must never alias any retail stage resource by filename.
bg = (ROOT / "src/game/bg.c").read_text(errors="replace")
row = next((line for line in bg.splitlines() if "LEVELID_MAP_MAKER" in line), "")
if row and ("bg_run_" in row or "bg_cat_" in row or "bg_len_" in row or "Tbg_run" in row or "Tbg_cat" in row):
    failures.append(f"bg.c: Map Maker row aliases a retail BG/STAN resource: {row.strip()}")

# Existing retail level IDs must stay numerically stable. Map Maker remains
# appended after PAM under GE_MAP_MAKER; later GoldenEye Plus stages may follow
# it before LEVELID_MAX as long as they are independently gated.
constants = (ROOT / "src/bondconstants.h").read_text(errors="replace")

stage_tail = re.search(
    r"LEVELID_PAM,\s*"
    r"#ifdef GE_MAP_MAKER\s*"
    r"/\* Dedicated GoldenEye Plus authoring/test stage\.\s+Never aliases a retail map\. \*/\s*"
    r"LEVELID_MAP_MAKER,\s*"
    r"#endif\s*"
    r"(?:#ifdef GE_MODDED_CHEATS\s*"
    r"(?:/\*.*?\*/\s*)?"
    r"LEVELID_COURTYARD,\s*"
    r"#endif\s*)?"
    r"LEVELID_MAX",
    constants,
    re.S,
)

if stage_tail is None:
    failures.append(
        "bondconstants.h: Map Maker/additive-stage tail is not isolated after PAM and before MAX as expected"
    )

print("Map Maker dedicated-stage isolation audit")
if failures:
    for failure in failures:
        print(f"[FAIL] {failure}")
    print("RESULT: FAIL")
    sys.exit(1)

print("[ OK ] Native Test routes only to LEVELID_MAP_MAKER")
print("[ OK ] no retail LEVELID is used as a Native Test host")
print("[ OK ] dedicated BG/STAN/setup/language/music/environment resources are present")
print("[ OK ] existing retail stage IDs remain unchanged in non-Map-Maker builds")
print("RESULT: PASS")
