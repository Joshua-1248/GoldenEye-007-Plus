#!/usr/bin/env python3
"""Offline GoldenEye Plus Dam Island Fixed Collisions integrity audit.

Verifies the source-side 1172 resource, tile topology, level-load hook,
menu entry, and append-only resource registration before any ROM build.
The input ROM is never required or redistributed by this audit.
"""
from pathlib import Path
import hashlib
import struct
import zlib
from collections import Counter

ROOT = Path(__file__).resolve().parent.parent
PATH = ROOT / 'assets/obseg/stan/Tbg_dam_island_stanZ.rz'
EXPECTED_SHA256 = '4ff9d5fe080d030e8ec79014709e16aeb8e2c49357b27a59cf939e0db01efc6f'
SIZE_TABLE = [32, 32, 32, 32] + [8 + 8 * n for n in range(4, 16)]


def check(condition, description):
    if not condition:
        raise AssertionError(description)
    print('[PASS]', description)


def main():
    rz = PATH.read_bytes()
    check(rz[:2] == b'\x11\x72', 'island asset uses native 0x1172 format')
    check(hashlib.sha256(rz).hexdigest() == EXPECTED_SHA256, 'resource matches verified island STAN hash')
    dec = zlib.decompressobj(-15)
    data = dec.decompress(rz[2:]) + dec.flush()
    check(dec.eof and not dec.unused_data and not dec.unconsumed_tail,
          'single valid full raw-deflate stream')
    check(len(data) == 91312, 'dual-state STAN uncompressed size is 91312')
    check(struct.unpack_from('>II', data) == (0, 32) and
          struct.unpack_from('>I', data, 8)[0] == 0x11e38 + 2272,
          'STAN region pointer table keeps relocated runtime pointers')

    off = 32
    rooms = Counter()
    tile_offsets = set()
    link_refs = []
    identifiers = set()
    island_tiles = {}
    while True:
        word, mid, tail = struct.unpack_from('>IHH', data, off)
        if word == 0:
            break
        n = tail >> 12
        check_n = 3 <= n <= 15 and off + SIZE_TABLE[n] <= len(data)
        if not check_n:
            raise AssertionError('tile size/count invalid at offset 0x%x' % off)
        room, identifier = word & 255, word >> 8
        if (room, identifier) in identifiers:
            raise AssertionError('duplicate tile identifier within room')
        identifiers.add((room, identifier))
        if room == 82:
            island_tiles[identifier] = [struct.unpack_from('>hhhH', data, off + 8 + i * 8) for i in range(n)]
        rooms[room] += 1
        tile_offsets.add(off)
        for i in range(n):
            ref = struct.unpack_from('>H', data, off + 8 + i * 8 + 6)[0]
            if ref != 0:
                link_refs.append((32 - 128) + 8 * ref)
        off += SIZE_TABLE[n]

    check(sum(rooms.values()) == 2826, 'dual STAN has 2826 valid polygon records')
    check(rooms[82] == 139, 'island room 82 contains 139 collision tiles')
    check(rooms[81] == 10, 'ten dormant retail room 81 triangles retained for off state')
    check(max(rooms) <= 138, 'room IDs fit the 139-room STAN engine tables')
    check(len(link_refs) == 6904 and all(x in tile_offsets for x in link_refs),
          'all 6904 hybrid tile links resolve to valid tile starts')
    check(data[off:off + 8] == bytes(8) and b'unstric\x00' in data[off:],
          'STAN terminator and trailing sentinel are intact')
    check(0xEF900 in island_tiles and 0xEF946 in island_tiles,
          'donor island floor and upper path polygons are present')

    bg = (ROOT / 'src/game/bg.c').read_text()
    mod = (ROOT / 'src/game/levelmodifiers.c').read_text()
    ids = (ROOT / 'assets/obseg/file_resource_id_enums.h').read_text()
    table = (ROOT / 'assets/obseg/file_resource_table.inc.c').read_text()
    asm = (ROOT / 'assets/obseg/ob_seg.s').read_text()
    header = (ROOT / 'src/game/levelmodifiers.h').read_text()
    check('levelModifiersDamIslandShouldLoad(levelid)' in bg and
          'fileIndexLoadToBank(BG_DAM_ISLAND_FIXED_STAN, 2, 0, 4)' in bg and
          '_fileNameLoadToBank(BG_DAM_ISLAND_FIXED_STAN' not in bg and
          '#include "ob.h"' in bg,
          'Dam loads numeric island resource ID through index API, not filename API')
    check('return g_LevelModifierDamRuntimeAvailable ? 4 : 0;' in mod and
          'Dam Island Fixed Collisions' in mod,
          'Dam modifier menu exposes its fourth entry')
    check('BG_DAM_ISLAND_FIXED_STAN,' in ids and
          '{BG_DAM_ISLAND_FIXED_STAN, "di", &Tbg_dam_island_stanZ}' in table and
          '.incbin "assets/obseg/stan/Tbg_dam_island_stanZ.rz"' in asm,
          'resource ID, lookup, physical segment ordering are registered')
    stan = (ROOT / 'src/game/stan.c').read_text()
    check('LEVELMOD_POLICY_LATCHED' in mod and
          'levelModifiersGetDamIslandPolicy' in header and
          'g_LevelModifierDamIslandActive || g_LevelModifierDamIslandPreload' in mod and
          'levelModifiersSetDamIslandPreload(TRUE);' in mod and
          'levelModifiersDamIslandApplyRepairedTile((u8 *)gptr_stan);' in mod and
          'levelModifiersDamIslandTileDisabled(tile)' in stan and
          'stanRebuildRoomData();' in mod and
          'stanResetHits();' in mod and
          'return g_LevelModifierDamIslandActive ? "ACTIVE" : "ACTIVATE";' in mod,
          'island activation updates geometry live, latches ACTIVE and refreshes STAN caches')
    print('DAM ISLAND FIXED COLLISIONS AUDIT: PASS')


if __name__ == '__main__':
    main()
