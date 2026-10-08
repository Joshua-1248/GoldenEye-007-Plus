#!/usr/bin/env python3
from pathlib import Path
import re,sys
R=Path(__file__).resolve().parents[1]
LM=(R/'src/game/levelmodifiers.c').read_text(errors='replace')
O=(R/'src/game/options.c').read_text(errors='replace')
MP=(R/'src/game/mpmenu.c').read_text(errors='replace')
LV=(R/'src/game/lv.c').read_text(errors='replace')
RD=(R/'README.md').read_text(errors='replace')
C=(R/'tools/1172compress.sh').read_text(errors='replace')
A=[]
def ck(n,c): A.append((n,bool(c))); print(('[PASS] ' if c else '[FAIL] ')+n)
m=re.search(r'static\s+const\s+char\s+\*g_LevelModifierMpNames\[\]\s*=\s*\{(.*?)\};',LM,re.S)
names=re.findall(r'"([^"]+)"',m.group(1) if m else '')
ck('MP catalog appends seven requested entries',names[-7:]==['Facility (MP)','Bunker (MP)','Statue (MP)','Archives (MP)','Caverns (MP)','Cradle (MP)','Egyptian (MP)'])
ck('Cradle MP catalog index 11 is implemented','LEVELMOD_CATEGORY_MULTIPLAYER && index == 11' in LM)
ck('generic backend owns Kill Plane UI','return "Kill Plane";' in LM and '"ACTIVE" : "ACTIVATE"' in LM)
ck('SP Watch remains generic','levelModifiersToggleCurrentStageModifier(row)' in O and 'levelModifiersGetCurrentStageModifierName(row)' in O)
ck('MP Watch remains generic','levelModifiersToggleCurrentStageModifier(row)' in MP and 'levelModifiersGetCurrentStageModifierName(row)' in MP)
ck('competitive MP context excludes Co-op','gamemode == GAMEMODE_MULTI' in LM and 'get_scenario() != SCENARIO_COOP' in LM)
ck('Zoinkity threshold exact','signed 0xF60A == -2550' in LM and 'playerY < -2550.0f' in LM)
ck('movement tick invokes kill-plane test','levelModifiersCradleKillPlaneShouldKill(g_CurrentPlayer->prop->pos.y)' in LV)
ck('Zoinkity credited','Cradle (MP) Kill Plane Level Modifier' in RD and 'Zoinkity' in RD)
ck('R18 tuner preserved','numiterations=80' in C and 'blocksplittingmax=64' in C)
bad=[n for n,c in A if not c]
print()
print('R27Y R9 CURRENT-BACKEND AUDIT: %s (%d/%d)' % ('PASS' if not bad else 'FAIL',len(A)-len(bad),len(A)))
if bad:
    [print(' - '+n) for n in bad]
    sys.exit(1)
