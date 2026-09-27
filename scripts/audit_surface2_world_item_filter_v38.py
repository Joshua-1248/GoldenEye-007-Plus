#!/usr/bin/env python3
from pathlib import Path
import sys
p=Path('src/game/propobj.c')
s=p.read_text()
checks=[
 ('player ownership helper exists', 'static bool weaponPropOwnedByPlayer(PropRecord *prop)' in s),
 ('walks full parent chain', 'prop = prop->parent;' in s),
 ('viewer roots rejected', 'prop->type == PROP_TYPE_VIEWER' in s),
 ('player roots rejected', 'prop->type == PROP_TYPE_PLAYER' in s),
 ('weaponFindThrown uses helper', 'if (weaponPropOwnedByPlayer(prop))' in s),
 ('generic recursive collectable search retained', 'obj = check_if_entry_is_collectable(KeyID, prop);' in s),
 ('projectile/stationary semantics retained', 'RUNTIMEBITFLAG_HASPROJECTILE' in s),
 ('no Surface II stage-specific conditional', 'LEVELID_SEVERNAYA' not in s and 'Surface 2' not in s[s.find('weaponPropOwnedByPlayer'):s.find('void add_obj_to_temp_proxmine_table')]),
]
failed=0
print('SURFACE II WORLD ITEM FILTER V38 AUDIT')
print('='*37)
for name,ok in checks:
    print(('[ OK ] ' if ok else '[FAIL] ')+name)
    failed += not ok
print()
if failed:
    print(f'RESULT: FAIL ({failed} error(s))')
    sys.exit(1)
