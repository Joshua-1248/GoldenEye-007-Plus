#include <ultra64.h>
#include "bondview.h"
#include "chr.h"
#include "player.h"
#include "textrelated.h"
#include <bondconstants.h>
#include "language.h"
#include "bondinv.h"
#include "gun.h"
#include "lv.h"
#include <bondtypes.h>

#ifdef GE_MODDED_CHEATS
static s16 g_MissionItemQueuedItem[4];
static s16 g_MissionItemQueuedIndex[4];
static u8 g_MissionItemQueueValid[4];

/* Campaign Co-Op mission inventory is team-owned.  Keep prop references in
 * stage lifetime storage so a death/respawn inventory rebuild cannot lose a
 * key, blueprint or other progression-critical prop. */
#define COOP_SHARED_PROP_MAX 64
static PropRecord *g_CoopSharedProps[COOP_SHARED_PROP_MAX];
static u8 g_CoopSharedItems[ITEM_IDS_MAX];
static s32 g_CoopSharedPropCount;

void bondinvResetCoopSharedProps(void)
{
    g_CoopSharedPropCount = 0;
    bzero(g_CoopSharedItems, sizeof(g_CoopSharedItems));
}

void bondinvShareCoopItem(ITEM_IDS item)
{
    s32 savedplayer = get_cur_playernum();
    s32 i;

    g_CoopSharedItems[item] = TRUE;

    for (i = 0; i < getPlayerCount(); i++)
    {
        if (i != savedplayer)
        {
            set_cur_player(i);
            bondinvAddInvItem(item);
        }
    }

    set_cur_player(savedplayer);
}

void bondinvShareCoopProp(PropRecord *prop)
{
    s32 savedplayer = get_cur_playernum();
    s32 i;

    /* A TICKOP_GIVETOPLAYER mission prop is detached on pickup, so the same
     * physical prop cannot normally be registered twice. */
    if (g_CoopSharedPropCount < COOP_SHARED_PROP_MAX)
    {
        g_CoopSharedProps[g_CoopSharedPropCount++] = prop;
    }

    for (i = 0; i < getPlayerCount(); i++)
    {
        if (i != savedplayer)
        {
            set_cur_player(i);
            bondinvAddPropToInv(prop);
        }
    }

    set_cur_player(savedplayer);
}

static s32 bondinvHandIsReloading(enum GUNHAND hand)
{
    s32 state = g_CurrentPlayer->hands[hand].weapon_action_state;

    return state >= GUN_ANIM_STATE_RELOAD_START
        && state <= GUN_ANIM_STATE_RELOAD_RAISE;
}

static s32 bondinvHandNeedsAutomaticReload(enum GUNHAND hand)
{
    ITEM_IDS item = getCurrentPlayerWeaponId(hand);

    if (item <= ITEM_UNARMED || item >= ITEM_BOMBCASE)
    {
        return FALSE;
    }

    return g_CurrentPlayer->hands[hand].weapon_ammo_in_magazine == 0
        && get_ammo_in_hands_weapon(hand) > 0;
}

static s32 bondinvHandIsSwitching(enum GUNHAND hand)
{
    s32 state = g_CurrentPlayer->hands[hand].weapon_action_state;

    return state == GUN_ANIM_STATE_SWITCH_LOWER
        || state == GUN_ANIM_STATE_SWITCH_SWAP
        || state == GUN_ANIM_STATE_SWITCH_HOLD
        || state == GUN_ANIM_STATE_SWITCH_RAISE;
}
#endif

void bondinvReinitInv(void)
{
    s32 i;

    for (i = 0; i < g_CurrentPlayer->equipmaxitems; i++)
    {
        g_CurrentPlayer->p_itemcur[i].type = -1;
    }

    g_CurrentPlayer->ptr_inventory_first_in_cycle = NULL;
    g_CurrentPlayer->textoverrides                = NULL;
    g_CurrentPlayer->equipcuritem                 = ITEM_UNARMED;

#ifdef GE_MODDED_CHEATS
    if (get_cur_playernum() >= 0 && get_cur_playernum() < 4)
    {
        g_MissionItemQueueValid[get_cur_playernum()] = FALSE;
    }

    /* On a Co-Op respawn, restore every team-owned mission prop.  At initial
     * stage setup the registry has already been reset and this loop is empty. */
    for (i = 0; i < g_CoopSharedPropCount; i++)
    {
        bondinvAddPropToInv(g_CoopSharedProps[i]);
    }

    for (i = ITEM_BOMBCASE; i <= ITEM_KEYBOLT; i++)
    {
        if (g_CoopSharedItems[i])
        {
            bondinvAddInvItem(i);
        }
    }
#endif
}

/**
 * Sorts subject into its correct position in the inventory list.
 *
 * Subject is expected to initially be at the head of the list. It works by
 * swapping the subject with the item to its right as many times as needed.
 */
void bondinvSortInv(InvItem *subject)
{
    InvItem *candidate;
    s32      subjweapon1 = -1;
    s32      subjweapon2 = -1;
    s32      candweapon1;
    s32      candweapon2;

    // Prepare subject's properties for comparisons
    if (subject->type == INV_ITEM_WEAPON)
    {
        subjweapon1 = subject->type_inv_item.type_weap.weapon;
    }
    else if (subject->type == INV_ITEM_DUAL)
    {
        subjweapon1 = subject->type_inv_item.type_dual.weapon_right;
        subjweapon2 = subject->type_inv_item.type_dual.weapon_left;
    }
    else if (subject->type == INV_ITEM_PROP)
    {
        subjweapon1 = 2000;
    }

    candidate = subject->next;

    while (g_CurrentPlayer->ptr_inventory_first_in_cycle != subject->next)
    {
        // Prepare candidate's properties for comparisons
        candweapon1 = -1;
        candweapon2 = -1;

        if (subject->next->type == INV_ITEM_WEAPON)
        {
            candweapon1 = subject->next->type_inv_item.type_weap.weapon;
        }
        else if (subject->next->type == INV_ITEM_DUAL)
        {
            candweapon1 = subject->next->type_inv_item.type_dual.weapon_right;
            candweapon2 = subject->next->type_inv_item.type_dual.weapon_left;
        }
        else if (subject->next->type == INV_ITEM_PROP)
        {
            candweapon1 = 1000;
        }

        // If the candidate should sort ahead of subject
        // then subject is in the desired position.
        if (candweapon1 >= subjweapon1 &&
            (subjweapon1 != candweapon1 || subjweapon2 <= candweapon2))
        {
            return;
        }

        // If there's only two items in the list then there's no point swapping
        // them. Just set the list head to the candidate.
        if (candidate->next == subject)
        {
            g_CurrentPlayer->ptr_inventory_first_in_cycle = candidate;
        }
        else
        {
            // Swap subject with candidate
            subject->next         = candidate->next;
            candidate->prev       = subject->prev;
            subject->prev         = candidate;
            candidate->next       = subject;
            subject->next->prev   = subject;
            candidate->prev->next = candidate;

            // Set new list head if subject was the head
            if (subject == g_CurrentPlayer->ptr_inventory_first_in_cycle)
            {
                g_CurrentPlayer->ptr_inventory_first_in_cycle = candidate;
            }
        }

        candidate = subject->next;
    }
}

void bondinvInsertItem(InvItem *item)
{
    if (g_CurrentPlayer->ptr_inventory_first_in_cycle)
    {
        item->next = g_CurrentPlayer->ptr_inventory_first_in_cycle;
        item->prev = g_CurrentPlayer->ptr_inventory_first_in_cycle->prev;

        item->next->prev = item;
        item->prev->next = item;
    }
    else
    {
        item->next = item;
        item->prev = item;
    }

    g_CurrentPlayer->ptr_inventory_first_in_cycle = item;
    bondinvSortInv(item);
    return;
}

void bondinvRemoveItem(InvItem *item)
{
    InvItem *prev;
    InvItem *next;

    next = item->next;
    prev = item->prev;

    if (item == g_CurrentPlayer->ptr_inventory_first_in_cycle)
    {
        if (item == item->next)
        {
            g_CurrentPlayer->ptr_inventory_first_in_cycle = NULL;
        }
        else
        {
            g_CurrentPlayer->ptr_inventory_first_in_cycle = item->next;
        }
    }

    next->prev = prev;
    prev->next = next;
    item->type = -1;
    return;
}

InvItem *bondinvGetNextAvailItem(void)
{
    int i;

    for (i = 0; i < g_CurrentPlayer->equipmaxitems; i++)
    {
        if (g_CurrentPlayer->p_itemcur[i].type == -1)
        {
            return &g_CurrentPlayer->p_itemcur[i];
        }
    }

    #ifdef DEBUG
    osSyncPrintf("equipgetfreeitem: No free equip items!!!!\n");
    #endif

    return NULL;
}

void bondinvSetAllGunsFlag(s32 all_guns)
{
    g_CurrentPlayer->equipallguns = all_guns;
}

s32 bondinvGetAllGunsFlag(void)
{
    return g_CurrentPlayer->equipallguns;
}

InvItem *bondinvGetInvItem(ITEM_IDS weapon)
{
    InvItem *first = g_CurrentPlayer->ptr_inventory_first_in_cycle;
    InvItem *item  = first;

    while (item)
    {
        if (item->type == INV_ITEM_WEAPON && item->type_inv_item.type_weap.weapon == weapon)
        {
            return item;
        }

        item = item->next;

        if (item == first)
        {
            break;
        }
    }

    return NULL;
}

/**
 * Is item in inventory
 * @param item: enum Item ID eg: ITEM_KNIFE
 * @return TRUE/FALSE
 */
int bondinvHasInvItem(ITEM_IDS item)
{
    return bondinvGetInvItem(item) != NULL;
}

InvItem *bondinvGetDualWeapon(ITEM_IDS right, ITEM_IDS left)
{
    InvItem *first = g_CurrentPlayer->ptr_inventory_first_in_cycle;
    InvItem *item  = first;

    while (item)
    {
        if (item->type == INV_ITEM_DUAL &&
            item->type_inv_item.type_dual.weapon_right == right &&
            item->type_inv_item.type_dual.weapon_left == left)
        {
            return item;
        }

        item = item->next;

        if (item == first)
        {
            break;
        }
    }

    return NULL;
}

/**
 * Is dual weapon in inventory
 * @param right: enum Item ID eg: ITEM_KNIFE
 * @param left: enum Item ID eg: ITEM_KNIFE
 * @return TRUE/FALSE
 */
bool bondinvHasDualWeapon(ITEM_IDS right, ITEM_IDS left)
{
    return bondinvGetDualWeapon(right, left) != NULL;
}

s32 bondinvItemAvailable(ITEM_IDS weaponid)
{
    if (((g_CurrentPlayer->equipallguns) && (weaponid != ITEM_UNARMED) && (weaponid < ITEM_BOMBCASE)))
    {
#ifdef BUGFIX_R1
        if ((!j_text_trigger || (weaponid != ITEM_KNIFE)))
        {
            return TRUE;
        }
#else
        return TRUE;
#endif
    }
    return bondinvHasInvItem(weaponid);
}

s32 bondinvItemAvailableForHand(ITEM_IDS right, ITEM_IDS left)
{
#ifdef GE_MODDED_CHEATS
    s32 allowalldual = TRUE;
#else
    s32 allowalldual = (getPlayerCount() == 1);
#endif
#ifdef BUGFIX_R0
    if (g_CurrentPlayer->equipallguns &&
        right < ITEM_BOMBCASE &&
        right == left &&
        allowalldual &&
        bondwalkItemCheckBitflags(right, WEAPONSTATBITFLAG_CAN_DUAL_WIELD))
    {
        return TRUE;
    }
#else
    if (left == ITEM_UNARMED)
    {
        return TRUE;
    }
    else
    {
        if (g_CurrentPlayer->equipallguns &&
            right < ITEM_BOMBCASE &&
            right == left &&
            allowalldual &&
            bondwalkItemCheckBitflags(right, WEAPONSTATBITFLAG_CAN_DUAL_WIELD) &&
            (j_text_trigger == FALSE || (right != ITEM_KNIFE)))
        {
            return TRUE;
        }
    }
#endif

    return bondinvHasDualWeapon(right, left);
}

int bondinvAddInvItem(ITEM_IDS item)
{
    InvItem *nextItem;

    if (bondinvHasInvItem(item) == FALSE)
    {
        nextItem = bondinvGetNextAvailItem();
        if (nextItem)
        {
            nextItem->type = INV_ITEM_WEAPON;
            nextItem->type_inv_item.type_weap.weapon = item;
            bondinvInsertItem(nextItem);
        }

        if ((g_CurrentPlayer->equipallguns) && (item < ITEM_BOMBCASE))
        {
#ifdef BUGFIX_R1
            if ((!j_text_trigger || (item != ITEM_KNIFE)))
            {
                return FALSE;
            }
#else
            return FALSE;
#endif
        }
        return TRUE;
    }
    return FALSE;
}

int bondinvAddDoublesInvItem(ITEM_IDS right, ITEM_IDS left)
{
    InvItem *item;

    if (bondinvHasDualWeapon(right, left) == FALSE)
    {
        item = bondinvGetNextAvailItem();

        if (item)
        {
            item->type                                 = INV_ITEM_DUAL;
            item->type_inv_item.type_dual.weapon_right = right;
            item->type_inv_item.type_dual.weapon_left  = left;
            bondinvInsertItem(item);
        }
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

WeaponObjRecord *bondinvRemovePropWeaponByID(ITEM_IDS weaponnum)
{
    if (g_CurrentPlayer->ptr_inventory_first_in_cycle)
    {
        InvItem *item = g_CurrentPlayer->ptr_inventory_first_in_cycle->next;

        while (TRUE)
        {
            InvItem *next = item->next;

            if (item->type == INV_ITEM_PROP)
            {
                PropRecord *prop = item->type_inv_item.type_prop.prop;

                if (prop->type == PROP_TYPE_WEAPON)
                {
                    ObjectRecord *obj = prop->obj;

                    if (obj->type == PROPDEF_COLLECTABLE)
                    {
                        WeaponObjRecord *weapon = (WeaponObjRecord *)prop->obj;

                        if (weapon->weaponnum == weaponnum)
                        {
                            bondinvRemoveItem(item);
                            return weapon;
                        }
                    }
                }
            }

            if ((item == g_CurrentPlayer->ptr_inventory_first_in_cycle) || (!g_CurrentPlayer->ptr_inventory_first_in_cycle))
            {
                break;
            }

            item = next;
        }
    }

    return NULL;
}

void bondinvRemoveItemByID(ITEM_IDS weaponnum)
{
    if (g_CurrentPlayer->ptr_inventory_first_in_cycle)
    {
        InvItem *item = g_CurrentPlayer->ptr_inventory_first_in_cycle->next;

        while (TRUE)
        {
            InvItem *next = item->next;

            if (item->type == INV_ITEM_PROP)
            {
                PropRecord *prop = item->type_inv_item.type_prop.prop;

                if (prop->type == PROP_TYPE_WEAPON)
                {
                    ObjectRecord *obj = prop->obj;

                    if (obj->type == PROPDEF_COLLECTABLE)
                    {
                        WeaponObjRecord *weapon = (WeaponObjRecord *)prop->obj;

                        if (weapon->weaponnum == weaponnum)
                        {
                            bondinvRemoveItem(item);
                        }
                    }
                }
            }
            else if (item->type == INV_ITEM_WEAPON)
            {
                if (item->type_inv_item.type_weap.weapon == weaponnum)
                {
                    bondinvRemoveItem(item);
                }
            }

            if ((item == g_CurrentPlayer->ptr_inventory_first_in_cycle) || (!g_CurrentPlayer->ptr_inventory_first_in_cycle))
            {
                break;
            }

            item = next;
        }
    }
}

int bondinvAddPropToInv(PropRecord *prop)
{
    InvItem *item;

    item = bondinvGetNextAvailItem();

    if (item)
    {
        item->type                         = INV_ITEM_PROP;
        item->type_inv_item.type_prop.prop = prop;
        bondinvInsertItem(item);
    }

    return TRUE;
}

int bondinvAddWeaponByProp(PropRecord *prop)
{
    int added;
    added = FALSE;

    if (prop->type == PROP_TYPE_WEAPON)
    {
        ObjectRecord *obj = prop->obj;

        if (obj->type == PROPDEF_COLLECTABLE)
        {
            WeaponObjRecord *weapon = (WeaponObjRecord *)prop->obj;
            WeaponObjRecord *otherweapon;

            s8 weaponnum = weapon->weaponnum;
            added = bondinvAddInvItem(weaponnum);

            otherweapon = weapon->dualweapon;
            if (otherweapon)
            {
                if (weapon->flags & PROPFLAG_WEAPON_LEFTHANDED)
                {
                    added = bondinvHasDualWeapon(otherweapon->weaponnum, weaponnum) == 0;
                }
                else
                {
                    added = bondinvHasDualWeapon(weaponnum, otherweapon->weaponnum) == 0;
                }
                weapon->dualweapon->LinkedWeaponType = weaponnum;
                weapon->dualweapon->dualweapon       = NULL;
                weapon->dualweapon                   = NULL;
            }
            else
            {
                if (weapon->LinkedWeaponType >= 0)
                {
                    if (weapon->flags & PROPFLAG_WEAPON_LEFTHANDED)
                    {
                        added = bondinvAddDoublesInvItem(weapon->LinkedWeaponType, weaponnum);
                    }
                    else
                    {
                        added = bondinvAddDoublesInvItem(weaponnum, weapon->LinkedWeaponType);
                    }
                }
            }
        }
    }
    return added;
}

void bondinvCycleForward(s32 *nextright, s32 *nextleft, s32 requireammo)
{
    s32      weapon1 = *nextright;
    s32      weapon2 = *nextleft;
    InvItem *item    = g_CurrentPlayer->ptr_inventory_first_in_cycle;

    while (item)
    {
        if (item->type == INV_ITEM_WEAPON)
        {
            if (item->type_inv_item.type_weap.weapon < ITEM_BOMBCASE && item->type_inv_item.type_weap.weapon > weapon1)
            {
                if (requireammo == FALSE || bondwalkItemHasAmmo(item->type_inv_item.type_weap.weapon))
                {
                    weapon1 = item->type_inv_item.type_weap.weapon;
                    weapon2 = 0;
                    break;
                }
            }
        }
        else if (item->type == INV_ITEM_DUAL)
        {
            if (item->type_inv_item.type_dual.weapon_right > weapon1 || (weapon1 == item->type_inv_item.type_dual.weapon_right && item->type_inv_item.type_dual.weapon_left > weapon2))
            {
                if (requireammo == FALSE || bondwalkItemHasAmmo(item->type_inv_item.type_dual.weapon_right) || bondwalkItemHasAmmo(item->type_inv_item.type_dual.weapon_left))
                {
                    weapon1 = item->type_inv_item.type_dual.weapon_right;
                    weapon2 = item->type_inv_item.type_dual.weapon_left;
                    break;
                }
            }
        }

        item = item->next;

        if (item == g_CurrentPlayer->ptr_inventory_first_in_cycle)
        {
            if (requireammo)
            {
                break;
            }

            weapon1 = -1;
            weapon2 = -1;
        }
    }

    if (g_CurrentPlayer->equipallguns)
    {
        s32 candidate = *nextright;

#ifdef GE_MODDED_CHEATS
        if (bondwalkItemCheckBitflags(*nextright, WEAPONSTATBITFLAG_CAN_DUAL_WIELD) && (*nextleft < *nextright) && (requireammo == FALSE || bondwalkItemHasAmmo(*nextright)) && (weapon1 != *nextright || *nextright < weapon2)
#else
        if (getPlayerCount() == 1 && bondwalkItemCheckBitflags(*nextright, WEAPONSTATBITFLAG_CAN_DUAL_WIELD) && (*nextleft < *nextright) && (requireammo == FALSE || bondwalkItemHasAmmo(*nextright)) && (weapon1 != *nextright || *nextright < weapon2)
#endif
#ifdef BUGFIX_R1
            && (!j_text_trigger || *nextright != ITEM_KNIFE)
#endif
        )
        {
            weapon1 = *nextright;
            weapon2 = *nextright;
        }
        else
        {
            if ((weapon1 != *nextright) || (weapon2 == *nextleft))
            {
                // Find next weapon
                do
                {
                    candidate = (candidate + 1) % ITEM_BOMBCASE;

                    if (candidate == ITEM_UNARMED)
                    {
                        candidate = (candidate + 1) % ITEM_BOMBCASE;
                    }

                    if ((requireammo == FALSE || bondwalkItemHasAmmo(candidate))
#ifdef BUGFIX_R1
                        && (!j_text_trigger || candidate != ITEM_KNIFE)
#endif
                    )
                    {
                        weapon1 = candidate;
                        weapon2 = ITEM_UNARMED;
                        break;
                    }
                } while (candidate != weapon1);
            }
        }
    }

    *nextright = weapon1;
    *nextleft  = weapon2;
}

void bondinvCycleBackward(s32 *nextright, s32 *nextleft, s32 requireammo)
{
    s32 weapon1 = *nextright;
    s32 weapon2 = *nextleft;

    if (g_CurrentPlayer->ptr_inventory_first_in_cycle != NULL)
    {
        InvItem *item = g_CurrentPlayer->ptr_inventory_first_in_cycle->prev;

        while (TRUE)
        {
            if (item->type == INV_ITEM_WEAPON)
            {
                if (item->type_inv_item.type_weap.weapon < ITEM_BOMBCASE && (item->type_inv_item.type_weap.weapon < weapon1 || (weapon1 == item->type_inv_item.type_weap.weapon && weapon2 > 0)))
                {
                    if (requireammo == FALSE || bondwalkItemHasAmmo(item->type_inv_item.type_weap.weapon))
                    {
                        weapon1 = item->type_inv_item.type_weap.weapon;
                        weapon2 = ITEM_UNARMED;
                        break;
                    }
                }
            }
            else if (item->type == INV_ITEM_DUAL)
            {
                if (item->type_inv_item.type_dual.weapon_right < weapon1 || (weapon1 == item->type_inv_item.type_dual.weapon_right && item->type_inv_item.type_dual.weapon_left < weapon2))
                {
                    if (requireammo == FALSE || bondwalkItemHasAmmo(item->type_inv_item.type_dual.weapon_right) || bondwalkItemHasAmmo(item->type_inv_item.type_dual.weapon_left))
                    {
                        weapon1 = item->type_inv_item.type_dual.weapon_right;
                        weapon2 = item->type_inv_item.type_dual.weapon_left;
                        break;
                    }
                }
            }

            if (item == g_CurrentPlayer->ptr_inventory_first_in_cycle)
            {
                if (requireammo)
                {
                    break;
                }

                weapon1 = 1000;
                weapon2 = 1000;
            }

            item = item->prev;
        }
    }

    if (g_CurrentPlayer->equipallguns)
    {
        s32 candidate = *nextright;

        if (*nextleft == ITEM_UNARMED)
        {
            candidate = (candidate + ITEM_BOMBCASE - 1) % ITEM_BOMBCASE;
            if (candidate == ITEM_UNARMED)
            {
                candidate = (candidate + ITEM_BOMBCASE - 1) % ITEM_BOMBCASE;
            }
        }

        while (TRUE)
        {
            if (candidate == weapon1)
            {
#ifdef GE_MODDED_CHEATS
                if (bondwalkItemCheckBitflags(candidate, WEAPONSTATBITFLAG_CAN_DUAL_WIELD) && (requireammo == FALSE || bondwalkItemHasAmmo(candidate)) && (candidate != *nextright || candidate < *nextleft) && (weapon2 < candidate)
#else
                if (getPlayerCount() == 1 && bondwalkItemCheckBitflags(candidate, WEAPONSTATBITFLAG_CAN_DUAL_WIELD) && (requireammo == FALSE || bondwalkItemHasAmmo(candidate)) && (candidate != *nextright || candidate < *nextleft) && (weapon2 < candidate)
#endif
#ifdef BUGFIX_R1
                    && (!j_text_trigger || candidate != ITEM_KNIFE)
#endif
                )
                {
                    weapon1 = candidate;
                    weapon2 = candidate;
                }

                break;
            }
            else if (
                (requireammo == FALSE || bondwalkItemHasAmmo(candidate))
#ifdef BUGFIX_R1
                && (!j_text_trigger || candidate != ITEM_KNIFE)
#endif
            )
            {
                if (getPlayerCount() == 1 && bondwalkItemCheckBitflags(candidate, WEAPONSTATBITFLAG_CAN_DUAL_WIELD) && (candidate != *nextright || candidate < *nextleft))
                {
                    weapon1 = candidate;
                    weapon2 = candidate;
                }
                else
                {
                    weapon1 = candidate;
                    weapon2 = ITEM_UNARMED;
                }

                break;
            }
            else
            {
                candidate = (candidate + ITEM_BOMBCASE - 1) % ITEM_BOMBCASE;
                if (candidate == ITEM_UNARMED)
                {
                    candidate = (candidate + ITEM_BOMBCASE - 1) % ITEM_BOMBCASE;
                }
            }
        }
    }

    *nextright = weapon1;
    *nextleft  = weapon2;
}

#ifdef GE_MODDED_CHEATS
/* R19: B+Z cycles only the equipment/mission-item tail of the same inventory
 * list used by the solo watch.  Normal weapon cycling remains unchanged and
 * returns the player to guns. */
s32 bondinvCycleMissionItem(void);

void bondinvProcessMissionItemQueue(void)
{
    s32 playernum = get_cur_playernum();
    ITEM_IDS item;

    if (playernum < 0 || playernum >= 4 || !g_MissionItemQueueValid[playernum])
    {
        return;
    }

    item = g_MissionItemQueuedItem[playernum];

    /* Once the normal switch state machine has completed, commit the watch
     * inventory cursor and release the queue. */
    if (getCurrentPlayerWeaponId(GUNRIGHT) == item
        && !bondinvHandIsSwitching(GUNRIGHT))
    {
        bondinvSetCurEquippedItem(g_MissionItemQueuedIndex[playernum]);
        g_MissionItemQueueValid[playernum] = FALSE;
        return;
    }

    /* An empty clip with reserve ammo is allowed to enter the game's normal
     * automatic reload first.  In dual wield, both relevant hands must finish
     * their reload sequence before either hand begins the holster transition. */
    if (bondinvHandIsReloading(GUNRIGHT)
        || bondinvHandIsReloading(GUNLEFT)
        || bondinvHandNeedsAutomaticReload(GUNRIGHT)
        || bondinvHandNeedsAutomaticReload(GUNLEFT))
    {
        return;
    }

    if (bondinvHandIsSwitching(GUNRIGHT) || bondinvHandIsSwitching(GUNLEFT))
    {
        return;
    }

    gunRequestHandWeaponChange(GUNRIGHT, item, 1);
    gunRequestHandWeaponChange(GUNLEFT, ITEM_UNARMED, 1);
}

s32 bondinvCycleMissionItem(void)
{
    s32 count = bondinvCountTotalItemsInInv();
    s32 index = bondinvGetCurEquippedItem();
    s32 remaining = count;
    s32 playernum = get_cur_playernum();

    /* A queued reload/holster/item change is one logical B+Z press.  Further
     * presses while it is pending remain consumed but do not skip ahead. */
    if (playernum >= 0 && playernum < 4 && g_MissionItemQueueValid[playernum])
    {
        return TRUE;
    }

    while (remaining-- > 0)
    {
        ITEM_IDS item;

        if (++index >= count)
        {
            index = 0;
        }

        item = bondinvGetTextbyInvIndex(index);

        if (item >= ITEM_BOMBCASE && item < ITEM_IDS_MAX)
        {
            /* R20: if a real weapon is still up, use the ordinary weapon-change
             * state machine so it visibly lowers all the way before the mission
             * item becomes current. Once already in the mission-item tail there
             * is no gun left to holster, so keep the fast item-to-item browse. */
            if (getCurrentPlayerWeaponId(GUNRIGHT) < ITEM_BOMBCASE)
            {
                if (playernum >= 0 && playernum < 4)
                {
                    g_MissionItemQueuedItem[playernum] = item;
                    g_MissionItemQueuedIndex[playernum] = index;
                    g_MissionItemQueueValid[playernum] = TRUE;
                    bondinvProcessMissionItemQueue();
                }
            }
            else
            {
                currentPlayerUnEquipWeaponWrapper(GUNRIGHT, item);
                currentPlayerUnEquipWeaponWrapper(GUNLEFT, ITEM_UNARMED);
                bondinvSetCurEquippedItem(index);
            }

            return TRUE;
        }
    }

    return FALSE;
}
#endif

bool bondinvCheckHasKeyFlags(u32 wantkeyflags)
{
    u32      heldkeyflags = 0;
    InvItem *item         = g_CurrentPlayer->ptr_inventory_first_in_cycle;

    while (item)
    {
        if (item->type == INV_ITEM_PROP)
        {
            PropRecord *prop = item->type_inv_item.type_prop.prop;

            if (prop->type == PROP_TYPE_OBJ)
            {
                ObjectRecord *obj = prop->obj;

                if (obj->type == PROPDEF_KEY)
                {
                    KeyRecord *key = (KeyRecord *)prop->obj;

                    heldkeyflags |= key->keyflags;

                    if ((wantkeyflags & heldkeyflags) == wantkeyflags)
                    {
                        return TRUE;
                    }
                }
            }
        }

        item = item->next;

        if (item == g_CurrentPlayer->ptr_inventory_first_in_cycle)
        {
            break;
        }
    }

    return FALSE;
}

bool bondinvHasGEKey(void)
{
    InvItem      *item;
    PropRecord   *prop;
    ObjectRecord *obj;

    item = g_CurrentPlayer->ptr_inventory_first_in_cycle;

    while (item)
    {
        if (item->type == INV_ITEM_PROP)
        {
            prop = item->type_inv_item.type_prop.prop;

            if (prop->type == PROP_TYPE_WEAPON)
            {
                obj = prop->obj;

                if (obj->obj == PROJECTILES_TYPE_GE_KEY)
                {
                    return TRUE;
                }
            }
        }

        item = item->next;

        if (item == g_CurrentPlayer->ptr_inventory_first_in_cycle)
        {
            break;
        }
    }

    return FALSE;
}

/**
 * Is the player alive with flag tag token in inventory
 * @return TRUE/FALSE
 */
bool bondinvIsAliveWithFlag(void)
{
    if (!g_CurrentPlayer->bonddead)
    {
        return bondinvHasInvItem(ITEM_TOKEN);
    }

    return FALSE;
}

/**
 * Is the Golden Gun in inventory
 * @return TRUE/FALSE
 */
bool bondinvHasGoldenGun(void)
{
    return bondinvHasInvItem(ITEM_GOLDENGUN);
}

bool bondinvHasPropInInv(PropRecord *prop)
{
    InvItem *item = g_CurrentPlayer->ptr_inventory_first_in_cycle;

    while (item)
    {
        if (item->type == INV_ITEM_PROP && item->type_inv_item.type_prop.prop == prop)
        {
            return TRUE;
        }

        item = item->next;

        if (item == g_CurrentPlayer->ptr_inventory_first_in_cycle)
        {
            break;
        }
    }

    return FALSE;
}

s32 bondinvCountTotalItemsInInv(void)
{
    InvItem *item;
    s32      numitems = 0;

    if (g_CurrentPlayer->equipallguns)
    {
#ifdef BUGFIX_R1
        numitems = (j_text_trigger ? ITEM_TASER : ITEM_TANKSHELLS);
#else
        numitems = ITEM_TANKSHELLS;
#endif
    }

    item = g_CurrentPlayer->ptr_inventory_first_in_cycle;

    while (item)
    {
        if (item->type == INV_ITEM_PROP)
        {
            PropRecord *prop = item->type_inv_item.type_prop.prop;

            if (prop->type == PROP_TYPE_WEAPON)
            {
                ObjectRecord *obj = prop->obj;

                if (obj->runtime_bitflags & 0x400)
                {
                    numitems = numitems + 1;
                }
            }
            else if (prop->type == PROP_TYPE_OBJ)
            {
                if ((prop->obj->flags2 & 0x40000) == 0)
                {
                    numitems = numitems + 1;
                }
            }
        }
        else if (item->type == INV_ITEM_WEAPON)
        {
            if ((g_CurrentPlayer->equipallguns == FALSE) || (item->type_inv_item.type_weap.weapon > ITEM_TANKSHELLS))
            {
                numitems = numitems + 1;
            }
        }

        item = item->next;

        if (item == g_CurrentPlayer->ptr_inventory_first_in_cycle)
        {
            break;
        }
    }

    return numitems;
}

InvItem *bondinvGetItemByIndex(s32 index)
{
    InvItem *item;

    if (g_CurrentPlayer->equipallguns)
    {
#ifdef BUGFIX_R1
        if (index < (j_text_trigger ? ITEM_TASER : ITEM_TANKSHELLS))
#else
        if (index < ITEM_TANKSHELLS)
#endif
        {
            return NULL;
        }

#ifdef BUGFIX_R1
        index = index - (j_text_trigger ? ITEM_TASER : ITEM_TANKSHELLS);
#else
        index = index - ITEM_TANKSHELLS;
#endif
    }

    item = g_CurrentPlayer->ptr_inventory_first_in_cycle;

    while (item)
    {
        if (item->type == INV_ITEM_PROP)
        {
            PropRecord *prop = item->type_inv_item.type_prop.prop;

            if (prop->type == PROP_TYPE_WEAPON)
            {
                ObjectRecord *obj = prop->obj;

                if (obj->runtime_bitflags & 0x400)
                {
                    if (index == 0)
                    {
                        return item;
                    }
                    index--;
                }
            }
            else if (prop->type == PROP_TYPE_OBJ)
            {
                if ((prop->obj->flags2 & 0x40000) == 0)
                {
                    if (index == 0)
                    {
                        return item;
                    }
                    index--;
                }
            }
        }
        else if (item->type == INV_ITEM_WEAPON)
        {
            if ((g_CurrentPlayer->equipallguns == FALSE) || (item->type_inv_item.type_weap.weapon > ITEM_TANKSHELLS))
            {
                if (index == 0)
                {
                    return item;
                }
                index--;
            }
        }

        item = item->next;

        if (item == g_CurrentPlayer->ptr_inventory_first_in_cycle)
        {
            break;
        }
    }

    return NULL;
}

textoverride *bondinvGetTextbyObj(ObjectRecord *obj)
{
    textoverride *override = g_CurrentPlayer->textoverrides;

    while (override)
    {
        if (override->obj == obj)
        {
            return override;
        }

        override = override->next;
    }

    return NULL;
}

textoverride *bondinvGetTextbyWeaponID(ITEM_IDS weaponnum)
{
    textoverride *override = g_CurrentPlayer->textoverrides;

    while (override)
    {
        if ((override->objoffset == 0) && (override->weapon == weaponnum))
        {
            return override;
        }

        override = override->next;
    }

    return NULL;
}

s32 bondinvGetTextbyInvIndex(s32 index)
{
    textoverride *override;
    InvItem *     inv_item;

    inv_item = bondinvGetItemByIndex(index);

    if (inv_item)
    {
        if (inv_item->type == INV_ITEM_PROP)
        {
            PropRecord *prop = inv_item->type_inv_item.type_prop.prop;

            override = bondinvGetTextbyObj(prop->obj);

            if (override)
            {
                return override->weapon;
            }
        }
        else if (inv_item->type == INV_ITEM_WEAPON)
        {
            return inv_item->type_inv_item.type_weap.weapon;
        }
    }
    else
    {
        if (g_CurrentPlayer->equipallguns)
        {
#ifdef BUGFIX_R1
            if (index < (j_text_trigger ? ITEM_TASER : ITEM_TANKSHELLS))
            {
                if (j_text_trigger && ((index + 1) >= ITEM_KNIFE))
                {
                    return index + 2;
                }

                return index + 1;
            }
#else
            if (index < ITEM_TANKSHELLS)
            {
                return index + 1;
            }
#endif
        }
    }

    return 0;
}

u16 *bondinvGetNameByIndex(s32 index)
{
    InvItem      *item      = bondinvGetItemByIndex(index);
    ITEM_IDS      weaponnum = 0;
    textoverride *override;

    if (item)
    {
        if (item->type == INV_ITEM_PROP)
        {
            PropRecord *prop = item->type_inv_item.type_prop.prop;
            override         = bondinvGetTextbyObj(prop->obj);

            if (override)
            {
                if (override->shorttext)
                {
                    return langGet(override->shorttext);
                }

                weaponnum = override->weapon;
            }
        }
        else if (item->type == INV_ITEM_WEAPON)
        {
            weaponnum = item->type_inv_item.type_weap.weapon;
            override  = bondinvGetTextbyWeaponID(weaponnum);

            if (override && override->shorttext)
            {
                return langGet(override->shorttext);
            }
        }
    }
    else
    {
        if (g_CurrentPlayer->equipallguns)
        {
#ifdef BUGFIX_R1
            if (index < (j_text_trigger ? ITEM_TASER : ITEM_TANKSHELLS))
            {
                if (j_text_trigger && ((index + 1) >= ITEM_KNIFE))
                {
                    return get_ptr_short_watch_text_for_item(index + 2);
                }

                return get_ptr_short_watch_text_for_item(index + 1);
            }
#else
            if (index < ITEM_TANKSHELLS)
            {
                return get_ptr_short_watch_text_for_item(index + 1);
            }
#endif
        }
    }

    return get_ptr_short_watch_text_for_item(weaponnum);
}

u16 *bondinvGetLongNameByIndex(s32 index)
{
    InvItem      *item      = bondinvGetItemByIndex(index);
    ITEM_IDS      weaponnum = 0;
    textoverride *override;

    if (item)
    {
        if (item->type == INV_ITEM_PROP)
        {
            PropRecord *prop = item->type_inv_item.type_prop.prop;
            override         = bondinvGetTextbyObj(prop->obj);

            if (override)
            {
                if (override->longtext)
                {
                    return langGet(override->longtext);
                }

                weaponnum = override->weapon;
            }
        }
        else if (item->type == INV_ITEM_WEAPON)
        {
            weaponnum = item->type_inv_item.type_weap.weapon;
            override  = bondinvGetTextbyWeaponID(weaponnum);

            if (override && override->longtext)
            {
                return langGet(override->longtext);
            }
        }
    }
    else
    {
        if (g_CurrentPlayer->equipallguns)
        {
#ifdef BUGFIX_R1
            if (index < (j_text_trigger ? ITEM_TASER : ITEM_TANKSHELLS))
            {
                if (j_text_trigger && ((index + 1) >= ITEM_KNIFE))
                {
                    return get_ptr_long_watch_text_for_item(index + 2);
                }

                return get_ptr_long_watch_text_for_item(index + 1);
            }
#else
            if (index < ITEM_TANKSHELLS)
            {
                return get_ptr_long_watch_text_for_item(index + 1);
            }
#endif
        }
    }

    return get_ptr_long_watch_text_for_item(weaponnum);
}

extern f32 get_45_degree_angle_0(s32 item);
f32 bondinvGet45AngleForIndex(int index)
{
    return get_45_degree_angle_0(bondinvGetTextbyInvIndex(index));
}

int bondinvGetHoffsetForIndex(int index)
{
    return get_horizontal_offset_on_solo_watch_menu_for_item(bondinvGetTextbyInvIndex(index));
}

int bondinvGetVoffsetForIndex(int index)
{
    return get_vertical_offset_on_solo_watch_menu_for_item(bondinvGetTextbyInvIndex(index));
}

int bondinvGetDepthForIndex(int index)
{
    return get_depth_offset_solo_watch_menu_inventory_page_for_item(bondinvGetTextbyInvIndex(index));
}

u16 *bondinvGetFirstTitlebyIndex(s32 index)
{
    InvItem      *item      = bondinvGetItemByIndex(index);
    ITEM_IDS      weaponnum = 0;
    textoverride *override;

    if (item)
    {
        if (item->type == INV_ITEM_PROP)
        {
            PropRecord *prop = item->type_inv_item.type_prop.prop;
            override         = bondinvGetTextbyObj(prop->obj);

            if (override)
            {
                if (override->titletext1)
                {
                    return langGet(override->titletext1);
                }

                weaponnum = override->weapon;
            }
        }
        else if (item->type == INV_ITEM_WEAPON)
        {
            weaponnum = item->type_inv_item.type_weap.weapon;
            override  = bondinvGetTextbyWeaponID(weaponnum);

            if (override && override->titletext1)
            {
                return langGet(override->titletext1);
            }
        }
    }
    else
    {
        if (g_CurrentPlayer->equipallguns)
        {
#ifdef BUGFIX_R1
            if (index < (j_text_trigger ? ITEM_TASER : ITEM_TANKSHELLS))
            {
                if (j_text_trigger && ((index + 1) >= ITEM_KNIFE))
                {
                    return get_ptr_first_title_line_item(index + 2);
                }

                return get_ptr_first_title_line_item(index + 1);
            }
#else
            if (index < ITEM_TANKSHELLS)
            {
                return get_ptr_first_title_line_item(index + 1);
            }
#endif
        }
    }

    return get_ptr_first_title_line_item(weaponnum);
}

u16 *bondinvGetSecondTitlebyIndex(s32 index)
{
    InvItem      *item      = bondinvGetItemByIndex(index);
    ITEM_IDS      weaponnum = 0;
    textoverride *override;

    if (item)
    {
        if (item->type == INV_ITEM_PROP)
        {
            PropRecord *prop = item->type_inv_item.type_prop.prop;
            override         = bondinvGetTextbyObj(prop->obj);

            if (override)
            {
                if (override->titletext2)
                {
                    return langGet(override->titletext2);
                }

                weaponnum = override->weapon;
            }
        }
        else if (item->type == INV_ITEM_WEAPON)
        {
            weaponnum = item->type_inv_item.type_weap.weapon;
            override  = bondinvGetTextbyWeaponID(weaponnum);

            if (override && override->titletext2)
            {
                return langGet(override->titletext2);
            }
        }
    }
    else
    {
        if (g_CurrentPlayer->equipallguns)
        {
#ifdef BUGFIX_R1
            if (index < (j_text_trigger ? ITEM_TASER : ITEM_TANKSHELLS))
            {
                if (j_text_trigger && ((index + 1) >= ITEM_KNIFE))
                {
                    return get_ptr_second_title_line_item(index + 2);
                }

                return get_ptr_second_title_line_item(index + 1);
            }
#else
            if (index < ITEM_TANKSHELLS)
            {
                return get_ptr_second_title_line_item(index + 1);
            }
#endif
        }
    }

    return get_ptr_second_title_line_item(weaponnum);
}
extern f32 get_45_degree_angle(s32 item);
f32 bondinvGetDifferent45AngleForIndex(int index)
{
    return get_45_degree_angle(bondinvGetTextbyInvIndex(index));
}

int bondinvGetVposWatchForIndex(int index)
{
    return get_vertical_position_solo_watch_menu_main_page_for_item(bondinvGetTextbyInvIndex(index));
}

int bondinvGetHposWatchForIndex(int index)
{
    return get_lateral_position_solo_watch_menu_main_page_for_item(bondinvGetTextbyInvIndex(index));
}

int bondinvGetDepthWatchForIndex(int index)
{
    return get_depth_on_solo_watch_menu_page_for_item(bondinvGetTextbyInvIndex(index));
}

int bondinvGetXrotWatchForIndex(int index)
{
    return get_xrotation_solo_watch_menu_for_item(bondinvGetTextbyInvIndex(index));
}

int bondinvGetYrotWatchForIndex(int index)
{
    return get_yrotation_solo_watch_menu_for_item(bondinvGetTextbyInvIndex(index));
}

void bondinvAddTextOverride(textoverride *override)
{
    override->next                 = g_CurrentPlayer->textoverrides;
    g_CurrentPlayer->textoverrides = override;
}

int bondinvGetCurEquippedItem(void)
{
    return g_CurrentPlayer->equipcuritem;
}

void bondinvSetCurEquippedItem(int current_item)
{
    g_CurrentPlayer->equipcuritem = current_item;
}

void bondinvDetermineEquippedItem(void)
{
    s32 current_weapon;
    s32 i;

    current_weapon = getCurrentPlayerWeaponId(GUNRIGHT);

    g_CurrentPlayer->equipcuritem = ITEM_UNARMED;

    for (i = 0; i < bondinvCountTotalItemsInInv(); i++)
    {
        if (bondinvGetTextbyInvIndex(i) == current_weapon)
        {
            g_CurrentPlayer->equipcuritem = i;
            return;
        }
    }
}

u8 *bondinvGetActivatedTextObject(ObjectRecord *obj)
{
    textoverride *override = bondinvGetTextbyObj(obj);

    if (override && override->pickuptext)
    {
        return langGet(override->pickuptext);
    }

    return NULL;
}

u8 *bondinvGetActivatedTextWeapon(ITEM_IDS weaponnum)
{
    textoverride *override = bondinvGetTextbyWeaponID(weaponnum);

    if (override && override->pickuptext)
    {
        return langGet(override->pickuptext);
    }

    return NULL;
}

void bondinvIncrementHeldTime(s32 weapon1, s32 weapon2)
{
    s32 leastusedtime;
    s32 leastusedindex;
    s32 i;

    if (!bondwalkItemCheckBitflags(weapon1, WEAPONSTATBITFLAG_USE_HOLD_TIME))
    {
        return;
    }

    leastusedtime  = 0x7fffffff;
    leastusedindex = 0;

    if (!bondwalkItemCheckBitflags(weapon2, WEAPONSTATBITFLAG_USE_HOLD_TIME))
    {
        weapon2 = ITEM_UNARMED;
    }

    for (i = 0; i < 10; i++)
    {
        s32 time = g_CurrentPlayer->gunheldarr[i].totaltime;

        if (time >= 0)
        {
            if (weapon1 == g_CurrentPlayer->gunheldarr[i].weapon1 &&
                weapon2 == g_CurrentPlayer->gunheldarr[i].weapon2)
            {
                g_CurrentPlayer->gunheldarr[i].totaltime = time + g_ClockTimer;
                break;
            }

            if (time < leastusedtime)
            {
                leastusedtime  = time;
                leastusedindex = i;
            }
        }
        else
        {
            leastusedindex = i;
            i              = 10;
            break;
        }
    }

    if (i == 10)
    {
        g_CurrentPlayer->gunheldarr[leastusedindex].totaltime = g_ClockTimer;
        g_CurrentPlayer->gunheldarr[leastusedindex].weapon1   = weapon1;
        g_CurrentPlayer->gunheldarr[leastusedindex].weapon2   = weapon2;
    }
}

s32 bondinvGetWeaponOfChoice(s32 *weapon1, s32 *weapon2)
{
    s32 mosttime = -1;
    s32 i;

    *weapon1 = ITEM_UNARMED;
    *weapon2 = ITEM_UNARMED;

    for (i = 0; i < 10; i++)
    {
        if (g_CurrentPlayer->gunheldarr[i].totaltime >= 0 && g_CurrentPlayer->gunheldarr[i].totaltime > mosttime)
        {
            mosttime = g_CurrentPlayer->gunheldarr[i].totaltime;
            *weapon1 = g_CurrentPlayer->gunheldarr[i].weapon1;
            *weapon2 = g_CurrentPlayer->gunheldarr[i].weapon2;
        }
    }
}
