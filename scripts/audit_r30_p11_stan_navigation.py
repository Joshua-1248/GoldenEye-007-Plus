#!/usr/bin/env python3
from pathlib import Path
import sys

R = Path(__file__).resolve().parents[1]
B = (R / 'src/game/mpbots.c').read_text()
S = (R / 'src/game/stan.c').read_text()
H = (R / 'src/game/stan.h').read_text()
M = (R / 'Makefile').read_text()
checks = []

def ck(name, cond):
    checks.append(bool(cond))
    print(('[PASS] ' if cond else '[FAIL] ') + name)

ck('Simulant navigation no longer consumes authored guard waypoints',
   'waypointFindRoute(' not in B and 'chrlvStanPathRelated(' not in B and 'navwaypoints' not in B)
ck('route corridor is made from native STAN polygons',
   'StandTile *navtiles[MAX_CHRWAYPOINTS]' in B and 'StandTile *navgoalstan' in B)
ck('STAN adjacency comes from the existing encoded polygon links',
   'stanGetLinkedTileAtEdge' in B and 'link = tile->points[edgeIndex].link;' in S and '(link << 3)' in S)
ck('STAN helper API is mirror/scale aware and shared with bot navigation',
   all(x in H for x in ('stanGetLinkedTileAtEdge', 'stanGetEdgeMidPointWorld', 'stanGetTileCenterClearanceWorld'))
   and 'stanMirrorPointX' in S and 'inv_level_scale' in S)
ck('A* workspace is fixed-size and shared rather than allocated per bot',
   'MODBOT_STAN_SEARCH_MAX 352' in B
   and 'g_ModMpBotStanNodes[MODBOT_STAN_SEARCH_MAX]' in B
   and 'g_ModMpBotStanHeap[MODBOT_STAN_SEARCH_MAX]' in B
   and 'g_ModMpBotStanHash[MODBOT_STAN_HASH_SIZE]' in B)
ck('A* uses an admissible geometric goal heuristic',
   'g_ModMpBotStanNodes[start].f = modMpBotsStanDistance(starttile, goaltile);' in B
   and 'heuristic = modMpBotsStanDistance(nexttile, goaltile);' in B)
ck('route cost intentionally prefers comfortable central travel',
   all(x in B for x in ('MODBOT_STAN_CLEARANCE_GOAL', 'MODBOT_STAN_PORTAL_GOAL',
                         'MODBOT_STAN_CLEARANCE_COST', 'MODBOT_STAN_PORTAL_COST',
                         'MODBOT_STAN_TURN_COST'))
   and 'stanGetTileCenterClearanceWorld(next)' in B)
ck('steering aims through portal centres and pulls inward to next tile centre',
   'goal.x = portal.x * 0.60f + center.x * 0.40f;' in B
   and 'goal.z = portal.z * 0.60f + center.z * 0.40f;' in B)
ck('long-distance visible targets do not bypass the corridor',
   'MODBOT_STAN_DIRECT_RANGE_SQ' in B
   and 'currenttile == targetprop->stan' in B
   and '<= MODBOT_STAN_DIRECT_RANGE_SQ' in B)
ck('corridor rebuild is event-driven instead of a periodic frame spike',
   'MODBOT_PD_NAV_REPATH_TICKS' not in B
   and 'g_GlobalTimer % participants' not in B
   and 'runtime->navgoalstan != targetprop->stan' in B)
ck('truncated fixed route chunks rebuild from the live STAN',
   'if (currenttile != runtime->navgoalstan)' in B
   and B.count('modMpBotsPdNavBuildRoute(slot, targetprop)') >= 3)
ck('door handling remains on the existing GE door primitives',
   'sub_GAME_7F0B1410' in B and 'doorsChooseSwingDirection' in B and 'doorActivate' in B)
ck('no invented stuck-strafe recovery was reintroduced',
   all(x not in B for x in ('navrecoveryside', 'navrecovery60', 'navstuck60', 'MODBOT_PD_NAV_STUCK_TICKS')))
ck('existing GE character collision motor remains authoritative',
   'runtime->speedmultforwards = 1.0f;' in B and 'runtime->speedmultsideways = 0.0f;' in B)
ck('P11 audit is mandatory',
   'r30-p11-stan-navigation-audit' in M and 'audit_r30_p11_stan_navigation.py' in M)

passed = sum(checks)
print(f'\nR30 P11 STAN NAVIGATION AUDIT: {"PASS" if passed == len(checks) else "FAIL"} ({passed}/{len(checks)})')
sys.exit(0 if passed == len(checks) else 1)
