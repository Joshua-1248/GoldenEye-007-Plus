#!/usr/bin/env python3
from pathlib import Path
import sys
R=Path(__file__).resolve().parents[1]; L=(R/'src/game/levelmodifiers.c').read_text(errors='replace'); P=(R/'src/game/prop.c').read_text(errors='replace')
C=[]
def ck(n,x): C.append((n,bool(x))); print(('[PASS] ' if x else '[FAIL] ')+n)
ck('R27S R2 marker','R27S_R2_MULTI_LEVEL_BETA_MODIFIERS' in L)
ck('Facility count 3','LEVELID_FACILITY)\n        return 3;' in L)
ck('Surface count 2','LEVELID_SURFACE) return 2;' in L)
ck('Bunker 1 count 1','LEVELID_BUNKER1) return 1;' in L)
ck('Frigate count 3','LEVELID_FRIGATE) return 3;' in L)
for s in ['Restore Beta Facility Doors','Restore Beta Facility Stacked Tanks','White Sky','Beta Doors','Restore Removed Doors']: ck('label '+s,s in L)
ck('label "Keep Clear" Doors Leading Into Engine Room', r'\"Keep Clear\" Doors Leading Into Engine Room' in L)
ck('White Sky semantic RGB','environment->Red   = enabled ? 0xc0 : 0x60;' in L and 'environment->Blue  = enabled ? 0xc0 : 0x80;' in L)
ck('Facility door retail/on bytes','lmPatchByteRun(base,0x0149,9,0x100,0x9f,0x8f,facilityDoors)' in P and 'lmPatchByteRun(base,0x0a49,16,0x100,0x9b,0x9a,facilityDoors)' in P)
ck('Surface door retail/on','lmPatchHalfRun(base,0x5148,8,0x100,0x00a6,0x00a7,surfaceDoors)' in P)
ck('Bunker door retail/on','lmPatchHalfRun(base,0x35a8,8,0x100,0x008a,0x0089,bunkerDoors)' in P and 'lmPatchByteRun(base,0x3da8,2,0x100,0x00,0x87,bunkerDoors)' in P)
ck('Frigate Keep Clear retail/on','lmPatchHalfRun(base,0x4fd8,2,0x200,0x0098,0x0099,frigateKeepClear)' in P)
ck('Frigate removed doors use symbolic pads',all(('g_CurrentSetup.pads[%d].stan' % i) in P for i in [70,71,74,85,86,89]) and 'gptr_stan' in P)
ck('No historical fixed setup RAM writes','*(u8 *)0x801' not in P and '*(u16 *)0x801' not in P)
bad=[n for n,x in C if not x]; print(); print('R27S R2 BETA MODIFIERS AUDIT: '+('PASS' if not bad else 'FAIL')+f' ({len(C)-len(bad)}/{len(C)})'); sys.exit(1 if bad else 0)
