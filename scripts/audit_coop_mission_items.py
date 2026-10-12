#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
BONDINV = ROOT / 'src/game/bondinv.c'
FACILITY = ROOT / 'assets/obseg/setup/UsetuparkZ.c'
FRIGATE = ROOT / 'assets/obseg/setup/u/UsetupdestZ.c'
CHRAI = ROOT / 'src/game/chrai.c'
GUNFIRE = ROOT / 'src/game/gunfire.c'


def fail(msg):
    print('COOP MISSION ITEM AUDIT: FAIL:', msg, file=sys.stderr)
    raise SystemExit(1)


def require(cond, msg):
    if not cond:
        fail(msg)

bi = BONDINV.read_text(errors='replace')
fa = FACILITY.read_text(errors='replace')
fr = FRIGATE.read_text(errors='replace')
ai = CHRAI.read_text(errors='replace')
gun = GUNFIRE.read_text(errors='replace')

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

# R12: Facility and Frigate both test gadget-on-tag followed by item equipped.
# In Co-Op the latter must refer to the actual initiating player/item.
require('if_bond_used_gadget_on_object(0x01' in fr
        and 'if_bond_has_item_equipped(0x27' in fr,
        'Frigate Bomb Defuser script regression anchor missing')
require('bondinvCoopRecordGadgetActivation(temp_v0_8->obj, var_s1);' in gun,
        'Z gadget activation does not record the initiating weapon')
require('coopUsedGadgetItem = bondinvCoopTakeGadgetActivation(obj);' in ai
        and 'equipped = coopUsedGadgetItem == ai->ITEM_NUM;' in ai,
        'AI gadget condition does not validate the actual gadget used')
require('p->hands[GUNRIGHT].weaponnum == ai->ITEM_NUM' in ai
        and 'p->hands[GUNLEFT].weaponnum == ai->ITEM_NUM' in ai,
        'script item-equipped condition ignores other living Co-Op humans')
require('bondinvCoopAnyPlayerHasPropInInv(obj->prop)' in ai,
        'Co-Op script object-collection test remains single-player-scoped')
addprop_start = bi.index('int bondinvAddPropToInv(PropRecord *prop)')
addprop_end = bi.index('int bondinvAddWeaponByProp', addprop_start)
require('bondinvShareCoopProp(prop);' in bi[addprop_start:addprop_end]
        and 'g_CoopSharingProp' in bi[addprop_start:addprop_end],
        'central mission prop grant no longer shares guarded team inventory')
require('g_CoopGadgetObjects[i] == obj' in bi
        and 'bzero(g_CoopGadgetObjects, sizeof(g_CoopGadgetObjects))' in bi,
        'script gadget attribution is not object-specific or stage-reset')

print('COOP MISSION ITEM R12 AUDIT: PASS (Facility + Frigate + team keyring)')

# R12 R1 checks the recovery path actually visits *every* player inventory,
# scans any inventory that missed a one-time grant, and reinserts into each.
LV = ROOT / 'src/game/lv.c'
lv = LV.read_text(errors='replace')
reconcile = bi.split('void bondinvCoopReconcileTeamInventory(void)', 1)[1].split('static s32 bondinvHandIsReloading', 1)[0]
require('bondinvCoopReconcileTeamInventory();' in lv
        and 'g_ClockTimer > 0' in lv,
        'Co-Op reconciliation does not run during multiplayer gameplay')
require('for (i = 0; i < count; i++)' in reconcile
        and 'g_playerPointers[i]' in reconcile
        and 'entry->type == INV_ITEM_PROP' in reconcile
        and 'entry->type == INV_ITEM_WEAPON' in reconcile,
        'reconciliation does not collect missing team props and gadgets')
require('!bondinvHasPropInInv(g_CoopSharedProps[j])' in reconcile
        and 'bondinvAddPropToInv(g_CoopSharedProps[j]);' in reconcile
        and 'bondinvAddInvItem(j);' in reconcile,
        'reconciliation does not give each player real inventory entries')
require('g_CoopSharingProp = TRUE;' in reconcile
        and 'set_cur_player(savedplayer);' in reconcile,
        'reconciliation may recursively share items or leave wrong current player')
print('COOP MISSION ITEM R12 R1 RECONCILIATION AUDIT: PASS')

# R12 R2: the inventory module does not declare the lv-local gamemode
# variable. The caller in lv.c already verifies GAMEMODE_MULTI, and this
# helper independently guards SCENARIO_COOP. Catch the compile regression.
require('gamemode' not in reconcile,
        'Co-Op inventory reconciliation references undeclared gamemode')
require('if (get_scenario() != SCENARIO_COOP' in reconcile
        and 'gamemode == GAMEMODE_MULTI' in lv,
        'Co-Op inventory reconciliation lost gameplay/scenario gating')
print('COOP MISSION ITEM R12 R2 BUILD-SCOPE AUDIT: PASS')

# R12 R3: The original bug was NOT just missing inventory copies. During
# prop setup, every PROPDEF_RENAME record was registered under the final Bond,
# so P1-P3 could have key/decoder props but couldn't resolve their usable IDs.
PROP = ROOT / 'src/game/prop.c'
prop = PROP.read_text(errors='replace')
require('for (i8 = 0; i8 < getPlayerCount(); i8++)' in prop
        and 'set_cur_player(i8);' in prop
        and 'bondinvAddTextOverride((struct textoverride *) pdef_ren);' in prop,
        'setup-stage last-player text-override ownership path changed')
head = bi.split('static textoverride *bondinvGetTextOverrideHead(void)', 1)[1].split('textoverride *bondinvGetTextbyObj', 1)[0]
require('g_CoopMissionTextOverrides != NULL' in head
        and 'get_scenario() == SCENARIO_COOP' in head,
        'Co-Op mission text override lookup is still viewport-local')
for funcname, stop in [('bondinvGetTextbyObj(ObjectRecord *obj)', 'bondinvGetTextbyWeaponID'),
                       ('bondinvGetTextbyWeaponID(ITEM_IDS weaponnum)', 'bondinvGetTextbyInvIndex')]:
    func = bi.split(funcname, 1)[1].split(stop, 1)[0]
    require('bondinvGetTextOverrideHead();' in func,
            funcname + ' bypasses the Co-Op stage rename list')
reg = bi.split('void bondinvAddTextOverride(textoverride *override)', 1)[1].split('int bondinvGetCurEquippedItem', 1)[0]
require('g_CoopMissionTextOverrides = override;' in reg,
        'setup PROPDEF_RENAME entries never enter Co-Op shared metadata')
reset = bi.split('void bondinvResetCoopSharedProps(void)', 1)[1].split('void bondinvShareCoopItem', 1)[0]
require('g_CoopMissionTextOverrides = NULL;' in reset,
        'Co-Op rename list could retain a pointer into the unloaded stage')
cycle = bi.split('s32 bondinvCycleMissionItem(void)', 2)[-1].split('bool bondinvCheckHasKeyFlags', 1)[0]
require('bondinvGetTextbyInvIndex(index)' in cycle
        and 'override = bondinvGetTextbyObj(prop->obj);' in bi,
        'A+B still cannot resolve shared inventory props through their rename entries')
print('COOP MISSION ITEM R12 R3 SHARED RENAME/SELECTOR AUDIT: PASS')
