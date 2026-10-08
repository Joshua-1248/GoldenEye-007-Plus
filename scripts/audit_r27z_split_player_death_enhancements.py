#!/usr/bin/env python3
from pathlib import Path
import sys
R=Path(__file__).resolve().parents[1]
O=(R/"src/game/options.c").read_text(errors="replace")
F2=(R/"src/game/file2.c").read_text(errors="replace")
SP=(R/"src/game/spectrum.c").read_text(errors="replace")
FR=(R/"src/game/front.c").read_text(errors="replace")
BC=(R/"src/bondconstants.h").read_text(errors="replace")
BV=(R/"src/game/bondview2.c").read_text(errors="replace")
MP=(R/"src/game/mpmenu.c").read_text(errors="replace")
CA=(R/"src/game/chraction.c").read_text(errors="replace")
PO=(R/"src/game/propobj.c").read_text(errors="replace")
A=[]
def ck(n,c):
    A.append((n,bool(c)))
    print(("[PASS] " if c else "[FAIL] ")+n)

ck("main frontend parent is one line",
   '"Additional Death Animations For Player >"' in FR
   and '"Additional Death Animations\\nFor Player >"' not in FR)

ck("main frontend parent uses normal one-row spacing",
   FR.count("s32 second_y = first_y + row_pitch;") >= 2
   and "first_y + wrapped_h" not in FR)

ck("main frontend child title is one line and not double-rendered",
   '"Additional Death Animations For Player"' in FR
   and '"Additional Death Animations\\nFor Player"' not in FR
   and 'y=title_y+line_h' not in FR)

ck("SP Watch parent and child render For Player as a second physical line",
   'label = "Additional Death Animations";' in O
   and 'draw_watch_mod_text(gdl,0x40,drawy + 0x12,"For Player >"' in O
   and 'draw_watch_mod_text(gdl,0xa0,0x1b,"ADDITIONAL DEATH ANIMATIONS"' in O
   and 'draw_watch_mod_text(gdl,0xa0,0x2d,"FOR PLAYER"' in O)

ck("frontend death menu uses live crosshair hit testing",
   "cursor_h_pos >= 53.0f && cursor_h_pos <= 390.0f" in FR)
ck("frontend death submenu exists","MENU_ADDITIONAL_DEATH_ANIMATIONS" in BC and "interface_menu_additional_death_animations" in FR)
enh_case = FR.rfind("case MENU_ENHANCEMENTS_OPTIONS:")
death_case = FR.find("case MENU_ADDITIONAL_DEATH_ANIMATIONS:", enh_case)
enh_ctor = FR.find("constructor_menu_enhancements_options(DL);", enh_case)
death_ctor = FR.find("constructor_menu_additional_death_animations(DL);", death_case)
ck("frontend Enhancements and death submenu constructors dispatch separately",
   enh_case >= 0
   and death_case > enh_case
   and enh_ctor > enh_case and enh_ctor < death_case
   and death_ctor > death_case)
ck("child names exact","Facility Gas Leak Death" in SP and "Staggering Backwards Death" in SP)
ck("stagger persists independently","g_ModStaggeringBackwardsDeathEnabled" in O and "GE_SRAM_EXT_TAIL_ENH_STAGGERING_BACKWARDS_DEATH 0x20" in F2)
ck("Facility gas keeps existing 0x01 setting","g_ModAdditionalPlayerDeathAnimationsEnabled" in PO)
ck("wall stagger uses split setting","g_ModStaggeringBackwardsDeathEnabled" in CA and "g_ModAdditionalPlayerDeathAnimationsEnabled" not in CA)
ck("SP nested submenu exists","#define MODWATCH_DEATH_SUBMENU_FLAG 0x800" in O and "g_ModStaggeringBackwardsDeathEnabled" in O)
ck("SP death submenu preserves nested mode while moving with D-pad/stick",
   O.count("MODWATCH_STATE = mode | deathsubmenu | row;") >= 2)
ck("MP nested submenu exists","if (mode == 10) return 2;" in MP and "g_ModStaggeringBackwardsDeathEnabled" in MP)
ck("Facility gas Death Neck stays shortened in death replay","R27Z R2: preserve shortened Facility gas Death Neck in death replay" in BV and "modelSetAnimEndFrame(g_CurrentPlayer->bodyModel, 241.0f);" in BV)
ck("first-person wall stagger remains exact BHEAD clip with natural end",
   "PTR_ANIM_death_stagger_back_to_wall" in BV
   and "g_CurrentPlayer->startnewbonddie - MOD_PLAYER_DEATH_WALL_FLIP0" in BV
   and "modelSetAnimEndFrame(&g_CurrentPlayer->model, 67.0f);" not in BV)
ck("no substitute BHEAD wall-death clip is installed",
   "R27Z R3: FP-safe Staggering Backwards Death" not in BV)
bad=[n for n,c in A if not c]
print(); print("R27Z R4 SPLIT PLAYER DEATH ENHANCEMENTS AUDIT: %s (%d/%d)" % ("PASS" if not bad else "FAIL",len(A)-len(bad),len(A)))
if bad:
    [print(" - "+n) for n in bad]
    sys.exit(1)
