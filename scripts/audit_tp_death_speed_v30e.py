#!/usr/bin/env python3
from pathlib import Path
import sys
ROOT=Path(__file__).resolve().parents[1]
bv=(ROOT/'src/game/bondview2.c').read_text(errors='replace')
checks=[]
def check(n,c):
    checks.append((n,bool(c))); print(('[PASS] ' if c else '[FAIL] ')+n)
check('V30D authoritative death animation remains', 'tpdeathanim = (s32)objecthandlerGetModelAnim((Model *)&ppointers[index]->model);' in bv)
check('V30D authoritative mirror remains', 'tpdeathflip = objecthandlerGetModelGunhand((Model *)&ppointers[index]->model);' in bv)
check('live TP death copies authoritative model speed', 'angle = modelGetAnimSpeed((Model *)&ppointers[index]->model);' in bv)
check('old doubled TP death speed expression is gone', 'angle = (g_ModGameplayOptions2 & MODOPT2_REALTIME_COLLAPSE) ? 1.0f : 0.5f;' not in bv)
check('retail head speed mapping remains untouched', 'bheadSetSpeed((gamemode != GAMEMODE_MULTI && (g_ModGameplayOptions2 & MODOPT2_REALTIME_COLLAPSE)) ? 1.0f : 0.5f);' in bv)
check('Stay In TP On Death gate remains', 'modStayInTpOnDeath(index)' in bv)
failed=[n for n,c in checks if not c]
print(f"\nTP DEATH SPEED V30E AUDIT: {'PASS' if not failed else 'FAIL'} ({len(checks)-len(failed)}/{len(checks)})")
sys.exit(1 if failed else 0)
