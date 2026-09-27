#!/usr/bin/env python3
from pathlib import Path
import sys

root = Path(__file__).resolve().parents[1]
chrprop = (root/'src/game/chrprop.c').read_text(errors='replace')
makefile = (root/'Makefile').read_text(errors='replace')

checks=[]
def check(name, cond):
    checks.append((name,bool(cond)))
    print(f"[{'PASS' if cond else 'FAIL'}] {name}")

needle = '''if (gamemode == GAMEMODE_MULTI\n                    && get_scenario() == SCENARIO_COOP\n                    && candidate_prop->type == PROP_TYPE_VIEWER)'''
check('Co-Op auto-aim rejects viewer/player candidates', needle in chrprop)
check('filter lives inside auto-aim target scan', chrprop.find(needle) > chrprop.find('void chrpropUpdateAutoaimTarget(void)'))
check('filter occurs before character weapon eligibility checks', chrprop.find(needle) < chrprop.find('Characters not holding a weapon are exempt from being a target.', chrprop.find('void chrpropUpdateAutoaimTarget(void)')))
check('filter occurs before render-bound scoring work', chrprop.find(needle) < chrprop.find('chrGetOnscreenRenderBounds(candidate_prop', chrprop.find('void chrpropUpdateAutoaimTarget(void)')))
check('filter only rejects viewer props, not enemy character props', 'candidate_prop->type == PROP_TYPE_VIEWER' in needle and 'PROP_TYPE_CHR' not in needle)
check('filter is Co-Op-only, preserving competitive multiplayer', 'get_scenario() == SCENARIO_COOP' in needle)
check('filter is multiplayer-scoped', 'gamemode == GAMEMODE_MULTI' in needle)
check('existing current-player exclusion remains intact', 'getPlayerPointerIndex(candidate_prop) == get_cur_playernum()' in chrprop)
prereq_line = makefile.split('prerequisites:', 1)[1].split('\n', 1)[0] if 'prerequisites:' in makefile else ''
check('V37 audit is mandatory build prerequisite',
      'coop-autoaim-v37-audit:' in makefile and
      'coop-autoaim-v37-audit' in prereq_line)

failed=[n for n,c in checks if not c]
print()
if failed:
    print(f"COOP AUTOAIM FRIENDLY FILTER V37 AUDIT: FAIL ({len(checks)-len(failed)}/{len(checks)})")
    for n in failed: print('  -', n)
    sys.exit(1)
print(f"COOP AUTOAIM FRIENDLY FILTER V37 AUDIT: PASS ({len(checks)}/{len(checks)})")
