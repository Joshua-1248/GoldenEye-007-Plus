#!/usr/bin/env python3
from pathlib import Path
import sys
ROOT = Path(__file__).resolve().parents[1]
bv = (ROOT/'src/game/bondview2.c').read_text(errors='replace')
mk = (ROOT/'Makefile').read_text(errors='replace')
checks=[]
def check(name, cond):
    checks.append((name,bool(cond)))
    print(('[PASS] ' if cond else '[FAIL] ') + name)
check('SP Stay-In-TP death is explicitly gated', 'gamemode != GAMEMODE_MULTI' in bv and 'modStayInTpOnDeath(index)' in bv)
check('live TP death uses authoritative player-model animation', 'tpdeathanim = (s32)objecthandlerGetModelAnim((Model *)&ppointers[index]->model);' in bv)
check('authoritative animation is verified against Bond death animation table', 'tpdeathanim == (g_bondviewBondDeathAnimations[i] + (s32)ptr_animation_table)' in bv)
check('live TP death copies authoritative mirror/flip', 'tpdeathflip = objecthandlerGetModelGunhand((Model *)&ppointers[index]->model);' in bv)
check('live TP body model applies copied flip', 'tpdeathflip,' in bv)
check('live TP death playback speed remains synchronized to authoritative SP model', 'angle = modelGetAnimSpeed((Model *)&ppointers[index]->model);' in bv and 'MODOPT2_REALTIME_COLLAPSE) ? 1.0f : 0.5f' not in bv)
check('retail/MP fallback random death path remains present', 'g_bondviewBondDeathAnimations[randomGetNext() % g_bondviewBondDeathAnimationsCount]' in bv)
check('death-camera replay still copies authoritative player animation', 'objecthandlerGetModelAnim((Model *) &g_CurrentPlayer->model)' in bv and 'objecthandlerGetModelGunhand(&g_CurrentPlayer->model)' in bv)
check('V30D audit is mandatory build prerequisite', 'tp-death-animation-v30d-audit' in mk and 'audit_tp_death_animation_match_v30d.py' in mk)
failed=[n for n,ok in checks if not ok]
print(f"\nTP DEATH ANIMATION MATCH V30D AUDIT: {'PASS' if not failed else 'FAIL'} ({len(checks)-len(failed)}/{len(checks)})")
sys.exit(1 if failed else 0)
