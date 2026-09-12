// Include paths for <> type includes - remember for x86 we need to use "/usr/include/" for system includes now
// x86 compile
// gcc -m32 -fms-extensions -I ./include/ -I ./src/ -o build/aiparse tools/aiParse.c
// mips compile
// gcc -mips2 -32 -I ./include/ -I ./src/ -o build/aiparse tools/aiParse.c
// ./build/aiparse
// #include <arpa/inet.h>
#define _LANGUAGE_C
#define TARGET_N64
#define _ULTRA64_TYPES_H_ 1
#define _OS_H_
typedef char *__gnuc_va_list;
#define DEBUG
#define AIPARSE
typedef unsigned char               u8;  /* unsigned  8-bit */
typedef unsigned short              u16; /* unsigned 16-bit */
typedef unsigned int                u32; /* unsigned 32-bit */
typedef unsigned long long          u64; /* unsigned 64-bit */

typedef signed char                 s8;  /* signed  8-bit */
typedef short                       s16; /* signed 16-bit */
typedef int                         s32; /* signed 32-bit */
typedef long long                   s64; /* signed 64-bit */

typedef volatile unsigned char      vu8;  /* unsigned  8-bit */
typedef volatile unsigned short     vu16; /* unsigned 16-bit */
typedef volatile unsigned int       vu32; /* unsigned 32-bit */
typedef volatile unsigned long long vu64; /* unsigned 64-bit */

typedef volatile signed char        vs8;  /* signed  8-bit */
typedef volatile short              vs16; /* signed 16-bit */
typedef volatile int                vs32; /* signed 32-bit */
typedef volatile long long          vs64; /* signed 64-bit */

typedef float                       f32; /* single prec floating point */
typedef double                      f64; /* double prec floating point */
typedef u32                         uintptr_t;
typedef long                        wchar_t;

/*************************************************************************
 * Common definitions
 */
#ifndef TRUE
    #define TRUE 1
#endif

#ifndef FALSE
    #define FALSE 0
#endif

#ifndef NULL
    #define NULL (void *)0
#endif
// use absolute paths since GE doesnt do I/O for x86
#include "/usr/include/stdlib.h"
#include "/usr/include/string.h"

#include "../include/math.h"
#include <stdio.h>
#include <time.h>
#include <unistd.h>

// #include "../include/PR/libaudio.h"
// #include "../include/PR/libultra.h"
// include from GE
#include <bondaicommands.h>
#include <bondconstants.h>
#include <bondtypes.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wint-conversion"
#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"
#pragma GCC diagnostic ignored "-Wformat="
// holds the current setup once filled from binary
void      *g_ptrStageSetupFile;
stagesetup g_chraiCurrentSetup;

// now importing from GE types
//  duplicate types are now gone

#pragma region "AICommands"

char *AI_CMD_ToString[] = {
    "GotoNext",
    "GotoFirst",
    "Label",
    "Yield",
    "EndList",
    "SetChrAiList",
    "SetReturnAiList",
    "Return",
    "Stop",
    "Kneel",
    "PlayAnimation",
    "IFPlayingAnimation",
    "PointAtBond",
    "LookSurprised",
    "TRYSidestepping",
    "TRYSideHopping",
    "TRYSideRunning",
    "TRYFiringWalk",
    "TRYFiringRun",
    "TRYFiringRoll",
    "TRYFireOrAimAtTarget",
    "TRYFireOrAimAtTargetKneel",
    "TRYFireOrAimAtTargetUpdate",
    "TRYFacingTarget",
    "HitChrWithItem",
    "ChrHitChr",
    "TRYThrowingGrenade",
    "TRYDroppingItem",
    "RunToPad",
    "RunToPadPreset",
    "WalkToPad",
    "SprintToPad",
    "StartPatrol",
    "Surrender",
    "RemoveMe",
    "ChrRemoveInstant",
    "TRYTriggeringAlarmAtPad",
    "AlarmOn",
    "AlarmOff",
    "TRYRunFromBond",
    "TRYRunToBond",
    "TRYWalkToBond",
    "TRYSprintToBond",
    "TRYFindCover",
    "TRYRunToChr",
    "TRYWalkToChr",
    "TRYSprintToChr",
    "IFImOnPatrolOrStopped",
    "IFChrDyingOrDead",
    "IFChrDoesNotExist",
    "IFISeeBond",
    "SetNewRandom",
    "IFRandomLessThan",
    "IFRandomGreaterThan",
    "IFICanHearAlarm",
    "IFAlarmIsOn",
    "IFGasIsLeaking",
    "IFIHeardBond",
    "IFISeeSomeoneShot",
    "IFISeeSomeoneDie",
    "IFICouldSeeBond",
    "IFICouldSeeBondsStan",
    "IFIWasShotRecently",
    "IFIHeardBondRecently",
    "IFImInRoomWithChr",
    "IFIveNotBeenSeen",
    "IFImOnScreen",
    "IFMyRoomIsOnScreen",
    "IFRoomWithPadIsOnScreen",
    "IFImTargetedByBond",
    "IFBondMissedMe",
    "IFMyAngleToBondLessThan",
    "IFMyAngleToBondGreaterThan",
    "IFMyAngleFromBondLessThan",
    "IFMyAngleFromBondGreaterThan",
    "IFMyDistanceToBondLessThanDecimeter",
    "IFMyDistanceToBondGreaterThanDecimeter",
    "IFChrDistanceToPadLessThanDecimeter",
    "IFChrDistanceToPadGreaterThanDecimeter",
    "IFMyDistanceToChrLessThanDecimeter",
    "IFMyDistanceToChrGreaterThanDecimeter",
    "TRYSettingMyPresetToChrWithinDistanceDecimeter",
    "IFBondDistanceToPadLessThanDecimeter",
    "IFBondDistanceToPadGreaterThanDecimeter",
    "IFChrInRoomWithPad",
    "IFBondInRoomWithPad",
    "IFBondCollectedObject",
    "IFKeyDropped",
    "IFItemIsAttachedToObject",
    "IFBondHasItemEquipped",
    "IFObjectExists",
    "IFObjectNotDestroyed",
    "IFObjectWasActivated",
    "IFBondUsedGadgetOnObject",
    "ActivateObject",
    "DestroyObject",
    "DropObject",
    "ChrDropAllConcealedItems",
    "ChrDropAllHeldItems",
    "BondCollectObject",
    "ChrEquipObject",
    "MoveObject",
    "DoorOpen",
    "DoorClose",
    "IFDoorStateEqual",
    "IFDoorHasBeenOpenedBefore",
    "DoorSetLock",
    "DoorUnsetLock",
    "IFDoorLockEqual",
    "IFObjectiveNumComplete",
    "TRYUnknown6e",
    "TRYUnknown6f",
    "IFGameDifficultyLessThan",
    "IFGameDifficultyGreaterThan",
    "IFMissionTimeLessThan",
    "IFMissionTimeGreaterThan",
    "IFSystemPowerTimeLessThan",
    "IFSystemPowerTimeGreaterThan",
    "IFLevelIdLessThan",
    "IFLevelIdGreaterThan",
    "IFMyNumArghsLessThan",
    "IFMyNumArghsGreaterThan",
    "IFMyNumCloseArghsLessThan",
    "IFMyNumCloseArghsGreaterThan",
    "IFChrHealthLessThan",
    "IFChrHealthGreaterThan",
    "IFChrWasDamagedSinceLastCheck",
    "IFBondHealthLessThan",
    "IFBondHealthGreaterThan",
    "SetMyMorale",
    "AddToMyMorale",
    "SubtractFromMyMorale",
    "IFMyMoraleLessThan",
    "IFMyMoraleLessThanRandom",
    "SetMyAlertness",
    "AddToMyAlertness",
    "SubtractFromMyAlertness",
    "IFMyAlertnessLessThan",
    "IFMyAlertnessLessThanRandom",
    "SetMyHearingScale",
    "SetMyVisionRange",
    "SetMyGrenadeProbability",
    "SetMyChrNum",
    "SetMyHealthTotal",
    "SetMyArmour",
    "SetMySpeedRating",
    "SetMyArghRating",
    "SetMyAccuracyRating",
    "SetMyFlags2",
    "UnsetMyFlags2",
    "IFMyFlags2Has",
    "SetChrBitfield",
    "UnsetChrBitfield",
    "IFChrBitfieldHas",
    "SetObjectiveBitfield",
    "UnsetObjectiveBitfield",
    "IFObjectiveBitfieldHas",
    "SetMychrflags",
    "UnsetMychrflags",
    "IFMychrflagsHas",
    "SetChrchrflags",
    "UnsetChrchrflags",
    "IFChrchrflagsHas",
    "SetObjectFlags",
    "UnsetObjectFlags",
    "IFObjectFlagsHas",
    "SetObjectFlags2",
    "UnsetObjectFlags2",
    "IFObjectFlags2Has",
    "SetMyChrPreset",
    "SetChrChrPreset",
    "SetMyPadPreset",
    "SetChrPadPreset",
    "PRINT",
    "MyTimerStart",
    "MyTimerReset",
    "MyTimerPause",
    "MyTimerResume",
    "IFMyTimerIsNotRunning",
    "IFMyTimerLessThanTicks",
    "IFMyTimerGreaterThanTicks",
    "HudCountdownShow",
    "HudCountdownHide",
    "HudCountdownSet",
    "HudCountdownStop",
    "HudCountdownStart",
    "IFHudCountdownIsNotRunning",
    "IFHudCountdownLessThan",
    "IFHudCountdownGreaterThan",
    "TRYSpawningChrAtPad",
    "TRYSpawningChrNextToChr",
    "TRYGiveMeItem",
    "TRYGiveMeHat",
    "TRYCloningChr",
    "TextPrintBottom",
    "TextPrintTop",
    "SfxPlay",
    "SfxEmitFromObject",
    "SfxEmitFromPad",
    "SfxSetChannelVolume",
    "SfxFadeChannelVolume",
    "SfxStopChannel",
    "IFSfxChannelVolumeLessThan",
    "VehicleStartPath",
    "VehicleSpeed",
    "AircraftRotorSpeed",
    "IFCameraIsInIntro",
    "IFCameraIsInBondSwirl",
    "TvChangeScreenBank",
    "IFBondInTank",
    "EndLevel",
    "CameraReturnToBond",
    "CameraLookAtBondFromPad",
    "CameraSwitch",
    "IFBondYPosLessThan",
    "BondDisableControl",
    "BondEnableControl",
    "TRYTeleportingChrToPad",
    "ScreenFadeToBlack",
    "ScreenFadeFromBlack",
    "IFScreenFadeCompleted",
    "HideAllChrs",
    "ShowAllChrs",
    "DoorOpenInstant",
    "ChrRemoveItemInHand",
    "IfNumberOfActivePlayersLessThan",
    "IFBondItemTotalAmmoLessThan",
    "BondEquipItem",
    "BondEquipItemCinema",
    "BondSetLockedVelocity",
    "IFObjectInRoomWithPad",
    "IFImFiringAndLockedForward",
    "IFImFiring",
    "SwitchSky",
    "TriggerFadeAndExitLevelOnButtonPress",
    "IFBondIsDead",
    "BondDisableDamageAndPickups",
    "BondHideWeapons",
    "CameraOrbitPad",
    "CreditsRoll",
    "IFCreditsHasCompleted",
    "IFObjectiveAllCompleted",
    "IFFolderActorIsEqual",
    "IFBondDamageAndPickupsDisabled",
    "MusicPlaySlot",
    "MusicStopSlot",
    "TriggerExplosionsAroundBond",
    "IFKilledCiviliansGreaterThan",
    "IFChrWasShotSinceLastCheck",
    "BondKilledInAction",
    "RaiseArms",
    "GasLeakAndFadeFog",
    "ObjectRocketLaunch"};

#define AISTACK_MAX 10

int  top = -1;
char stack[AISTACK_MAX];

typedef struct
{
    int id;
    int used;
    int num0s;
} LLEnum;
LLEnum labels[100];
LLEnum tags[100];
LLEnum keys[100];
LLEnum chrs[100];
LLEnum pads[1000];
LLEnum SubIDs[100];

char  *hasName;

/*PUSH FUNCTION*/
int    push(int item)
{
    if (top == (AISTACK_MAX - 1))
    {
        return 0;
    }
    else
    {
        ++top;
        stack[top] = item;
        return 1;
    }
}

/*POP */
int pop()
{
    if (top == -1)
    {
        return 0;
    }
    else
    {
        --top;
        return stack[top + 1];
    }
}

/*Sniff */
int sniff()
{
    if (top == -1)
    {
        return 0;
    }
    else
    {
        return stack[top];
    }
}
int stackFind(int find)
{
    int i;
    if (top == -1)
    {
        return -1;
    }
    else
    {
        for (i = top; i >= 0; --i)
        {
            if (stack[i] == find) return i;
        }
    }
    return -1;
}
int stackFindAndremove(int find)
{
    int i = stackFind(find);
    if (i >= 0)
    {
        for (; i <= top; i++)
        {
            stack[i] = i < AISTACK_MAX ? stack[i + 1] : 0;
        }
        top--;
        return 1;
    }
    return 0;
}
void displayStack()
{
    int i;

    printf("\nThe label Stack is: ");

    if (top == -1)
    {
        printf("empty");
    }
    else
    {
        for (i = top; i >= 0; --i)
        {
            printf("\n%3d", stack[i]);
        }
    }

    printf("\n---------------\n\n");
}
void _AddLabel(int lbl, void *ptr)
{
    int     i      = 0;
    int     found  = 0;
    LLEnum *labels = ptr;
    if (lbl == 0)
    {
        labels[0].num0s++;
        return;
    }

    while (labels[i].id != 0)
    {
        if (lbl == labels[i].id)
        {
            found = 1;
            labels[i].used++;
        }
        i++;
    }

    if (!found)
    {
        labels[i].id = lbl;
        labels[i].used++;
    }
}
void AddLabel(char lbl)
{
    _AddLabel(lbl, labels);
}
void AddTag(char lbl)
{
    _AddLabel(lbl, tags);
}
void AddKey(char lbl)
{
    _AddLabel(lbl, keys);
}
void AddChr(char lbl)
{
    _AddLabel(lbl, chrs);
}
void AddPad(short lbl)
{
    _AddLabel(lbl, pads);
}
void AddSubID(int lbl)
{
    _AddLabel(lbl, SubIDs);
}

void SortEnums(LLEnum *labels)
{
    int i, j, tempid, tempnum;

    for (i = 0; i < 100; ++i)
    {
        if (labels[i].id != 0)
        {
            for (j = i + 1; j < 100; ++j)
            {
                if (labels[j].id != 0)
                {
                    if (labels[i].id > labels[j].id)
                    {
                        tempid         = labels[i].id;
                        tempnum        = labels[i].used;
                        labels[i].id   = labels[j].id;
                        labels[i].used = labels[j].used;
                        labels[j].id   = tempid;
                        labels[j].used = tempnum;
                    }
                }
            }
        }
    }
}

void printenum(LLEnum labels[], char *enumname)
{
    int i = 0;
    if (labels[0].num0s || labels[i].id)
    {
        printf("    enum %s\n    {\n", enumname);
        if (labels[0].num0s) printf("        %s0, //used %d times\n", enumname, labels[0].num0s);
        while (labels[i].id != 0)
        {
            if (!(enumname == "chr" && ((signed char)labels[i].id < 0 && (signed char)labels[i].id > -9)))
            {
                if ((i > 0 && labels[i - 1].id == labels[i].id - 1) || (i == 0 && labels[0].num0s && labels[0].id == 1))
                {
                    printf("        %s%u, //Used %d times\n", enumname, labels[i].id, labels[i].used);
                }
                else
                {
                    printf("        %s%u = %u, //Used %d times\n", enumname, labels[i].id, labels[i].id, labels[i].used);
                }
            }
            i++;
        }
        printf("    };\n");
    }
}
int chraiitemsize(unsigned char *AIList, int offset)
{
    switch (AIList[offset])
    {
        case AI_GotoNext:
            return sizeof(AiGotoNextRecord);
        case AI_GotoFirst:
            return sizeof(AiGotoFirstRecord);
        case AI_Label:
            return sizeof(AiLabelRecord);
        case AI_Yield:
            return sizeof(AiYieldRecord);
        case AI_EndList:
            return sizeof(AiEndListRecord);
        case AI_SetChrAiList:
            return sizeof(AiSetChrAiListRecord);
        case AI_SetReturnAiList:
            return sizeof(AiSetReturnAiListRecord);
        case AI_Return:
            return sizeof(AiReturnRecord);
        case AI_Stop:
            return sizeof(AiStopRecord);
        case AI_Kneel:
            return sizeof(AiKneelRecord);
        case AI_PlayAnimation:
            return sizeof(AiPlayAnimationRecord);
        case AI_IFPlayingAnimation:
            return sizeof(AiIFPlayingAnimationRecord);
        case AI_PointAtBond:
            return sizeof(AiPointAtBondRecord);
        case AI_LookSurprised:
            return sizeof(AiLookSurprisedRecord);
        case AI_TRYSidestepping:
            return sizeof(AiTRYSidesteppingRecord);
        case AI_TRYSideHopping:
            return sizeof(AiTRYSideHoppingRecord);
        case AI_TRYSideRunning:
            return sizeof(AiTRYSideRunningRecord);
        case AI_TRYFiringWalk:
            return sizeof(AiTRYFiringWalkRecord);
        case AI_TRYFiringRun:
            return sizeof(AiTRYFiringRunRecord);
        case AI_TRYFiringRoll:
            return sizeof(AiTRYFiringRollRecord);
        case AI_TRYFireOrAimAtTarget:
            return sizeof(AiTRYFireOrAimAtTargetRecord);
        case AI_TRYFireOrAimAtTargetKneel:
            return sizeof(AiTRYFireOrAimAtTargetKneelRecord);
        case AI_IFImFiring:
            return sizeof(AiIFImFiringRecord);
        case AI_IFImFiringAndLockedForward:
            return sizeof(AiIFImFiringAndLockedForwardRecord);
        case AI_TRYFireOrAimAtTargetUpdate:
            return sizeof(AiTRYFireOrAimAtTargetUpdateRecord);
        case AI_TRYFacingTarget:
            return sizeof(AiTRYFacingTargetRecord);
        case AI_HitChrWithItem:
            return sizeof(AiHitChrWithItemRecord);
        case AI_ChrHitChr:
            return sizeof(AiChrHitChrRecord);
        case AI_TRYThrowingGrenade:
            return sizeof(AiTRYThrowingGrenadeRecord);
        case AI_TRYDroppingItem:
            return sizeof(AiTRYDroppingItemRecord);
        case AI_RunToPad:
            return sizeof(AiRunToPadRecord);
        case AI_RunToPadPreset:
            return sizeof(AiRunToPadPresetRecord);
        case AI_WalkToPad:
            return sizeof(AiWalkToPadRecord);
        case AI_SprintToPad:
            return sizeof(AiSprintToPadRecord);
        case AI_StartPatrol:
            return sizeof(AiStartPatrolRecord);
        case AI_Surrender:
            return sizeof(AiSurrenderRecord);
        case AI_RemoveMe:
            return sizeof(AiRemoveMeRecord);
        case AI_ChrRemoveInstant:
            return sizeof(AiChrRemoveInstantRecord);
        case AI_TRYTriggeringAlarmAtPad:
            return sizeof(AiTRYTriggeringAlarmAtPadRecord);
        case AI_AlarmOn:
            return sizeof(AiAlarmOnRecord);
        case AI_AlarmOff:
            return sizeof(AiAlarmOffRecord);
        case AI_TRYRunFromBond:
            return sizeof(AiTRYRunFromBondRecord);
        case AI_TRYRunToBond:
            return sizeof(AiTRYRunToBondRecord);
        case AI_TRYWalkToBond:
            return sizeof(AiTRYWalkToBondRecord);
        case AI_TRYSprintToBond:
            return sizeof(AiTRYSprintToBondRecord);
        case AI_TRYFindCover:
            return sizeof(AiTRYFindCoverRecord);
        case AI_TRYRunToChr:
            return sizeof(AiTRYRunToChrRecord);
        case AI_TRYWalkToChr:
            return sizeof(AiTRYWalkToChrRecord);
        case AI_TRYSprintToChr:
            return sizeof(AiTRYSprintToChrRecord);
        case AI_IFImOnPatrolOrStopped:
            return sizeof(AiIFImOnPatrolOrStoppedRecord);
        case AI_IFChrDyingOrDead:
            return sizeof(AiIFChrDyingOrDeadRecord);
        case AI_IFChrDoesNotExist:
            return sizeof(AiIFChrDoesNotExistRecord);
        case AI_IFISeeBond:
            return sizeof(AiIFISeeBondRecord);
        case AI_SetNewRandom:
            return sizeof(AiSetNewRandomRecord);
        case AI_IFRandomLessThan:
            return sizeof(AiIFRandomLessThanRecord);
        case AI_IFRandomGreaterThan:
            return sizeof(AiIFRandomGreaterThanRecord);
        case AI_IFICanHearAlarm:
            return sizeof(AiIFICanHearAlarmRecord);
        case AI_IFAlarmIsOn:
            return sizeof(AiIFAlarmIsOnRecord);
        case AI_IFGasIsLeaking:
            return sizeof(AiIFGasIsLeakingRecord);
        case AI_IFIHeardBond:
            return sizeof(AiIFIHeardBondRecord);
        case AI_IFISeeSomeoneShot:
            return sizeof(AiIFISeeSomeoneShotRecord);
        case AI_IFISeeSomeoneDie:
            return sizeof(AiIFISeeSomeoneDieRecord);
        case AI_IFICouldSeeBond:
            return sizeof(AiIFICouldSeeBondRecord);
        case AI_IFICouldSeeBondsStan:
            return sizeof(AiIFICouldSeeBondsStanRecord);
        case AI_IFIWasShotRecently:
            return sizeof(AiIFIWasShotRecentlyRecord);
        case AI_IFIHeardBondRecently:
            return sizeof(AiIFIHeardBondRecentlyRecord);
        case AI_IFImInRoomWithChr:
            return sizeof(AiIFImInRoomWithChrRecord);
        case AI_IFIveNotBeenSeen:
            return sizeof(AiIFIveNotBeenSeenRecord);
        case AI_IFImOnScreen:
            return sizeof(AiIFImOnScreenRecord);
        case AI_IFMyRoomIsOnScreen:
            return sizeof(AiIFMyRoomIsOnScreenRecord);
        case AI_IFRoomWithPadIsOnScreen:
            return sizeof(AiIFRoomWithPadIsOnScreenRecord);
        case AI_IFImTargetedByBond:
            return sizeof(AiIFImTargetedByBondRecord);
        case AI_IFBondMissedMe:
            return sizeof(AiIFBondMissedMeRecord);
        case AI_IFMyAngleToBondLessThan:
            return sizeof(AiIFMyAngleToBondLessThanRecord);
        case AI_IFMyAngleToBondGreaterThan:
            return sizeof(AiIFMyAngleToBondGreaterThanRecord);
        case AI_IFMyAngleFromBondLessThan:
            return sizeof(AiIFMyAngleFromBondLessThanRecord);
        case AI_IFMyAngleFromBondGreaterThan:
            return sizeof(AiIFMyAngleFromBondGreaterThanRecord);
        case AI_IFMyDistanceToBondLessThanDecimeter:
            return sizeof(AiIFMyDistanceToBondLessThanDecimeterRecord);
        case AI_IFMyDistanceToBondGreaterThanDecimeter:
            return sizeof(AiIFMyDistanceToBondGreaterThanDecimeterRecord);
        case AI_IFChrDistanceToPadLessThanDecimeter:
            return sizeof(AiIFChrDistanceToPadLessThanDecimeterRecord);
        case AI_IFChrDistanceToPadGreaterThanDecimeter:
            return sizeof(AiIFChrDistanceToPadGreaterThanDecimeterRecord);
        case AI_IFMyDistanceToChrLessThanDecimeter:
            return sizeof(AiIFMyDistanceToChrLessThanDecimeterRecord);
        case AI_IFMyDistanceToChrGreaterThanDecimeter:
            return sizeof(AiIFMyDistanceToChrGreaterThanDecimeterRecord);
        case AI_TRYSettingMyPresetToChrWithinDistanceDecimeter:
            return sizeof(AiTRYSettingMyPresetToChrWithinDistanceDecimeterRecord);
        case AI_IFBondDistanceToPadLessThanDecimeter:
            return sizeof(AiIFBondDistanceToPadLessThanDecimeterRecord);
        case AI_IFBondDistanceToPadGreaterThanDecimeter:
            return sizeof(AiIFBondDistanceToPadGreaterThanDecimeterRecord);
        case AI_IFChrInRoomWithPad:
            return sizeof(AiIFChrInRoomWithPadRecord);
        case AI_IFBondInRoomWithPad:
            return sizeof(AiIFBondInRoomWithPadRecord);
        case AI_IFBondCollectedObject:
            return sizeof(AiIFBondCollectedObjectRecord);
        case AI_IFKeyDropped:
            return sizeof(AiIFKeyDroppedRecord);
        case AI_IFItemIsAttachedToObject:
            return sizeof(AiIFItemIsAttachedToObjectRecord);
        case AI_IFBondHasItemEquipped:
            return sizeof(AiIFBondHasItemEquippedRecord);
        case AI_IFObjectExists:
            return sizeof(AiIFObjectExistsRecord);
        case AI_IFObjectNotDestroyed:
            return sizeof(AiIFObjectNotDestroyedRecord);
        case AI_IFObjectWasActivated:
            return sizeof(AiIFObjectWasActivatedRecord);
        case AI_IFBondUsedGadgetOnObject:
            return sizeof(AiIFBondUsedGadgetOnObjectRecord);
        case AI_ActivateObject:
            return sizeof(AiActivateObjectRecord);
        case AI_DestroyObject:
            return sizeof(AiDestroyObjectRecord);
        case AI_DropObject:
            return sizeof(AiDropObjectRecord);
        case AI_ChrDropAllConcealedItems:
            return sizeof(AiChrDropAllConcealedItemsRecord);
        case AI_ChrDropAllHeldItems:
            return sizeof(AiChrDropAllHeldItemsRecord);
        case AI_BondCollectObject:
            return sizeof(AiBondCollectObjectRecord);
        case AI_ChrEquipObject:
            return sizeof(AiChrEquipObjectRecord);
        case AI_MoveObject:
            return sizeof(AiMoveObjectRecord);
        case AI_DoorOpen:
            return sizeof(AiDoorOpenRecord);
        case AI_DoorClose:
            return sizeof(AiDoorCloseRecord);
        case AI_IFDoorStateEqual:
            return sizeof(AiIFDoorStateEqualRecord);
        case AI_IFDoorHasBeenOpenedBefore:
            return sizeof(AiIFDoorHasBeenOpenedBeforeRecord);
        case AI_DoorSetLock:
            return sizeof(AiDoorSetLockRecord);
        case AI_DoorUnsetLock:
            return sizeof(AiDoorUnsetLockRecord);
        case AI_IFDoorLockEqual:
            return sizeof(AiIFDoorLockEqualRecord);
        case AI_IFObjectiveNumComplete:
            return sizeof(AiIFObjectiveNumCompleteRecord);
        case AI_TRYUnknown6e:
            return sizeof(AiTRYUnknown6eRecord);
        case AI_TRYUnknown6f:
            return sizeof(AiTRYUnknown6fRecord);
        case AI_IFGameDifficultyLessThan:
            return sizeof(AiIFGameDifficultyLessThanRecord);
        case AI_IFGameDifficultyGreaterThan:
            return sizeof(AiIFGameDifficultyGreaterThanRecord);
        case AI_IFMissionTimeLessThan:
            return sizeof(AiIFMissionTimeLessThanRecord);
        case AI_IFMissionTimeGreaterThan:
            return sizeof(AiIFMissionTimeGreaterThanRecord);
        case AI_IFSystemPowerTimeLessThan:
            return sizeof(AiIFSystemPowerTimeLessThanRecord);
        case AI_IFSystemPowerTimeGreaterThan:
            return sizeof(AiIFSystemPowerTimeGreaterThanRecord);
        case AI_IFLevelIdLessThan:
            return sizeof(AiIFLevelIdLessThanRecord);
        case AI_IFLevelIdGreaterThan:
            return sizeof(AiIFLevelIdGreaterThanRecord);
        case AI_IFMyNumArghsLessThan:
            return sizeof(AiIFMyNumArghsLessThanRecord);
        case AI_IFMyNumArghsGreaterThan:
            return sizeof(AiIFMyNumArghsGreaterThanRecord);
        case AI_IFMyNumCloseArghsLessThan:
            return sizeof(AiIFMyNumCloseArghsLessThanRecord);
        case AI_IFMyNumCloseArghsGreaterThan:
            return sizeof(AiIFMyNumCloseArghsGreaterThanRecord);
        case AI_IFChrHealthLessThan:
            return sizeof(AiIFChrHealthLessThanRecord);
        case AI_IFChrHealthGreaterThan:
            return sizeof(AiIFChrHealthGreaterThanRecord);
        case AI_IFChrWasDamagedSinceLastCheck:
            return sizeof(AiIFChrWasDamagedSinceLastCheckRecord);
        case AI_IFBondHealthLessThan:
            return sizeof(AiIFBondHealthLessThanRecord);
        case AI_IFBondHealthGreaterThan:
            return sizeof(AiIFBondHealthGreaterThanRecord);
        case AI_SetMyMorale:
            return sizeof(AiSetMyMoraleRecord);
        case AI_AddToMyMorale:
            return sizeof(AiAddToMyMoraleRecord);
        case AI_SubtractFromMyMorale:
            return sizeof(AiSubtractFromMyMoraleRecord);
        case AI_IFMyMoraleLessThan:
            return sizeof(AiIFMyMoraleLessThanRecord);
        case AI_IFMyMoraleLessThanRandom:
            return sizeof(AiIFMyMoraleLessThanRandomRecord);
        case AI_SetMyAlertness:
            return sizeof(AiSetMyAlertnessRecord);
        case AI_AddToMyAlertness:
            return sizeof(AiAddToMyAlertnessRecord);
        case AI_SubtractFromMyAlertness:
            return sizeof(AiSubtractFromMyAlertnessRecord);
        case AI_IFMyAlertnessLessThan:
            return sizeof(AiIFMyAlertnessLessThanRecord);
        case AI_IFMyAlertnessLessThanRandom:
            return sizeof(AiIFMyAlertnessLessThanRandomRecord);
        case AI_SetMyHearingScale:
            return sizeof(AiSetMyHearingScaleRecord);
        case AI_SetMyVisionRange:
            return sizeof(AiSetMyVisionRangeRecord);
        case AI_SetMyGrenadeProbability:
            return sizeof(AiSetMyGrenadeProbabilityRecord);
        case AI_SetMyChrNum:
            return sizeof(AiSetMyChrNumRecord);
        case AI_SetMyHealthTotal:
            return sizeof(AiSetMyHealthTotalRecord);
        case AI_SetMyArmour:
            return sizeof(AiSetMyArmourRecord);
        case AI_SetMySpeedRating:
            return sizeof(AiSetMySpeedRatingRecord);
        case AI_SetMyArghRating:
            return sizeof(AiSetMyArghRatingRecord);
        case AI_SetMyAccuracyRating:
            return sizeof(AiSetMyAccuracyRatingRecord);
        case AI_SetMyFlags2:
            return sizeof(AiSetMyFlags2Record);
        case AI_UnsetMyFlags2:
            return sizeof(AiUnsetMyFlags2Record);
        case AI_IFMyFlags2Has:
            return sizeof(AiIFMyFlags2HasRecord);
        case AI_SetChrBitfield:
            return sizeof(AiSetChrBitfieldRecord);
        case AI_UnsetChrBitfield:
            return sizeof(AiUnsetChrBitfieldRecord);
        case AI_IFChrBitfieldHas:
            return sizeof(AiIFChrBitfieldHasRecord);
        case AI_SetObjectiveBitfield:
            return sizeof(AiSetObjectiveBitfieldRecord);
        case AI_UnsetObjectiveBitfield:
            return sizeof(AiUnsetObjectiveBitfieldRecord);
        case AI_IFObjectiveBitfieldHas:
            return sizeof(AiIFObjectiveBitfieldHasRecord);
        case AI_SetMychrflags:
            return sizeof(AiSetMychrflagsRecord);
        case AI_UnsetMychrflags:
            return sizeof(AiUnsetMychrflagsRecord);
        case AI_IFMychrflagsHas:
            return sizeof(AiIFMychrflagsHasRecord);
        case AI_SetChrchrflags:
            return sizeof(AiSetChrchrflagsRecord);
        case AI_UnsetChrchrflags:
            return sizeof(AiUnsetChrchrflagsRecord);
        case AI_IFChrchrflagsHas:
            return sizeof(AiIFChrchrflagsHasRecord);
        case AI_SetObjectFlags:
            return sizeof(AiSetObjectFlagsRecord);
        case AI_UnsetObjectFlags:
            return sizeof(AiUnsetObjectFlagsRecord);
        case AI_IFObjectFlagsHas:
            return sizeof(AiIFObjectFlagsHasRecord);
        case AI_SetObjectFlags2:
            return sizeof(AiSetObjectFlags2Record);
        case AI_UnsetObjectFlags2:
            return sizeof(AiUnsetObjectFlags2Record);
        case AI_IFObjectFlags2Has:
            return sizeof(AiIFObjectFlags2HasRecord);
        case AI_SetMyChrPreset:
            return sizeof(AiSetMyChrPresetRecord);
        case AI_SetChrChrPreset:
            return sizeof(AiSetChrChrPresetRecord);
        case AI_SetMyPadPreset:
            return sizeof(AiSetMyPadPresetRecord);
        case AI_SetChrPadPreset:
            return sizeof(AiSetChrPadPresetRecord);
        case AI_MyTimerStart:
            return sizeof(AiMyTimerStartRecord);
        case AI_MyTimerReset:
            return sizeof(AiMyTimerResetRecord);
        case AI_MyTimerPause:
            return sizeof(AiMyTimerPauseRecord);
        case AI_MyTimerResume:
            return sizeof(AiMyTimerResumeRecord);
        case AI_IFMyTimerIsNotRunning:
            return sizeof(AiIFMyTimerIsNotRunningRecord);
        case AI_IFMyTimerLessThanTicks:
            return sizeof(AiIFMyTimerLessThanTicksRecord);
        case AI_IFMyTimerGreaterThanTicks:
            return sizeof(AiIFMyTimerGreaterThanTicksRecord);
        case AI_HudCountdownShow:
            return sizeof(AiHudCountdownShowRecord);
        case AI_HudCountdownHide:
            return sizeof(AiHudCountdownHideRecord);
        case AI_HudCountdownSet:
            return sizeof(AiHudCountdownSetRecord);
        case AI_HudCountdownStop:
            return sizeof(AiHudCountdownStopRecord);
        case AI_HudCountdownStart:
            return sizeof(AiHudCountdownStartRecord);
        case AI_IFHudCountdownIsNotRunning:
            return sizeof(AiIFHudCountdownIsNotRunningRecord);
        case AI_IFHudCountdownLessThan:
            return sizeof(AiIFHudCountdownLessThanRecord);
        case AI_IFHudCountdownGreaterThan:
            return sizeof(AiIFHudCountdownGreaterThanRecord);
        case AI_TRYSpawningChrAtPad:
            return sizeof(AiTRYSpawningChrAtPadRecord);
        case AI_TRYSpawningChrNextToChr:
            return sizeof(AiTRYSpawningChrNextToChrRecord);
        case AI_TRYGiveMeItem:
            return sizeof(AiTRYGiveMeItemRecord);
        case AI_TRYGiveMeHat:
            return sizeof(AiTRYGiveMeHatRecord);
        case AI_TRYCloningChr:
            return sizeof(AiTRYCloningChrRecord);
        case AI_TextPrintBottom:
            return sizeof(AiTextPrintBottomRecord);
        case AI_TextPrintTop:
            return sizeof(AiTextPrintTopRecord);
        case AI_SfxPlay:
            return sizeof(AiSfxPlayRecord);
        case AI_SfxEmitFromObject:
            return sizeof(AiSfxEmitFromObjectRecord);
        case AI_SfxEmitFromPad:
            return sizeof(AiSfxEmitFromPadRecord);
        case AI_SfxSetChannelVolume:
            return sizeof(AiSfxSetChannelVolumeRecord);
        case AI_SfxFadeChannelVolume:
            return sizeof(AiSfxFadeChannelVolumeRecord);
        case AI_SfxStopChannel:
            return sizeof(AiSfxStopChannelRecord);
        case AI_IFSfxChannelVolumeLessThan:
            return sizeof(AiIFSfxChannelVolumeLessThanRecord);
        case AI_VehicleStartPath:
            return sizeof(AiVehicleStartPathRecord);
        case AI_VehicleSpeed:
            return sizeof(AiVehicleSpeedRecord);
        case AI_AircraftRotorSpeed:
            return sizeof(AiAircraftRotorSpeedRecord);
        case AI_IFCameraIsInIntro:
            return sizeof(AiIFCameraIsInIntroRecord);
        case AI_IFCameraIsInBondSwirl:
            return sizeof(AiIFCameraIsInBondSwirlRecord);
        case AI_TvChangeScreenBank:
            return sizeof(AiTvChangeScreenBankRecord);
        case AI_IFBondInTank:
            return sizeof(AiIFBondInTankRecord);
        case AI_EndLevel:
            return sizeof(AiEndLevelRecord);
        case AI_CameraReturnToBond:
            return sizeof(AiCameraReturnToBondRecord);
        case AI_CameraLookAtBondFromPad:
            return sizeof(AiCameraLookAtBondFromPadRecord);
        case AI_CameraSwitch:
            return sizeof(AiCameraSwitchRecord);
        case AI_IFBondYPosLessThan:
            return sizeof(AiIFBondYPosLessThanRecord);
        case AI_BondDisableControl:
            return sizeof(AiBondDisableControlRecord);
        case AI_BondEnableControl:
            return sizeof(AiBondEnableControlRecord);
        case AI_TRYTeleportingChrToPad:
            return sizeof(AiTRYTeleportingChrToPadRecord);
        case AI_ScreenFadeToBlack:
            return sizeof(AiScreenFadeToBlackRecord);
        case AI_ScreenFadeFromBlack:
            return sizeof(AiScreenFadeFromBlackRecord);
        case AI_IFScreenFadeCompleted:
            return sizeof(AiIFScreenFadeCompletedRecord);
        case AI_HideAllChrs:
            return sizeof(AiHideAllChrsRecord);
        case AI_ShowAllChrs:
            return sizeof(AiShowAllChrsRecord);
        case AI_DoorOpenInstant:
            return sizeof(AiDoorOpenInstantRecord);
        case AI_ChrRemoveItemInHand:
            return sizeof(AiChrRemoveItemInHandRecord);
        case AI_IfNumberOfActivePlayersLessThan:
            return sizeof(AiIfNumberOfActivePlayersLessThanRecord);
        case AI_IFBondItemTotalAmmoLessThan:
            return sizeof(AiIFBondItemTotalAmmoLessThanRecord);
        case AI_BondEquipItem:
            return sizeof(AiBondEquipItemRecord);
        case AI_BondEquipItemCinema:
            return sizeof(AiBondEquipItemCinemaRecord);
        case AI_BondSetLockedVelocity:
            return sizeof(AiBondSetLockedVelocityRecord);
        case AI_IFObjectInRoomWithPad:
            return sizeof(AiIFObjectInRoomWithPadRecord);
        case AI_SwitchSky:
            return sizeof(AiSwitchSkyRecord);
        case AI_TriggerFadeAndExitLevelOnButtonPress:
            return sizeof(AiTriggerFadeAndExitLevelOnButtonPressRecord);
        case AI_IFBondIsDead:
            return sizeof(AiIFBondIsDeadRecord);
        case AI_BondDisableDamageAndPickups:
            return sizeof(AiBondDisableDamageAndPickupsRecord);
        case AI_BondHideWeapons:
            return sizeof(AiBondHideWeaponsRecord);
        case AI_CameraOrbitPad:
            return sizeof(AiCameraOrbitPadRecord);
        case AI_CreditsRoll:
            return sizeof(AiCreditsRollRecord);
        case AI_IFCreditsHasCompleted:
            return sizeof(AiIFCreditsHasCompletedRecord);
        case AI_IFObjectiveAllCompleted:
            return sizeof(AiIFObjectiveAllCompletedRecord);
        case AI_IFFolderActorIsEqual:
            return sizeof(AiIFFolderActorIsEqualRecord);
        case AI_IFBondDamageAndPickupsDisabled:
            return sizeof(AiIFBondDamageAndPickupsDisabledRecord);
        case AI_MusicPlaySlot:
            return sizeof(AiMusicPlaySlotRecord);
        case AI_MusicStopSlot:
            return sizeof(AiMusicStopSlotRecord);
        case AI_TriggerExplosionsAroundBond:
            return sizeof(AiTriggerExplosionsAroundBondRecord);
        case AI_IFKilledCiviliansGreaterThan:
            return sizeof(AiIFKilledCiviliansGreaterThanRecord);
        case AI_IFChrWasShotSinceLastCheck:
            return sizeof(AiIFChrWasShotSinceLastCheckRecord);
        case AI_BondKilledInAction:
            return sizeof(AiBondKilledInActionRecord);
        case AI_RaiseArms:
            return sizeof(AiRaiseArmsRecord);
        case AI_GasLeakAndFadeFog:
            return sizeof(AiGasLeakAndFadeFogRecord);
        case AI_ObjectRocketLaunch:
            return sizeof(AiObjectRocketLaunchRecord);
        case AI_PRINT:
        {
            int pos = offset + 1;
            while (AIList[pos] != 0)
            {
                ++pos;
            }
            return (pos - offset) + 1;
        }
        default:
            printf("chraiitemsize: unknown type %d!\n", AIList[offset]);
            printf("%d,%d,%d,[%d],%d,%d,%d\n", AIList[offset - 3], AIList[offset - 2], AIList[offset - 1], AIList[offset], AIList[offset + 1], AIList[offset + 2], AIList[offset + 3]);
            return 1;
    }
}

#define EraseCommand printf("\r%*s\r", (int)strlen(AI_CMD_ToString[AiList[Offset]]) + ((int)strlen((char *)tabs) + 1), "")
// #define NOMACROS
/**
 * @brief Parse AI list and print out C Macros
 * @param AiList: Bytestream for ai
 * @param ID: List ID
 */
void ai(unsigned char *AiList, short ID)
{
    int Offset;
    for (Offset = 0;;)
    {
        int hasClosingBrace = FALSE;
        int hasLabel        = FALSE;
        int hasMacro        = FALSE;
        int tabs[AISTACK_MAX + 1]; // 4 char wide string
        int i;
        for (i = 0; i < AISTACK_MAX; i++)
        {
            tabs[i] = 0;
        }
        for (i = 0; i < top + 2 && i < AISTACK_MAX; i++)
        {
            tabs[i] = 0x20202020; // 4 spaces
        }

        printf("%s%s(", (char *)tabs, AI_CMD_ToString[AiList[Offset]]);

        switch (AiList[Offset])
        {
            case AI_GotoFirst: // BYTE(LABEL)
            {
                AiGotoFirstRecord *ai  = &AiList + Offset;
                int                lbl = ai->GOTOLABEL;
#ifndef NOMACROS
                if (lbl == sniff()) // either contnue or loop
                {
                    int found = 0;
                    for (i = Offset; AiList[i] != AI_EndList; i += chraiitemsize(AiList, i))
                    {
                        if (AiList[i] == AI_GotoFirst && AiList[i + 1] == lbl) found++;
                    }

                    EraseCommand;
                    if (found == 1)
                    {
                        pop();
                        for (i = top + 2; i < AISTACK_MAX; i++)
                        {
                            tabs[i] = 0;
                        }
                        printf("%sLOOP(", (char *)tabs);
                    }
                    else
                    {
                        printf("%sCONTINUE(", (char *)tabs);
                    }
                }
#endif
                printf("lbl%d)\n\n", lbl);
                AddLabel(lbl);
                hasClosingBrace = TRUE;
                break;
            }
            case AI_Label: // BYTE(ID)
            {
                AiLabelRecord *ai = &AiList + Offset;

                int            lbl = AiList[Offset + 1];
#ifndef NOMACROS
                Offset += sizeof(AiLabelRecord);
                hasMacro = TRUE;
                if (AiList[Offset] == AI_Yield)
                {
                    int hasLoop = FALSE;
                    Offset += sizeof(AiYieldRecord);
                    for (i = Offset; AiList[i] != AI_EndList; i += chraiitemsize(AiList, i))
                    {
                        if (AiList[i] == AI_GotoFirst && AiList[i + 1] == lbl) hasLoop = TRUE;
                    }
                    if (hasLoop)
                    {
                        EraseCommand;
                        if (AiList[Offset] != AI_GotoFirst)
                        {
                            push(lbl);

                            printf("\n%sDO(", (char *)tabs);
                        }
                        else
                        {
                            printf("\n%sYIELD_FOREVER( lbl%d )\n\n", (char *)tabs, lbl);
                            Offset += sizeof(AiGotoFirstRecord);
                            hasClosingBrace = TRUE;
                        }
                    }
                    else
                    {
                        Offset -= sizeof(AiYieldRecord);
                    }
                }
#endif

                if (!hasClosingBrace) printf(" lbl%d ", lbl);
                AddLabel(lbl);
                break;
            }

            case AI_EndList /*canonical name*/: // /*NONE*/
            {
                AiEndListRecord *ai = &AiList + Offset;
                printf(")\n\n");

                return;
            }
            case AI_SetChrAiList: // BYTE(CHR_NUM), DBYTE(AI_LIST_ID)
            {
                AiSetChrAiListRecord *ai         = &AiList + Offset;
                unsigned short        AI_LIST_ID = CharArrayTo16(AiList, (Offset + 1 + 1));
                signed char           CHR_NUM    = AiList[Offset + 1];

                if (CHR_NUM < 0 && CHR_NUM > -10)
                {
                    EraseCommand;

                    switch (CHR_NUM)
                    {
                        case CHR_SELF:
                        {
                            int found = 0;
                            i         = Offset;
                            while (i > 0)
                            {
                                if (AiList[i] == AI_SetReturnAiList &&
                                    CharArrayTo16(AiList, i + 1) == ID)
                                {
                                    found = 1;
                                    if (i < Offset - sizeof(AiSetReturnAiListRecord)) printf("// tentative, Please confirm\n");
                                    break;
                                }
                                i--;
                            }
                            if (found)
                            {
                                printf("%s%s(", (char *)tabs, "CALL");
                            }
                            else
                            {
                                printf("%s%s(", (char *)tabs, "JumpTo");
                            }
                            break;
                        }
                        case CHR_BOND_CINEMA:
                        {
                            printf("%s%s(", (char *)tabs, "SetBondsAiList");
                            break;
                        }
                        case CHR_CLONE:
                        {
                            printf("%s%s(", (char *)tabs, "SetMyClonesAiList");
                            break;
                        }
                    }
                }
                else
                {
                    if (CHR_NUM < 0 && CHR_NUM > -10)
                    {
                        printf("%s,", CHR_ToString[abs(CHR_NUM)]);
                    }
                    else
                    {
                        printf("chr%hd,", CHR_NUM);
                    }
                }

                if (isGlobalAIListID(AI_LIST_ID))
                {
                    printf("%s", GAILIST_ToString[AI_LIST_ID]);
                }
                else if (isBGAIListID(AI_LIST_ID))
                {
                    printf("bgai_%hd", getBGAIListID(AI_LIST_ID));
                }
                else
                {
                    printf("chrai_%hd", getChrAIListID(AI_LIST_ID));
                }

                if (hasClosingBrace) printf(")\n\n");

                break;
            }
            case AI_SetReturnAiList: // DBYTE(AI_LIST_ID)
            {
                AiSetReturnAiListRecord *ai         = &AiList + Offset;
                unsigned short           AI_LIST_ID = CharArrayTo16(AiList, (Offset + 1));

                if (!(AiList[Offset + sizeof(AiSetReturnAiListRecord)] == AI_SetChrAiList &&
                      AI_LIST_ID == ID &&
                      (signed char)AiList[Offset + sizeof(AiSetReturnAiListRecord) + 1] == CHR_SELF))
                {
                    if (isGlobalAIListID(AI_LIST_ID))
                    {
                        printf("%s", GAILIST_ToString[AI_LIST_ID]);
                    }
                    else if (isBGAIListID(AI_LIST_ID))
                    {
                        printf("bgai_%hd", getBGAIListID(AI_LIST_ID));
                    }
                    else
                    {
                        printf("chrai_%hd", getChrAIListID(AI_LIST_ID));
                    }
                }
                else
                {
                    EraseCommand; // CALL should already be printed
                    hasClosingBrace = TRUE;
                }

                break;
            }
            case AI_Return: // /*NONE*/
            {
                AiReturnRecord *ai = &AiList + Offset;
                AddSubID(ID);
                break;
            }

            case AI_PlayAnimation: // DBYTE(ANIMATION_ID), DBYTE(START_TIME30),DBYTE(END_TIME30), BYTE(BITFIELD), BYTE(INTERPOL_TIME60)
            {
                AiPlayAnimationRecord *ai = &AiList + Offset;
                int                    anim_id, zero;
                short                  startframe, endframe;
                anim_id    = CharArrayTo16(AiList, Offset + 1 + 0);
                startframe = CharArrayTo16(AiList, Offset + 1 + 2);
                endframe   = CharArrayTo16(AiList, Offset + 1 + 4);
                if (startframe == -1 && endframe == -1 && AiList[Offset + 1 + 6] == 6 && AiList[Offset + 1 + 7] == 16)
                {
                    EraseCommand;
                    printf("%s%s(", (char *)tabs, "PlayAnimationSimple");
                    printf("%s", ANIMATIONS_ToString[anim_id]);
                }
                else
                {
                    printf("%s,%d,%d,0x%02X,%d", ANIMATIONS_ToString[anim_id], startframe, endframe, AiList[Offset + 1 + 6], AiList[Offset + 1 + 7]);
                }

                break;
            }
            case AI_IFChrDyingOrDead: // BYTE(CHR_NUM), BYTE(GOTOLABEL)
            {
                AiIFChrDyingOrDeadRecord *ai  = &AiList + Offset;
                signed char               chr = AiList[Offset + 1];
                if (chr == CHR_SELF)
                {
                    EraseCommand;
                    printf("%s%s(", (char *)tabs, "IFImDyingOrDead");
                }
                else
                {
                    if (chr < 0 && chr > -10)
                    {
                        printf("%s,", CHR_ToString[abs(chr)]);
                    }
                    else
                    {
                        printf("chr%hd,", chr);
                        AddChr(chr);
                    }
                }

                hasLabel = TRUE;

                break;
            }
            case AI_IFChrDoesNotExist: // BYTE(CHR_NUM), BYTE(GOTOLABEL)
            {
                AiIFChrDoesNotExistRecord *ai  = &AiList + Offset;
                signed char                chr = AiList[Offset + 1];
                switch (chr)
                {
                    case CHR_SELF:
                    {
                        EraseCommand;
                        printf("%s%s(", (char *)tabs, "IFIDoNotExist");
                        break;
                    }
                    case CHR_CLONE:
                    {
                        EraseCommand;
                        printf("%s%s(", (char *)tabs, "IFMyCloneDoesNotExist");
                        break;
                    }
                    default:
                    {
                        if (chr < 0 && chr > -10)
                        {
                            printf("%s,", CHR_ToString[abs(chr)]);
                        }
                        else
                        {
                            printf("chr%hd,", chr);
                            AddChr(chr);
                        }
                    }
                }

                hasLabel = TRUE;
                break;
            }

            case AI_TRYFireOrAimAtTarget: // DBYTE(BITFIELD), DBYTE(TARGET), BYTE(GOTOLABEL)
            {
                AiTRYFireOrAimAtTargetRecord *ai         = &AiList + Offset;
                int                           targetid   = CharArrayTo16(AiList, Offset + 1 + 2);
                int                           targettype = CharArrayTo16(AiList, Offset + 1 + 0);

                if (targettype == TARGET_BOND)
                {
                    EraseCommand;
                    printf("%s%s(", (char *)tabs, "TRYFireAtBond");
                }
                else if (targettype == TARGET_PAD)
                {
                    EraseCommand;
                    printf("%s%s(", (char *)tabs, "TRYFireAtPad");
                    printf("%d", targetid);
                }
                else if (targettype == TARGET_AIM_ONLY | TARGET_BOND)
                {
                    EraseCommand;
                    printf("%s%s(", (char *)tabs, "TRYAimAtBond");
                }
                else if (targettype == TARGET_AIM_ONLY | TARGET_PAD)
                {
                    EraseCommand;
                    printf("%s%s(", (char *)tabs, "TRYAimAtPad");
                    printf("%d", targetid);
                }
                else
                {
                    printf("%04X,", targettype);
                    printf("chr%d", targetid);
                    AddChr(targetid);
                }
                hasLabel = TRUE;
                break;
            }
            case AI_TRYFireOrAimAtTargetKneel: // DBYTE(BITFIELD), DBYTE(TARGET), BYTE(GOTOLABEL)
            {
                AiTRYFireOrAimAtTargetKneelRecord *ai         = &AiList + Offset;
                int                                targetid   = CharArrayTo16(AiList, Offset + 1 + 2);
                int                                targettype = CharArrayTo16(AiList, Offset + 1 + 0);

                if (targettype == TARGET_BOND)
                {
                    EraseCommand;

                    printf("%s%s(", (char *)tabs, "TRYFireAtBondKneeling");
                }
                else if (targettype == TARGET_PAD)
                {
                    EraseCommand;

                    printf("%s%s(", (char *)tabs, "TRYFireAtPadKneeling");
                    printf("%d", targetid);
                }
                else if (targettype == TARGET_AIM_ONLY | TARGET_BOND)
                {
                    EraseCommand;

                    printf("%s%s(", (char *)tabs, "TRYAimAtBondKneeling");
                }
                else if (targettype == TARGET_AIM_ONLY | TARGET_PAD)
                {
                    EraseCommand;

                    printf("%s%s(", (char *)tabs, "TRYAimAtPadKneeling");
                    printf("%d", targetid);
                }
                else
                {
                    printf("%04X,", targettype);
                    printf("chr%d", targetid);
                    AddChr(targetid);
                }

                hasLabel = TRUE;
                break;
            }

            case AI_TRYFireOrAimAtTargetUpdate: // DBYTE(BITFIELD), DBYTE(TARGET), BYTE(GOTOLABEL)
            {
                AiTRYFireOrAimAtTargetUpdateRecord *ai         = &AiList + Offset;
                int                                 targetid   = CharArrayTo16(AiList, Offset + 1 + 2);
                int                                 targettype = CharArrayTo16(AiList, Offset + 1 + 0);

                if (targettype == TARGET_BOND)
                {
                    EraseCommand;

                    printf("%s%s(", (char *)tabs, "TRYFireAtBondUpdate");
                }
                else if (targettype == TARGET_PAD)
                {
                    EraseCommand;

                    printf("%s%s(", (char *)tabs, "TRYFireAtPadUpdate");
                    printf("%d", targetid);
                }
                else if (targettype == TARGET_AIM_ONLY | TARGET_BOND)
                {
                    EraseCommand;

                    printf("%s%s(", (char *)tabs, "TRYAimAtBondUpdate");
                }
                else if (targettype == TARGET_AIM_ONLY | TARGET_PAD)
                {
                    EraseCommand;

                    printf("%s%s(", (char *)tabs, "TRYAimAtPadUpdate");
                    printf("%d", targetid);
                }
                else
                {
                    printf("%04X,", targettype);
                    printf("chr%d", targetid);
                    AddChr(targetid);
                }

                hasLabel = TRUE;

                break;
            }
            case AI_TRYFacingTarget: // DBYTE(BITFIELD),DBYTE(TARGET),BYTE(GOTOLABEL)
            {
                AiTRYFacingTargetRecord *ai         = &AiList + Offset;
                int                      targetid   = CharArrayTo16(AiList, Offset + 1 + 2);
                int                      targettype = CharArrayTo16(AiList, Offset + 1 + 0);

                if (targettype == TARGET_BOND)
                {
                    EraseCommand;

                    printf("%s%s(", (char *)tabs, "TRYFacingBond");
                }
                else if (targettype == TARGET_PAD)
                {
                    EraseCommand;

                    printf("%s%s(", (char *)tabs, "TRYFacingPad");
                    printf("%d", targetid);
                }
                else
                {
                    printf("%04X,", targettype);
                    printf("chr%d", targetid);
                    AddChr(targetid);
                }

                hasLabel = TRUE;
                break;
            }
            case AI_HitChrWithItem: // BYTE(CHR_NUM),BYTE(PART_NUM),BYTE(ITEM_NUM)
            {
                AiHitChrWithItemRecord *ai      = &AiList + Offset;
                signed char             CHR_NUM = AiList[Offset + 1];

                if (CHR_NUM == CHR_SELF)
                {
                    EraseCommand;
                    printf("%s%s(", (char *)tabs, "HitMeWithItem");
                    printf("%d,%d", AiList[Offset + 2], AiList[Offset + 1 + 2]);
                }
                else
                {
                    if (CHR_NUM < 0 && CHR_NUM > -10)
                    {
                        printf("%s,", CHR_ToString[abs(CHR_NUM)]);
                    }
                    else
                    {
                        printf("chr%hd,", CHR_NUM);
                        AddChr(CHR_NUM);
                    }
                    printf("%s,%s", HITTARGET_ToString[AiList[Offset + 1 + 1]], ITEM_IDS_ToString[AiList[Offset + 1 + 2]]);
                }

                break;
            }
            case AI_ChrHitChr: // BYTE(CHR_NUM),BYTE(CHR_NUM_TARGET),BYTE(PART_NUM)
            {
                AiChrHitChrRecord *ai       = &AiList + Offset;
                signed char        CHR_NUM  = AiList[Offset + 1];
                signed char        CHR_NUM2 = AiList[Offset + 2];
                char               PART_NUM = AiList[Offset + 3];

                if (CHR_NUM == CHR_SELF)
                {
                    EraseCommand;
                    printf("%s%s(", (char *)tabs, "IHitChr");
                    if (CHR_NUM2 < 0 && CHR_NUM2 > -10)
                    {
                        printf("%s,", CHR_ToString[abs(CHR_NUM2)]);
                    }
                    else
                    {
                        printf("chr%hd,", CHR_NUM2);
                        AddChr(CHR_NUM2);
                    }
                }
                else if (CHR_NUM2 == CHR_SELF)
                {
                    EraseCommand;
                    printf("%s%s(", (char *)tabs, "ChrHitMe");
                    if (CHR_NUM < 0 && CHR_NUM > -10)
                    {
                        printf("%s,", CHR_ToString[abs(CHR_NUM)]);
                    }
                    else
                    {
                        printf("chr%hd,", CHR_NUM);
                        AddChr(CHR_NUM);
                    }
                }
                else
                {
                    if (CHR_NUM < 0 && CHR_NUM > -10)
                    {
                        printf("%s,", CHR_ToString[abs(CHR_NUM)]);
                    }
                    else
                    {
                        printf("chr%hd,", CHR_NUM);
                        AddChr(CHR_NUM);
                    }
                    if (CHR_NUM2 < 0 && CHR_NUM2 > -10)
                    {
                        printf("%s,", CHR_ToString[abs(CHR_NUM2)]);
                    }
                    else
                    {
                        printf("chr%hd,", CHR_NUM2);
                        AddChr(CHR_NUM2);
                    }
                }
                printf("%s", HITTARGET_ToString[PART_NUM]);
                break;
            }

            case AI_TRYDroppingItem: // DBYTE(PROP_NUM),BYTE(ITEM_NUM),BYTE(GOTOLABEL)
            {
                AiTRYDroppingItemRecord *ai       = &AiList + Offset;
                unsigned short           modelnum = CharArrayTo16(AiList, Offset + 1 + 0);

                printf("%s,%s", PROP_ToString[modelnum], ITEM_IDS_ToString[AiList[Offset + 1 + 2]]);

                hasLabel = TRUE;

                break;
            }
            case AI_ChrRemoveInstant: // BYTE(CHR_NUM)
            {
                AiChrRemoveInstantRecord *ai  = AiList[Offset];
                signed char               chr = AiList[Offset + 1];

                if (chr == CHR_SELF)
                {
                    EraseCommand;
                    printf("%s%s(", (char *)tabs, "RemoveMeInstantly");
                }
                else
                {
                    if (chr < 0 && chr > -10)
                    {
                        printf("%s,", CHR_ToString[abs(chr)]);
                    }
                    else
                    {
                        printf("chr%hd,", chr);
                        AddChr(chr);
                    }
                }

                break;
            }
            case AI_TRYTriggeringAlarmAtPad: // DBYTE(PAD),BYTE(GOTOLABEL)
            {
                AiTRYTriggeringAlarmAtPadRecord *ai  = &AiList + Offset;
                unsigned short                   pad = CharArrayTo16(AiList, Offset + 1 + 0);

                if (isNotBoundPad(pad))
                {
                    printf("%d,", pad);
                }
                else
                {
                    printf("setBoundPadNum(%d),", getBoundPadNum(pad));
                }

                hasLabel = TRUE;
                break;
            }

            case AI_TRYRunToChr: // BYTE(CHR_NUM), BYTE(GOTOLABEL)
            {
                AiTRYRunToChrRecord *ai  = &AiList + Offset;
                signed char          chr = AiList[Offset + 1];

                if (chr == CHR_PRESET)
                {
                    EraseCommand;
                    printf("%s%s(", (char *)tabs, "TRYRunToPresetChr");
                }
                else
                {
                    if (chr < 0 && chr > -10)
                    {
                        printf("%s,", CHR_ToString[abs(chr)]);
                    }
                    else
                    {
                        printf("chr%hd,", chr);
                        AddChr(chr);
                    }
                }

                hasLabel = TRUE;
                break;
            }
            case AI_TRYWalkToChr: // BYTE(CHR_NUM), BYTE(GOTOLABEL)
            {
                AiTRYWalkToChrRecord *ai  = &AiList + Offset;
                signed char           chr = AiList[Offset + 1];

                if (chr == CHR_PRESET)
                {
                    EraseCommand;
                    printf("%s%s(", (char *)tabs, "TRYWalkToPresetChr");
                }
                else
                {
                    if (chr < 0 && chr > -10)
                    {
                        printf("%s,", CHR_ToString[abs(chr)]);
                    }
                    else
                    {
                        printf("chr%hd,", chr);
                        AddChr(chr);
                    }
                }

                hasLabel = TRUE;

                break;
            }
            case AI_TRYSprintToChr: // BYTE(CHR_NUM), BYTE(GOTOLABEL)
            {
                AiTRYSprintToChrRecord *ai  = &AiList + Offset;
                signed char             chr = AiList[Offset + 1];

                if (chr == CHR_PRESET)
                {
                    EraseCommand;
                    printf("%s%s(", (char *)tabs, "TRYSprintToPresetChr");
                }
                else
                {
                    if (chr < 0 && chr > -10)
                    {
                        printf("%s,", CHR_ToString[abs(chr)]);
                    }
                    else
                    {
                        printf("chr%hd,", chr);
                        AddChr(chr);
                    }
                }
                hasLabel = TRUE;

                break;
            }
            case AI_SetNewRandom:
            {
                AiSetNewRandomRecord *ai = &AiList + Offset;
                if (AiList[Offset + 1] == AI_IFRandomLessThan || AiList[Offset + 1] == AI_IFRandomGreaterThan)
                {
                    EraseCommand;
                    hasClosingBrace = TRUE;
                }
                break;
            }
            case AI_IFRandomLessThan: // BYTE(BYTE), BYTE(GOTOLABEL)
            {
                AiIFRandomLessThanRecord *ai = &AiList + Offset;
                if (AiList[Offset - 1] == AI_SetNewRandom)
                {
                    EraseCommand;
                    printf("%s%s(", (char *)tabs, "IFNewRandomLessThan");
                }
                hasLabel = TRUE;
                break;
            }
            case AI_IFRandomGreaterThan: // BYTE(BYTE), BYTE(GOTOLABEL)
            {
                AiIFRandomGreaterThanRecord *ai = &AiList + Offset;
                if (AiList[Offset - 1] == AI_SetNewRandom)
                {
                    EraseCommand;
                    printf("%s%s(", (char *)tabs, "IFNewRandomGreaterThan");
                }
                hasLabel = TRUE;
                break;
            }
            case AI_RunToPad: // DBYTE(PAD)
            {
                AiRunToPadRecord *ai  = &AiList + Offset;
                unsigned short    pad = CharArrayTo16(AiList, Offset + 1 + 0);

                if (isNotBoundPad(pad))
                {
                    printf("%d", pad);
                }
                else
                {
                    printf("setBoundPadNum(%d)", getBoundPadNum(pad));
                }

                break;
            }

            case AI_WalkToPad: // DBYTE(PAD)
            {
                AiWalkToPadRecord *ai  = &AiList + Offset;
                unsigned short     pad = CharArrayTo16(AiList, Offset + 1 + 0);

                if (isNotBoundPad(pad))
                {
                    printf("%d", pad);
                }
                else
                {
                    printf("setBoundPadNum(%d)", getBoundPadNum(pad));
                }

                break;
            }
            case AI_SprintToPad: // DBYTE(PAD)
            {
                AiSprintToPadRecord *ai  = &AiList + Offset;
                unsigned short       pad = CharArrayTo16(AiList, Offset + 1 + 0);

                if (isNotBoundPad(pad))
                {
                    printf("%d", pad);
                }
                else
                {
                    printf("setBoundPadNum(%d)", getBoundPadNum(pad));
                }

                break;
            }
            case AI_StartPatrol: // BYTE(PATH_NUM)
            {
                AiStartPatrolRecord *ai = &AiList + Offset;
                printf("%d", AiList[Offset + 1]);

                break;
            }

            case AI_IFImInRoomWithChr: // BYTE(CHR_NUM), BYTE(GOTOLABEL)
            {
                AiIFImInRoomWithChrRecord *ai  = &AiList + Offset;
                signed char                chr = AiList[Offset + 1];

                if (chr < 0 && chr > -10)
                {
                    printf("%s,", CHR_ToString[abs(chr)]);
                }
                else
                {
                    printf("chr%hd,", chr);
                    AddChr(chr);
                }
                hasLabel = TRUE;
                break;
            }

            case AI_IFRoomWithPadIsOnScreen: // DBYTE(PAD), BYTE(GOTOLABEL)
            {
                AiIFRoomWithPadIsOnScreenRecord *ai  = &AiList + Offset;
                unsigned short                   pad = CharArrayTo16(AiList, Offset + 1 + 0);

                if (isNotBoundPad(pad))
                {
                    printf("%d,", pad);
                }
                else
                {
                    printf("setBoundPadNum(%d),", getBoundPadNum(pad));
                }

                hasLabel = TRUE;
                break;
            }

            case AI_IFMyAngleToBondLessThan: // BYTE(ANGLE), BYTE(GOTOLABEL)
            {
                AiIFMyAngleToBondLessThanRecord *ai = &AiList + Offset;
                EraseCommand;
                printf("%s%sDeg(", (char *)tabs, AI_CMD_ToString[AiList[Offset]]);
                printf("%f,", RadToDeg(ByteToRadian((AiList[Offset + 1 + 0]))));

                hasLabel = TRUE;

                break;
            }
            case AI_IFMyAngleToBondGreaterThan: // BYTE(ANGLE), BYTE(GOTOLABEL)
            {
                AiIFMyAngleToBondGreaterThanRecord *ai = &AiList + Offset;
                EraseCommand;

                printf("%s%sDeg(", (char *)tabs, AI_CMD_ToString[AiList[Offset]]);
                printf("%f,", RadToDeg(ByteToRadian((AiList[Offset + 1 + 0]))));
                hasLabel = TRUE;

                break;
            }
            case AI_IFMyAngleFromBondLessThan: // BYTE(ANGLE), BYTE(GOTOLABEL)
            {
                AiIFMyAngleFromBondLessThanRecord *ai = &AiList + Offset;
                EraseCommand;

                printf("%s%sDeg(", (char *)tabs, AI_CMD_ToString[AiList[Offset]]);
                printf("%f,", RadToDeg(ByteToRadian((AiList[Offset + 1 + 0]))));
                hasLabel = TRUE;

                break;
            }
            case AI_IFMyAngleFromBondGreaterThan: // BYTE(ANGLE), BYTE(GOTOLABEL)
            {
                AiIFMyAngleFromBondGreaterThanRecord *ai = &AiList + Offset;
                EraseCommand;

                printf("%s%sDeg(", (char *)tabs, AI_CMD_ToString[AiList[Offset]]);
                printf("%f,", RadToDeg(ByteToRadian((AiList[Offset + 1 + 0]))));

                hasLabel = TRUE;
                break;
            }
            case AI_IFMyDistanceToBondLessThanDecimeter: // DBYTE(DISTANCE), BYTE(GOTOLABEL)
            {
                AiIFMyDistanceToBondLessThanDecimeterRecord *ai       = &AiList + Offset;
                float                                        distance = CharArrayTo16(AiList, Offset + 1 + 0) * 10.0f;
                EraseCommand;

                printf("%s%s(", (char *)tabs, "IFMyDistanceToBondLessThanMeter");
                printf("%f,", distance / 100);
                hasLabel = TRUE;

                break;
            }
            case AI_IFMyDistanceToBondGreaterThanDecimeter: // DBYTE(DISTANCE), BYTE(GOTOLABEL)
            {
                AiIFMyDistanceToBondGreaterThanDecimeterRecord *ai       = &AiList + Offset;
                float                                           distance = CharArrayTo16(AiList, Offset + 1 + 0) * 10.0f;
                EraseCommand;

                printf("%s%s(", (char *)tabs, "IFMyDistanceToBondGreaterThanMeter");
                printf("%f,", distance / 100);

                hasLabel = TRUE;
                break;
            }
            case AI_IFChrDistanceToPadLessThanDecimeter: // BYTE(CHR_NUM), DBYTE(DISTANCE), DBYTE(PAD), BYTE(GOTOLABEL)
            {
                AiIFChrDistanceToPadLessThanDecimeterRecord *ai    = &AiList + Offset;
                unsigned short                               pad   = CharArrayTo16(AiList, Offset + 1 + 3);
                float                                        value = CharArrayTo16(AiList, Offset + 1 + 1) * 10.0f;
                signed char                                  chr   = AiList[Offset + 1];

                EraseCommand;

                if (chr == CHR_SELF)
                {
                    printf("%s%s(", (char *)tabs, "IFMyDistanceToPadLessThanMeter");
                }
                else
                {
                    printf("%s%s(", (char *)tabs, "IFChrDistanceToPadLessThanMeter");
                    if (chr < 0 && chr > -10)
                    {
                        printf("%s,", CHR_ToString[abs(chr)]);
                    }
                    else
                    {
                        printf("chr%hd,", chr);
                        AddChr(chr);
                    }
                }
                printf("%f,", value / 100);
                if (isNotBoundPad(pad))
                {
                    printf("%d,", pad);
                }
                else
                {
                    printf("setBoundPadNum(%d),", getBoundPadNum(pad));
                }

                hasLabel = TRUE;
                break;
            }
            case AI_IFChrDistanceToPadGreaterThanDecimeter: // BYTE(CHR_NUM), DBYTE(DISTANCE), DBYTE(PAD), BYTE(GOTOLABEL)
            {
                AiIFChrDistanceToPadGreaterThanDecimeterRecord *ai    = &AiList + Offset;
                unsigned short                                  pad   = CharArrayTo16(AiList, Offset + 1 + 3);
                float                                           value = CharArrayTo16(AiList, Offset + 1 + 1) * 10.0f;
                signed char                                     chr   = AiList[Offset + 1];

                EraseCommand;

                if ((signed char)AiList[Offset + 1 + 0] == CHR_SELF)
                {
                    printf("%s%s(", (char *)tabs, "IFMyDistanceToPadGreaterThanMeter");
                }
                else
                {
                    printf("%s%s(", (char *)tabs, "IFChrDistanceToPadGreaterThanMeter");
                    if (chr < 0 && chr > -10)
                    {
                        printf("%s,", CHR_ToString[abs(chr)]);
                    }
                    else
                    {
                        printf("chr%hd,", chr);
                        AddChr(chr);
                    }
                }
                printf("%f,", value / 100);
                if (isNotBoundPad(pad))
                {
                    printf("%d,", pad);
                }
                else
                {
                    printf("setBoundPadNum(%d),", getBoundPadNum(pad));
                }
                hasLabel = TRUE;
                break;
            }
            case AI_IFMyDistanceToChrLessThanDecimeter: // DBYTE(DISTANCE), BYTE(CHR_NUM), BYTE(GOTOLABEL)
            {
                AiIFMyDistanceToChrLessThanDecimeterRecord *ai     = &AiList + Offset;
                float                                       cutoff = CharArrayTo16(AiList, Offset + 1 + 0) * 10.0f;
                signed char                                 chr    = AiList[Offset + 3];
                EraseCommand;

                printf("%s%s(", (char *)tabs, "AIFMyDistanceToChrLessThanMeter");
                printf("%f,", cutoff / 100);
                if (chr < 0 && chr > -10)
                {
                    printf("%s,", CHR_ToString[abs(chr)]);
                }
                else
                {
                    printf("chr%hd,", chr);
                    AddChr(chr);
                }

                hasLabel = TRUE;
                break;
            }
            case AI_IFMyDistanceToChrGreaterThanDecimeter: // DBYTE(DISTANCE), BYTE(CHR_NUM), BYTE(GOTOLABEL)
            {
                AiIFMyDistanceToChrGreaterThanDecimeterRecord *ai     = &AiList + Offset;
                float                                          cutoff = CharArrayTo16(AiList, Offset + 1 + 0) * 10.0f;
                signed char                                    chr    = AiList[Offset + 3];

                EraseCommand;

                printf("%s%s(", (char *)tabs, "AIFMyDistanceToChrGreaterThanMeter");
                printf("%f,", cutoff / 100);
                if (chr < 0 && chr > -10)
                {
                    printf("%s,", CHR_ToString[abs(chr)]);
                }
                else
                {
                    printf("chr%hd,", chr);
                    AddChr(chr);
                }

                hasLabel = TRUE;
                break;
            }
            case AI_TRYSettingMyPresetToChrWithinDistanceDecimeter: // DBYTE(DISTANCE), BYTE(GOTOLABEL)
            {
                AiTRYSettingMyPresetToChrWithinDistanceDecimeterRecord *ai       = &AiList + Offset;
                float                                                   distance = CharArrayTo16(AiList, Offset + 1 + 0) * 10.0f;
                EraseCommand;

                printf("%s%s(", (char *)tabs, "TRYSettingMyPresetToChrWithinDistanceMeter");
                printf("%f,", distance / 100);

                hasLabel = TRUE;
                break;
            }
            case AI_IFBondDistanceToPadLessThanDecimeter: // DBYTE(DISTANCE), DBYTE(PAD), BYTE(GOTOLABEL)
            {
                AiIFBondDistanceToPadLessThanDecimeterRecord *ai    = &AiList + Offset;
                unsigned short                                pad   = CharArrayTo16(AiList, Offset + 1 + 2);
                float                                         value = CharArrayTo16(AiList, Offset + 1 + 0) * 10.0f;
                EraseCommand;

                printf("%s%s(", (char *)tabs, "IFBondDistanceToPadLessThanMeter");
                printf("%f,", value / 100);

                if (isNotBoundPad(pad))
                {
                    printf("%d,", pad);
                }
                else
                {
                    printf("setBoundPadNum(%d),", getBoundPadNum(pad));
                }

                hasLabel = TRUE;
                break;
            }
            case AI_IFBondDistanceToPadGreaterThanDecimeter: // DBYTE(DISTANCE), DBYTE(PAD), BYTE(GOTOLABEL)
            {
                AiIFBondDistanceToPadGreaterThanDecimeterRecord *ai    = &AiList + Offset;
                unsigned short                                   pad   = CharArrayTo16(AiList, Offset + 1 + 2);
                float                                            value = CharArrayTo16(AiList, Offset + 1 + 0) * 10.0f;
                EraseCommand;

                printf("%s%s(", (char *)tabs, "IFBondDistanceToPadGreaterThanMeter");
                printf("%f,", value / 100);

                if (isNotBoundPad(pad))
                {
                    printf("%d,", pad);
                }
                else
                {
                    printf("setBoundPadNum(%d),", getBoundPadNum(pad));
                }

                hasLabel = TRUE;
                break;
            }
            case AI_IFChrInRoomWithPad: // BYTE(CHR_NUM), DBYTE(PAD), BYTE(GOTOLABEL)
            {
                AiIFChrInRoomWithPadRecord *ai  = &AiList + Offset;
                unsigned short              pad = CharArrayTo16(AiList, Offset + 1 + 1);
                if ((signed char)AiList[Offset + 1 + 0] == CHR_SELF)
                {
                    EraseCommand;
                    printf("%s%s(", (char *)tabs, "IFImInRoomWithPad");
                }
                else
                {
                    printf("%d,", AiList[Offset + 1 + 0]);
                }
                if (isNotBoundPad(pad))
                {
                    printf("%d,", pad);
                }
                else
                {
                    printf("setBoundPadNum(%d),", getBoundPadNum(pad));
                }

                hasLabel = TRUE;
                break;
            }
            case AI_IFBondInRoomWithPad: // DBYTE(PAD), BYTE(GOTOLABEL)
            {
                AiIFBondInRoomWithPadRecord *ai  = &AiList + Offset;
                unsigned short               pad = CharArrayTo16(AiList, Offset + 1 + 0);

                if (isNotBoundPad(pad))
                {
                    printf("%d,", pad);
                }
                else
                {
                    printf("setBoundPadNum(%d),", getBoundPadNum(pad));
                }

                hasLabel = TRUE;
                break;
            }
            case AI_IFBondCollectedObject: // BYTE(OBJECT_TAG), BYTE(GOTOLABEL)
            {
                AiIFBondCollectedObjectRecord *ai = &AiList + Offset;
                printf("tag%d,", AiList[Offset + 1]);

                AddTag(AiList[Offset + 1]);

                hasLabel = TRUE;

                break;
            }
            case AI_IFKeyDropped: // BYTE(KEY_ID), BYTE(GOTOLABEL)
            {
                AiIFKeyDroppedRecord *ai = &AiList + Offset;
                printf("key%d,", AiList[Offset + 1]);

                AddKey(AiList[Offset + 1]);
                hasLabel = TRUE;

                break;
            }
            case AI_IFItemIsAttachedToObject: // BYTE(ITEM_NUM), BYTE(OBJECT_TAG), BYTE(GOTOLABEL)
            {
                AiIFItemIsAttachedToObjectRecord *ai = &AiList + Offset;
                printf("%s,tag%d,", ITEM_IDS_ToString[AiList[Offset + 1]], AiList[Offset + 2]);

                hasLabel = TRUE;
                AddTag(AiList[Offset + 2]);

                break;
            }
            case AI_IFBondHasItemEquipped: // BYTE(ITEM_NUM), BYTE(GOTOLABEL)
            {
                AiIFBondHasItemEquippedRecord *ai = &AiList + Offset;
                printf("%s,", ITEM_IDS_ToString[AiList[Offset + 1]]);

                hasLabel = TRUE;
                break;
            }
            case AI_IFObjectExists: // BYTE(OBJECT_TAG), BYTE(GOTOLABEL)
            {
                AiIFObjectExistsRecord *ai = &AiList + Offset;
                printf("tag%d,", AiList[Offset + 1]);

                hasLabel = TRUE;
                AddTag(AiList[Offset + 1]);
                break;
            }
            case AI_IFObjectNotDestroyed: // BYTE(OBJECT_TAG), BYTE(GOTOLABEL)
            {
                AiIFObjectNotDestroyedRecord *ai = &AiList + Offset;
                printf("tag%d,", AiList[Offset + 1]);

                hasLabel = TRUE;
                AddTag(AiList[Offset + 1]);

                break;
            }
            case AI_IFObjectWasActivated: // BYTE(OBJECT_TAG), BYTE(GOTOLABEL)
            {
                AiIFObjectWasActivatedRecord *ai = &AiList + Offset;
                printf("tag%d,", AiList[Offset + 1]);

                hasLabel = TRUE;
                AddTag(AiList[Offset + 1]);

                break;
            }
            case AI_IFBondUsedGadgetOnObject: // BYTE(OBJECT_TAG), BYTE(GOTOLABEL)
            {
                AiIFBondUsedGadgetOnObjectRecord *ai = &AiList + Offset;
                printf("tag%d,", AiList[Offset + 1]);

                hasLabel = TRUE;
                AddTag(AiList[Offset + 1]);

                break;
            }
            case AI_ActivateObject: // BYTE(OBJECT_TAG)
            {
                AiActivateObjectRecord *ai = &AiList + Offset;
                printf("tag%d", AiList[Offset + 1]);

                break;
            }
            case AI_DestroyObject: // BYTE(OBJECT_TAG)
            {
                AiDestroyObjectRecord *ai = &AiList + Offset;
                printf("tag%d", AiList[Offset + 1]);

                AddTag(AiList[Offset + 1]);

                break;
            }
            case AI_DropObject: // BYTE(OBJECT_TAG)
            {
                AiDropObjectRecord *ai = &AiList + Offset;
                printf("tag%d", AiList[Offset + 1]);
                AddTag(AiList[Offset + 1]);

                break;
            }
            case AI_ChrDropAllConcealedItems: // BYTE(CHR_NUM)
            {
                AiChrDropAllConcealedItemsRecord *ai = &AiList + Offset;
                printf("chr%d", AiList[Offset + 1]);
                AddChr(AiList[Offset + 1]);

                break;
            }
            case AI_ChrDropAllHeldItems: // BYTE(CHR_NUM)
            {
                AiChrDropAllHeldItemsRecord *ai = &AiList + Offset;
                printf("chr%d", AiList[Offset + 1]);
                AddChr(AiList[Offset + 1]);

                break;
            }
            case AI_BondCollectObject: // BYTE(OBJECT_TAG)
            {
                AiBondCollectObjectRecord *ai = &AiList + Offset;
                printf("tag%d", AiList[Offset + 1]);
                AddTag(AiList[Offset + 1]);

                break;
            }
            case AI_ChrEquipObject: // BYTE(OBJECT_TAG), BYTE(CHR_NUM)
            {
                AiChrEquipObjectRecord *ai = &AiList + Offset;
                printf("tag%d,chr%d", AiList[Offset + 1], AiList[Offset + 2]);
                AddTag(AiList[Offset + 1]);
                AddChr(AiList[Offset + 2]);

                break;
            }
            case AI_MoveObject: // BYTE(OBJECT_TAG), DBYTE(PAD)
            {
                AiMoveObjectRecord *ai  = &AiList + Offset;
                unsigned short      pad = CharArrayTo16(AiList, Offset + 1 + 1);

                printf("tag%d,", AiList[Offset + 1]);

                if (isNotBoundPad(pad))
                {
                    printf("%d", pad);
                }
                else
                {
                    printf("setBoundPadNum(%d)", getBoundPadNum(pad));
                }
                AddTag(AiList[Offset + 1]);
                AddPad(pad);

                break;
            }
            case AI_DoorOpen: // BYTE(OBJECT_TAG)
            {
                AiDoorOpenRecord *ai = &AiList + Offset;
                printf("tag%d", AiList[Offset + 1]);
                AddTag(AiList[Offset + 1]);

                break;
            }
            case AI_DoorClose: // BYTE(OBJECT_TAG)
            {
                AiDoorCloseRecord *ai = &AiList + Offset;
                printf("tag%d", AiList[Offset + 1]);
                AddTag(AiList[Offset + 1]);

                break;
            }
            case AI_IFDoorStateEqual: // BYTE(OBJECT_TAG), BYTE(DOOR_STATE), BYTE(GOTOLABEL)
            {
                AiIFDoorStateEqualRecord *ai = &AiList + Offset;
                printf("tag%d,%s,", AiList[Offset + 1], DOORSTATE_ToString[AiList[Offset + 2]]);

                hasLabel = TRUE;
                AddTag(AiList[Offset + 1]);

                break;
            }
            case AI_IFDoorHasBeenOpenedBefore: // BYTE(OBJECT_TAG), BYTE(GOTOLABEL)
            {
                AiIFDoorHasBeenOpenedBeforeRecord *ai = &AiList + Offset;
                printf("tag%d,", AiList[Offset + 1]);

                hasLabel = TRUE;
                AddTag(AiList[Offset + 1]);

                break;
            }
            case AI_DoorSetLock: // BYTE(OBJECT_TAG), BYTE(LOCK_FLAG)
            {
                AiDoorSetLockRecord *ai = &AiList + Offset;
                printf("tag%d,%d", AiList[Offset + 1], AiList[Offset + 2]);
                AddTag(AiList[Offset + 1]);

                break;
            }
            case AI_DoorUnsetLock: // BYTE(OBJECT_TAG), BYTE(LOCK_FLAG)
            {
                AiDoorUnsetLockRecord *ai = &AiList + Offset;
                printf("tag%d,%d", AiList[Offset + 1], AiList[Offset + 2]);
                AddTag(AiList[Offset + 1]);

                break;
            }
            case AI_IFDoorLockEqual: // BYTE(OBJECT_TAG), BYTE(LOCK_FLAG), BYTE(GOTOLABEL)
            {
                AiIFDoorLockEqualRecord *ai = &AiList + Offset;
                printf("tag%d,%d,%d", AiList[Offset + 1], AiList[Offset + 2], AiList[Offset + 3]);
                AddTag(AiList[Offset + 1]);

                break;
            }
            case AI_IFObjectiveNumComplete: // BYTE(OBJ_NUM), BYTE(GOTOLABEL)
            {
                AiIFObjectiveNumCompleteRecord *ai = &AiList + Offset;
                printf("%d,", AiList[Offset + 1]);

                hasLabel = TRUE;
                break;
            }
            case AI_TRYUnknown6e: // BYTE(UNKNOWN_FLAG), BYTE(GOTOLABEL)
            {
                AiTRYUnknown6eRecord *ai = &AiList + Offset;
                printf("%d,", AiList[Offset + 1]);

                hasLabel = TRUE;
                break;
            }
            case AI_TRYUnknown6f: // BYTE(UNKNOWN_FLAG), BYTE(GOTOLABEL)
            {
                AiTRYUnknown6fRecord *ai = &AiList + Offset;
                printf("%d,", AiList[Offset + 1]);

                hasLabel = TRUE;
                break;
            }
            case AI_IFMyNumArghsLessThan: // BYTE(HIT_NUM), BYTE(GOTOLABEL)
            {
                AiIFMyNumArghsLessThanRecord *ai = &AiList + Offset;
                printf("%d,", AiList[Offset + 1]);

                hasLabel = TRUE;
                break;
            }
            case AI_IFMyNumArghsGreaterThan: // BYTE(HIT_NUM), BYTE(GOTOLABEL)
            {
                AiIFMyNumArghsGreaterThanRecord *ai = &AiList + Offset;
                printf("%d,", AiList[Offset + 1]);
                hasLabel = TRUE;
                break;
            }
            case AI_IFMyNumCloseArghsLessThan: // BYTE(MISSED_NUM), BYTE(GOTOLABEL)
            {
                AiIFMyNumCloseArghsLessThanRecord *ai = &AiList + Offset;
                printf("%d,", AiList[Offset + 1]);

                hasLabel = TRUE;
                break;
            }
            case AI_IFMyNumCloseArghsGreaterThan: // BYTE(MISSED_NUM), BYTE(GOTOLABEL)
            {
                AiIFMyNumCloseArghsGreaterThanRecord *ai = &AiList + Offset;
                printf("%d,", AiList[Offset + 1]);

                hasLabel = TRUE;
                break;
            }
            case AI_IFChrHealthLessThan: // BYTE(CHR_NUM), BYTE(HEALTH), BYTE(GOTOLABEL)
            {
                AiIFChrHealthLessThanRecord *ai    = &AiList + Offset;
                float                        value = (AiList[Offset + 1 + 1]) * 0.1f;

                printf("%d,%f,", AiList[Offset + 1], value * 10);

                hasLabel = TRUE;
                AddChr(AiList[Offset + 1]);

                break;
            }
            case AI_IFChrHealthGreaterThan: // BYTE(CHR_NUM), BYTE(HEALTH), BYTE(GOTOLABEL)
            {
                AiIFChrHealthGreaterThanRecord *ai    = &AiList + Offset;
                float                           value = (AiList[Offset + 1 + 1]) * 0.1f;

                printf("%d,%f,", AiList[Offset + 1], value * 10);

                hasLabel = TRUE;
                AddChr(AiList[Offset + 1]);

                break;
            }
            case AI_IFChrWasDamagedSinceLastCheck: // BYTE(CHR_NUM), BYTE(GOTOLABEL)
            {
                AiIFChrWasDamagedSinceLastCheckRecord *ai = &AiList + Offset;
                printf("%d,", AiList[Offset + 1]);

                hasLabel = TRUE;
                AddChr(AiList[Offset + 1]);

                break;
            }
            case AI_IFBondHealthLessThan: // BYTE(HEALTH), BYTE(GOTOLABEL)
            {
                AiIFBondHealthLessThanRecord *ai  = &AiList + Offset;
                float                         val = (AiList[Offset + 1 + 0]) / 255.0f;

                printf("%f,", val * 255);

                hasLabel = TRUE;
                break;
            }
            case AI_IFBondHealthGreaterThan: // BYTE(HEALTH), BYTE(GOTOLABEL)
            {
                AiIFBondHealthGreaterThanRecord *ai  = &AiList + Offset;
                float                            val = (AiList[Offset + 1 + 0]) / 255.0f;

                printf("%f,", val * 255);

                hasLabel = TRUE;
                break;
            }
            case AI_IFGameDifficultyLessThan: // BYTE(DIFICULTY_ID), BYTE(GOTOLABEL)
            {
                AiIFGameDifficultyLessThanRecord *ai = &AiList + Offset;
                printf("%d,", AiList[Offset + 1]);

                hasLabel = TRUE;
                break;
            }
            case AI_IFGameDifficultyGreaterThan: // BYTE(DIFICULTY_ID), BYTE(GOTOLABEL)
            {
                AiIFGameDifficultyGreaterThanRecord *ai = &AiList + Offset;
                printf("%d,", AiList[Offset + 1]);

                hasLabel = TRUE;
                break;
            }
            case AI_IFMissionTimeLessThan: // DBYTE(SECONDS), BYTE(GOTOLABEL)
            {
                AiIFMissionTimeLessThanRecord *ai     = &AiList + Offset;
                float                          target = CharArrayTo16(AiList, Offset + 1 + 0);

                printf("%f,", target);

                hasLabel = TRUE;
                break;
            }
            case AI_IFMissionTimeGreaterThan: // DBYTE(SECONDS), BYTE(GOTOLABEL)
            {
                AiIFMissionTimeGreaterThanRecord *ai     = &AiList + Offset;
                float                             target = CharArrayTo16(AiList, Offset + 1 + 0);

                printf("%f,", target);

                hasLabel = TRUE;
                break;
            }
            case AI_IFSystemPowerTimeLessThan: // DBYTE(MINUTES), BYTE(GOTOLABEL)
            {
                AiIFSystemPowerTimeLessThanRecord *ai     = &AiList + Offset;
                float                              target = CharArrayTo16(AiList, Offset + 1 + 0);

                printf("%f,", target);

                break;
            }
            case AI_IFSystemPowerTimeGreaterThan: // DBYTE(MINUTES), BYTE(GOTOLABEL)
            {
                AiIFSystemPowerTimeGreaterThanRecord *ai     = &AiList + Offset;
                float                                 target = CharArrayTo16(AiList, Offset + 1 + 0);

                printf("%f,", target);

                hasLabel = TRUE;
                break;
            }
            case AI_IFLevelIdLessThan: // BYTE(LEVEL_ID), BYTE(GOTOLABEL)
            {
                AiIFLevelIdLessThanRecord *ai = &AiList + Offset;
                printf("%s,", LEVELID_ToString[AiList[Offset + 1] + 1]);

                hasLabel = TRUE;
                break;
            }
            case AI_IFLevelIdGreaterThan: // BYTE(LEVEL_ID), BYTE(GOTOLABEL)
            {
                AiIFLevelIdGreaterThanRecord *ai = &AiList + Offset;
                printf("%d,", AiList[Offset + 1]);

                hasLabel = TRUE;

                break;
            }
            case AI_SetMyMorale: // BYTE(CHRBYTE)
            {
                AiSetMyMoraleRecord *ai = &AiList + Offset;
                printf("%d", AiList[Offset + 1]);

                break;
            }
            case AI_AddToMyMorale: // BYTE(CHRBYTE)
            {
                AiAddToMyMoraleRecord *ai = &AiList + Offset;
                printf("%d", AiList[Offset + 1]);

                break;
            }
            case AI_SubtractFromMyMorale: // BYTE(CHRBYTE)
            {
                AiSubtractFromMyMoraleRecord *ai = &AiList + Offset;
                printf("%d", AiList[Offset + 1]);

                break;
            }
            case AI_IFMyMoraleLessThan: // BYTE(CHRBYTE), BYTE(GOTOLABEL)
            {
                AiIFMyMoraleLessThanRecord *ai = &AiList + Offset;
                printf("%d,", AiList[Offset + 1]);

                hasLabel = TRUE;

                break;
            }

            case AI_SetMyAlertness: // BYTE(CHRBYTE)
            {
                AiSetMyAlertnessRecord *ai = &AiList + Offset;
                printf("%d", AiList[Offset + 1]);

                break;
            }
            case AI_AddToMyAlertness: // BYTE(CHRBYTE)
            {
                AiAddToMyAlertnessRecord *ai = &AiList + Offset;
                printf("%d", AiList[Offset + 1]);

                break;
            }
            case AI_SubtractFromMyAlertness: // BYTE(CHRBYTE)
            {
                AiSubtractFromMyAlertnessRecord *ai = &AiList + Offset;
                printf("%d", AiList[Offset + 1]);

                break;
            }
            case AI_IFMyAlertnessLessThan: // BYTE(CHRBYTE), BYTE(GOTOLABEL)
            {
                AiIFMyAlertnessLessThanRecord *ai = &AiList + Offset;
                printf("%d,", AiList[Offset + 1]);

                hasLabel = TRUE;

                break;
            }
            case AI_SetMyHearingScale: // DBYTE(HEARING_SCALE)
            {
                AiSetMyHearingScaleRecord *ai       = &AiList + Offset;
                float                      distance = CharArrayTo16(AiList, Offset + 1 + 0) / 1000.0f;

                printf("%f", distance * 1000);

                break;
            }
            case AI_SetMyVisionRange: // BYTE(VISION_RANGE)
            {
                AiSetMyVisionRangeRecord *ai = &AiList + Offset;
                printf("%d", AiList[Offset + 1]);

                break;
            }
            case AI_SetMyGrenadeProbability: // BYTE(GRENADE_PROB)
            {
                AiSetMyGrenadeProbabilityRecord *ai = &AiList + Offset;
                printf("%d", AiList[Offset + 1]);

                break;
            }
            case AI_SetMyChrNum: // BYTE(CHR_NUM)
            {
                AiSetMyChrNumRecord *ai = &AiList + Offset;
                printf("%d", AiList[Offset + 1]);

                break;
            }
            case AI_SetMyHealthTotal: // DBYTE(HEALTH)
            {
                AiSetMyHealthTotalRecord *ai     = &AiList + Offset;
                float                     amount = CharArrayTo16(AiList, Offset + 1 + 0) * 0.1f;

                printf("%f", amount * 10);

                break;
            }
            case AI_SetMyArmour: // DBYTE(AMOUNT)
            {
                AiSetMyArmourRecord *ai     = &AiList + Offset;
                float                amount = CharArrayTo16(AiList, Offset + 1 + 0) * 0.1f; /*if (cheatIsActive(CHEAT_ENEMYSHIELDS)) { amount = amount < 8 ? 8 : amount; } */

                printf("%f", amount * 10);

                break;
            }
            case AI_SetMySpeedRating: // BYTE(SPEED_RATING)
            {
                AiSetMySpeedRatingRecord *ai = &AiList + Offset;
                printf("%d", AiList[Offset + 1]);

                break;
            }
            case AI_SetMyArghRating: // BYTE(ARGH_RATING)
            {
                AiSetMyArghRatingRecord *ai = &AiList + Offset;
                printf("%d", AiList[Offset + 1]);

                break;
            }
            case AI_SetMyAccuracyRating: // BYTE(ACCURACY_RATING)
            {
                AiSetMyAccuracyRatingRecord *ai = &AiList + Offset;
                printf("%d", AiList[Offset + 1]);

                break;
            }
            case AI_SetMyFlags2: // BYTE(BITS)
            {
                AiSetMyFlags2Record *ai = &AiList + Offset;
                printf("0x%x", AiList[Offset + 1]);

                break;
            }
            case AI_UnsetMyFlags2: // BYTE(BITS)
            {
                AiUnsetMyFlags2Record *ai = &AiList + Offset;
                printf("0x%x", AiList[Offset + 1]);

                break;
            }
            case AI_IFMyFlags2Has: // BYTE(BITS) BYTE(GOTOLABEL)
            {
                AiIFMyFlags2HasRecord *ai = &AiList + Offset;
                printf("0x%x,", AiList[Offset + 1]);

                hasLabel = TRUE;

                break;
            }
            case AI_SetChrBitfield: // BYTE(CHR_NUM), BYTE(BITS)
            {
                AiSetChrBitfieldRecord *ai = &AiList + Offset;
                printf("%d,0x%x", AiList[Offset + 1], AiList[Offset + 2]);

                break;
            }
            case AI_UnsetChrBitfield: // BYTE(CHR_NUM), BYTE(BITS)
            {
                AiUnsetChrBitfieldRecord *ai = &AiList + Offset;
                printf("%d,0x%x", AiList[Offset + 1], AiList[Offset + 2]);

                break;
            }
            case AI_IFChrBitfieldHas: // BYTE(CHR_NUM), BYTE(BITS), BYTE(GOTOLABEL)
            {
                AiIFChrBitfieldHasRecord *ai = &AiList + Offset;
                printf("%d,0x%x,", AiList[Offset + 1], AiList[Offset + 2]);

                hasLabel = TRUE;

                break;
            }
            case AI_SetObjectiveBitfield: // QBYTE(BITFIELD)
            {
                AiSetObjectiveBitfieldRecord *ai    = &AiList + Offset;
                int                           flags = CharArrayTo32(AiList, Offset + 1 + 0);

                printf("0x%x", flags);

                break;
            }
            case AI_UnsetObjectiveBitfield: // QBYTE(BITFIELD)
            {
                AiUnsetObjectiveBitfieldRecord *ai    = &AiList + Offset;
                int                             flags = CharArrayTo32(AiList, Offset + 1 + 0);

                printf("0x%x", flags);

                break;
            }
            case AI_IFObjectiveBitfieldHas: // QBYTE(BITS), BYTE(GOTOLABEL)
            {
                AiIFObjectiveBitfieldHasRecord *ai    = &AiList + Offset;
                int                             flags = CharArrayTo32(AiList, Offset + 1 + 0);

                printf("0x%x,", flags);

                hasLabel = TRUE;

                break;
            }
            case AI_SetMychrflags: // QBYTE(CHRFLAGS)
            {
                AiSetMychrflagsRecord *ai    = &AiList + Offset;
                int                    flags = CharArrayTo32(AiList, Offset + 1 + 0);

                printf("0x%x", flags);

                break;
            }
            case AI_UnsetMychrflags: // QBYTE(CHRFLAGS)
            {
                AiUnsetMychrflagsRecord *ai    = &AiList + Offset;
                int                      flags = CharArrayTo32(AiList, Offset + 1 + 0);

                printf("0x%x", flags);

                break;
            }
            case AI_IFMychrflagsHas: // QBYTE(CHRFLAGS), BYTE(GOTOLABEL)
            {
                AiIFMychrflagsHasRecord *ai    = &AiList + Offset;
                int                      flags = CharArrayTo32(AiList, Offset + 1 + 0);

                printf("0x%x,", flags);

                hasLabel = TRUE;

                break;
            }
            case AI_SetChrchrflags: // BYTE(CHR_NUM), QBYTE(CHRFLAGS)
            {
                AiSetChrchrflagsRecord *ai    = &AiList + Offset;
                int                     flags = CharArrayTo32(AiList, Offset + 1 + 1);

                printf("%d,0x%x", AiList[Offset + 1], flags);

                break;
            }
            case AI_UnsetChrchrflags: // BYTE(CHR_NUM), QBYTE(CHRFLAGS)
            {
                AiUnsetChrchrflagsRecord *ai    = &AiList + Offset;
                int                       flags = CharArrayTo32(AiList, Offset + 1 + 1);

                printf("%d,0x%x", AiList[Offset + 1], flags);

                break;
            }
            case AI_IFChrchrflagsHas: // BYTE(CHR_NUM), QBYTE(CHRFLAGS), BYTE(GOTOLABEL)
            {
                AiIFChrchrflagsHasRecord *ai    = &AiList + Offset;
                int                       flags = CharArrayTo32(AiList, Offset + 1 + 1);

                printf("%d,0x%x,", AiList[Offset + 1], flags);

                hasLabel = TRUE;

                break;
            }
            case AI_SetObjectFlags: // BYTE(OBJECT_TAG), QBYTE(BITFIELD)
            {
                AiSetObjectFlagsRecord *ai    = &AiList + Offset;
                int                     flags = CharArrayTo32(AiList, Offset + 1 + 1);

                printf("%d,0x%x", AiList[Offset + 1], flags);

                break;
            }
            case AI_UnsetObjectFlags: // BYTE(OBJECT_TAG), QBYTE(BITFIELD)
            {
                AiUnsetObjectFlagsRecord *ai    = &AiList + Offset;
                int                       flags = CharArrayTo32(AiList, Offset + 1 + 1);

                printf("%d,0x%x", AiList[Offset + 1], flags);

                break;
            }
            case AI_IFObjectFlagsHas: // BYTE(OBJECT_TAG), QBYTE(BITS), BYTE(GOTOLABEL)
            {
                AiIFObjectFlagsHasRecord *ai    = &AiList + Offset;
                int                       flags = CharArrayTo32(AiList, Offset + 1 + 1);

                printf("%d,0x%x,", AiList[Offset + 1], flags);

                hasLabel = TRUE;

                break;
            }
            case AI_SetObjectFlags2: // BYTE(OBJECT_TAG), QBYTE(BITS)
            {
                AiSetObjectFlags2Record *ai    = &AiList + Offset;
                int                      flags = CharArrayTo32(AiList, Offset + 1 + 1);

                printf("%d,0x%x", AiList[Offset + 1], flags);

                break;
            }
            case AI_UnsetObjectFlags2: // BYTE(OBJECT_TAG), QBYTE(BITS)
            {
                AiUnsetObjectFlags2Record *ai    = &AiList + Offset;
                int                        flags = CharArrayTo32(AiList, Offset + 1 + 1);

                printf("%d,0x%x", AiList[Offset + 1], flags);

                break;
            }
            case AI_IFObjectFlags2Has: // BYTE(OBJECT_TAG), QBYTE(BITFIELD), BYTE(GOTOLABEL)
            {
                AiIFObjectFlags2HasRecord *ai    = &AiList + Offset;
                int                        flags = CharArrayTo32(AiList, Offset + 1 + 1);

                printf("%d,0x%x,", AiList[Offset + 1], flags);

                hasLabel = TRUE;

                break;
            }
            case AI_SetMyChrPreset: // BYTE(CHR_PRESET)
            {
                AiSetMyChrPresetRecord *ai = &AiList + Offset;
                printf("%d", AiList[Offset + 1]);

                break;
            }
            case AI_SetChrChrPreset: // BYTE(CHR_NUM), BYTE(CHR_PRESET)
            {
                AiSetChrChrPresetRecord *ai = &AiList + Offset;
                printf("%d,%d", AiList[Offset + 1], AiList[Offset + 2]);

                break;
            }
            case AI_SetMyPadPreset: // DBYTE(PAD_PRESET)
            {
                AiSetMyPadPresetRecord *ai  = &AiList + Offset;
                unsigned short          pad = CharArrayTo16(AiList, Offset + 1 + 0);

                if (isNotBoundPad(pad))
                {
                    printf("%d", pad);
                }
                else
                {
                    printf("setBoundPadNum(%d)", getBoundPadNum(pad));
                }

                break;
            }
            case AI_SetChrPadPreset: // BYTE(CHR_NUM), DBYTE(PAD_PRESET)
            {
                AiSetChrPadPresetRecord *ai  = &AiList + Offset;
                unsigned short           pad = CharArrayTo16(AiList, Offset + 1 + 1);

                printf("%s(%d,", AI_CMD_ToString[AiList[Offset]], AiList[Offset + 1]);

                if (isNotBoundPad(pad))
                {
                    printf("%d", pad);
                }
                else
                {
                    printf("setBoundPadNum(%d)", getBoundPadNum(pad));
                }

                break;
            }
            case AI_PRINT:
            {
                AiPRINTRecord *ai = &AiList + Offset;
                int            j  = 1;
                printf("\"");
                for (; AiList[Offset + j] != 0; j++)
                {
                    if (AiList[Offset + j] == '\n')
                    {
                        printf("\\n");
                    }
                    else
                    {
                        printf("%c", AiList[Offset + j]);
                    }
                }
                if (!hasName)
                {
                    hasName = malloc(j);
                    hasName = &AiList[Offset + 1];
                }

                printf("\"");
                break;
            }

            case AI_IFMyTimerLessThanTicks: // TBYTE(TICKS), BYTE(GOTOLABEL)
            {
                AiIFMyTimerLessThanTicksRecord *ai   = &AiList + Offset;
                float                           valf = ((unsigned)CharArrayTo24(AiList, Offset + 1 + 0));

                printf("%f,", valf);

                hasLabel = TRUE;

                break;
            }
            case AI_IFMyTimerGreaterThanTicks: // TBYTE(TICKS), BYTE(GOTOLABEL)
            {
                AiIFMyTimerGreaterThanTicksRecord *ai   = &AiList + Offset;
                float                              valf = ((unsigned)CharArrayTo24(AiList, Offset + 1 + 0));

                printf("%f,", valf);

                hasLabel = TRUE;

                break;
            }

            case AI_HudCountdownSet: // DBYTE(SECONDS)
            {
                AiHudCountdownSetRecord *ai      = &AiList + Offset;
                float                    seconds = CharArrayTo16(AiList, Offset + 1 + 0);

                printf("%f", seconds);

                break;
            }

            case AI_IFHudCountdownLessThan: // DBYTE(SECONDS), BYTE(GOTOLABEL)
            {
                AiIFHudCountdownLessThanRecord *ai    = &AiList + Offset;
                float                           value = CharArrayTo16(AiList, Offset + 1 + 0);

                printf("%f,", value);

                hasLabel = TRUE;

                break;
            }
            case AI_IFHudCountdownGreaterThan: // DBYTE(SECONDS), BYTE(GOTOLABEL)
            {
                AiIFHudCountdownGreaterThanRecord *ai    = &AiList + Offset;
                float                              value = CharArrayTo16(AiList, Offset + 1 + 0);

                printf("%f,", value);

                hasLabel = TRUE;

                break;
            }
            case AI_TRYSpawningChrAtPad: // BYTE(BODY_NUM), BYTE(HEAD_NUM), DBYTE(PAD), DBYTE(AI_LIST_ID), QBYTE(BITFIELD), BYTE(GOTOLABEL)
            {
                AiTRYSpawningChrAtPadRecord *ai       = &AiList + Offset;
                unsigned short               pad      = CharArrayTo16(AiList, Offset + 1 + 2);
                int                          flags    = CharArrayTo32(AiList, Offset + 1 + 6);
                unsigned short               ailistid = CharArrayTo16(AiList, Offset + 1 + 4);

                printf("%d,", AiList[Offset + 1]);
                if (isNotBoundPad(pad))
                {
                    printf("%d,", pad);
                }
                else
                {
                    printf("setBoundPadNum(%d),", getBoundPadNum(pad));
                }

                printf("%d,0x%x,%d,", ailistid, flags, AiList[Offset + 10]);

                hasLabel = TRUE;

                break;
            }
            case AI_TRYSpawningChrNextToChr: // BYTE(BODY_NUM), BYTE(HEAD_NUM), BYTE(CHR_NUM_TARGET), DBYTE(AI_LIST_ID), QBYTE(BITFIELD), BYTE(GOTOLABEL)
            {
                AiTRYSpawningChrNextToChrRecord *ai       = &AiList + Offset;
                int                              flags    = CharArrayTo32(AiList, Offset + 1 + 5);
                unsigned short                   ailistid = CharArrayTo16(AiList, Offset + 1 + 3);

                printf("%d,%d,%d,%d,0x%x,", AiList[Offset + 1], AiList[Offset + 1 + 1], AiList[Offset + 1 + 2], ailistid, flags);

                hasLabel = TRUE;
                break;
            }
            case AI_TRYGiveMeItem: // DBYTE(PROP_NUM), BYTE(ITEM_NUM), QBYTE(PROPFLAG), BYTE(GOTOLABEL)
            {
                AiTRYGiveMeItemRecord *ai    = &AiList + Offset;
                int                    flags = CharArrayTo32(AiList, Offset + 1 + 3);
                int                    model = CharArrayTo16(AiList, Offset + 1 + 0);

                printf("%d,%d,0x%x,", model, AiList[Offset + 1 + 2], flags);

                hasLabel = TRUE;

                break;
            }
            case AI_TRYGiveMeHat: // DBYTE(PROP_NUM), QBYTE(PROP_BITFIELD), BYTE(GOTOLABEL)
            {
                AiTRYGiveMeHatRecord *ai       = &AiList + Offset;
                int                   flags    = CharArrayTo32(AiList, Offset + 1 + 2);
                int                   modelnum = CharArrayTo16(AiList, Offset + 1 + 0);

                printf("%d,0x%x,", modelnum, flags);

                hasLabel = TRUE;
                break;
            }
            case AI_TRYCloningChr: // BYTE(CHR_NUM), DBYTE(AI_LIST_ID), BYTE(GOTOLABEL)
            {
                AiTRYCloningChrRecord *ai       = &AiList + Offset;
                unsigned short         ailistid = CharArrayTo16(AiList, Offset + 1 + 1);

                printf("%d,%d,", AiList[Offset + 4], ailistid);

                hasLabel = TRUE;
                break;
            }
            case AI_TextPrintBottom: // DBYTE(TEXT_SLOT)
            {
                AiTextPrintBottomRecord *ai = &AiList + Offset;
                // printf("//USING HUD MESSAGE Stringy = %d, ai->txt = %d\n", 0, CharArrayTo16(AiList, Offset + 1 + 0));

                printf("TEXT(%s,%d)", TEXTBANK_LEVEL_INDEX_ToString[(CharArrayTo16(AiList, Offset + 1 + 0)) / 1024], (CharArrayTo16(AiList, Offset + 1 + 0)) % 1024);

                break;
            }
            case AI_TextPrintTop: // DBYTE(TEXT_SLOT)
            {
                AiTextPrintTopRecord *ai = &AiList + Offset;
                // printf("//USING HUD MESSAGE Stringy = %d, ai->txt = %d\n", 0, CharArrayTo16(AiList, Offset + 1 + 0));

                printf("TEXT(%s,%d)", TEXTBANK_LEVEL_INDEX_ToString[(CharArrayTo16(AiList, Offset + 1 + 0)) / 1024], (CharArrayTo16(AiList, Offset + 1 + 0)) % 1024);

                break;
            }
            case AI_SfxPlay: // DBYTE(SOUND_NUM), BYTE(CHANNEL_NUM)
            {
                AiSfxPlayRecord *ai       = &AiList + Offset;
                short            audio_id = CharArrayTo16(AiList, Offset + 1 + 0);

                printf("%d,%d", audio_id, AiList[Offset + 3]);

                break;
            }
            case AI_SfxStopChannel: // BYTE(CHANNEL_NUM)
            {
                AiSfxStopChannelRecord *ai = &AiList + Offset;
                printf("%d", AiList[Offset + 3]);

                break;
            }
            case AI_SfxSetChannelVolume: // BYTE(CHANNEL_NUM), DBYTE(TARGET_VOLUME), DBYTE(TRANSITION_TIME60)
            {
                AiSfxSetChannelVolumeRecord *ai    = &AiList + Offset;
                short                        vol   = CharArrayTo16(AiList, Offset + 2 + 0);
                unsigned short               sfxID = CharArrayTo16(AiList, Offset + 2 + 2);

                printf("%d, %d,%d", AiList[Offset + 1], vol, sfxID);

                break;
            }
            case AI_SfxFadeChannelVolume: // BYTE(CHANNEL_NUM), DBYTE(TARGET_VOLUME), DBYTE(TRANSITION_TIME60)
            {
                AiSfxFadeChannelVolumeRecord *ai    = &AiList + Offset;
                short                         vol   = CharArrayTo16(AiList, Offset + 2 + 0);
                unsigned short                sfxID = CharArrayTo16(AiList, Offset + 2 + 2);

                printf("%d, %d,%d", AiList[Offset + 1], vol, sfxID);

                break;
            }
            case AI_SfxEmitFromObject: // BYTE(CHANNEL_NUM), BYTE(OBJECT_TAG), DBYTE(VOL_DECAY_TIME60)
            {
                AiSfxEmitFromObjectRecord *ai    = &AiList + Offset;
                unsigned short             sfxID = CharArrayTo16(AiList, Offset + 2 + 1);

                printf("%d, %d,%d", AiList[Offset + 1], AiList[Offset + 2], sfxID);

                break;
            }
            case AI_SfxEmitFromPad: // BYTE(CHANNEL_NUM), DBYTE(PAD), DBYTE(VOL_DECAY_TIME60)
            {
                AiSfxEmitFromPadRecord *ai    = &AiList + Offset;
                unsigned short          pad   = CharArrayTo16(AiList, Offset + 2 + 0);
                unsigned short          sfxID = CharArrayTo16(AiList, Offset + 2 + 2);

                printf("%d,", AiList[Offset + 1]);

                if (isNotBoundPad(pad))
                {
                    printf("%d,", pad);
                }
                else
                {
                    printf("setBoundPadNum(%d),", getBoundPadNum(pad));
                }
                printf("%d", sfxID);

                break;
            }
            case AI_IFSfxChannelVolumeLessThan: // BYTE(CHANNEL_NUM), DBYTE(VOLUME), BYTE(GOTOLABEL)
            {
                AiIFSfxChannelVolumeLessThanRecord *ai  = &AiList + Offset;
                short                               vol = CharArrayTo16(AiList, Offset + 2 + 0);

                printf("%d, %d,", AiList[Offset + 1], vol);

                hasLabel = TRUE;
                break;
            }
            case AI_VehicleStartPath: // BYTE(PATH_NUM)
            {
                AiVehicleStartPathRecord *ai = &AiList + Offset;
                printf("%d", AiList[Offset + 1 + 0]);

                break;
            }
            case AI_VehicleSpeed: // DBYTE(TOP_SPEED), DBYTE(ACCELERATION_TIME60)
            {
                AiVehicleSpeedRecord *ai        = &AiList + Offset;
                float                 speedtime = CharArrayTo16(AiList, Offset + 1 + 2);
                float                 speedaim  = CharArrayTo16(AiList, Offset + 1 + 0) * 100.0f / 15360.0f;

                printf("%d,%f", CharArrayTo16(AiList, Offset + 1 + 0), speedtime);

                break;
            }
            case AI_AircraftRotorSpeed: // DBYTE(ROTOR_SPEED), DBYTE(ACCELERATION_TIME60)
            {
                AiAircraftRotorSpeedRecord *ai        = &AiList + Offset;
                float                       speedtime = CharArrayTo16(AiList, Offset + 1 + 2);
                float                       speedaim  = CharArrayTo16(AiList, Offset + 1 + 0) * M_TAU_F / 3600.0f;

                printf("%d,%f", CharArrayTo16(AiList, Offset + 1 + 0), speedtime);

                break;
            }
            case AI_TvChangeScreenBank: // BYTE(OBJECT_TAG), BYTE(SCREEN_INDEX), BYTE(SCREEN_BANK)
            {
                AiTvChangeScreenBankRecord *ai = &AiList + Offset;
                printf("%d,%d,%d", AiList[Offset + 1], AiList[Offset + 2], AiList[Offset + 3]);

                break;
            }

            case AI_CameraLookAtBondFromPad: // DBYTE(PAD)
            {
                AiCameraLookAtBondFromPadRecord *ai  = &AiList + Offset;
                unsigned short                   pad = CharArrayTo16(AiList, Offset + 1 + 0);

                if (isNotBoundPad(pad))
                {
                    printf("%d", pad);
                }
                else
                {
                    printf("setBoundPadNum(%d),", getBoundPadNum(pad));
                }

                break;
            }
            case AI_CameraSwitch: // BYTE(OBJECT_TAG), DBYTE(LOOK_AT_BOND_FLAG), DBYTE(UNUSED_FLAG)
            {
                AiCameraSwitchRecord *ai = &AiList + Offset;
                printf("%d,%d,%d", AiList[Offset + 1 + 0], CharArrayTo16(AiList, Offset + 1 + 1), CharArrayTo16(AiList, Offset + 1 + 3));

                break;
            }
            case AI_IFBondYPosLessThan: // DBYTE(Y_POS), BYTE(GOTOLABEL)
            {
                AiIFBondYPosLessThanRecord *ai      = &AiList + Offset;
                float                       bondpos = (short)CharArrayTo16(AiList, Offset + 1 + 0);

                printf("%f,", bondpos);
                hasLabel = TRUE;

                break;
            }
            case AI_BondDisableControl: // BYTE(BITFIELD)
            {
                AiBondDisableControlRecord *ai = &AiList + Offset;
                printf("%d", AiList[Offset + 1]);

                break;
            }

            case AI_TRYTeleportingChrToPad: // BYTE(CHR_NUM), DBYTE(PAD), BYTE(GOTOLABEL)
            {
                AiTRYTeleportingChrToPadRecord *ai  = &AiList + Offset;
                int                             pad = CharArrayTo16(AiList, Offset + 1 + 1);

                printf("%d,", AiList[Offset + 1]);
                if (isNotBoundPad(pad))
                {
                    printf("%d,", pad);
                }
                else
                {
                    printf("setBoundPadNum(%d),", getBoundPadNum(pad));
                }
                hasLabel = TRUE;

                break;
            }

            case AI_DoorOpenInstant: // BYTE(OBJECT_TAG)
            {
                AiDoorOpenInstantRecord *ai = &AiList + Offset;
                printf("tag%d", AiList[Offset + 1]);

                break;
            }
            case AI_ChrRemoveItemInHand: // BYTE(CHR_NUM), BYTE(HAND_INDEX)
            {
                AiChrRemoveItemInHandRecord *ai = &AiList + Offset;
                printf("chr%d,%s", AiList[Offset + 1], AiList[Offset + 2] == 0 ? "GUNRIGHT" : "GUNLEFT");

                break;
            }
            case AI_IfNumberOfActivePlayersLessThan: // BYTE(NUMBER), BYTE(GOTOLABEL)
            {
                AiIfNumberOfActivePlayersLessThanRecord *ai = &AiList + Offset;
                if (AiList[Offset + 1] == 2)
                {
                    printf("%sIFSinglePlayer(", (char *)tabs);
                }
                else
                {
                    printf("%d,", AiList[Offset + 1]);
                }

                hasLabel = TRUE;

                break;
            }
            case AI_IFBondItemTotalAmmoLessThan: // BYTE(ITEM_NUM), BYTE(AMMO_TOTAL), BYTE(GOTOLABEL)
            {
                AiIFBondItemTotalAmmoLessThanRecord *ai = &AiList + Offset;
                printf("%s,%d,", ITEM_IDS_ToString[AiList[Offset + 1]], AiList[Offset + 2]);

                hasLabel = TRUE;

                break;
            }
            case AI_BondEquipItem: // BYTE(ITEM_NUM)
            {
                AiBondEquipItemRecord *ai = &AiList + Offset;
                printf("%s", ITEM_IDS_ToString[AiList[Offset + 1]]);

                break;
            }
            case AI_BondEquipItemCinema: // BYTE(ITEM_NUM)
            {
                AiBondEquipItemCinemaRecord *ai = &AiList + Offset;
                printf("%s", ITEM_IDS_ToString[AiList[Offset + 1]]);

                break;
            }
            case AI_BondSetLockedVelocity: // BYTE(X_SPEED60), BYTE(Z_SPEED60)
            {
                AiBondSetLockedVelocityRecord *ai = &AiList + Offset;
                printf("%d,%d", AiList[Offset + 1], AiList[Offset + 2]);

                break;
            }
            case AI_IFObjectInRoomWithPad: // BYTE(OBJECT_TAG), DBYTE(PAD), BYTE(GOTOLABEL)
            {
                AiIFObjectInRoomWithPadRecord *ai  = &AiList + Offset;
                unsigned short                 pad = CharArrayTo16(AiList, Offset + 1 + 1);

                printf("tag%d,", AiList[Offset + 1]);
                if (isNotBoundPad(pad))
                {
                    printf("%d,", pad);
                }
                else
                {
                    printf("setBoundPadNum(%d),", getBoundPadNum(pad));
                }

                hasLabel = TRUE;

                break;
            }

            case AI_CameraOrbitPad:
            {
                AiCameraOrbitPadRecord *ai = &AiList + Offset;
                int                     pad;
                int                     speed60;
                int                     camDististance;
                int                     targetHeight;
                int                     camHeight;
                int                     start;
                camDististance = CharArrayTo16(AiList, Offset + 1 + 0);
                camHeight      = (short)CharArrayTo16(AiList, Offset + 1 + 2);
                speed60        = (short)CharArrayTo16(AiList, Offset + 1 + 4);
                pad            = CharArrayTo16(AiList, Offset + 1 + 6);
                targetHeight   = (short)CharArrayTo16(AiList, Offset + 1 + 8);
                start          = CharArrayTo16(AiList, Offset + 1 + 10);

                printf("%d,%d,%d,", camDististance, camHeight, speed60);
                if (isNotBoundPad(pad))
                {
                    printf("%d,", pad);
                }
                else
                {
                    printf("setBoundPadNum(%d),", getBoundPadNum(pad));
                }

                printf("%d,%d", targetHeight, start);

                break;
            }

            case AI_IFFolderActorIsEqual: // BYTE(BOND_ACTOR_INDEX), BYTE(GOTOLABEL)
            {
                AiIFFolderActorIsEqualRecord *ai = &AiList + Offset;
                printf("%d,", AiList[Offset + 1]);

                hasLabel = TRUE;

                break;
            }

            case AI_MusicPlaySlot: // BYTE(MUSIC_SLOT), BYTE(SECONDS_STOPPED_DURATION), BYTE(SECONDS_TOTAL_DURATION)
            {
                AiMusicPlaySlotRecord *ai = &AiList + Offset;
                // printf("/*ai: enery tune on (%d, %d, %d)*/", AiList[Offset + 1 + 0], AiList[Offset + 1 + 1], AiList[Offset + 1 + 2]);

                printf("%d,%d,%d", AiList[Offset + 1 + 0], AiList[Offset + 1 + 1], AiList[Offset + 1 + 2]);

                break;
            }
            case AI_MusicStopSlot:
            {
                AiMusicStopSlotRecord *ai = &AiList + Offset;
                // printf("/*ai: enery tune off (%d)*/", AiList[Offset + 1 + 0]);

                printf("%d", AiList[Offset + 1 + 0]);

                break;
            }
            case AI_IFKilledCiviliansGreaterThan:
            {
                AiIFKilledCiviliansGreaterThanRecord *ai = &AiList + Offset;
                printf("%d,", AiList[Offset + 1]);

                hasLabel = TRUE;

                break;
            }
            case AI_IFChrWasShotSinceLastCheck: // BYTE(CHR_NUM), BYTE(GOTOLABEL)
            {
                AiIFChrWasShotSinceLastCheckRecord *ai = &AiList + Offset;
                printf("chr%d,", AiList[Offset + 1]);

                hasLabel = TRUE;

                break;
            }
            case AI_ObjectRocketLaunch: // BYTE(OBJECT_TAG)
            {
                AiObjectRocketLaunchRecord *ai = &AiList + Offset;
                printf("tag%d", AiList[Offset + 1]);

                break;
            }
            case AI_GotoNext:                    // BYTE(LABEL)
            case AI_IFPlayingAnimation:          // BYTE(GOTOLABEL)
            case AI_IFImOnPatrolOrStopped:       // BYTE(GOTOLABEL)
            case AI_IFISeeBond:                  // BYTE(GOTOLABEL)
            case AI_TRYSidestepping:             // BYTE(GOTOLABEL)
            case AI_TRYSideHopping:              // BYTE(GOTOLABEL)
            case AI_TRYSideRunning:              // BYTE(GOTOLABEL)
            case AI_TRYFiringWalk:               // BYTE(GOTOLABEL)
            case AI_TRYFiringRun:                // BYTE(GOTOLABEL)
            case AI_TRYFiringRoll:               // BYTE(GOTOLABEL)
            case AI_IFImFiringAndLockedForward:  // BYTE(GOTOLABEL)
            case AI_IFImFiring:                  // BYTE(GOTOLABEL)
            case AI_TRYThrowingGrenade:          // BYTE(GOTOLABEL)
            case AI_TRYRunFromBond:              // BYTE(GOTOLABEL)
            case AI_TRYRunToBond:                // BYTE(GOTOLABEL)
            case AI_TRYWalkToBond:               // BYTE(GOTOLABEL)
            case AI_TRYSprintToBond:             // BYTE(GOTOLABEL)
            case AI_TRYFindCover:                // BYTE(GOTOLABEL)
            case AI_IFICanHearAlarm:             // BYTE(GOTOLABEL)
            case AI_IFAlarmIsOn:                 // BYTE(GOTOLABEL)
            case AI_IFGasIsLeaking:              // BYTE(GOTOLABEL)
            case AI_IFIHeardBond:                // BYTE(GOTOLABEL)
            case AI_IFISeeSomeoneShot:           // BYTE(GOTOLABEL)
            case AI_IFISeeSomeoneDie:            // BYTE(GOTOLABEL)
            case AI_IFICouldSeeBond:             // BYTE(GOTOLABEL)
            case AI_IFICouldSeeBondsStan:        // BYTE(GOTOLABEL)
            case AI_IFIWasShotRecently:          // BYTE(GOTOLABEL)
            case AI_IFIHeardBondRecently:        // BYTE(GOTOLABEL)
            case AI_IFIveNotBeenSeen:            // BYTE(GOTOLABEL)
            case AI_IFImOnScreen:                // BYTE(GOTOLABEL)
            case AI_IFMyRoomIsOnScreen:          // BYTE(GOTOLABEL)
            case AI_IFImTargetedByBond:          // BYTE(GOTOLABEL)
            case AI_IFBondMissedMe:              // BYTE(GOTOLABEL)
            case AI_IFMyMoraleLessThanRandom:    // BYTE(GOTOLABEL)
            case AI_IFMyAlertnessLessThanRandom: // BYTE(GOTOLABEL)
            case AI_IFMyTimerIsNotRunning:       // BYTE(GOTOLABEL)
            case AI_IFHudCountdownIsNotRunning:  // BYTE(GOTOLABEL)
            case AI_IFCameraIsInIntro:           // BYTE(GOTOLABEL)
            case AI_IFCameraIsInBondSwirl:       // BYTE(GOTOLABEL)
            case AI_IFBondInTank:                // canonical name
            case AI_IFScreenFadeCompleted:       // BYTE(GOTOLABEL)
            case AI_IFBondIsDead:
            case AI_IFCreditsHasCompleted:
            case AI_IFObjectiveAllCompleted:
            case AI_IFBondDamageAndPickupsDisabled:
            {
                AiIFBondDamageAndPickupsDisabledRecord *ai = &AiList + Offset;
                hasLabel                                   = TRUE;
                break;
            }
            case AI_Yield:         // /*NONE*/
            case AI_Stop:          // /*NONE*/
            case AI_Kneel:         // /*NONE*/
            case AI_PointAtBond:   // /*NONE*/
            case AI_LookSurprised: // /*NONE*/
            case AI_Surrender:     // /*NONE*/
            case AI_RemoveMe:      // /*NONE*/
            case AI_AlarmOn:
            case AI_AlarmOff:
            case AI_RunToPadPreset: // /*NONE*/
            case AI_MyTimerStart:
            case AI_MyTimerReset:
            case AI_MyTimerPause:
            case AI_MyTimerResume:
            case AI_HudCountdownShow:
            case AI_HudCountdownHide:
            case AI_HudCountdownStop:
            case AI_HudCountdownStart:
            case AI_EndLevel: // canonical name
            case AI_CameraReturnToBond:
            case AI_BondEnableControl:
            case AI_ScreenFadeToBlack:
            case AI_ScreenFadeFromBlack:
            case AI_HideAllChrs:
            case AI_ShowAllChrs:
            case AI_TriggerFadeAndExitLevelOnButtonPress:
            case AI_BondDisableDamageAndPickups:
            case AI_BondHideWeapons:
            case AI_CreditsRoll:
            case AI_SwitchSky:
            case AI_BondKilledInAction:
            case AI_RaiseArms:
            case AI_TriggerExplosionsAroundBond:
            case AI_GasLeakAndFadeFog:
            default:
            {
                break;
            }

        } // switch
        if (!hasMacro) Offset += chraiitemsize(AiList, Offset);
        if (hasLabel)
        {
            printf(" lbl%d ", AiList[Offset - 1]);
            AddLabel(AiList[Offset - 1]);
        }
        if (!hasClosingBrace) printf(")\n");
    }
}

#pragma endregion // AICommands

#pragma region "Intro"

char *INTRO_CMD_ToString[] = {
    "IntroSpawn",
    "IntroItem",
    "IntroAmmo",
    "IntroSwirl",
    "IntroAnim",
    "IntroCuff",
    "IntroCamera",
    "IntroWatch",
    "IntroCredits",
    "IntroEnd"};

void Intro(unsigned char *IntroList)
{
    int Offset;
    int command;
    for (Offset = 0; Offset < 900;)
    {
        char *strDemoSlot = "Demo Slot";

        command = CharArrayTo32(IntroList, Offset);
        if (command < INTROTYPE_MAX)
        {
            printf("%s(", INTRO_CMD_ToString[command]);
        }
        else
        {
            printf("Intro ERROR: Unknown Command %X", command);
            return;
        }
        switch (command)
        {
            case INTROTYPE_SPAWN:
            {
                SetupIntroSpawn *intro = &IntroList[Offset];
                Offset += sizeof(SetupIntroSpawn);

                btol(intro->index);
                btol(intro->demoSlot);
                if (intro->demoSlot == 0) strDemoSlot = "Normal Gameplay";

                printf(" %d,/*%s*/ %d ),\n", intro->index, strDemoSlot, intro->demoSlot);
                break;
            }
            case INTROTYPE_ITEM:
            {
                SetupIntroItem *intro = &IntroList[Offset];
                Offset += sizeof(SetupIntroItem);
                btol(intro->item_right);
                btol(intro->item_left);
                btol(intro->demoSlot);
                if (intro->demoSlot == 0) strDemoSlot = "Normal Gameplay";

                printf(" %s, %s,/*%s*/ %d ),\n", intro->item_right < 0 ? "ITEM_NOTHING" : ITEM_IDS_ToString[intro->item_right], intro->item_left < 0 ? "ITEM_NOTHING" : ITEM_IDS_ToString[intro->item_left], strDemoSlot, intro->demoSlot);
                break;
            }
            case INTROTYPE_AMMO:
            {
                SetupIntroAmmo *intro = &IntroList[Offset];
                Offset += sizeof(SetupIntroAmmo);
                btol(intro->ammo_type);
                btol(intro->ammo_amount);
                btol(intro->demoSlot);
                if (intro->demoSlot == 0) strDemoSlot = "Normal Gameplay";
                printf(" %s, %d,/*%s*/ %d ),\n", AMMOTYPE_ToString[intro->ammo_type], intro->ammo_amount, strDemoSlot, intro->demoSlot);
                break;
            }
            case INTROTYPE_SWIRL:
            {
                SetupIntroSwirl *intro = &IntroList[Offset];
                int              w;
                char            *PadName   = "pad";
                char            *flagnames = "";
                Offset += sizeof(SetupIntroSwirl);
                btol(intro->bitflags);
                btol(intro->offsetfromBond[0].full);
                intro->offsetfromBond[0].fval = intro->offsetfromBond[0].full / M_U16_MAX_VALUE_F;
                btol(intro->offsetfromBond[1].full);
                intro->offsetfromBond[1].fval = intro->offsetfromBond[1].full / M_U16_MAX_VALUE_F;
                btol(intro->offsetfromBond[2].full);
                intro->offsetfromBond[2].fval = intro->offsetfromBond[2].full / M_U16_MAX_VALUE_F;
                btol(intro->scale.full);
                intro->scale.fval = intro->scale.full / M_U16_MAX_VALUE_F;
                btol(intro->duration.full);
                intro->duration.fval = intro->duration.full / M_U16_MAX_VALUE_F;
                btol(intro->pad);

                for (w = 0; w < 4; w++)
                {
                    if (intro->bitflags && 1)
                    {
                        flagnames = "SWIRL_FADE";
                    }
                }
                for (w = 0; g_chraiCurrentSetup.pathwaypoints[w].padID >= 0; w++)
                {
                    if (g_chraiCurrentSetup.pathwaypoints[w].padID == intro->pad)
                    {
                        PadName = g_chraiCurrentSetup.padnames[w].p;
                        break;
                    }
                }
                if (intro->pad < 0)
                {
                    PadName = "BOND";
                }
                printf("%d, /*Offset*/ %g, %g, %g, /*scale*/ %g, /*seconds*/ %g, /*%s*/ %d),\n", intro->bitflags, intro->offsetfromBond[0].fval, intro->offsetfromBond[1].fval, intro->offsetfromBond[2].fval, intro->scale.fval, intro->duration.fval, PadName, intro->pad);
                break;
            }
            case INTROTYPE_ANIM:
            {
                SetupIntroAnim *intro = &IntroList[Offset];
                Offset += sizeof(SetupIntroAnim);

                btol(intro->intro_anim);
                printf("%s),\n", ANIMATIONS_ToString[intro->intro_anim]);
                break;
            }
            case INTROTYPE_CUFF:
            {
                SetupIntroCuff *intro = &IntroList[Offset];
                Offset += sizeof(SetupIntroCuff);
                btol(intro->bondtype);
                printf("%s),\n", CUFF_TYPES_ToString[intro->bondtype]);
                break;
            }
            case INTROTYPE_CAMERA:
            {
                SetupIntroCamera *intro = &IntroList[Offset];
                int               w;
                char             *PadName = "pad";

                Offset += sizeof(SetupIntroCamera);

                btol(intro->coords[0].ival);
                intro->coords[0].fval = intro->coords[0].ival / 100.0f;
                btol(intro->coords[1].ival);
                intro->coords[1].fval = intro->coords[1].ival / 100.0f;
                btol(intro->coords[2].ival);
                intro->coords[2].fval = intro->coords[2].ival / 100.0f;
                btol(intro->unk10.ival);
                intro->unk10.fval = intro->unk10.ival / M_U16_MAX_VALUE_F;
                btol(intro->unk14.ival);
                intro->unk14.fval = intro->unk14.ival / M_U16_MAX_VALUE_F;
                btol(intro->unk18);
                // intro->unk18.fval = intro->unk18.ival / 100.0f;
                btol(intro->lang1c);
                btol(intro->lang20);
                btol(intro->prev);
                for (w = 0; g_chraiCurrentSetup.pathwaypoints[w].padID >= 0; w++)
                {
                    if (g_chraiCurrentSetup.pathwaypoints[w].padID == intro->unk18)
                    {
                        PadName = g_chraiCurrentSetup.padnames[w].p;
                        break;
                    }
                }
                printf("/*pos*/ %.2f, %.2f, %.2f,\n\t/*Yaw*/ DegToRad(%g),/*Pitch*/ DegToRad(%g),\n\t/*%s*/ %d, \n\t TEXT(%s,%d), TEXT(%s,%d)),\n", intro->coords[0].fval, intro->coords[1].fval, intro->coords[2].fval, RadToDeg(intro->unk10.fval), RadToDeg(intro->unk14.fval), PadName, intro->unk18, TEXTBANK_LEVEL_INDEX_ToString[(intro->lang1c.lang_index[0]) / 1024], intro->lang1c.lang_index[0] % 1024, TEXTBANK_LEVEL_INDEX_ToString[(intro->lang20.lang_index) / 1024], intro->lang20.lang_index % 1024);

                break;
            }
            case INTROTYPE_WATCH:
            {
                SetupIntroWatch *intro = &IntroList[Offset];
                Offset += sizeof(SetupIntroWatch);
                btol(intro->hours);
                btol(intro->minutes);
                printf("%d, %d),\n", intro->hours, intro->minutes);

                break;
            }
            case INTROTYPE_CREDITS:
            {
                SetupIntroCredits *intro = &IntroList[Offset];
                Offset += sizeof(SetupIntroCredits);
                btol(intro->unk04);
                printf("%d),\n", intro->unk04);
                break;
            }
            case INTROTYPE_END:
            {
                printf(")\n\n");
                Offset++;
                return;
            }
        }
    }
}
#pragma endregion // Intro

int getDataSize(FILE *pfile, void *pointer, int endtag)
{
    int i = 0;

    fseek(pfile, (int)pointer, SEEK_SET);

    // scan for end of Record (NULL or -1)
    do
    {
        fread(&i, 4, 1, pfile);

    } while (i != endtag);

    return (ftell(pfile)) - (int)pointer;
}

void hexDump(char *desc, void *addr, int len)
{
    int            i;
    unsigned char  buff[17];
    unsigned char *pc = (unsigned char *)addr;

    // Output description if given.
    if (desc != NULL)
        printf("%s:\n", desc);

    // Process every byte in the data.
    for (i = 0; i < len; i++)
    {
        // Multiple of 16 means new line (with line offset).

        if ((i % 16) == 0)
        {
            // Just don't print ASCII for the zeroth line.
            if (i != 0)
                printf("  %s\n", buff);

            // Output the offset.
            printf("  %04x ", i);
        }

        // Now the hex code for the specific character.
        printf(" %02x", pc[i]);

        // And store a printable ASCII character for later.
        if ((pc[i] < 0x20) || (pc[i] > 0x7e))
        {
            buff[i % 16] = '.';
        }
        else
        {
            buff[i % 16] = pc[i];
        }

        buff[(i % 16) + 1] = '\0';
    }

    // Pad out last line if not exactly 16 characters.
    while ((i % 16) != 0)
    {
        printf("   ");
        i++;
    }

    // And print the final ASCII bit.
    printf("  %s\n", buff);
}

int main(int argc, char *argv[])
{
#define PRINTDEBUG
    char   fname[1000];
    FILE  *pfile;
    char   cwd[1000];
    void  *lastAddr;
    int    fsize;

    int    i, j;
    time_t current_time;
    time(&current_time);

    getcwd(cwd, sizeof(cwd));

    if (argc > 1)
    {
        strcat(cwd, "/");
        strcat(cwd, argv[1]);
        strcpy(fname, argv[1]);
    }
    else
    {
        printf("Enter a filename:\n");
        scanf("%99s", fname);
        strcat(cwd, "/");
        strcat(cwd, fname);
    }

    for (i = strlen(fname); i > 0; i--)
    {
        j = i + 1;
        if (fname[i] == '/') break;
    }

    for (i = 0; i < strlen(fname); i++, j++)
    {
        if (fname[j] == '.')
        {
            fname[i] = 0;
            break;
        }
        fname[i] = fname[j];
    }
    pfile = fopen(cwd, "rb");

    if (!pfile)
    {
        printf("Error! opening file: %s\n", cwd);
        // exit from program if file pointer returns NULL.
        return 0;
    }

    // Get file size
    fseek(pfile, 0, SEEK_END);
    fsize = ftell(pfile);
    rewind(pfile);

    // allocate setup file
    g_ptrStageSetupFile = malloc(fsize);
    rewind(pfile);

    // Load setup file
    if (1 != fread(g_ptrStageSetupFile, fsize, 1, pfile))
    {
        fclose(pfile);
        free(g_ptrStageSetupFile);
        printf("entire read fails\n");
        return 0;
    }
    fclose(pfile);

    {
        g_chraiCurrentSetup = *(stagesetup *)g_ptrStageSetupFile;

        // convert to host endian and turn file pointer to RAM pointer
        // iterate over all items and sub-items converting from file to ram pointer
#pragma region "foreach item in setupheader"

        if (g_chraiCurrentSetup.padnames)
        {
            g_chraiCurrentSetup.padnames = g_ptrStageSetupFile + ntohl(g_chraiCurrentSetup.padnames);
            for (i = 0; g_chraiCurrentSetup.padnames[i].p; i++)
            {
                g_chraiCurrentSetup.padnames[i].p = g_ptrStageSetupFile + ntohl(g_chraiCurrentSetup.padnames[i].p);
#ifdef PRINTDEBUG
                printf("/*Waypoint %d: Pad Reference %s %X */\n", i, g_chraiCurrentSetup.padnames[i].p, g_chraiCurrentSetup.padnames[i].p);
#endif
            }
        }
        if (g_chraiCurrentSetup.boundpadnames)
        {
            g_chraiCurrentSetup.boundpadnames = g_ptrStageSetupFile + ntohl(g_chraiCurrentSetup.boundpadnames);
            for (i = 0; g_chraiCurrentSetup.boundpadnames[i].p; i++)
            {
                g_chraiCurrentSetup.boundpadnames[i].p = g_ptrStageSetupFile + ntohl(g_chraiCurrentSetup.boundpadnames[i].p);
#ifdef PRINTDEBUG
                printf("/*NavMesh Group %d: name %s  %X */\n", i, g_chraiCurrentSetup.boundpadnames[i].p, g_chraiCurrentSetup.boundpadnames[i].p);
#endif
            }
        }
        if (g_chraiCurrentSetup.pathwaypoints)
        {
            g_chraiCurrentSetup.pathwaypoints = g_ptrStageSetupFile + ntohl(g_chraiCurrentSetup.pathwaypoints);
            // NAV Points - use Pads as NavPoints
            for (i = 0; (signed)(btol(g_chraiCurrentSetup.pathwaypoints[i].padID)) >= 0; i++)
            {
                g_chraiCurrentSetup.pathwaypoints[i].neighbours = g_ptrStageSetupFile + ntohl(g_chraiCurrentSetup.pathwaypoints[i].neighbours);
                btol(g_chraiCurrentSetup.pathwaypoints[i].groupNum);
                btol(g_chraiCurrentSetup.pathwaypoints[i].dist);
#ifdef PRINTDEBUG
                printf("/*waypoint %d: pad %d (%s), Group %d, Neighbors (", i, g_chraiCurrentSetup.pathwaypoints[i].padID, (g_chraiCurrentSetup.padnames ? g_chraiCurrentSetup.padnames[i].p : "No Name"), g_chraiCurrentSetup.pathwaypoints[i].groupNum);
#endif
                for (j = 0; (signed)(btol(g_chraiCurrentSetup.pathwaypoints[i].neighbours[j])) >= 0; j++)
                {
#ifdef PRINTDEBUG
                    printf("%d, ", g_chraiCurrentSetup.pathwaypoints[i].neighbours[j]);
#endif
                }
#ifdef PRINTDEBUG
                printf("-1 )*/\n");
#endif
            }
        }
        if (g_chraiCurrentSetup.waypointgroups)
        {
            g_chraiCurrentSetup.waypointgroups = g_ptrStageSetupFile + ntohl(g_chraiCurrentSetup.waypointgroups);
            // NAV Mesh, make a mesh from NAV Points
            for (i = 0; g_chraiCurrentSetup.waypointgroups[i].neighbours; i++)
            {
                g_chraiCurrentSetup.waypointgroups[i].neighbours = g_ptrStageSetupFile + ntohl(g_chraiCurrentSetup.waypointgroups[i].neighbours);
                g_chraiCurrentSetup.waypointgroups[i].waypoints  = g_ptrStageSetupFile + ntohl(g_chraiCurrentSetup.waypointgroups[i].waypoints);
#ifdef PRINTDEBUG
                printf("\nwaygrp %d %s: nb %X, wp%X, dst%d\n", i, g_chraiCurrentSetup.boundpadnames[i].p, g_chraiCurrentSetup.waypointgroups[i].neighbours, g_chraiCurrentSetup.waypointgroups[i].waypoints, g_chraiCurrentSetup.waypointgroups[i].dist);
#endif
                for (j = 0; (signed)(btol(g_chraiCurrentSetup.waypointgroups[i].neighbours[j])) >= 0; j++)
                {
#ifdef PRINTDEBUG
                    printf("%d, ", g_chraiCurrentSetup.waypointgroups[i].neighbours[j]);
#endif
                }
#ifdef PRINTDEBUG
                printf("\n");
#endif
                for (j = 0; (signed)(btol(g_chraiCurrentSetup.waypointgroups[i].waypoints[j])) >= 0; j++)
                {
#ifdef PRINTDEBUG
                    printf("%d, ", g_chraiCurrentSetup.waypointgroups[i].waypoints[j]);
#endif
                }
            }
        }

        if (g_chraiCurrentSetup.intro)
        {
            g_chraiCurrentSetup.intro = g_ptrStageSetupFile + ntohl(g_chraiCurrentSetup.intro);
#ifdef PRINTDEBUG
            printf("\nintro %X: \n", g_chraiCurrentSetup.intro);
#endif
        }

        if (g_chraiCurrentSetup.propDefs)
        {
            g_chraiCurrentSetup.propDefs = g_ptrStageSetupFile + ntohl(g_chraiCurrentSetup.propDefs);
            // g_chraiCurrentSetup.propDefs = g_ptrStageSetupFile + ntohl(g_chraiCurrentSetup.propDefs);
#ifdef PRINTDEBUG
            printf("\npropDefs %X: \n", g_chraiCurrentSetup.propDefs);
#endif
        }
        if (g_chraiCurrentSetup.patrolpaths)
        {
            g_chraiCurrentSetup.patrolpaths = g_ptrStageSetupFile + ntohl(g_chraiCurrentSetup.patrolpaths);

            for (i = 0; g_chraiCurrentSetup.patrolpaths[i].waypoints; i++)
            {
                g_chraiCurrentSetup.patrolpaths[i].waypoints = g_ptrStageSetupFile + ntohl(g_chraiCurrentSetup.patrolpaths[i].waypoints);

#ifdef PRINTDEBUG
                printf("path %d: wp %X, ID%d, isLoop %d, len %d\n", i, g_chraiCurrentSetup.patrolpaths[i].waypoints, g_chraiCurrentSetup.patrolpaths[i].ID, g_chraiCurrentSetup.patrolpaths[i].isLoop, g_chraiCurrentSetup.patrolpaths[i].len);
#endif
                for (j = 0; g_chraiCurrentSetup.patrolpaths[i].waypoints[j] > -1; j++)
                {
                    g_chraiCurrentSetup.patrolpaths[i].waypoints[j] = ntohl(g_chraiCurrentSetup.patrolpaths[i].waypoints[j]);
#ifdef PRINTDEBUG
                    printf("wp %d: %d\n", j, g_chraiCurrentSetup.patrolpaths[i].waypoints[j]);
#endif
                }
                g_chraiCurrentSetup.patrolpaths[i].len = j;
            }
        }
        if (g_chraiCurrentSetup.ailists)
        {
            g_chraiCurrentSetup.ailists = g_ptrStageSetupFile + ntohl(g_chraiCurrentSetup.ailists);
            for (i = 0; g_chraiCurrentSetup.ailists[i].ailist; i++)
            {
                g_chraiCurrentSetup.ailists[i].ailist = g_ptrStageSetupFile + ntohl(g_chraiCurrentSetup.ailists[i].ailist);
                btol(g_chraiCurrentSetup.ailists[i].ID);
#ifdef PRINTDEBUG
                printf("/*AI %d: Lst %X, ID %d */\n", i, g_chraiCurrentSetup.ailists[i].ailist, g_chraiCurrentSetup.ailists[i].ID);
#endif
            }
        }
        if (g_chraiCurrentSetup.pads)
        {
            g_chraiCurrentSetup.pads = g_ptrStageSetupFile + ntohl(g_chraiCurrentSetup.pads);
            for (i = 0; g_chraiCurrentSetup.pads[i].plink; i++)
            {
                for (j = 0; j < 3; j++)
                {
                    btol(g_chraiCurrentSetup.pads[i].pos.f[j]);
                    btol(g_chraiCurrentSetup.pads[i].up.f[j]);
                    btol(g_chraiCurrentSetup.pads[i].look.f[j]);
                }

                g_chraiCurrentSetup.pads[i].plink = g_ptrStageSetupFile + ntohl(g_chraiCurrentSetup.pads[i].plink);

                if (g_chraiCurrentSetup.pads[i].plink == 0)
                {
                    printf("//pad number %d has no stan! (%s)\n", i, g_chraiCurrentSetup.pads[i].plink);
                }
            }
        }
        if (g_chraiCurrentSetup.boundpads)
        {
            g_chraiCurrentSetup.boundpads = g_ptrStageSetupFile + ntohl(g_chraiCurrentSetup.boundpads);
            for (i = 0; g_chraiCurrentSetup.boundpads[i].plink; i++)
            {
                for (j = 0; j < 6; j++)
                {
                    if (j < 3)
                    {
                        btol(g_chraiCurrentSetup.boundpads[i].pos.f[j]);
                        btol(g_chraiCurrentSetup.boundpads[i].up.f[j]);
                        btol(g_chraiCurrentSetup.boundpads[i].look.f[j]);
                    }
                    btol(g_chraiCurrentSetup.boundpads[i].bbox.f[j]);
                }
                g_chraiCurrentSetup.boundpads[i].plink = g_ptrStageSetupFile + ntohl(g_chraiCurrentSetup.boundpads[i].plink);

                if (g_chraiCurrentSetup.boundpads[i].plink == 0)
                {
                    printf("//vol number %d has no stan! (%s)\n", i, g_chraiCurrentSetup.boundpads[i].plink);
                }
            }
        }
        printf("test12/2/25 1505\n\n\n");
        return 1;

#pragma endregion // foreach item in setupheader

        printf("/*\n* This file was automatically generated from %s\n*\n* $Date: %s*/\n\n#include <bondtypes.h>\n#include <bondaicommands.h>\n\n// forward declarations\nwaypoint       pathwaypoints[];\nwaygroup       pathsets[];\ns32            intro[];\ns32            propDefs[];\nPathRecord     patrolpaths[];\nAIListRecord   ailists[];\nPadRecord      pads[];\nBoundPadRecord vols[];\nchar          *NAVPADs[];\nchar          *NAVnames[];\n\nstagesetup %s = {\n    &pathwaypoints,\n    &pathsets,\n    &intro,\n    &propDefs,\n    &patrolpaths,\n    &ailists,\n    &pads,\n    &vols,\n    &NAVPADs,\n    &NAVnames\n};\n\n", fname, ctime(&current_time), fname);

        // retain file-order
        for (lastAddr = g_ptrStageSetupFile; lastAddr < g_ptrStageSetupFile + fsize; lastAddr += 4)
        {
            if (g_chraiCurrentSetup.pathwaypoints && lastAddr == &g_chraiCurrentSetup.pathwaypoints[0])
            {
                printf("char pathwaypoints[] = {\n");

                printf("   };\n //nothing to see here yet\n");
            }
            if (g_chraiCurrentSetup.waypointgroups && lastAddr == &g_chraiCurrentSetup.waypointgroups[0])
            {
                printf("char waypointgroups[] = {\n");

                printf("   };\n //nothing to see here yet\n");
            }
            if (g_chraiCurrentSetup.intro && lastAddr == &g_chraiCurrentSetup.intro[0])
            {
                printf("char intro[] = {\n");
                Intro(g_chraiCurrentSetup.intro);

                printf("   };\n\n");
            }
            if (g_chraiCurrentSetup.propDefs && lastAddr == &g_chraiCurrentSetup.propDefs[0])
            {
                printf("char propDefs[] = {\n");

                printf("   };\n //nothing to see here yet\n");
            }
            if (g_chraiCurrentSetup.patrolpaths && lastAddr == &g_chraiCurrentSetup.patrolpaths[0])
            {
                printf("char patrolpaths[] = {\n");

                printf("   };\n //nothing to see here yet\n");
            }

            if (g_chraiCurrentSetup.ailists)
            {
                for (i = 0; g_chraiCurrentSetup.ailists[i].ailist; i++)
                {
                    if (lastAddr == g_chraiCurrentSetup.ailists[i].ailist)
                    {
                        if (isChrAIListID(g_chraiCurrentSetup.ailists[i].ID))
                        {
                            printf("u8 chrAI_%hd[] = {\n    #define THIS chrai_%hd\n\n", getChrAIListID(g_chraiCurrentSetup.ailists[i].ID), getChrAIListID(g_chraiCurrentSetup.ailists[i].ID));
                        }
                        else
                        {
                            printf("u8 bgAi_%hd[] = {\n    #define THIS bgai_%hd\n\n", getBGAIListID(g_chraiCurrentSetup.ailists[i].ID), getBGAIListID(g_chraiCurrentSetup.ailists[i].ID));
                        }
                        hasName = 0;
                        ai(g_chraiCurrentSetup.ailists[i].ailist, g_chraiCurrentSetup.ailists[i].ID);

                        // un-closed loops
                        if (top != -1) printf("\nUnused Labels Found!"), displayStack();
                        top = -1;

                        printf("    #undef THIS \n};%s%s\n\n", hasName ? " //Possible Name " : "", hasName ? hasName : "");
                    }
                }
            }

            if (g_chraiCurrentSetup.ailists && lastAddr == &g_chraiCurrentSetup.ailists[0])
            {
                printf("AIListRecord ailists[] = {\n");
                for (i = 0; g_chraiCurrentSetup.ailists[i].ailist; i++)
                {
                    if (isChrAIListID(g_chraiCurrentSetup.ailists[i].ID))
                    {
                        printf("    { &chrAI_%hd, chrai_%hd }, \n", getChrAIListID(g_chraiCurrentSetup.ailists[i].ID), getChrAIListID(g_chraiCurrentSetup.ailists[i].ID));
                    }
                    else if (isBGAIListID(g_chraiCurrentSetup.ailists[i].ID))
                    {
                        printf("    { &bgAi_%hd, bgai_%hd },\n", getBGAIListID(g_chraiCurrentSetup.ailists[i].ID), getBGAIListID(g_chraiCurrentSetup.ailists[i].ID));
                    }
                    else
                    {
                        printf("    /*Invalid ID*/{ &Ai_%hd, %hd }, //index %d\n", g_chraiCurrentSetup.ailists[i].ID, g_chraiCurrentSetup.ailists[i].ID, i);
                    }
                }
                printf("    { NULL, 0 }\n};\n");
            }

            if (g_chraiCurrentSetup.pads && lastAddr == &g_chraiCurrentSetup.pads[0])
            {
                printf("PadRecord pads[] = {\n    //%30s%30s%40s%40s\tstan\n", "pos", "up", "look", "plink");
                for (i = 0; g_chraiCurrentSetup.pads[i].plink; i++)
                {
                    printf("    {{%# 10gf, %# 10gf, %# 10gf},\t{%# 10gf, %# 10gf, %# 10gf},\t{%# 10gf, %# 10gf, %# 10gf}, \t\"%#s\", NULL }, \n", g_chraiCurrentSetup.pads[i].pos.x, g_chraiCurrentSetup.pads[i].pos.y, g_chraiCurrentSetup.pads[i].pos.z, g_chraiCurrentSetup.pads[i].up.x, g_chraiCurrentSetup.pads[i].up.y, g_chraiCurrentSetup.pads[i].up.z, g_chraiCurrentSetup.pads[i].look.x, g_chraiCurrentSetup.pads[i].look.y, g_chraiCurrentSetup.pads[i].look.z, g_chraiCurrentSetup.pads[i].plink);
                }
                printf("    {  0 }\n};\n");
            }
            if (g_chraiCurrentSetup.boundpads && lastAddr == &g_chraiCurrentSetup.boundpads[0])
            {
                printf("BoundPadRecord boundpads[] = {\n    //%30s%30s%40s%40s\tstan\tbbox\n", "pos", "up", "look", "plink");
                for (i = 0; g_chraiCurrentSetup.boundpads[i].plink; i++)
                {
                    printf("    {{%# 10gf, %# 10gf, %# 10gf},\t{%# 10gf, %# 10gf, %# 10gf},\t{%# 10gf, %# 10gf, %# 10gf}, \t\"%#s\", NULL, {%# 10gf, %# 10gf, %# 10gf, %# 10gf, %# 10gf, %# 10gf} }, \n", g_chraiCurrentSetup.boundpads[i].pos.x, g_chraiCurrentSetup.boundpads[i].pos.y, g_chraiCurrentSetup.boundpads[i].pos.z, g_chraiCurrentSetup.boundpads[i].up.x, g_chraiCurrentSetup.boundpads[i].up.y, g_chraiCurrentSetup.boundpads[i].up.z, g_chraiCurrentSetup.boundpads[i].look.x, g_chraiCurrentSetup.boundpads[i].look.y, g_chraiCurrentSetup.boundpads[i].look.z, g_chraiCurrentSetup.boundpads[i].plink, g_chraiCurrentSetup.boundpads[i].bbox.f[0], g_chraiCurrentSetup.boundpads[i].bbox.f[1], g_chraiCurrentSetup.boundpads[i].bbox.f[2], g_chraiCurrentSetup.boundpads[i].bbox.f[3], g_chraiCurrentSetup.boundpads[i].bbox.f[4], g_chraiCurrentSetup.boundpads[i].bbox.f[5]);
                }
                printf("    {  0 }\n};\n");
            }
            if (g_chraiCurrentSetup.padnames && lastAddr == &g_chraiCurrentSetup.padnames[0])
            {
                printf("char propDefs[] = {\n");

                printf("   };\n //nothing to see here yet");
            }
            if (g_chraiCurrentSetup.boundpadnames && lastAddr == &g_chraiCurrentSetup.boundpadnames[0])
            {
                printf("char propDefs[] = {\n");

                printf("   };\n //nothing to see here yet");
            }
        }

        printf("#pragma region Enums \n //Move the following Enums to the top of file\n");

        if (g_chraiCurrentSetup.ailists)
        {
            printf("    enum chrAilist\n    {\n");
            for (i = 0; g_chraiCurrentSetup.ailists[i].ailist; i++)
            {
                if (g_chraiCurrentSetup.ailists[i].ID)
                {
                    if (isChrAIListID(g_chraiCurrentSetup.ailists[i].ID))
                    {
                        if (getChrAIListID(g_chraiCurrentSetup.ailists[i].ID))
                        {
                            printf("        chrai_%hd,\n", getChrAIListID(g_chraiCurrentSetup.ailists[i].ID));
                        }
                        else
                        {
                            printf("        chrai_%hd = setChrAIListID(%hd),\n", getChrAIListID(g_chraiCurrentSetup.ailists[i].ID), getChrAIListID(g_chraiCurrentSetup.ailists[i].ID));
                        }
                    }
                }
            }
            printf("    };\n");
        }
        if (g_chraiCurrentSetup.ailists)
        {
            printf("    enum bgAilist\n    {\n");
            for (i = 0; g_chraiCurrentSetup.ailists[i].ailist; i++)
            {
                if (g_chraiCurrentSetup.ailists[i].ID)
                {
                    if (isBGAIListID(g_chraiCurrentSetup.ailists[i].ID))
                    {
                        if (getBGAIListID(g_chraiCurrentSetup.ailists[i].ID))
                        {
                            printf("        bgai_%hd,\n", getBGAIListID(g_chraiCurrentSetup.ailists[i].ID));
                        }
                        else
                        {
                            printf("        bgai_%hd = setBGAIListID(%hd),\n", getBGAIListID(g_chraiCurrentSetup.ailists[i].ID), getBGAIListID(g_chraiCurrentSetup.ailists[i].ID));
                        }
                    }
                }
            }
            printf("    };\n");
        }

        SortEnums(labels);
        SortEnums(keys);
        SortEnums(tags);
        SortEnums(chrs);
        SortEnums(pads);

        printenum(labels, "lbl");
        printenum(tags, "tag");
        printenum(keys, "key");
        printenum(chrs, "chr");
        printenum(pads, "pad");
        i = 0;

        if (SubIDs[i].id)
        {
            printf("    #define SETUPSUBROUTINES(ID) ");
            while (SubIDs[i].id != 0)
            {
                if (isBGAIListID(SubIDs[i].id))
                {
                    printf("        (ID == bgai_%hd)", getBGAIListID(SubIDs[i].id));
                }
                else
                {
                    printf("        (ID == chrai_%hd)", getChrAIListID(SubIDs[i].id));
                }
                i++;
                if (SubIDs[i].id) printf(" | \\\n");
            }
            printf("\n");
        }
        printf("#pragma endregion\n");
    }

    free(g_ptrStageSetupFile);

    return 0;
}
