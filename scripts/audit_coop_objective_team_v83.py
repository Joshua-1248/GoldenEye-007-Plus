#!/usr/bin/env python3
from pathlib import Path
import sys

root = Path(__file__).resolve().parents[1]
bondinv = (root / 'src/game/bondinv.c').read_text()
bondinv_h = (root / 'src/game/bondinv.h').read_text()
objective = (root / 'src/game/objective_status.c').read_text()
cheat = (root / 'src/game/cheat.c').read_text()

checks = [
    ('team inventory helper exported',
     'bondinvCoopAnyPlayerHasPropInInv(PropRecord *prop);' in bondinv_h),
    ('team inventory helper implemented',
     'bool bondinvCoopAnyPlayerHasPropInInv(PropRecord *prop)' in bondinv),
    ('team helper examines every co-op player directly',
     'g_playerPointers[player]' in bondinv and 'for (player = 0; player < getPlayerCount(); player++)' in bondinv),
    ('team helper does not switch global current-player context',
     'set_cur_player' not in bondinv[bondinv.find('bool bondinvCoopAnyPlayerHasPropInInv'):bondinv.find('bool bondinvCoopAnyPlayerHasPropInInv') + 1200]),
    ('collect-object criterion uses team inventory in co-op',
     'case PROPDEF_OBJECTIVE_COLLECT_OBJECT:' in objective
     and 'hasprop = bondinvCoopAnyPlayerHasPropInInv(obj->prop);' in objective),
    ('deposit-object criterion uses same team inventory authority',
     objective.count('hasprop = bondinvCoopAnyPlayerHasPropInInv(obj->prop);') >= 2),
    ('team path is campaign co-op only',
     objective.count('get_scenario() == SCENARIO_COOP') >= 2
     and objective.count('gamemode == GAMEMODE_MULTI') >= 2),
    ('retail/single-player inventory path retained',
     objective.count('hasprop = bondinvHasPropInInv(obj->prop);') >= 2),
    ('All Objectives Complete remains global override',
     'if (get_debug_all_obj_complete_flag())' in objective
     and 'status = OBJECTIVESTATUS_COMPLETE;' in objective),
    ('in-game complete-objectives action refreshes shared objective cache immediately',
     'set_debug_all_obj_complete_flag(TRUE);' in cheat
     and 'display_objective_status_text_on_status_change();' in cheat),
]

passed = 0
for name, ok in checks:
    print(f"[{'PASS' if ok else 'FAIL'}] {name}")
    passed += bool(ok)

print(f"CO-OP OBJECTIVE TEAM V83 AUDIT: {'PASS' if passed == len(checks) else 'FAIL'} ({passed}/{len(checks)})")
sys.exit(0 if passed == len(checks) else 1)
