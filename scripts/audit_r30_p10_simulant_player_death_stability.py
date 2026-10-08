#!/usr/bin/env python3
from pathlib import Path
import sys

R = Path(__file__).resolve().parents[1]
A = (R / 'src/game/chraction.c').read_text()
B = (R / 'src/game/mpbots.c').read_text()
C = (R / 'src/game/chr_b.c').read_text()
P = (R / 'src/game/prop.c').read_text()
L = (R / 'src/game/lv.c').read_text()
checks = []

def ck(name, cond):
    checks.append(bool(cond))
    print(('[PASS] ' if cond else '[FAIL] ') + name)

bot_branch = '''if (hitpart != HIT_HAT
            && modMpBotsGetSlotForChr(self) >= 0 && damageToCause > 0.0f)'''
helper_start = A.find('static void chrlvStartSimulantPlayerDeath')
helper_end = A.find('#endif', helper_start)
helper = A[helper_start:helper_end] if helper_start >= 0 and helper_end >= 0 else ''
bullet_start = A.find(bot_branch)
bullet_end = A.find('        else\n#endif', bullet_start)
bullet = A[bullet_start:bullet_end] if bullet_start >= 0 and bullet_end >= 0 else ''
expl_start = A.find('s32 chrlvExplosionDamage')
expl_end = A.find('/**', expl_start + 10)
expl = A[expl_start:expl_end] if expl_start >= 0 and expl_end >= 0 else ''

ck('Simulants have a dedicated human-player death starter',
   'chrlvStartSimulantPlayerDeath' in helper)
ck('player death starter uses Bond multiplayer death animation pool',
   'g_bondviewBondDeathAnimations' in helper and 'g_bondviewBondDeathAnimationsCount' in helper)
ck('player death starter matches human 0.5 speed and 12-frame merge',
   'randomGetNext() & 1, 0.0f, 0.5f, 12.0f' in helper)
ck('player death starter has no guard body-part death selection',
   'g_HitReactionTable' not in helper and 'death_neck' not in helper)
ck('ordinary bot gunshots still accumulate ChrRecord damage',
   'self->damage += damageToCause;' in A and bullet_start > A.find('self->damage += damageToCause;'))
ck('nonlethal bot gunshots bypass guard ACT_ARGH/PREARGH',
   bot_branch in A and 'self->actiontype = ACT_ARGH' not in bullet and 'self->actiontype = ACT_PREARGH' not in bullet)
ck('lethal bot gunshots enter player-style death without triggered_on_shot_hit',
   'self->damage >= self->maxdamage' in bullet
   and 'chrlvStartSimulantPlayerDeath(self);' in bullet
   and 'triggered_on_shot_hit' not in bullet)
ck('bot explosions preserve impulse but bypass guard explosion animation table',
   'self->fallspeed.f[0] = sp2C.f[0];' in expl
   and 'chrlvStartSimulantPlayerDeath(self);' in expl
   and expl.find('chrlvStartSimulantPlayerDeath(self);') < expl.find('explosion_animation_table[sp40]'))
ck('bot corpse lifecycle remains natural GE ACT_DIE/ACT_DEAD/fade/free',
   'ACT_DIE -> ACT_DEAD -> corpse fade -> TICKOP_FREE' in B)
ck('only Cheap trait receives a starting gun',
   'if (config->traits & MODBOT_TRAIT_CHEAP)' in B
   and B.find('if (config->traits & MODBOT_TRAIT_CHEAP)') < B.find('weaponindex = modMpBotsChooseWeaponIndex(slot);', B.find('if (config->traits & MODBOT_TRAIT_CHEAP)')))
ck('bot cache reset does not add stale-header ownership machinery',
   'modMpBotsInvalidateCachedModelHeaders' not in B and 'g_ModMpBotModelCacheCursor = _modMpBotModelCacheStart;' in B)
ck('bot animated-model slots are primed during level-reset allocation without live chr spawn',
   'Model *primedmodels[MOD_MP_BOT_MAX];' in B
   and 'retrieve_header_for_body_and_head(body, head, 0);' in B
   and 'clear_aircraft_model_obj(primedmodels[i]);' in B
   and 'modMpBotsSpawn(i);' not in B[B.find('void modMpBotsPrepareStage'):B.find('s32 modMpBotsGetSlotForChr')])
ck('level reset reserves one exact animated model slot per bot',
   'numAnimatedObjects += modMpBotsGetCount();' in P)
ck('initial bot instantiation happens after frontend cheat state is restored',
   L.find('cheatApplyFrontendSelectionsForStage();') < L.find('modMpBotsPrepareStage();'))
ck('failed body+head instantiation always restores temporary numRecords',
   'if (addedHeadRecords)\n        bodyHeader->numRecords -= headHeader->numRecords;' in C
   and C.find('bodyHeader->numRecords -= headHeader->numRecords;') < C.find('if (model != 0)', C.find('bodyHeader->numRecords -= headHeader->numRecords;')))
ck('bot pickups run retail object cleanup before prop free/regeneration',
   'objFree(obj, 0, obj->state & RUNTIMEBITFLAG_REMOVE);' in B
   and 'modMpBotsPdConsumePickupProp(prop);' in B)
ck('navigation never dereferences waypoint pad IDs under STAN backend',
   'step->padID' not in B and 'waypointFindRoute(' not in B and 'stanGetLinkedTileAtEdge' in B)

passed = sum(checks)
print(f'\nR30 P10 SIMULANT PLAYER-DEATH/STABILITY AUDIT: {"PASS" if passed == len(checks) else "FAIL"} ({passed}/{len(checks)})')
sys.exit(0 if passed == len(checks) else 1)
