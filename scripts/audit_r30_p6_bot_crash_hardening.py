#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
BOT = (ROOT / 'src/game/mpbots.c').read_text()
BOTH = (ROOT / 'src/game/mpbots.h').read_text()
LV = (ROOT / 'src/game/lv.c').read_text()
MAKE = (ROOT / 'Makefile').read_text()
checks = []

def ck(name, cond):
    ok = bool(cond)
    checks.append(ok)
    print(('[PASS] ' if ok else '[FAIL] ') + name)

ck('P4 independent Simulant navigation remains authoritative',
   'plot_course_for_actor(runtime->chr' not in BOT
   and 'modMpBotsPdNavTick(slot, targetprop);' in BOT)
ck('P5 player animation remains active',
   'modMpBotsApplyPlayerAnimation(i);' in BOT
   and 'firing_animation_groups[group][sub]' in BOT)
ck('stage readiness uses authoritative player collision tile rather than viewer prop stan',
   'g_playerPointers[i]->field_488.current_tile_ptr == NULL' in BOT
   and 'g_playerPointers[i]->prop->stan == NULL' not in BOT)
ck('bot resources are explicitly prepared during stage load',
   'void modMpBotsPrepareStage(void)' in BOT
   and 'void modMpBotsPrepareStage(void);' in BOTH
   and 'modMpBotsPrepareStage();' in LV)
ck('runtime spawn requires stage-prepared resources',
   'u8 resourcesready;' in BOT
   and 'if (!runtime->resourcesready)' in BOT)
ck('physical cache exhaustion never falls back to live MEMPOOL_STAGE loading',
   'Never fall back to GoldenEye' in BOT
   and BOT.find('Never fall back to GoldenEye') < BOT.find('load_object_fill_header(header'))
ck('death teardown follows the natural GE corpse lifecycle',
   'u8 removequeued;' not in BOT
   and 'modMpBotsQueueActorRemoval' not in BOT
   and 'chr->hidden |= CHRHIDDEN_REMOVE;' not in BOT)
ck('mpbots no longer manually frees its ChrRecord prop',
   'chrpropCleanupForRemoval(prop);' not in BOT
   and 'chrpropDelist(prop);' not in BOT
   and 'chrpropDisable(prop);' not in BOT
   and 'chrpropFree(prop);' not in BOT)
ck('raw bot target pointers are invalidated before actor teardown',
   'modMpBotsInvalidateTargetProp' in BOT
   and 'g_ModMpBotRuntime[i].targetprop == victim' in BOT
   and 'modMpBotsInvalidateTargetProp(runtime->chr->prop);' in BOT)
ck('dead Simulants run no further AI while GE owns ACT_DIE/ACT_DEAD',
   'if (chrIsDead(runtime->chr))' in BOT
   and 'runtime->deathrecorded = TRUE;' in BOT
   and 'continue;' in BOT)
ck('respawn waits across a frame boundary after natural corpse cleanup',
   's32 finisheddeath = runtime->deathrecorded;' in BOT
   and 'runtime->respawntimer = 1;' in BOT)
ck('newly spawned bot still survives a normal chr tick before AI/pickups',
   'if (!runtime->spawnready)' in BOT
   and 'runtime->spawnready = TRUE;' in BOT)
ck('P3 pickup mutation remains behind the post-chr seam',
   BOT.find('runtime->spawnready = TRUE;') < BOT.rfind('modMpBotsPdCheckPickups(i);'))
ck('P6 audit is mandatory build prerequisite',
   'r30-p6-bot-crash-hardening-audit' in MAKE
   and 'python3 scripts/audit_r30_p6_bot_crash_hardening.py' in MAKE)

passed = sum(checks)
print(f'\nR30 P6 BOT CRASH HARDENING AUDIT: {"PASS" if passed == len(checks) else "FAIL"} ({passed}/{len(checks)})')
sys.exit(0 if passed == len(checks) else 1)
