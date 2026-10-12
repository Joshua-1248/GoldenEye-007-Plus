#!/usr/bin/env python3
"""R30 P26: source-structure and two-ended allocator invariant checks.

This is not a substitute for a MIPS build or N64 runtime/memory testing.
"""
from pathlib import Path
import re
import sys

root = Path(__file__).resolve().parents[1]
ld = (root / 'ge007.ld').read_text()
bot = (root / 'src/game/mpbots.c').read_text()
image = (root / 'src/game/image.c').read_text()
header = (root / 'src/game/image.h').read_text()
errors = []

def check(name, condition):
    print(f"[{'PASS' if condition else 'FAIL'}] {name}")
    if not condition:
        errors.append(name)

check('one adjacent stage resource arena',
      '_modMpBotTextureCacheStart = _modMpBotModelCacheStart;' in ld
      and '_modMpBotModelCacheEnd = _modMpBotDecodeScratchStart;' in ld)
check('12 KiB staging scratch isolated at top of expansion memory',
      '_modMpBotDecodeScratchStart = _modMpBotTextureCacheEnd - 0x3000;' in ld
      and '_modMpBotTextureCacheEnd = 0x80800000;' in ld
      and 'BOT_DECODE_CAPACITY 0x3000' in image)
check('model low cursor and texture high cursor used for loader capacity',
      'loadend = (u8 *)g_ModMpBotTexturePool.rightpos - MODBOT_MODEL_LOAD_MIN_FREE;' in bot
      and 'remaining = (s32)(loadend - cursor);' in bot)
check('temporary compressed input/GDL region protected during loads',
      'texSetBotDynamicFloor(loadend);' in bot
      and 'newhigh < (u32)g_BotDynamicFloor' in image)
check('texture chunks grow from the high end without a fixed split',
      'newhigh = oldhigh - allocbytes;' in image
      and 'pool->rightpos = newtex;' in image
      and 'newtex->next = pool->rightpos == pool->end ? 0' in image)
check('linked-list lookup deduplicates live textures',
      'if (arg1 == g_BotDynamicPool)' in image
      and 'cur = (struct tex *)((u32)cur->next | 0x80000000U);' in image)
check('pixel decompression guarded before writes',
      image.count('g_BotScratchActive && !texBotScratchHasRoom(') == 2
      and 'g_BotScratchOverflow' in image)
check('rollback on incomplete model load',
      'oldtexhead = g_ModMpBotTexturePool.rightpos;' in bot
      and 'g_ModMpBotTexturePool.rightpos = oldtexhead;' in bot
      and 'header->RootNode = NULL;' in bot)
check('arena bindings remain bounded and stage-owned',
      bot.count('texBindBotDynamicPool(&g_ModMpBotTexturePool);') == 1
      and 'modMpBotsBindResourceArena(_modMpBotModelCacheStart,' in bot
      and 'modMpBotsBindResourceArena(_coopDynBuffersStart, _coopDynBuffersEnd);' in bot
      and 'texSetBotDynamicFloor(g_ModMpBotModelCacheCursor);' in bot)
check('nonphysical compatibility path preserved',
      '#else\n    /* Non-physical compatibility path' in bot)
check('C header exposes new APIs only in the physical build',
      '#if defined(GE_MODDED_CHEATS) && defined(GE_PHYSICAL_CODE) && defined(GE_PHYSICAL_FASTPATHS)' in header
      and 'void texBindBotDynamicPool(struct texpool *pool);' in header)
check('retail stage texture pool fallback unchanged',
      'if (isstagepool && g_StageTexOverflowReady' in image
      and '*updateword = osVirtualToPhysical(pool->start);' in image)

# Simulate the ownership arithmetic using the current linker-layout snapshot.
# Models and texture payloads can interleave over stage setup, but each load
# keeps 64 KiB transiently between the model loader and committed textures.
START, END = 0x80791840, 0x807FD000
GUARD = 0x10000

def scenario(models, textures, abort_at=None):
    lo, hi = START, END
    for j, (model, tex) in enumerate(zip(models, textures)):
        ceiling = hi - GUARD
        if lo + model >= ceiling:
            return False
        saved = hi
        if hi - tex < ceiling:
            return False
        hi -= tex
        if abort_at == j:
            hi = saved
        else:
            lo = (lo + model + 15) & ~15
        assert lo < hi <= END
    return True

check('simulation: eight distinct moderate-sized resource sets',
      scenario([0x4000] * 8, [0x1800] * 8))
check('simulation: rollback does not consume committed space',
      scenario([0x4000] * 8, [0x1800] * 8, abort_at=3))
check('simulation: a collision is rejected instead of overwritten',
      not scenario([0xa000] * 8, [0x3000] * 8))

# P26 R2: the competitive-only reserve is already protected in ge007.ld.
# The same mode predicate is used in dynInitMemory for Co-Op GFX ownership.
dyn = (root / 'src/game/dyn.c').read_text()
check('competitive arena borrows exact linker-reserved Co-Op GFX window',
      '_coopDynBuffersStart = 0x804A0000;' in ld
      and '_coopDynBuffersEnd = 0x8056C000;' in ld
      and 'g_ModMpBotBorrowedCoopArena = get_scenario() != SCENARIO_COOP;' in bot
      and 'modMpBotsBindResourceArena(_coopDynBuffersStart, _coopDynBuffersEnd);' in bot)
check('four-player Co-Op graphics ownership remains exclusive',
      'gamemode == GAMEMODE_MULTI && get_scenario() == SCENARIO_COOP' in dyn
      and 'g_GfxBuffers[0] = base;' in dyn
      and 'g_VtxBuffers[2] = g_VtxBuffers[1] + vtxsize;' in dyn
      and 'modMpBotsBindResourceArena(_modMpBotModelCacheStart,' in bot)
check('competitive arena overflow retains existing asset pointers',
      'if (g_ModMpBotBorrowedCoopArena)' in bot
      and 'g_ModMpBotBorrowedCoopArena = FALSE;' in bot
      and 'return modMpBotsTryCharacterResources(body, head);' in bot
      and 'header->RootNode != NULL' in bot)
check('larger competitive arena can accommodate eight distinct resources',
      (0x8056c000-0x804a0000) == 0xcc000
      and 8 * (0x9000 + 0x2800) + 2 * GUARD < 0xcc000)

# P26 R5: 4-human Co-Op must NEVER borrow live graphics or weapon buffers.
# Its extra arena is a *bounded stage-heap allocation*, created on demand
# after player/stage initialization.  The 128 KiB stage-heap reserve is
# mandatory, so a crowded mission may decline the extra arena safely.
check('Co-Op fallback never aliases dedicated four-player graphics buffers',
      'if (get_scenario() != SCENARIO_COOP)' in bot
      and 'modMpBotsBindResourceArena(_coopDynBuffersStart, _coopDynBuffersEnd);' in bot
      and 'if (get_scenario() != SCENARIO_COOP)' in bot)
check('Co-Op reserve uses ordinary bounded stage heap rather than a reserved graphics cache',
      'stage = &g_mempPools[MEMPOOL_STAGE];' in bot
      and 'base = mempAllocBytesInBank((u32)arenasize, MEMPOOL_STAGE);' in bot
      and 'modMpBotsBindResourceArena(base, base + arenasize);' in bot)
check('Co-Op fallback is late, one-shot, and stage scoped',
      'if (g_ModMpBotCoopStageArenaTried)' in bot
      and 'g_ModMpBotCoopStageArenaTried = TRUE;' in bot
      and 'g_ModMpBotCoopStageArenaTried = FALSE;' in bot
      and bot.count('modMpBotsTryBindCoopStageArena()') == 1
      and bot.count('modMpBotsTryBindCoopStageArena(void)') == 1)
check('stage arena never eats last 128 KiB or exceeds 512 KiB',
      'if (freebytes <= 0x40000)' in bot
      and 'arenasize = (freebytes - 0x20000) & ~0xf;' in bot
      and 'if (arenasize > 0x80000)' in bot
      and 'arenasize = 0x80000;' in bot
      and 'if (arenasize < 0x20000)' in bot)
check('guarded stage heap pointer and positive range required before allocation',
      'if (stage->pos == NULL || stage->end <= stage->pos)' in bot
      and 'freebytes = (s32)(stage->end - stage->pos);' in bot)
check('Co-Op fallback preserves primary resource pointers through arena switching',
      '&& modMpBotsTryBindCoopStageArena())' in bot
      and 'return modMpBotsTryCharacterResources(body, head);' in bot
      and 'if (header->RootNode != NULL)' in bot)

def coop_tail_arena(free):
    if free <= 0x40000:
        return 0
    size = (free - 0x20000) & ~15
    return min(size, 0x80000) if size >= 0x20000 else 0

check('simulation: small stage tail is refused; larger tails retain reserve',
      coop_tail_arena(0x20000) == 0
      and coop_tail_arena(0x40000) == 0
      and coop_tail_arena(0x50000) == 0x30000
      and coop_tail_arena(0xA0000) == 0x80000
      and all(free - coop_tail_arena(free) >= 0x20000
              for free in range(0x20000, 0x110000, 0x4000)))

# P26 R7: dynInitMemory() already sizes both Co-Op GFX and VTX double
# buffers by actual human count. Only the untouched tail past VTX[2]
# can be lent to bot models/textures, stage-scoped and without copies.
check('Co-Op bot arena starts after the live double-buffered VTX end',
      'g_GfxBuffers[0] == _coopDynBuffersStart' in bot
      and 'g_VtxBuffers[2] > _coopDynBuffersStart' in bot
      and 'g_VtxBuffers[2] < _coopDynBuffersEnd' in bot
      and 'modMpBotsBindResourceArena(g_VtxBuffers[2], _coopDynBuffersEnd);' in bot)
check('Co-Op slack requires loader staging capacity before binding',
      '(u32)(_coopDynBuffersEnd - g_VtxBuffers[2]) >' in bot
      and 'MODBOT_MODEL_LOAD_MIN_FREE * 2 + MODBOT_MODEL_LOAD_ROM_GUARD' in bot)
check('Co-Op slack overflow switches to existing upper-resident arena',
      'g_ModMpBotBorrowedCoopArena || g_ModMpBotCoopSlackArena' in bot
      and 'g_ModMpBotCoopSlackArena = FALSE;' in bot
      and 'modMpBotsBindResourceArena(_modMpBotModelCacheStart,' in bot)
check('one to three human Co-Op never consumes its stage heap for bot overflow',
      'get_scenario() == SCENARIO_COOP && getPlayerCount() >= 4' in bot
      and bot.count('modMpBotsTryBindCoopStageArena()') == 1)

# The actual dyn.c sizes are (GFX+VTX) * 2 per viewport.
# Four-person Co-Op gets no slack, and the human allocation is untouched.
GFX = [0x10000, 0x20000, 0x2C000, 0x3E000]
VTX = [0x10000, 0x18000, 0x20000, 0x28000]
slack = [0xCC000 - 2 * (gfx + vtx) for gfx, vtx in zip(GFX, VTX)]
check('simulation: exact unused Co-Op gfx tail for 1-4 humans',
      slack == [0x8C000, 0x5C000, 0x34000, 0]
      and all(n >= 0 for n in slack))
check('actual dyn buffers still independently reserve every human viewport',
      'g_VtxBuffers[2] = g_VtxBuffers[1] + vtxsize;' in dyn
      and 'g_GfxBuffers[0] = base;' in dyn)

print(f'\nR30 P26 R7 CO-OP SLACK ARENA AUDIT: {"FAIL" if errors else "PASS"} ({32-len(errors)}/32)')
sys.exit(bool(errors))
