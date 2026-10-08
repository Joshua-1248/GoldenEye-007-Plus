#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
BOT = (ROOT / 'src/game/mpbots.c').read_text()
CHR = (ROOT / 'src/game/chr.c').read_text()
HDR = (ROOT / 'src/game/chraction.h').read_text()
MAKE = (ROOT / 'Makefile').read_text()

checks=[]
def ck(name, cond):
    ok=bool(cond); checks.append(ok); print(('[PASS] ' if ok else '[FAIL] ')+name)

ck('Simulant controller no longer calls guard plot_course_for_actor', 'plot_course_for_actor(' not in BOT)
ck('Simulant movement no longer keys forward input from ACT_GOPOS', 'runtime->chr->actiontype == ACT_GOPOS' not in BOT)
ck('Simulant lateral motor no longer captures GE model yaw as route authority', 'runtime->roty = getsubroty(chr->model);' not in BOT)
ck('bot-owned STAN corridor exists', 'navtiles[MAX_CHRWAYPOINTS]' in BOT and 'StandTile *navgoalstan' in BOT)
ck('guard waypoint routing is fully absent', 'waypointFindRoute(' not in BOT and 'chrlvStanPathRelated(' not in BOT)
ck('native STAN adjacency/portal helpers are used', 'stanGetLinkedTileAtEdge' in BOT and 'stanGetEdgeMidPointWorld' in BOT)
ck('partial STAN corridor chunks rebuild safely', 'currenttile != runtime->navgoalstan' in BOT and 'modMpBotsPdNavBuildRoute(slot, targetprop)' in BOT)
ck('visible close target allows direct player-like travel', 'MODBOT_STAN_DIRECT_RANGE_SQ' in BOT and 'goal = targetprop->pos;' in BOT)
ck('STAN search is fixed-memory and bounded', 'MODBOT_STAN_SEARCH_MAX 352' in BOT and 'g_ModMpBotStanNodes[MODBOT_STAN_SEARCH_MAX]' in BOT and 'g_ModMpBotStanHeap[MODBOT_STAN_SEARCH_MAX]' in BOT)
ck('route cost prefers central comfortable travel', 'MODBOT_STAN_CLEARANCE_COST' in BOT and 'MODBOT_STAN_PORTAL_COST' in BOT and 'MODBOT_STAN_TURN_COST' in BOT and 'stanGetTileCenterClearanceWorld' in BOT)
ck('invented stuck-recovery state is absent', all(x not in BOT for x in (
    'navrecovery60', 'navrecoveryside', 'navstuck60', 'navprogresstimer60', 'navlastpos',
    'MODBOT_PD_NAV_STUCK_TICKS', 'MODBOT_PD_NAV_RECOVERY_TICKS',
    'MODBOT_PD_NAV_PROGRESS_TICKS', 'MODBOT_PD_NAV_PROGRESS_SQ')))
ck('active route movement follows PD forward-only motor contract',
   'runtime->speedmultforwards = 1.0f;' in BOT
   and 'runtime->speedmultsideways = 0.0f;' in BOT
   and 'runtime->speedmultsideways = runtime->navrecoveryside * 0.85f;' not in BOT)
ck('native chr collision seam remains authoritative', 'modMpBotsApplyLateral(chr, src, dst);' in CHR)
ck('combat guard brain remains absent', 'actor_fire_or_aim_at_target_update(runtime->chr' not in BOT and 'actor_aim_at_actor(runtime->chr' not in BOT)
ck('P3 inventory and pickup work remains present', 'modMpBotsPdCheckPickups(i);' in BOT and 'modMpBotsPdSwitchToWeapon(slot, bestweapon);' in BOT)
ck('P4 navigation audit is mandatory', 'r30-p4-bot-navigation-audit' in MAKE and 'audit_r30_p4_bot_navigation.py' in MAKE)

passed=sum(checks)
print(f'\nR30 P4 SIMULANT NAVIGATION AUDIT: {"PASS" if passed == len(checks) else "FAIL"} ({passed}/{len(checks)})')
sys.exit(0 if passed == len(checks) else 1)
