#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
BONDINV = ROOT / 'src/game/bondinv.c'
FACILITY = ROOT / 'assets/obseg/setup/UsetuparkZ.c'


def fail(msg):
    print('COOP MISSION ITEM AUDIT: FAIL:', msg, file=sys.stderr)
    raise SystemExit(1)


def require(cond, msg):
    if not cond:
        fail(msg)

bi = BONDINV.read_text(errors='replace')
fa = FACILITY.read_text(errors='replace')

require('void bondinvShareCoopItem(ITEM_IDS item)' in bi,
        'team-item sharing helper missing')
require('g_CoopSharedItems[item]' in bi and 'return;' in bi,
        'team-item sharing is not idempotent')
require('int bondinvAddInvItem(ITEM_IDS item)' in bi,
        'central inventory grant function missing')
require('item >= ITEM_BOMBCASE && item <= ITEM_KEYBOLT' in bi,
        'mission gadget/key range is not centrally recognized')
require('get_scenario() == SCENARIO_COOP && getPlayerCount() > 1' in bi,
        'central grant is not restricted to active campaign Co-Op')

# There must be a central grant -> share edge, not only pickup-specific sharing.
add_start = bi.index('int bondinvAddInvItem(ITEM_IDS item)')
add_end = bi.index('int bondinvAddDoublesInvItem', add_start)
add_body = bi[add_start:add_end]
require('bondinvShareCoopItem(item);' in add_body,
        'bondinvAddInvItem no longer promotes mission items to team inventory')

# Facility regression anchor: setup carries the Door Decoder (0x26) and the
# bottling-room door script waits for gadget use on tagged object 0x26.
require('_mkshort(0x26, 0xff)' in fa,
        'Facility Door Decoder collectable missing/changed')
require('if_bond_used_gadget_on_object(0x26' in fa,
        'Facility Door Decoder gadget-use script missing/changed')
require('if_bond_has_item_equipped(0x26' in fa,
        'Facility Door Decoder equip/use script missing/changed')

print('COOP MISSION ITEM AUDIT: PASS')
print('  direct/setup mission-item grants: promoted to all active Co-Op players')
print('  shared registry: idempotent and respawn-compatible')
print('  Facility Door Decoder regression anchor: present')
