#!/usr/bin/env python3
from pathlib import Path
import re, sys
R=Path(__file__).resolve().parents[1]
LM=(R/'src/game/levelmodifiers.c').read_text(errors='replace')
LH=(R/'src/game/levelmodifiers.h').read_text(errors='replace')
F=(R/'src/game/front.c').read_text(errors='replace')
O=(R/'src/game/options.c').read_text(errors='replace')
MP=(R/'src/game/mpmenu.c').read_text(errors='replace')
BG=(R/'src/game/bg.c').read_text(errors='replace')
BV=(R/'src/game/bondview_r.c').read_text(errors='replace')
ST=(R/'src/game/stan.c').read_text(errors='replace')
BC=(R/'src/bondconstants.h').read_text(errors='replace')
MK=(R/'Makefile').read_text(errors='replace')
checks=[]
def ck(name,cond):
    checks.append((name,bool(cond))); print(('[PASS] ' if cond else '[FAIL] ')+name)

ck('Level Modifiers has Single-Player/Multiplayer/Miscellaneous categories',
   all(x in LM for x in ['LEVELMOD_CATEGORY_SINGLE_PLAYER','LEVELMOD_CATEGORY_MULTIPLAYER','LEVELMOD_CATEGORY_MISCELLANEOUS']))
def names_in(array_name):
    m = re.search(r'static const char \*' + re.escape(array_name) + r'\[\]\s*=\s*\{(.*?)\};', LM, re.S)
    return re.findall(r'"([^"]+)"', m.group(1) if m else '')

ck('Single-Player list exactly matches requested 20-level order',
   names_in('g_LevelModifierSpNames') == ['Dam','Facility','Runway','Surface 1','Bunker 1','Silo','Frigate','Surface 2','Bunker 2','Statue','Archives','Streets','Depot','Train','Jungle','Control','Caverns','Cradle','Aztec','Egyptian'])
ck('Multiplayer list exactly matches requested six maps',
   names_in('g_LevelModifierMpNames') == ['Temple','Complex','Caves','Library','Basement','Stack'])
ck('Miscellaneous list exactly matches Cuba/Citadel order',
   names_in('g_LevelModifierMiscNames') == ['Cuba','Citadel'])
ck('only Silo is implemented initially',
   'category == LEVELMOD_CATEGORY_SINGLE_PLAYER && index == 5' in LM)
ck('unimplemented frontend levels are dimmed', '0x606060C0' in F and 'levelModifiersLevelImplemented' in F)
ck('frontend entry is directly below Map Maker', '"Map Maker"' in F and '"Level Modifiers"' in F and 'y = 103;' in F)
ck('frontend category browser and level browser menu IDs are appended',
   all(x in BC for x in ['MENU_LEVEL_MODIFIERS,','MENU_LEVEL_MODIFIERS_LEVELS,','MENU_LEVEL_MODIFIERS_DETAIL,']))
ck('SP Watch places Level Modifiers before In-Game Cheats',
   'else if (row == 22) { label = "Level Modifiers"; value = ">"; }' in O and
   'else { label = "In-Game Cheats"; value = ">"; }' in O)
ck('MP Watch hub places Level Modifiers before In-Game Cheats',
   '{"Options","Special Options","Level Modifiers","In-Game Cheats"}' in MP)
ck('in-game Silo option is ACTIVATE/ACTIVE, not a reversible toggle',
   '"ACTIVE" : "ACTIVATE"' in O and '"ACTIVE":"ACTIVATE"' in MP and
   'levelModifiersActivateSiloBetaVent' in O and 'levelModifiersActivateSiloBetaVent' in MP)
ck('modifier policy framework supports both latched and reversible future modifiers',
   'LEVELMOD_POLICY_LATCHED' in LH and 'LEVELMOD_POLICY_REVERSIBLE' in LH)
ck('Silo currently declares latched policy', 'return LEVELMOD_POLICY_LATCHED;' in LM)
ck('historical beta start uses semantic pad 230 override', 'return 230;' in LM and 'levelModifiersAdjustStartPadIndex' in BV)
ck('Silo STAN patch is stage-local and happens before STAN relocation/load',
   'levelModifiersOnStanLoaded(levelid, (u8 *)gptr_stan);' in BG and
   BG.find('levelModifiersOnStanLoaded(levelid') < BG.find('stanDetermineEOF((struct StanPrefixRecord *) gptr_stan'))
ck('in-game activation rebuilds STAN room metadata', 'stanRebuildRoomData();' in LM and 'void stanRebuildRoomData(void)' in ST)
# Validate the generated patch structure: 63 runs and 223 data bytes.
run_block=re.search(r'g_SiloBetaVentPatchRuns\[\]\s*=\s*\{(.*?)\};',LM,re.S)
data_block=re.search(r'g_SiloBetaVentPatchData\[\]\s*=\s*\{(.*?)\};',LM,re.S)
runs=re.findall(r'\{0x[0-9a-fA-F]+,\s*\d+,\s*\d+\}',run_block.group(1) if run_block else '')
vals=re.findall(r'0x[0-9a-fA-F]{2}',data_block.group(1) if data_block else '')
ck('dump-derived Silo STAN patch has all 63 changed runs', len(runs)==63)
ck('dump-derived Silo STAN patch carries all 223 changed bytes', len(vals)==223)
ck('V88 audit remains mandatory', 'fly-turbo-tp-y-v88-audit' in next((x for x in MK.splitlines() if x.startswith('prerequisites:')),''))
ck('V89 audit is mandatory', 'level-modifiers-v89-audit:' in MK and 'level-modifiers-v89-audit' in next((x for x in MK.splitlines() if x.startswith('prerequisites:')),''))

bad=[n for n,c in checks if not c]
print(); print(f"LEVEL MODIFIERS V89 AUDIT: {'PASS' if not bad else 'FAIL'} ({len(checks)-len(bad)}/{len(checks)})")
if bad:
    for n in bad: print(' - '+n)
    sys.exit(1)
