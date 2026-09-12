#ifndef _BONDINV_H_
#define _BONDINV_H_
#include <ultra64.h>
#include <bondconstants.h>
#include "bondview.h"
#include "bondtypes.h"

void bondinvReinitInv(void);
s32 bondinvIsAliveWithFlag(void);
s32 bondinvCountTotalItemsInInv(void);
s32 bondinvGetTextbyInvIndex(s32 index);
int bondinvGetCurEquippedItem(void);
void bondinvSetCurEquippedItem(int current_item);
u16 *bondinvGetNameByIndex(s32 index);

InvItem *bondinvGetItemByIndex(s32 index);
textoverride *bondinvGetTextbyObj(ObjectRecord *obj);
textoverride *bondinvGetTextbyWeaponID(ITEM_IDS weaponnum);

void bondinvCycleBackward(s32 *nextright, s32 *nextleft, s32 requireammo);
void bondinvCycleForward(s32 *nextright, s32 *nextleft, s32 requireammo);
bool bondinvHasGoldenGun(void);
int bondinvAddInvItem(ITEM_IDS item);
int bondinvAddDoublesInvItem(ITEM_IDS right, ITEM_IDS left);
s32 bondinvGetAllGunsFlag(void);
void bondinvSetAllGunsFlag(s32 all_guns);
bool          bondinvHasPropInInv(PropRecord *prop);
int bondinvAddPropToInv(PropRecord *prop);
WeaponObjRecord *bondinvRemovePropWeaponByID(ITEM_IDS weaponnum);
void bondinvRemoveItemByID(ITEM_IDS weaponnum);
s32 bondinvGetWeaponOfChoice(s32 *weapon1, s32 *weapon2);
s32 bondinvItemAvailableForHand(ITEM_IDS right, ITEM_IDS left);
s32 bondinvItemAvailable(ITEM_IDS weaponid);
void bondinvAddTextOverride(textoverride *override);
#ifdef GE_MODDED_CHEATS
void bondinvResetCoopSharedProps(void);
void bondinvShareCoopItem(ITEM_IDS item);
void bondinvShareCoopProp(PropRecord *prop);
s32 bondinvCycleMissionItem(void);
void bondinvProcessMissionItemQueue(void);
#endif

#endif
