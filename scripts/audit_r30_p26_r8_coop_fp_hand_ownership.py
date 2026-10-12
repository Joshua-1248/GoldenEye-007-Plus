#!/usr/bin/env python3
"""P26 R8: verify stage-heap relief without losing either human's weapon buffers.

These checks verify ownership and layout invariants, not emulator behavior.
"""
from pathlib import Path
import re
import sys

root = Path(__file__).resolve().parents[1]
init = (root / 'src/game/initBondDATA.c').read_text()
front = (root / 'src/game/dyn.c').read_text()
ld = (root / 'ge007.ld').read_text()
gun = (root / 'src/game/gun.c').read_text()
make = (root / 'Makefile').read_text()
bots = (root / 'src/game/mpbots.c').read_text()
checks = []

def check(label, condition):
    checks.append(bool(condition))
    print(f"[{'PASS' if condition else 'FAIL'}] {label}")

check('co-op hand relocation is restricted to physical fastpaths with mod cheats',
      '#if defined(GE_MODDED_CHEATS) && defined(GE_PHYSICAL_CODE) && defined(GE_PHYSICAL_FASTPATHS)' in init)
check('two-human scenario is the only relocated configuration',
      'gamemode == GAMEMODE_MULTI && get_scenario() == SCENARIO_COOP' in init
      and 'getPlayerCount() == 2 && get_cur_playernum() >= 0' in init
      and 'get_cur_playernum() < 2' in init)
check('two-player right-hand buffers use slots two and three only',
      'u32 slot = (u32)get_cur_playernum() + 2U;' in init
      and '_mpLeftWeaponBuffersStart + slot * stride;' in init)
check('two-player left-hand buffers keep slots zero and one',
      '_mpLeftWeaponBuffersStart + (get_cur_playernum() * size_item_buffer[1]);' in init)
check('right-hand slots use the actual runtime stride',
      'u32 stride = size_item_buffer[1];' in init
      and 'size_item_buffer[0] <= stride' in init)
check('right-hand slots are bounded by the linker reservation',
      'extern u8 _mpLeftWeaponBuffersEnd[];' in init
      and '(u32)(_mpLeftWeaponBuffersEnd - _mpLeftWeaponBuffersStart)' in init
      and '>= (slot + 1U) * stride' in init)
check('out-of-bounds reservation falls back to retail stage heap',
      re.search(r'if \(size_item_buffer\[0\] <= stride.*?\n\s*else\n\s*g_CurrentPlayer->ptr_hand_weapon_buffer\[GUNRIGHT\] =\n\s*mempAllocBytesInBank\(size_item_buffer\[0\], MEMPOOL_STAGE\);', init, re.S) is not None)
check('ordinary one-player and three/four-player stage allocation retained',
      'else\n#endif\n    {\n        g_CurrentPlayer->ptr_hand_weapon_buffer[GUNRIGHT] =\n            mempAllocBytesInBank(size_item_buffer[0], MEMPOOL_STAGE);' in init)
check('single-human original left hand remains stage owned',
      'if (getPlayerCount() == 1)' in init and
      'g_CurrentPlayer->ptr_hand_weapon_buffer[GUNLEFT] = mempAllocBytesInBank(size_item_buffer[1], MEMPOOL_STAGE);' in init)
check('both right and left hand buffer pointers remain populated separately',
      'ptr_hand_weapon_buffer[GUNRIGHT]' in init and 'ptr_hand_weapon_buffer[GUNLEFT]' in init)
check('dedicated four-slot physical reserve retains full four buffers',
      '(_mpLeftWeaponBuffersEnd - _mpLeftWeaponBuffersStart) >= (0x14820 * 4)' in ld)
check('exact gun hand-buffer sizes match linker slots',
      'size_item_buffer[] = {0x14820, 0x14820}' in gun)
check('co-op dynamic render reservation is not altered',
      'g_GfxBuffers[0] = base;' in front and
      'g_VtxBuffers[2] = g_VtxBuffers[1] + vtxsize;' in front and
      'modMpBotsBindResourceArena(g_VtxBuffers[2], _coopDynBuffersEnd);' in bots)
check('no change to model and texture resource pipeline is required',
      'modMpBotsTryCharacterResources(body, head)' in bots and
      'texBindBotDynamicPool(&g_ModMpBotTexturePool);' in bots)
check('audit is required by the main P26 resource-build prerequisite',
      'r30-p26-pd-dynamic-resource-audit: r30-p26-r8-coop-fp-hand-audit' in make)

start = 0x8056c000
end = 0x805be080
stride = 0x14820
left = [start + p * stride for p in (0, 1)]
right = [start + (p + 2) * stride for p in (0, 1)]
check('simulated two humans get four fully distinct hand model buffers',
      len(set(left + right)) == 4)
check('simulated two-human hand allocations exactly fill the dedicated reserve',
      start + 4 * stride == end and all(start <= p < end for p in left+right))
check('simulated no live hand buffers overlap',
      all(abs(a-b) >= stride for i, a in enumerate(left+right)
          for b in (left+right)[i+1:]))
check('simulated two-human stage-heap saving is 168000 bytes',
      2 * stride == 168000)
check('original 8 bot maximum is still present',
      '#define MOD_MP_BOT_MAX 8' in (root/'src/game/mpbots.h').read_text())

passed = sum(checks)
print(f"\nR30 P26 R8 TWO-HUMAN CO-OP HAND OWNERSHIP AUDIT: {'PASS' if passed==len(checks) else 'FAIL'} ({passed}/{len(checks)})")
sys.exit(passed != len(checks))
