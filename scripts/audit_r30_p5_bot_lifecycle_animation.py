#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
BOT = (ROOT / 'src/game/mpbots.c').read_text()
LDS = (ROOT / 'ge007.ld').read_text()
MAKE = (ROOT / 'Makefile').read_text()
checks = []

def ck(name, cond):
    ok = bool(cond)
    checks.append(ok)
    print(('[PASS] ' if ok else '[FAIL] ') + name)

ck('P4 independent navigation remains authoritative',
   'plot_course_for_actor(runtime->chr' not in BOT
   and 'modMpBotsPdNavTick(slot, targetprop);' in BOT)
ck('spawn lifecycle waits for real MP world state',
   'modMpBotsStageReadyForSpawn' in BOT
   and 'g_CurrentSetup.pathwaypoints == NULL' not in BOT
   and 'g_playerPointers[i]->field_488.current_tile_ptr == NULL' in BOT)
ck('new bots must survive a chr tick before Simulant AI',
   'u8 spawnready;' in BOT
   and 'if (!runtime->spawnready)' in BOT
   and 'runtime->spawnready = TRUE;' in BOT)
ck('post-chr pickup mutation is behind spawnready',
   BOT.find('runtime->spawnready = TRUE;') < BOT.rfind('modMpBotsPdCheckPickups(i);'))
ck('spawn burst is limited to one bot per frame',
   'g_ModMpBotSpawnCursor' in BOT
   and 'for (n = 0; n < g_ModMpBotCount; n++)' in BOT
   and 'modMpBotsSpawn(slot);' in BOT)
ck('bot body/head loads have a dedicated high-memory cache',
   '_modMpBotModelCacheStart' in BOT and '_modMpBotModelCacheEnd' in BOT
   and 'load_object_fill_header(header' in BOT)
ck('model load checks compressed resource and cache headroom',
   'resource_lookup_data_array[filenum].rom_size' in BOT
   and 'MODBOT_MODEL_LOAD_MIN_FREE' in BOT
   and 'rombytes + MODBOT_MODEL_LOAD_MIN_FREE' in BOT
   and '+ MODBOT_MODEL_LOAD_ROM_GUARD >= remaining' in BOT)
ck('nonphysical path refuses unsafe stage-heap lazy load',
   'mempGetBankSizeLeft(MEMPOOL_STAGE)' in BOT)
ck('linker keeps bounded upper-RDRAM Simulant model storage after explosions',
   '_modMpBotModelCacheStart = ALIGN(_unlimitedExplosionPoolEnd, 16);' in LDS
   and '_modMpBotModelCacheEnd = _modMpBotTextureCacheStart;' in LDS
   and '(_modMpBotModelCacheEnd - _modMpBotModelCacheStart) >= 0x5E000' in LDS
   and '_modMpBotTextureCacheEnd <= 0x80800000' in LDS)
ck('spawn no longer installs guard face/reaim standing state',
   'check_set_actor_standing_still(chr, 0, 0);' in BOT
   and 'check_set_actor_standing_still(chr, 0x10' not in BOT)
ck('Simulants use GoldenEye multiplayer player animation tables',
   'modMpBotsApplyPlayerAnimation' in BOT
   and 'firing_animation_groups[group][sub]' in BOT
   and 'ptr_animation_table' in BOT)
ck('player animation classifies dual, unarmed, pistol and rifle states',
   'group = 3;' in BOT and 'group = 2;' in BOT
   and 'WEAPONSTATBITFLAG_ONLY_1_HANDED' in BOT
   and 'group = 0;' in BOT and 'group = 1;' in BOT)
ck('player animation covers standing, walking/running and strafing',
   'sub = 0;' in BOT and 'sub = 4;' in BOT and 'sub = 3;' in BOT
   and 'sub = amount >= 0.4f ? 2 : 1;' in BOT)
ck('bot firing tables drive authored upper-body shoulder aim',
   'chrlvUpdateAimendbackShoulders(chr, firingtable, group == 3, 1, pitch);' in BOT)
ck('P5 audit is mandatory build prerequisite',
   'r30-p5-bot-lifecycle-animation-audit' in MAKE
   and 'python3 scripts/audit_r30_p5_bot_lifecycle_animation.py' in MAKE)

passed = sum(checks)
print(f'\nR30 P5 BOT LIFECYCLE/PLAYER ANIMATION AUDIT: {"PASS" if passed == len(checks) else "FAIL"} ({passed}/{len(checks)})')
sys.exit(0 if passed == len(checks) else 1)
