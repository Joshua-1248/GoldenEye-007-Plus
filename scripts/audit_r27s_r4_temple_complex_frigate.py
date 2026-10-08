#!/usr/bin/env python3
from pathlib import Path
import sys
R=Path(__file__).resolve().parents[1]
L=(R/'src/game/levelmodifiers.c').read_text(errors='replace')
H=(R/'src/game/levelmodifiers.h').read_text(errors='replace')
F=(R/'src/game/front.c').read_text(errors='replace')
B=(R/'src/game/bondview2.c').read_text(errors='replace')
BR=(R/'src/game/bondview_r.c').read_text(errors='replace')
P=(R/'src/game/prop.c').read_text(errors='replace')
C=[]
def ck(n,x): C.append((n,bool(x))); print(('[PASS] ' if x else '[FAIL] ')+n)
ck('R27S R4 marker','R27S_R4_TEMPLE_COMPLEX_FRIGATE_MODIFIERS' in L)
ck('Temple exposes two reversible rows','LEVELID_TEMPLE) return 2;' in L and 'Fall Down Into Hole' in L and 'Better Respawning' in L)
ck('Complex exposes one reversible row','LEVELID_COMPLEX) return 1;' in L and 'Fall Down Into Holes' in L)
ck('Frigate exposes third row','LEVELID_FRIGATE) return 3;' in L and 'Clipping Change In Room With 3 Pipes' in L)
ck('Temple STAN offsets/values are exact',all(x in L for x in ['0x032a, enabled ? 0x02c0 : 0x0000','0x0b0a, enabled ? 0x0258 : 0x0000','0x0b4a, enabled ? 0x0258 : 0x0000','0x0b8a, enabled ? 0x0258 : 0x0000','0x0be2, enabled ? 0x0258 : 0x0000']))
ck('Complex STAN offsets/values are exact',all(x in L for x in ['0x0452, enabled ? 0x0398 : 0x0000','0x0492, enabled ? 0x0398 : 0x0000','0x058a, enabled ? 0x0398 : 0x0000','0x05b2, enabled ? 0x0398 : 0x0000','0x05ca, enabled ? 0x0398 : 0x0000','0x05ea, enabled ? 0x0398 : 0x0000','0x0602, enabled ? 0x0398 : 0x0000','0x1fc2, enabled ? 0x0470 : 0x0000','0x1ffa, enabled ? 0x0470 : 0x0000','0x2002, enabled ? 0x0470 : 0x0000']))
frig=['0xb994, enabled ? 0x0085 : 0x0095','0xb998, enabled ? 0xfb93 : 0xfb86','0xb9aa, enabled ? 0x1717 : 0x0000','0xb9b4, enabled ? 0x0085 : 0x0095','0xb9b8, enabled ? 0xfb93 : 0xfb86','0xba64, enabled ? 0x0085 : 0x0095','0xba68, enabled ? 0xfb93 : 0xfb86','0xba6a, enabled ? 0x173b : 0x0000','0xb862, enabled ? 0x173f : 0x0000','0xb97a, enabled ? 0x1758 : 0x0000','0xb964, enabled ? 0x0081 : 0x0091','0xb968, enabled ? 0xfbad : 0xfb9f','0xb962, enabled ? 0x1707 : 0x0000','0xba1c, enabled ? 0x0081 : 0x0091','0xba20, enabled ? 0xfbad : 0xfb9f','0xba22, enabled ? 0x172f : 0x0000','0xb7e2, enabled ? 0x1737 : 0x0000','0xb91a, enabled ? 0x174f : 0x0000']
ck('Frigate three-pipe STAN retail/on values are exact',all(x in L for x in frig))
ck('all live STAN toggles rebuild room metadata',L.count('stanRebuildRoomData();') >= 4)
ck('Temple Better Respawning swaps exact resolved pointer pairs',all(x in B for x in ['g_Startpad[1] = g_Startpad[2]','g_Startpad[2] = tmp','g_Startpad[3] = g_Startpad[4]','g_Startpad[4] = tmp']))
ck('Temple respawn preload is applied after intro startpads are rebuilt','levelModifiersOnStartPadsLoaded();' in BR and 'g_LevelModifierTempleBetterRespawningApplied = FALSE;' in L)
ck('historical fixed RDRAM addresses are not runtime writes',all(x not in (L+B+BR) for x in ['80189CEA','8018A4CA','8018CD42','801D5E64','80079C2F']))
ck('frontend description API is wired','levelModifiersGetFrontendModifierDescription' in H and 'levelModifiersGetFrontendModifierDescription(' in F)
ck('Zoinkity credit is visible when highlighted','Code by Zoinkity' in L and "credit[0] != '\\0'" in F)
ck('RSB credit is visible when highlighted','Code by RSB' in L and 'Better Respawning' in L)
ck('Temple/Complex are enabled in Multiplayer catalog','LEVELMOD_CATEGORY_MULTIPLAYER && (index == 0 || index == 1)' in L)
ck('R27S setup-table data reclaimed without semantic loss','R27S_R4_NO_TABLE_RODATA' in P and 'static const u16 to[]' not in P and 'static const u8 pi[]' not in P)
ck('R27R drivable speedboat source remains present','R27R_DRIVABLE_SPEEDBOAT' in L and 'propLevelModifierDamDrivableBoat' in P)
bad=[n for n,x in C if not x]
print(); print('R27S R4 TEMPLE/COMPLEX/FRIGATE AUDIT: '+('PASS' if not bad else 'FAIL')+f' ({len(C)-len(bad)}/{len(C)})')
if bad:
    for n in bad: print(' - '+n)
    sys.exit(1)
