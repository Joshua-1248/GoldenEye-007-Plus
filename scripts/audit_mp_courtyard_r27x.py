#!/usr/bin/env python3
from pathlib import Path
import hashlib, re, struct, sys

ROOT = Path(__file__).resolve().parents[1]
checks = []

def ck(name, ok):
    checks.append((name, bool(ok)))
    print(("[PASS] " if ok else "[FAIL] ") + name)

def txt(path):
    return (ROOT / path).read_text(errors="replace")

def sh(path):
    return hashlib.sha256((ROOT / path).read_bytes()).hexdigest()

const = txt("src/bondconstants.h")
chrai = txt("src/game/chraidata.c")
bg = txt("src/game/bg.c")
lang = txt("src/game/language.c")
front = txt("src/game/front.c")
boss = txt("src/boss.c")
oddh = txt("assets/oddtextures.h")
oddc = txt("assets/oddtextures.c")
images = txt("assets/images.def")
obsegh = txt("assets/obseg/obseg.h")
obseg = txt("assets/obseg/ob_seg.s")
resids = txt("assets/obseg/file_resource_id_enums.h")
restable = txt("assets/obseg/file_resource_table.inc.c")
title = txt("assets/obseg/text/LtitleE.c")
make = txt("Makefile")

ck("exact recovered Courtyard BG is installed",
   sh("assets/obseg/bg/bg_courtyard_all_p.bin") ==
   "f3f0b8bfd914473291c8b5d361da332ccb77a9631d913319a6234012ea1bc7e5")
ck("exact recovered Courtyard compressed STAN is installed",
   sh("assets/obseg/stan/Tbg_courtyard_all_p_stanZ.rz") ==
   "b60118f8eb8d3d73aa0548da6e09fc1cfa5fd37a60f7b46dafa771ff7a37ecfd")
ck("exact recovered Courtyard compressed MP setup is installed",
   sh("assets/obseg/setup/Ump_setupcourtyardZ.rz") ==
   "53cddbff266e50aba4deb09df9a381ff0f7b431ab2ab3ca6c5ac7657fabd05a3")
ck("exact BMW Courtyard portrait is additive",
   sh("assets/images/split/MP_COURTYARD.bin") ==
   "a4441c032cb27f6ca0cf28b67ac54041970f103358683d106527482ba3257377")

ck("Courtyard stage ID appends after existing Plus stages",
   "LEVELID_COURTYARD" in const and const.find("LEVELID_COURTYARD") < const.find("LEVELID_MAX"))
ck("Courtyard MP enum follows Citadel",
   re.search(r"MP_STAGE_CITADEL,\s*MP_STAGE_COURTYARD,", const) is not None)
ck("setup loader can synthesize compact Ump_cZ Courtyard resource",
   '"UcZ"' in chrai)
ck("Courtyard has native BG/STAN registration at exact Dam scale",
   'LEVELID_COURTYARD, "cb", "cs", 0.23363999, 0.2, 100.0' in bg)
ck("Courtyard avoids unknown-stage language hang",
   re.search(r"case\s+LEVELID_COURTYARD:.*?return_id\s*=\s*LCAT;", lang, re.S) is not None)
ck("Courtyard is selectable in multiplayer",
   "IMG_MP_COURTYARD, LEVELID_COURTYARD, -1, 1, 4" in front)
ck("Courtyard reuses the existing Caves MP memory-profile text",
   '{ LEVELID_COURTYARD,    "-ml0 -me0 -mgfx130 -mvtx100 -mt400 -ma300"}' in boss and
   boss.count('"-ml0 -me0 -mgfx130 -mvtx100 -mt400 -ma300"') >= 2)
ck("Courtyard title occupies dormant title slots only in modded builds",
   '#ifdef GE_MODDED_CHEATS\n "Courtyard", //TITLE_STR_176' in title and
   '"COURTYARD", //TITLE_STR_177' in title)
ck("Courtyard portrait enum/table appends after Citadel",
   re.search(r"IMG_MP_CITADEL,\s*IMG_MP_COURTYARD", oddh) is not None and
   "IMAGE_MP_COURTYARD" in oddc)
ck("Courtyard gets additive image ID after Citadel",
   "IMAGE(MP_COURTYARD, 0x3BC, HIT_DEFAULT, HIT_DEFAULT, 0, 0, 0, 0)" in images and
   images.find("IMAGE(MP_CITADEL") < images.find("IMAGE(MP_COURTYARD"))
ck("retail 2697 and additive Citadel image remain intact",
   "IMAGE(2697, 0x53D" in images and "IMAGE(MP_CITADEL, 0xBB4" in images)
ck("Courtyard resource symbols are exported",
   all(x in obsegh for x in ("bg_courtyard_all_p_seg", "Tbg_courtyard_all_p_stanZ", "Ump_setupcourtyardZ")))
ck("physical obseg order is Citadel -> Courtyard BG -> STAN -> setup -> end",
   obseg.find("obseg_file_Z setup, Ump_setupcatZ") <
   obseg.find(".global bg_courtyard_all_p_seg") <
   obseg.find(".global Tbg_courtyard_all_p_stanZ") <
   obseg.find(".global Ump_setupcourtyardZ") <
   obseg.find(".global ob__ob_end_seg"))
ck("Courtyard resource IDs append after Citadel",
   resids.find("MP_SETUPCAT,") <
   resids.find("BG_COURTYARD_ALL_P,") <
   resids.find("BG_COURTYARD_ALL_P_STAN,") <
   resids.find("MP_SETUPCOURTYARD,") <
   resids.find("OBENDSEG"))
ck("compact Courtyard resource lookup names match stage/setup loaders",
   '{BG_COURTYARD_ALL_P, "cb", &bg_courtyard_all_p_seg}' in restable and
   '{BG_COURTYARD_ALL_P_STAN, "cs", &Tbg_courtyard_all_p_stanZ}' in restable and
   '{MP_SETUPCOURTYARD, "Ump_cZ", &Ump_setupcourtyardZ}' in restable)
ck("Makefile tracks Courtyard incbin assets",
   all(x in make for x in (
       "assets/obseg/bg/bg_courtyard_all_p.bin",
       "assets/obseg/stan/Tbg_courtyard_all_p_stanZ.rz",
       "assets/obseg/setup/Ump_setupcourtyardZ.rz")))
ck("Courtyard audit is a build prerequisite",
   "mp-courtyard-r27x-audit" in make and
   re.search(r"^prerequisites:.*mp-courtyard-r27x-audit", make, re.M) is not None)

setup_rz = (ROOT / "assets/obseg/setup/Ump_setupcourtyardZ.rz").read_bytes()
stan_rz = (ROOT / "assets/obseg/stan/Tbg_courtyard_all_p_stanZ.rz").read_bytes()
bg_bin = (ROOT / "assets/obseg/bg/bg_courtyard_all_p.bin").read_bytes()
ck("setup/STAN preserve Rare 1172 compression magic",
   setup_rz[:2] == b"\x11\x72" and stan_rz[:2] == b"\x11\x72")
ck("Courtyard BG has recovered compact room table header",
   len(bg_bin) == 0x5680 and struct.unpack_from(">I", bg_bin, 4)[0] == 0x0F000014)

passed = sum(ok for _, ok in checks)
print(f"\nMP COURTYARD R27X AUDIT: {'PASS' if passed == len(checks) else 'FAIL'} ({passed}/{len(checks)})")
sys.exit(0 if passed == len(checks) else 1)
