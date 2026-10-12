#!/usr/bin/env python3
"""P26 R4: check that 4 humans + 8 live bots + corpse overlap fit.

Static inspection and slot arithmetic, not an emulator or hardware run.
"""
from pathlib import Path
import re
import sys

r = Path(__file__).resolve().parents[1]
init = (r / 'src/game/initguards.c').read_text()
prop = (r / 'src/game/prop.c').read_text()
bot = (r / 'src/game/mpbots.c').read_text()
chrfile = (r / 'src/game/chraction.c').read_text()
model = (r / 'src/game/model.c').read_text()
pool = (r / 'src/game/chrprop.c').read_text()
header = (r / 'src/game/mpbots.h').read_text()
makefile = (r / 'Makefile').read_text()
checks = []

def check(label, ok):
    checks.append(bool(ok))
    print(f"[{'PASS' if ok else 'FAIL'}] {label}")

check('retail ten spare ChrRecords preserved',
      'g_NumChrSlots = count + 10;' in init)
check('simulants reserve a live and a corpse ChrRecord each',
      re.search(r'if\s*\(gamemode == GAMEMODE_MULTI\)\s*'
                r'g_NumChrSlots \+= modMpBotsGetCount\(\) \* 2;', init) is not None)
alloc = 'g_NumChrSlots += modMpBotsGetCount() * 2;'
check('bot ChrRecord capacity is sized before stage allocation',
      alloc in init and 'g_ChrSlots = mempAllocBytesInBank(' in init and
      init.find(alloc) < init.find('g_ChrSlots = mempAllocBytesInBank('))
check('Co-Op target-cache sidecar follows the expanded ChrRecord count',
      'chrCoopAllocTargetCache(g_NumChrSlots);' in init)
check('multiplayer Co-Op and competitive stages use the same predicate',
      'if (gamemode == GAMEMODE_MULTI)' in init and
      'if (gamemode == GAMEMODE_MULTI)' in prop)
check('spawn keeps its final-two-record safety reserve',
      'if (chrGetNumFree() >= 3)' in chrfile)
check('full original 600-PropRecord pool and free list unchanged',
      'PropRecord g_Props[MAX_PROPS];' in pool and
      'prop = g_FreeProps;' in pool)
check('bot animated model slots include corpse overlap',
      'numAnimatedObjects += modMpBotsGetCount() * 2;' in prop)
check('stage-time priming holds sixteen distinct model slots',
      'Model *primedmodels[MOD_MP_BOT_MAX * 2];' in bot and
      bot.count('model = retrieve_header_for_body_and_head(body, head, 0);') == 2)
check('all primed instances release only after both passes',
      'primedmodels[primedcount++] = model;' in bot and
      bot.count('clear_aircraft_model_obj(primedmodels[i]);') == 1 and
      bot.find('clear_aircraft_model_obj(primedmodels[i]);') >
      bot.rfind('model = retrieve_header_for_body_and_head(body, head, 0);'))
check('model allocation assigns RWdata during level reset',
      'if (g_ModelIsLvResetting)' in model and
      'rwdatas = mempAllocBytesInBank(' in model)
check('configured maximum remains eight bots', '#define MOD_MP_BOT_MAX 8' in header)
check('R4 actor-capacity check is a mandatory P5 audit dependency',
      'r30-p5-bot-lifecycle-animation-audit: r30-p26-pd-dynamic-resource-audit r30-p26-r4-actor-capacity-audit' in makefile)

for humans in range(1, 5):
    for bots in range(1, 9):
        # All programmed guards consume their stage-specific base allocation.
        # Conservatively count one live ChrRecord and one unfreed corpse per bot.
        free = (10 + 2 * bots) - humans - bots - bots
        check(f'simulated {humans}-human / {bots}-bot + {bots}-corpse free-record floor', free >= 3)

passed = sum(checks)
print(f'\nR30 P26 R4 ACTOR CAPACITY AUDIT: {"PASS" if passed == len(checks) else "FAIL"} ({passed}/{len(checks)})')
sys.exit(0 if passed == len(checks) else 1)
