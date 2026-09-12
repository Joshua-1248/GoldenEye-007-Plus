#include "chrai.h"
#include "bg.h"
#include "bgfog.h"
#include "bondinv.h"
#include "bondview.h"
#include "cheat.h"
#include "chr.h"
#include "chraction.h"
#include "chraidata.h"
#include "file.h"
#include "gun.h"
#include "initanitable.h"
#include "language.h"
#include "loadobjectmodel.h"
#include "lv.h"
#include "math_atan2f.h"
#include "math_ceil.h"
#include "math_floor.h"
#include "model.h"
#include "mp_music.h"
#include "objecthandler.h"
#include "objective_status.h"
#include "player.h"
#include "propobj.h"
#include "stan.h"
#include <assert.h>
#include <bondaicommands.h>
#include <bondgame.h>
#include <bondtypes.h>
#include <boss.h>
#include <limits.h>
#include <music.h>
#include <random.h>
#include <snd.h>
#include <ultra64.h>

// hack? used to match as called with 2 args, but decompiled code takes 1
extern s32 objectiveGetStatus_WEAK(s32 objectiveNum, s32);

// bss
/**
 * Address 0x80069B70.
 */
sfxRecord  sfx_related[SFX_RELATED_LEN];

/**
 * Play Audio in slot X from Prop or Pad
 * @param slot: Where audio is loaded
 */
void       audioPlayFromProp2(s32 slot)
{
    int        tempvol;
    sfxRecord *sfx = &sfx_related[slot]; // always added to stack anyway, cleaner to use
    int        clock_timer;

    if ((sfx->state) && (sndGetPlayingState(sfx->state)))
    {
        if (sfx->pos)
        {
            sfx->Volume = sub_GAME_7F0539E4(sfx->pos);
        }
        else
        {
            if (sfx->Obj && sfx->Obj->prop)
            {
                sfx->Volume = sub_GAME_7F0539E4(&sfx->Obj->runtime_pos);
            }
        }

        tempvol = sfx->Volume;
        if (sfx->sfxID >= 0)
        {
            clock_timer = g_ClockTimer;
            if (clock_timer < sfx->sfxID)
            {
                tempvol = (((sfx->Volume - sfx->Volume2) * clock_timer) / sfx->sfxID) + sfx->Volume2;
            }
            sfx->sfxID = sfx->sfxID - clock_timer;
        }
        if (lvlGetControlsLockedFlag() != 0)
        {
            tempvol = 0;
        }
        if (tempvol != sfx->Volume2)
        {
            sndCreatePostEvent(sfx->state, 8, tempvol);
            sfx->Volume2 = tempvol;
            return;
        }
        return;
    }
    sfx->Volume2 = 0;
}

/**
 Play All Sounds in all slots
*/
void loop_set_sound_effect_all_slots(void)
{
    int i;
    for (i = 0; i < SFX_RELATED_LEN; i++)
    {
        audioPlayFromProp2(i);
    }
}

/**
 * Load Audio Slot with sound and Play
 * @param slot: where to load sound
 * @param soundIndex: SFX_ID
 */
void audioPlayFromProp(s32 slot, s16 soundIndex)
{
    sfxRecord *sfx = NULL; // always added to stack anyway, cleaner to use
    //"Existing ai sound number %d!\n"
    if (slot >= 0 && slot < SFX_RELATED_LEN)
    {
        if (!sfx_related[slot].state || !sndGetPlayingState(sfx_related[slot].state))
        {
            sfx = &sfx_related[slot];

            sfx->Volume  = SHRT_MAX;
            sfx->Volume2 = SHRT_MAX;
            sfx->sfxID   = -1;
            sfx->pos     = NULL;
            sfx->Obj     = NULL;
        }
#ifdef DEBUG
        else
        {
            osSyncPrintf("Existing ai sound number %d!\n", slot);
        }
#endif
    }
    sndPlaySfx(g_musicSfxBufferPtr, soundIndex, sfx);
}

/**
 Stop All Sounds in all slots
*/
void sub_GAME_7F0349BC(s32 slot)
{
    if ((slot >= 0) && (slot < SFX_RELATED_LEN))
    {
        sndDeactivate(sfx_related[slot].state);
    }
}

/**
 * Get AI Command Size in bytes
 * @param AIList: u8 Pointer to The AI list containing the command
 * @param offset: The offset (in bytes) to the command you want the size of
 * @return The number of bytes of AI command
 * @canonical name
 */
s32 chraiitemsize(u8 *AIList, s32 offset)
{
    // matches only as u8* despite text evidence of ai->val structs
    switch (AIList[offset])
    {
        // Cant Use CMD Builder due to different order...
#if defined(USECMDBUILDER)
    #ifndef _SYNHILITE
        #define _AI_CMD(CMD)        /*  \
                                     */ \
            case CAT(AI_, CMDNAME): /*  \
                                     */ \
                return CAT(CAT(AI_, CMDNAME), _LENGTH);
        #define _AI_DEBUG()
        #define _AI_CMD_POLYMORPH(CMD, A, P, Q, D)
        #define DEFINE(x)
        #include <aicommands.def>
    #endif
#endif
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
        case AI_IFImFiring: /* enum = 232 despite following enum = 21 in the list */
            return sizeof(AiIFImFiringRecord);
        case AI_IFImFiringAndLockedForward: /* enum = 231 despite being followed by enum = 22 in the list */
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
            s32 pos = offset + 1;
            while (AIList[pos] != 0)
            {
                ++pos;
            }
            return (pos - offset) + 1;
        }
        default:
#if defined(ENABLE_LOG)
            osSyncPrintf("chraiitemsize: unknown type %d!\n", *AIList);
#endif
            return 1;
    }
}

/**
 * Get ID of AIList
 * @param AIList: Ailist to get ID of
 * @return ID of AIList
 */
s32 chraiGetAIListID(AIRecord *AIList, bool *isGlobalAIList)
{
    s32 i;

    if (g_CurrentSetup.ailists)
    {
        for (i = 0; g_CurrentSetup.ailists[i].ailist; i++)
        {
            if (g_CurrentSetup.ailists[i].ailist == AIList)
            {
                *isGlobalAIList = FALSE;
                return g_CurrentSetup.ailists[i].ID;
            }
        }
    }

    for (i = 0; g_GlobalAILists[i].ailist; i++)
    {
        if (g_GlobalAILists[i].ailist == AIList)
        {
            *isGlobalAIList = TRUE;
            return g_GlobalAILists[i].ID;
        }
    }

    return -1;
}

/**
 * GoTo Label
 * @param AIlist: AIList containing label
 * @param LabelNum: Integer/enum ID to go to
 * @return Offset of label from beggining of AIList.
 */
s32 chraiGoToLabel(AIRecord *AIList, s32 Offset, u8 LabelNum)
{
    s32   listID;
    char *debAIListTypeString;
    bool  isGlobalAIList;

    for (;;)
    {
        if (AIList[Offset].cmd == AI_Label)
        {
            if (AIList[Offset].val[0] == LabelNum)
            {
                // exit loop and return offset to label number
                return Offset;
            }
        }
        else if (AIList[Offset].cmd == AI_EndList)
        {
            // restart ai list PC if next label not found - causes infinite loop outside of debug
            listID = chraiGetAIListID(AIList, &isGlobalAIList);
#ifdef DEBUG
            if (isGlobalAIList)
            {
                debAIListTypeString = "global";
            }
            else
            {
                debAIListTypeString = "local";
            }
            osSyncPrintf("AI error: endlist reached jump label=%d %s list=%d!\n", LabelNum, debAIListTypeString, listID);
#endif
            return 0;
        }

        Offset += chraiitemsize(AIList, Offset);
    }
}

AIRecord *ailistFindById(s32 ID)
{
    s32 i;

    if (!isGlobalAIListID(ID))
    {
        if (g_CurrentSetup.ailists)
        {
            for (i = 0; g_CurrentSetup.ailists[i].ailist; i++)
            {
                if (g_CurrentSetup.ailists[i].ID == ID)
                {
                    return g_CurrentSetup.ailists[i].ailist;
                }
            }
        }
    }
    else
    {
        for (i = 0; g_GlobalAILists[i].ailist; i++)
        {
            if (g_GlobalAILists[i].ID == ID)
            {
                return g_GlobalAILists[i].ailist;
            }
        }
    }
    return NULL;
}

PathRecord *pathFindById(s32 ID)
{
    int i;

    for (i = 0; g_CurrentSetup.patrolpaths[i].waypoints; i++)
    {
        if (ID == g_CurrentSetup.patrolpaths[i].ID)
        {
            return &g_CurrentSetup.patrolpaths[i];
        }
    }

    return NULL;
}

// forward
extern PadRecord      *g_CameraLookAtBondPad;
extern CutsceneRecord *gBondViewCutscene;
extern enum CAMERAMODE dword_CODE_bss_80079A18;
extern s32             dword_CODE_bss_80079A1C;
extern vec3d           g_ForceBondMoveOffset;
// CODE.bss:80079A00
extern f32             flt_CODE_bss_80079A00;
// CODE.bss:80079A04
extern f32             flt_CODE_bss_80079A04;
// CODE.bss:80079A08
extern f32             flt_CODE_bss_80079A08;
// CODE.bss:80079A0C
extern f32             flt_CODE_bss_80079A0C;
// CODE.bss:80079A10
extern f32             flt_CODE_bss_80079A10;
// CODE.bss:80079A14
extern s32             dword_CODE_bss_80079A14;
extern bool            g_isBondKIA;
// end forward

/**
 Execute AI List from Character, Stage, Vehichle(truck) or Aircraft(heli)
 Note: GE has little error checking (eg, using a chr action on aircraft) - this was fixed in PD
 @param *Entityp: Pointer to Entity (non-typed)
 @param EntityType: PROPTYPE_CHR or PROPTYPE_OBJ
 @param         PROPTYPE_CHR = Character or BG
 @param         PROPTYPE_OBJ = Object (Vehichle or Aircraft)
 @canonical name ~ maybe
*/
void                   ai(PropDefHeaderRecord *Entityp, PROP_TYPE EntityType)
{
    /*
     * (void *Param, int ParamType) is the correct way to pass a variable
     *    "object" eg Param can be Either ChrRecord or VehichleRecord
     *
     * Function Name derived from internal strings:
     *    ai(void) enery tune on (%d, %d, %d)
     *    ai(void) enery tune off (%d)
     *
     * Stack requires that each case declare its own AIRecord variable
     *
     */

    ChrRecord      *ChrEntityp      = NULL;
    VehichleRecord *VehichleEntityp = NULL;
    AircraftRecord *AircraftEntityp = NULL;
    AIRecord       *AiListp         = NULL;
    s32             Offset;

    if (EntityType == PROP_TYPE_CHR)
    {
        ChrEntityp = Entityp;
    }
    else if (EntityType == PROP_TYPE_OBJ)
    {
        if (Entityp->type == PROPDEF_VEHICHLE)
        {
            VehichleEntityp = Entityp;
        }
        else if (Entityp->type == PROPDEF_AIRCRAFT)
        {
            AircraftEntityp = Entityp;
        }
    }

    // Load ailist
    if (ChrEntityp)
    {
        Offset  = ChrEntityp->aioffset;
        AiListp = ChrEntityp->ailist;
    }
    else if (VehichleEntityp)
    {
        Offset  = VehichleEntityp->aioffset;
        AiListp = VehichleEntityp->ailist;
    }
    else if (AircraftEntityp)
    {
        Offset  = AircraftEntityp->aioffset;
        AiListp = AircraftEntityp->ailist;
    }

    if (AiListp) // continue if Has ailist (check once)
    {
        // loop forever (or until broken)
        for (;;)
        {
            /*
             * GE uses long Switch/case while PD uses Bool functions and tests
             * for TRUE/FALSE if(funcpointer[ai]) break;
             */
            switch ((AiListp + Offset)->cmd)
            {
                // unfortunately we cannot use the cmdbuilder in matching rom as the ordering is not sequential
#ifdef USECMDBUILDER
    #define _AI_DEBUG_ID(CMD, AI_NUMBER_OF_PARAMS, PARAM, DESC)
    #define _AI_CMD_POLYMORPH(C, N, P1, P2, D)
    #define _AI_CMD_ID(CMD, AI_NUMBER_OF_PARAMS, PARAM, DESC, CODE) /*  HACK: Multiline Comments make Newlines in macro */ \
        case CAT(CAT(AI_, CMD), ):                                  /*                                                     \
                                                                     */                                                    \
            CODE
    #include "aicommands.def"
#else
                case AI_GotoNext:
                {
                    AiGotoNextRecord *ai = AiListp + Offset;
                    Offset               = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
    #ifdef ENABLE_LOG
                    osSyncPrintf("GOTO Next (%d)\n", ai->GOTOLABEL);
    #endif
                    break;
                }
                case AI_GotoFirst:
                {
                    AiGotoFirstRecord *ai = AiListp + Offset;
                    Offset                = chraiGoToLabel(AiListp, 0, ai->GOTOLABEL);
    #ifdef ENABLE_LOG
                    osSyncPrintf("GOTO First (%d)\n", ai->GOTOLABEL);
    #endif
                    break;
                }
                case AI_Label:
                {
                    Offset += sizeof(AiLabelRecord);
                    break;
                }
                case AI_Yield:
                {
                    Offset += sizeof(AiYieldRecord);
                    if (ChrEntityp)
                    {
                        ChrEntityp->ailist   = AiListp;
                        ChrEntityp->aioffset = Offset;
                    }
                    else if (VehichleEntityp)
                    {
                        VehichleEntityp->ailist   = AiListp;
                        VehichleEntityp->aioffset = Offset;
                    }
                    else if (AircraftEntityp)
                    {
                        AircraftEntityp->ailist   = AiListp;
                        AircraftEntityp->aioffset = Offset;
                    }
                    return;
                }
                case AI_EndList:
                {
                    // Not an error to be here, same as yield except without pushing offset past it. (just keeps looping)
    #ifdef ENABLE_LOG
        #ifdef IS_PD
                    listID = chraiGetAIListID(AIList, &isGlobalAIList);
                    if (isGlobalAIList)
                    {
                        debAIListTypeString = "global";
                    }
                    else
                    {
                        debAIListTypeString = "local";
                    }
                    osSyncPrintf("AI error: endlist reached %s list=%d!\n", debAIListTypeString, listID);
        #endif
                    osSyncPrintf("AI error: endlist reached!\n");
    #endif

                    return;
                }
                case AI_SetChrAiList:
                {
                    AiSetChrAiListRecord *ai = AiListp + Offset;              /* needed for stack count inflation */
                    ChrRecord            *chr;                                // ok, so mips does not hoist vars in stack, they are in order so must be declaired here
                    u16                   AI_LIST_ID = ntohs(ai->AI_LIST_ID); /* This is the only way to match despite assetrs below */
                    u8                    CHR_NUM    = ai->CHR_NUM;

                    if (CHR_NUM == ((u8)CHR_SELF))
                    {
                        AiListp = ailistFindById(AI_LIST_ID);
                        Offset  = 0;
                    }
                    else
                    {
                        chr = chrFindById(ChrEntityp, CHR_NUM);
                        if (chr)
                        {
                            chr->ailist   = ailistFindById(AI_LIST_ID);
                            chr->aioffset = 0;
                            chr->sleep    = 0;
                        }
                        Offset += sizeof(AiSetChrAiListRecord);
                    }
                    break;
                }
                case AI_SetReturnAiList:
                {
                    AiSetReturnAiListRecord *ai         = AiListp + Offset;
                    u16                      AI_LIST_ID = ntohs(ai->AI_LIST_ID);

                    if (ChrEntityp)
                    {
                        ChrEntityp->aireturnlist = AI_LIST_ID;
                    }
                    else if (VehichleEntityp)
                    {
                        VehichleEntityp->aireturnlist = AI_LIST_ID;
                    }
                    else if (AircraftEntityp)
                    {
                        AircraftEntityp->aireturnlist = AI_LIST_ID;
                    }

                    Offset += sizeof(AiSetReturnAiListRecord);
                    break;
                }
                case AI_Return:
                {
                    if (ChrEntityp)
                    {
                        AiListp = ailistFindById(ChrEntityp->aireturnlist);
                    }
                    else if (VehichleEntityp)
                    {
                        AiListp = ailistFindById(VehichleEntityp->aireturnlist);
                    }
                    else if (AircraftEntityp)
                    {
                        AiListp = ailistFindById(AircraftEntityp->aireturnlist);
                    }
                    Offset = 0;
                    break;
                }
                case AI_Stop:
                {
                    chraiStopAnimation(ChrEntityp);
                    Offset += sizeof(AiStopRecord);
                    break;
                }
                case AI_Kneel:
                {
                    check_if_able_to_then_kneel(ChrEntityp);
                    Offset += sizeof(AiKneelRecord);
                    break;
                }
                case AI_PlayAnimation:
                {
                    AiPlayAnimationRecord *ai = AiListp + Offset;
                    s32                    startframe, anim_id, zero, endframe;

                    anim_id    = ntohs(ai->ANIMATION_ID);
                    startframe = ntohs(ai->START_TIME30);
                    endframe   = ntohs(ai->END_TIME30);

                    if (startframe == (u16)-1)
                    {
                        startframe = 0;
                    }
                    if (endframe == (u16)-1)
                    {
                        endframe = -1;
                    }

                    if (ChrEntityp)
                    {
                        check_if_able_to_then_perform_animation(ChrEntityp, anim_id, startframe, endframe, ai->BITFIELD, ai->INTERPOL_TIME60);
                    }
                    else if (AircraftEntityp)
                    {
                        zero = 0; // debug value maybe?
                        modelSetAnimation(AircraftEntityp->model, animation_table_ptrs2[anim_id], zero, startframe, 0.5f, (s32)ai->INTERPOL_TIME60);
                        if (endframe >= 0)
                        {
                            modelSetAnimEndFrame(AircraftEntityp->model, endframe);
                        }
                    }
                    Offset += sizeof(AiPlayAnimationRecord);
                    break;
                }
                case AI_IFPlayingAnimation:
                {
                    AiIFPlayingAnimationRecord *ai = (AiListp + Offset);

                    if (ChrEntityp->actiontype == ACT_ANIM)
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFPlayingAnimationRecord);
                    }
                    break;
                }
                case AI_PointAtBond:
                {
                    chrTrySurprisedOneHand(ChrEntityp);
                    Offset += sizeof(AiPointAtBondRecord);
                    break;
                }
                case AI_LookSurprised:
                {
                    chrTrySurprisedLookAround(ChrEntityp);
                    Offset += sizeof(AiLookSurprisedRecord);
                    break;
                }
                case AI_IFImOnPatrolOrStopped:
                {
                    AiIFImOnPatrolOrStoppedRecord *ai = AiListp + Offset;
                    if (chrHasStoppedOrPatroling(ChrEntityp))
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFImOnPatrolOrStoppedRecord);
                    }
                    break;
                }
                case AI_IFChrDyingOrDead:
                {
                    AiIFChrDyingOrDeadRecord *ai  = AiListp + Offset;
                    ChrRecord                *chr = chrFindById(ChrEntityp, ai->CHR_NUM);

                    if (!chr || chrIsDead(chr))
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFChrDyingOrDeadRecord);
                    }
                    break;
                }
                case AI_IFChrDoesNotExist:
                {
                    AiIFChrDoesNotExistRecord *ai  = AiListp + Offset;
                    ChrRecord                 *chr = chrFindById(ChrEntityp, ai->CHR_NUM);

                    if (!chr || !chr->model)
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFChrDoesNotExistRecord);
                    }
                    break;
                }
                case AI_IFISeeBond:
                {
                    AiIFISeeBondRecord *ai = AiListp + Offset;

                    if (chrCheckTargetInSight(ChrEntityp))
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFISeeBondRecord);
                    }
                    break;
                }

                case AI_TRYSidestepping:
                {
                    AiTRYSidesteppingRecord *ai = AiListp + Offset;

                    if (actor_steps_sideways(ChrEntityp))
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiTRYSidesteppingRecord);
                    }
                    break;
                }
                case AI_TRYSideHopping:
                {
                    AiTRYSideHoppingRecord *ai = AiListp + Offset;

                    if (actor_hops_sideways(ChrEntityp))
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiTRYSideHoppingRecord);
                    }
                    break;
                }
                case AI_TRYSideRunning:
                {
                    AiTRYSideRunningRecord *ai = AiListp + Offset;

                    if (actor_jogs_sideways(ChrEntityp))
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiTRYSideRunningRecord);
                    }
                    break;
                }
                case AI_TRYFiringWalk:
                {
                    AiTRYFiringWalkRecord *ai = AiListp + Offset;

                    if (actor_walks_and_fires(ChrEntityp))
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiTRYFiringWalkRecord);
                    }
                    break;
                }
                case AI_TRYFiringRun:
                {
                    AiTRYFiringRunRecord *ai = AiListp + Offset;

                    if (actor_runs_and_fires(ChrEntityp))
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiTRYFiringRunRecord);
                    }
                    break;
                }
                case AI_TRYFiringRoll:
                {
                    AiTRYFiringRollRecord *ai = AiListp + Offset;

                    if (actor_rolls_fires_crouched(ChrEntityp))
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiTRYFiringRollRecord);
                    }
                    break;
                }
                case AI_TRYFireOrAimAtTarget:
                {
                    AiTRYFireOrAimAtTargetRecord *ai         = AiListp + Offset;
                    s32                           targetid   = ntohs(ai->TARGET);
                    s32                           targettype = ntohs(ai->BITFIELD);
                    if (actor_aim_at_actor(ChrEntityp, targettype, targetid))
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiTRYFireOrAimAtTargetRecord);
                    }
                    break;
                }
                case AI_TRYFireOrAimAtTargetKneel:
                {
                    AiTRYFireOrAimAtTargetKneelRecord *ai         = AiListp + Offset;
                    s32                                targetid   = ntohs(ai->TARGET);
                    s32                                targettype = ntohs(ai->BITFIELD);
                    if (actor_kneel_aim_at_actor(ChrEntityp, targettype, targetid))
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiTRYFireOrAimAtTargetKneelRecord);
                    }
                    break;
                }
                case AI_IFImFiringAndLockedForward:
                {
                    AiIFImFiringAndLockedForwardRecord *ai = AiListp + Offset;

                    if (ChrEntityp->actiontype == ACT_ATTACK &&
                        !ChrEntityp->act_attack.type_of_motion &&
                        ChrEntityp->act_attack.attacktype & ATTACKTYPE_DONTTURN)
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFImFiringAndLockedForwardRecord);
                    }
                    break;
                }
                case AI_IFImFiring:
                {
                    AiIFImFiringRecord *ai = AiListp + Offset;

                    if (ChrEntityp->actiontype == ACT_ATTACK)
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFImFiringRecord);
                    }
                    break;
                }

                case AI_TRYFireOrAimAtTargetUpdate:
                {
                    AiTRYFireOrAimAtTargetUpdateRecord *ai         = AiListp + Offset;
                    s32                                 targetid   = ntohs(ai->TARGET);
                    s32                                 targettype = ntohs(ai->BITFIELD);
                    if (actor_fire_or_aim_at_target_update(ChrEntityp, targettype, targetid))
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiTRYFireOrAimAtTargetUpdateRecord);
                    }
                    break;
                }
                case AI_TRYFacingTarget:
                {
                    AiTRYFacingTargetRecord *ai         = AiListp + Offset;
                    s32                      targetid   = ntohs(ai->TARGET);
                    s32                      targettype = ntohs(ai->BITFIELD);
                    if (check_set_actor_standing_still(ChrEntityp, targettype, targetid))
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiTRYFacingTargetRecord);
                    }
                    break;
                }
                case AI_HitChrWithItem:
                {
                    AiHitChrWithItemRecord *ai  = AiListp + Offset;
                    ChrRecord              *chr = chrFindById(ChrEntityp, ai->CHR_NUM);
                    vec3d                   vec = New_Vector();

                    if (chr && chr->prop)
                    {
                        handles_shot_actors(chr, ai->PART_NUM, &vec, ai->ITEM_NUM, FALSE);
                    }

                    Offset += sizeof(AiHitChrWithItemRecord);
                    break;
                }
                case AI_ChrHitChr:
                {
                    AiChrHitChrRecord *ai   = AiListp + Offset;
                    ChrRecord         *chr1 = chrFindById(ChrEntityp, ai->CHR_NUM);
                    ChrRecord         *chr2 = chrFindById(ChrEntityp, ai->CHR_NUM_TARGET);

                    if (chr1 && chr2 && chr1->prop && chr2->prop)
                    {
                        PropRecord      *prop = chrGetEquippedWeaponPropWithCheck(chr1, GUNRIGHT);
                        WeaponObjRecord *weapon;
                        vec3d            vec = New_Vector();

                        if (!prop) // not Right hand? try left
                        {
                            prop = chrGetEquippedWeaponPropWithCheck(chr1, GUNLEFT);
                        }

                        if (prop) // hopefully have gun else exit
                        {
                            vec.x = chr2->prop->pos.x - chr1->prop->pos.x;
                            vec.y = chr2->prop->pos.y - chr1->prop->pos.y;
                            vec.z = chr2->prop->pos.z - chr1->prop->pos.z;
                            guNormalize(&vec.x, &vec.y, &vec.z);
                            if (prop)
                            {
                            } // prop needs upgrading to v1 instead of t
                            weapon = prop->weapon;
                            handles_shot_actors(chr2, ai->PART_NUM, &vec, weapon->weaponnum, 0);
                        }
                    }
                    Offset += sizeof(AiChrHitChrRecord);
                    break;
                }
                case AI_TRYThrowingGrenade:
                {
                    AiTRYThrowingGrenadeRecord *ai = AiListp + Offset;

                    if (actor_draws_throws_grenade_at_player_if_possible(ChrEntityp))
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiTRYThrowingGrenadeRecord);
                    }
                    break;
                }
                case AI_TRYDroppingItem:
                {
                    AiTRYDroppingItemRecord *ai       = AiListp + Offset;
                    u16                      modelnum = ntohs(ai->PROP_NUM);
                    if (chrDropItem(ChrEntityp, modelnum, ai->ITEM_NUM))
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiTRYDroppingItemRecord);
                    }
                    break;
                }

                case AI_Surrender:
                {
                    chrTrySurrender(ChrEntityp);
                    Offset += sizeof(AiSurrenderRecord);
                    break;
                }
                case AI_RemoveMe:
                {
                    chrFadeOut(ChrEntityp);
                    Offset += sizeof(AiRemoveMeRecord);
                    break;
                }
                case AI_ChrRemoveInstant:
                {
                    AiChrRemoveInstantRecord *ai  = AiListp + Offset;
                    ChrRecord                *chr = chrFindById(ChrEntityp, ai->CHR_NUM);
                    if (chr && chr->prop)
                    {
                        chr->hidden |= CHRHIDDEN_REMOVE;
                    }
                    Offset += sizeof(AiChrRemoveInstantRecord);
                    break;
                }
                case AI_TRYTriggeringAlarmAtPad:
                {
                    AiTRYTriggeringAlarmAtPadRecord *ai     = AiListp + Offset;
                    u16                              pad_id = ntohs(ai->PAD);
                    if (chrTryStartAlarm(ChrEntityp, pad_id))
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiTRYTriggeringAlarmAtPadRecord);
                    }
                    break;
                }
                case AI_AlarmOn:
                {
                    alarmActivate();
                    Offset += sizeof(AiAlarmOnRecord);
                    break;
                }
                case AI_AlarmOff:
                {
                    alarmDeactivate();
                    Offset += sizeof(AiAlarmOffRecord);
                    break;
                }
                case AI_TRYRunFromBond:
                { // run from bond
                    AiTRYRunFromBondRecord *ai = AiListp + Offset;
                    if (removed_animation_routine_27(ChrEntityp))
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiTRYRunFromBondRecord);
                    }
                    break;
                }
                case AI_TRYRunToBond:
                {
                    AiTRYRunToBondRecord *ai = AiListp + Offset;
                    if (chrGoToBond(ChrEntityp, SPEED_RUN))
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiTRYRunToBondRecord);
                    }
                    break;
                }
                case AI_TRYWalkToBond:
                {
                    AiTRYWalkToBondRecord *ai = AiListp + Offset;
                    if (chrGoToBond(ChrEntityp, SPEED_WALK))
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiTRYWalkToBondRecord);
                    }
                    break;
                }
                case AI_TRYSprintToBond:
                {
                    AiTRYSprintToBondRecord *ai = AiListp + Offset;
                    if (chrGoToBond(ChrEntityp, SPEED_SPRINT))
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiTRYSprintToBondRecord);
                    }
                    break;
                }
                case AI_TRYFindCover:
                { // Find Cover
                    AiTRYFindCoverRecord *ai = AiListp + Offset;
                    if (removed_animation_routine_2B(ChrEntityp))
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiTRYFindCoverRecord);
                    }
                    break;
                }
                case AI_TRYRunToChr:
                {
                    AiTRYRunToChrRecord *ai = AiListp + Offset;
                    if (chrGoToChr(ChrEntityp, ai->CHR_NUM, SPEED_RUN))
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiTRYRunToChrRecord);
                    }
                    break;
                }
                case AI_TRYWalkToChr:
                {
                    AiTRYWalkToChrRecord *ai = AiListp + Offset;
                    if (chrGoToChr(ChrEntityp, ai->CHR_NUM, SPEED_WALK))
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiTRYWalkToChrRecord);
                    }
                    break;
                }
                case AI_TRYSprintToChr:
                {
                    AiTRYSprintToChrRecord *ai = AiListp + Offset;

                    if (chrGoToChr(ChrEntityp, ai->CHR_NUM & 0xff, SPEED_SPRINT)) // &0xff is here to increase t reg by 1
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiTRYSprintToChrRecord);
                    }
                    break;
                }

                case AI_SetNewRandom:
                {
                    ChrEntityp->random = randomGetNext();
                    Offset += sizeof(AiSetNewRandomRecord);
                    break;
                }
                case AI_IFRandomLessThan:
                {
                    AiIFRandomLessThanRecord *ai = AiListp + Offset;

                    if (ai->BYTE > ChrEntityp->random)
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFRandomLessThanRecord);
                    }
                    break;
                }
                case AI_IFRandomGreaterThan:
                {
                    AiIFRandomGreaterThanRecord *ai = AiListp + Offset;
                    if (ai->BYTE < ChrEntityp->random)
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFRandomGreaterThanRecord);
                    }
                    break;
                }

                case AI_RunToPad:
                {
                    AiRunToPadRecord *ai  = AiListp + Offset;
                    u16               pad = ntohs(ai->PAD);
                    chrGoToPad(ChrEntityp, pad, SPEED_RUN);
                    Offset += sizeof(AiRunToPadRecord);
                    break;
                }
                case AI_RunToPadPreset:
                {
                    /* PD uses GoTo Pad (speed) which seems better
                    switch (ai->val[0])
                    {
                        case SPEED_WALK:
                            chrGoToPad(ChrEntityp, ChrEntityp->padpreset1, SPEED_WALK);
                            break;
                        case SPEED_RUN:
                            etc...
                     */
                    chrGoToPad(ChrEntityp, ChrEntityp->padpreset1, SPEED_RUN);
                    Offset += sizeof(AiRunToPadPresetRecord);
                    break;
                }
                case AI_WalkToPad:
                {
                    AiWalkToPadRecord *ai  = AiListp + Offset;
                    u16                pad = ntohs(ai->PAD);
                    chrGoToPad(ChrEntityp, pad, SPEED_WALK);
                    Offset += sizeof(AiWalkToPadRecord);
                    break;
                }
                case AI_SprintToPad:
                {
                    AiSprintToPadRecord *ai  = AiListp + Offset;
                    u16                  pad = ntohs(ai->PAD);
                    chrGoToPad(ChrEntityp, pad, SPEED_SPRINT);
                    Offset += sizeof(AiSprintToPadRecord);
                    break;
                }
                case AI_StartPatrol:
                {
                    AiStartPatrolRecord *ai   = AiListp + Offset;
                    PathRecord          *path = pathFindById(ai->PATH_NUM);
                    if_actor_able_set_on_path(ChrEntityp, path);
                    Offset += sizeof(AiStartPatrolRecord);
                    break;
                }
                case AI_IFICanHearAlarm:
                {
                    AiIFICanHearAlarmRecord *ai = AiListp + Offset;
                    if (chrCanHearAlarm(ChrEntityp))
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFICanHearAlarmRecord);
                    }
                    break;
                }
                case AI_IFAlarmIsOn:
                {
                    AiIFAlarmIsOnRecord *ai = AiListp + Offset;
                    if (alarmIsActive())
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFAlarmIsOnRecord);
                    }
                    break;
                }
                case AI_IFGasIsLeaking:
                {
                    AiIFGasIsLeakingRecord *ai = AiListp + Offset;
                    if (check_if_toxic_gas_activated())
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFGasIsLeakingRecord);
                    }
                    break;
                }
                case AI_IFIHeardBond:
                {
                    AiIFIHeardBondRecord *ai = AiListp + Offset;
                    if (chrIsHearingBond(ChrEntityp))
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFIHeardBondRecord);
                    }
                    break;
                }
                case AI_IFISeeSomeoneShot:
                {
                    AiIFISeeSomeoneShotRecord *ai = AiListp + Offset;
                    if (chrSawInjury(ChrEntityp))
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFISeeSomeoneShotRecord);
                    }
                    break;
                }
                case AI_IFISeeSomeoneDie:
                {
                    AiIFISeeSomeoneDieRecord *ai = AiListp + Offset;
                    if (chrSawDeath(ChrEntityp))
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFISeeSomeoneDieRecord);
                    }
                    break;
                }
                case AI_IFICouldSeeBond:
                {
                    AiIFICouldSeeBondRecord *ai = AiListp + Offset;
                    if (chrCanSeeBond(ChrEntityp))
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFICouldSeeBondRecord);
                    }
                    break;
                }
                case AI_IFICouldSeeBondsStan:
                {
                    AiIFICouldSeeBondsStanRecord *ai = AiListp + Offset;
                    if (chrIsTargetNearlyInSight(ChrEntityp))
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFICouldSeeBondsStanRecord);
                    }
                    break;
                }
                case AI_IFIWasShotRecently:
                {
                    AiIFIWasShotRecentlyRecord *ai = AiListp + Offset;
                    if (chrSawTargetRecently(ChrEntityp))
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFIWasShotRecentlyRecord);
                    }
                    break;
                }
                case AI_IFIHeardBondRecently:
                {
                    AiIFIHeardBondRecentlyRecord *ai = AiListp + Offset;
                    if (chrHeardTargetRecently(ChrEntityp))
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFIHeardBondRecentlyRecord);
                    }
                    break;
                }
                case AI_IFImInRoomWithChr:
                {
                    AiIFImInRoomWithChrRecord *ai  = AiListp + Offset;
                    ChrRecord                 *chr = chrFindById(ChrEntityp, ai->CHR_NUM);
                    if (chr && chr->prop && check_if_position_in_same_room(ChrEntityp, &chr->prop->pos, chr->prop->stan))
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFImInRoomWithChrRecord);
                    }
                    break;
                }
                case AI_IFIveNotBeenSeen:
                {
                    AiIFIveNotBeenSeenRecord *ai = AiListp + Offset;
                    if (!(ChrEntityp->chrflags & CHRFLAG_HAS_BEEN_ON_SCREEN))
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFIveNotBeenSeenRecord);
                    }
                    break;
                }
                case AI_IFImOnScreen:
                {
                    AiIFImOnScreenRecord *ai = AiListp + Offset;
                    if ((ChrEntityp->prop->flags & PROPFLAG_ONSCREEN))
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFImOnScreenRecord);
                    }
                    break;
                }
                case AI_IFMyRoomIsOnScreen:
                {
                    AiIFMyRoomIsOnScreenRecord *ai = AiListp + Offset;

                    if (getROOMID_isRendered(getTileRoom(ChrEntityp->prop->stan))) // embedded func to match, must be s32 not u8
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFMyRoomIsOnScreenRecord);
                    }
                    break;
                }
                case AI_IFRoomWithPadIsOnScreen:
                {
                    AiIFRoomWithPadIsOnScreenRecord *ai     = AiListp + Offset;
                    u16                              pad_id = ntohs(ai->PAD);
                    if (check_if_room_for_preset_loaded(ChrEntityp, pad_id))
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFRoomWithPadIsOnScreenRecord);
                    }
                    break;
                }
                case AI_IFImTargetedByBond:
                {
                    AiIFImTargetedByBondRecord *ai = AiListp + Offset;
                    if (sub_GAME_7F0333F8(ChrEntityp))
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFImTargetedByBondRecord);
                    }
                    break;
                }
                case AI_IFBondMissedMe:
                {
                    AiIFBondMissedMeRecord *ai = AiListp + Offset;
                    if (chrIfNearMiss(ChrEntityp))
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFBondMissedMeRecord);
                    }
                    break;
                }
                case AI_IFMyAngleToBondLessThan:
                {
                    // Alternative Names?
                    // aiIfTargetInFovLeft or aiIfBondOutOfFov
                    AiIFMyAngleToBondLessThanRecord *ai  = AiListp + Offset;
                    float                            rad = chrGetAngleToBond(ChrEntityp); // must use float to save "hidden var"
                    if (ByteToRadian((ai->ANGLE)) > rad)
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFMyAngleToBondLessThanRecord);
                    }
                    break;
                }
                case AI_IFMyAngleToBondGreaterThan:
                {
                    AiIFMyAngleToBondGreaterThanRecord *ai  = AiListp + Offset;
                    float                               rad = chrGetAngleToBond(ChrEntityp);
                    if (ByteToRadian((ai->ANGLE)) < rad)
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFMyAngleToBondGreaterThanRecord);
                    }
                    break;
                }
                case AI_IFMyAngleFromBondLessThan:
                {
                    AiIFMyAngleFromBondLessThanRecord *ai  = AiListp + Offset;
                    float                              rad = chrGetAngleFromBond(ChrEntityp);
                    if (ByteToRadian((ai->ANGLE)) > rad)
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFMyAngleFromBondLessThanRecord);
                    }
                    break;
                }
                case AI_IFMyAngleFromBondGreaterThan:
                {
                    AiIFMyAngleFromBondGreaterThanRecord *ai  = AiListp + Offset;
                    float                                 rad = chrGetAngleFromBond(ChrEntityp);
                    if (ByteToRadian((ai->ANGLE)) < rad)
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFMyAngleFromBondGreaterThanRecord);
                    }
                    break;
                }
                case AI_IFMyDistanceToBondLessThanDecimeter:
                {
                    AiIFMyDistanceToBondLessThanDecimeterRecord *ai       = AiListp + Offset;
                    f32                                          distance = ntohs(ai->DISTANCE) * 10.0f;
                    if (distance > chrGetDistanceToBond(ChrEntityp))
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFMyDistanceToBondLessThanDecimeterRecord);
                    }
                    break;
                }
                case AI_IFMyDistanceToBondGreaterThanDecimeter:
                {
                    AiIFMyDistanceToBondGreaterThanDecimeterRecord *ai       = AiListp + Offset;
                    f32                                             distance = ntohs(ai->DISTANCE) * 10.0f;
                    if (distance < chrGetDistanceToBond(ChrEntityp))
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFMyDistanceToBondGreaterThanDecimeterRecord);
                    }
                    break;
                }
                case AI_IFChrDistanceToPadLessThanDecimeter:
                {
                    AiIFChrDistanceToPadLessThanDecimeterRecord *ai     = AiListp + Offset;
                    ChrRecord                                   *chr    = chrFindById(ChrEntityp, ai->CHR_NUM);
                    u16                                          padnum = ntohs(ai->PAD);
                    f32                                          value  = ntohs(ai->DISTANCE) * 10.0f;
                    if (chr && (value > chrGetDistanceToPad(chr, padnum)))
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFChrDistanceToPadLessThanDecimeterRecord);
                    }
                    break;
                }
                case AI_IFChrDistanceToPadGreaterThanDecimeter:
                {
                    AiIFChrDistanceToPadGreaterThanDecimeterRecord *ai     = AiListp + Offset;
                    ChrRecord                                      *chr    = chrFindById(ChrEntityp, ai->CHR_NUM);
                    u16                                             padnum = ntohs(ai->PAD);
                    f32                                             value  = ntohs(ai->DISTANCE) * 10.0f;
                    if (chr && (value < chrGetDistanceToPad(chr, padnum)))
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFChrDistanceToPadGreaterThanDecimeterRecord);
                    }
                    break;
                }
                case AI_IFMyDistanceToChrLessThanDecimeter:
                {
                    AiIFMyDistanceToChrLessThanDecimeterRecord *ai     = AiListp + Offset;
                    f32                                         cutoff = ntohs(ai->DISTANCE) * 10.0f;
                    if (cutoff > chrGetDistanceToChr(ChrEntityp, ai->CHR_NUM))
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFMyDistanceToChrLessThanDecimeterRecord);
                    }
                    break;
                }
                case AI_IFMyDistanceToChrGreaterThanDecimeter:
                {
                    AiIFMyDistanceToChrGreaterThanDecimeterRecord *ai     = AiListp + Offset;
                    f32                                            cutoff = ntohs(ai->DISTANCE) * 10.0f;
                    if (cutoff < chrGetDistanceToChr(ChrEntityp, ai->CHR_NUM))
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFMyDistanceToChrGreaterThanDecimeterRecord);
                    }
                    break;
                }
                case AI_TRYSettingMyPresetToChrWithinDistanceDecimeter:
                {
                    AiTRYSettingMyPresetToChrWithinDistanceDecimeterRecord *ai       = AiListp + Offset;
                    f32                                                     distance = ntohs(ai->DISTANCE) * 10.0f;
                    if (sub_GAME_7F033B38(ChrEntityp, distance))
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiTRYSettingMyPresetToChrWithinDistanceDecimeterRecord);
                    }
                    break;
                }
                case AI_IFBondDistanceToPadLessThanDecimeter:
                {
                    AiIFBondDistanceToPadLessThanDecimeterRecord *ai    = AiListp + Offset;
                    u16                                           pad   = ntohs(ai->PAD);
                    f32                                           value = ntohs(ai->DISTANCE) * 10.0f;
                    if (value > chrGetDistanceFromBondToPad(ChrEntityp, pad))
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFBondDistanceToPadLessThanDecimeterRecord);
                    }
                    break;
                }
                case AI_IFBondDistanceToPadGreaterThanDecimeter:
                {
                    AiIFBondDistanceToPadGreaterThanDecimeterRecord *ai    = AiListp + Offset;
                    u16                                              pad   = ntohs(ai->PAD);
                    f32                                              value = ntohs(ai->DISTANCE) * 10.0f;
                    if (value < chrGetDistanceFromBondToPad(ChrEntityp, pad))
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFBondDistanceToPadGreaterThanDecimeterRecord);
                    }
                    break;
                }
                case AI_IFChrInRoomWithPad:
                {
                    AiIFChrInRoomWithPadRecord *ai     = AiListp + Offset;
                    u16                         pad_id = ntohs(ai->PAD);
                    if (chrIfInPadRoom(ChrEntityp, ai->CHR_NUM, pad_id))
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFChrInRoomWithPadRecord);
                    }
                    break;
                }
                case AI_IFBondInRoomWithPad:
                {
                    AiIFBondInRoomWithPadRecord *ai     = AiListp + Offset;
                    u16                          pad_id = ntohs(ai->PAD);
                    if (check_if_actor_is_at_preset(ChrEntityp, pad_id))
                    {
    #ifdef ENABLE_LOG
                        osSyncPrintf("BOND IN ROOM\n");
    #endif
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
    #ifdef ENABLE_LOG
                        osSyncPrintf("bond not in room\n");
    #endif
                        Offset += sizeof(AiIFBondInRoomWithPadRecord);
                    }
                    break;
                }
                case AI_IFBondCollectedObject:
                {
                    AiIFBondCollectedObjectRecord *ai  = AiListp + Offset;
                    ObjectRecord                  *obj = objFindByTagId(ai->OBJECT_TAG);
                    if (obj && obj->prop && bondinvHasPropInInv(obj->prop))
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFBondCollectedObjectRecord);
                    }
                    break;
                }
                case AI_IFKeyDropped:
                {
                    AiIFKeyDroppedRecord *ai = AiListp + Offset;
                    if (weaponFindThrown(ai->KEY_ID))
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFKeyDroppedRecord);
                    }
                    break;
                }
                case AI_IFItemIsAttachedToObject:
                {
                    AiIFItemIsAttachedToObjectRecord *ai   = AiListp + Offset;
                    ObjectRecord                     *obj  = objFindByTagId(ai->OBJECT_TAG);
                    bool                              pass = FALSE;
                    if (obj && obj->prop)
                    {
                        PropRecord *prop = obj->prop->child;
                        while (prop)
                        {
                            if (prop->type == PROP_TYPE_WEAPON)
                            {
                                WeaponObjRecord *weapon = prop->weapon;
                                if (weapon->weaponnum == ai->ITEM_NUM)
                                {
                                    pass = TRUE;
                                    break;
                                }
                            }
                            prop = prop->prev;
                        }
                    }
                    if (pass)
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFItemIsAttachedToObjectRecord);
                    }
                    break;
                }
                case AI_IFBondHasItemEquipped:
                {
                    AiIFBondHasItemEquippedRecord *ai = AiListp + Offset;
                    if (ai->ITEM_NUM == getCurrentPlayerWeaponId(GUNRIGHT) || ai->ITEM_NUM == getCurrentPlayerWeaponId(GUNLEFT)) // order matters
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFBondHasItemEquippedRecord);
                    }
                    break;
                }
                case AI_IFObjectExists:
                {
                    AiIFObjectExistsRecord *ai  = AiListp + Offset;
                    ObjectRecord           *obj = objFindByTagId(ai->OBJECT_TAG);
                    if (obj && obj->prop)
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFObjectExistsRecord);
                    }
                    break;
                }
                case AI_IFObjectNotDestroyed:
                {
                    AiIFObjectNotDestroyedRecord *ai  = AiListp + Offset;
                    ObjectRecord                 *obj = objFindByTagId(ai->OBJECT_TAG);
                    if (obj && obj->prop && objIsHealthy(obj))
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFObjectNotDestroyedRecord);
                    }
                    break;
                }
                case AI_IFObjectWasActivated:
                {
                    AiIFObjectWasActivatedRecord *ai  = AiListp + Offset;
                    ObjectRecord                 *obj = objFindByTagId(ai->OBJECT_TAG);
                    if (obj && obj->prop && (obj->runtime_bitflags & RUNTIMEBITFLAG_ACTIVATED))
                    {
                        obj->runtime_bitflags &= ~RUNTIMEBITFLAG_ACTIVATED;
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFObjectWasActivatedRecord);
                    }
                    break;
                }
                case AI_IFBondUsedGadgetOnObject:
                {
                    AiIFBondUsedGadgetOnObjectRecord *ai  = AiListp + Offset;
                    ObjectRecord                     *obj = objFindByTagId(ai->OBJECT_TAG);
                    if (obj && obj->prop && (obj->state & PROPSTATE_ACTIVATED))
                    {
                        obj->state &= ~PROPSTATE_ACTIVATED;
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFBondUsedGadgetOnObjectRecord);
                    }
                    break;
                }
                case AI_ActivateObject:
                {
                    AiActivateObjectRecord *ai  = AiListp + Offset;
                    ObjectRecord           *obj = objFindByTagId(ai->OBJECT_TAG);
                    if (obj && obj->prop)
                    {
                        if (obj->prop->type == PROP_TYPE_DOOR)
                        {
                            doorActivateWrapper(obj->prop);
                        }
                        else if (obj->prop->type == PROP_TYPE_OBJ || obj->prop->type == PROP_TYPE_WEAPON)
                        {
                            propobjInteract(obj->prop);
                        }
                    }
                    Offset += sizeof(AiActivateObjectRecord);
                    break;
                }
                case AI_DestroyObject: // canonicly destroyobj
                {
                    AiDestroyObjectRecord *ai  = AiListp + Offset;
                    ObjectRecord          *obj = objFindByTagId(ai->OBJECT_TAG);
    #ifdef ENABLE_LOG
                    osSyncPrintf("ai_destroyobj 1 : \n");
    #endif
                    if (obj && obj->prop)
                    {
                        if (!objGetDestroyedLevel(obj))
                        {
                            f32 damage = ((obj->damage - obj->maxdamage) + 1) / 250.0f;
                            /*
                            osSyncPrintf("ai_destroyobj 2 : (def->obj == PROP_ELVIS_SAUCER)\n");
                            osSyncPrintf("Elvis BOOM\n");
                            */
    #ifdef ENABLE_LOG
                            osSyncPrintf("ai_destroyobj 3 : adddamageobj\n");
    #endif

                            objApplyDamage(obj, damage, &obj->runtime_pos, 0x1D, -1);
                        }
                    }
                    Offset += sizeof(AiDestroyObjectRecord);
                    break;
                }
                case AI_DropObject:
                {
                    AiDropObjectRecord *ai  = AiListp + Offset;
                    ObjectRecord       *obj = objFindByTagId(ai->OBJECT_TAG);
                    if (obj && obj->prop && obj->prop->parent && obj->prop->parent->type == PROP_TYPE_CHR)
                    {
                        ChrRecord *chr = obj->prop->parent->chr;
                        propobjSetDropped(obj->prop, 2);
                        chr->hidden |= CHRHIDDEN_DROP_HELD_ITEMS;
                    }
                    Offset += sizeof(AiDropObjectRecord);
                    break;
                }
                case AI_ChrDropAllConcealedItems:
                {
                    AiChrDropAllConcealedItemsRecord *ai  = AiListp + Offset;
                    ChrRecord                        *chr = chrFindById(ChrEntityp, ai->CHR_NUM);
                    if (chr && chr->prop)
                    {
                        chrDropItems(chr);
                    }
                    Offset += sizeof(AiChrDropAllConcealedItemsRecord);
                    break;
                }
                case AI_ChrDropAllHeldItems:
                {
                    AiChrDropAllHeldItemsRecord *ai  = AiListp + Offset;
                    ChrRecord                   *chr = chrFindById(ChrEntityp, ai->CHR_NUM);
                    if (chr && chr->prop)
                    {
                        if (chr->weapons_held[GUNRIGHT])
                        {
                            propobjSetDropped(chr->weapons_held[GUNRIGHT], 1);
                            chr->hidden |= CHRHIDDEN_DROP_HELD_ITEMS;
                        }
                        if (chr->weapons_held[GUNLEFT])
                        {
                            propobjSetDropped(chr->weapons_held[GUNLEFT], 1);
                            chr->hidden |= CHRHIDDEN_DROP_HELD_ITEMS;
                        }
                    }
                    Offset += sizeof(AiChrDropAllHeldItemsRecord);
                    break;
                }
                case AI_BondCollectObject:
                {
                    AiBondCollectObjectRecord *ai  = AiListp + Offset;
                    ObjectRecord              *obj = objFindByTagId(ai->OBJECT_TAG);
                    if (obj && obj->prop)
                    {
                        TICKOP op = propPickupByPlayer(obj->prop, FALSE);
                        propExecuteTickOperation(obj->prop, op);
                    }
                    Offset += sizeof(AiBondCollectObjectRecord);
                    break;
                }
                case AI_ChrEquipObject:
                {
                    AiChrEquipObjectRecord *ai  = AiListp + Offset;
                    ObjectRecord           *obj = objFindByTagId(ai->OBJECT_TAG);
                    ChrRecord              *chr = chrFindById(ChrEntityp, ai->CHR_NUM);
                    if (obj && obj->prop && chr)
                    {
                        if (obj->prop->parent)
                        {
                            objDetach(obj->prop);
                        }
                        else
                        {
                            chrpropDeregisterRooms(obj->prop);
                            chrpropDelist(obj->prop);
                            chrpropDisable(obj->prop);
                        }
                        if (obj->type != PROPDEF_COLLECTABLE || !chrEquipWeapon(obj, chr))
                        {
                            chrpropReparent(obj->prop, chr->prop);
                        }
                    }
                    Offset += sizeof(AiChrEquipObjectRecord);
                    break;
                }
                case AI_MoveObject: // canonicly aiMoveObj
                {
                    AiMoveObjectRecord *ai  = AiListp + Offset;
                    ObjectRecord       *obj = objFindByTagId(ai->OBJECT_TAG);
                    volatile PadRecord *pad;
                    u16                 padnum = ntohs(ai->PAD);
                    Mtxf                matrix;

                    if (obj && obj->prop)
                    {
                        if (isNotBoundPad(padnum))
                        {
                            pad = &g_CurrentSetup.pads[padnum];
                        }
                        else
                        {
                            pad = (PadRecord *)&g_CurrentSetup.boundpads[getBoundPadNum(padnum)];
                        }
    #ifdef ENABLE_LOG
                        osSyncPrintf("aiMoveObj: moving object to pad %d\n", pad);
    #endif
                        matrix_4x4_set_basis_and_position_target(&matrix, 0, 0, 0, -pad->look.x, -pad->look.y, -pad->look.z, pad->up.x, pad->up.y, pad->up.z);

                        if (obj->model)
                        {
                            matrix_scalar_multiply(obj->model->scale, &matrix);
                        }
                        sub_GAME_7F04088C(obj, &pad->pos, &matrix, pad->stan, &pad->pos);
                        setupUpdateObjectRoomPosition(obj);
                    }
                    Offset += sizeof(AiMoveObjectRecord);
                    break;
                }
                case AI_DoorOpen:
                {
                    AiDoorOpenRecord *ai  = AiListp + Offset;
                    DoorRecord       *obj = objFindByTagId(ai->OBJECT_TAG);
                    if (obj && obj->prop && obj->prop->type == PROP_TYPE_DOOR)
                    {
                        // DoorRecord *door = (DoorRecord *)obj;
                        doorActivate(obj, DOORSTATE_OPENING);
                    }
                    Offset += sizeof(AiDoorOpenRecord);
                    break;
                }
                case AI_DoorClose:
                {
                    AiDoorCloseRecord *ai  = AiListp + Offset;
                    DoorRecord        *obj = objFindByTagId(ai->OBJECT_TAG);
                    if (obj && obj->prop && obj->prop->type == PROP_TYPE_DOOR)
                    {
                        // DoorRecord *door = (DoorRecord *)obj;
                        doorActivate(obj, DOORSTATE_CLOSING);
                    }
                    Offset += sizeof(AiDoorCloseRecord);
                    break;
                }
                case AI_IFDoorStateEqual:
                {
                    AiIFDoorStateEqualRecord *ai   = AiListp + Offset;
                    ObjectRecord             *obj  = objFindByTagId(ai->OBJECT_TAG);
                    bool                      pass = FALSE;
                    if (obj && obj->prop && obj->type == PROPDEF_DOOR)
                    {
                        DoorRecord *door = (DoorRecord *)obj;
                        if (door->openstate == DOORSTATE_STATIONARY)
                        {
                            if (door->openPosition <= 0)
                            {
                                pass = (ai->DOOR_STATE & AI_DOOR_STATE_CLOSED) != 0;
                            }
                            else
                            {
                                pass = (ai->DOOR_STATE & AI_DOOR_STATE_OPEN) != 0;
                            }
                        }
                        else if (door->openstate == DOORSTATE_OPENING || door->openstate == DOORSTATE_WAITING)
                        {
                            pass = (ai->DOOR_STATE & AI_DOOR_STATE_OPENING) != 0;
                        }
                        else if (door->openstate == DOORSTATE_CLOSING)
                        {
                            pass = (ai->DOOR_STATE & AI_DOOR_STATE_CLOSING) != 0;
                        }
                    }
                    if (pass)
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFDoorStateEqualRecord);
                    }
                    break;
                }
                case AI_IFDoorHasBeenOpenedBefore:
                {
                    AiIFDoorHasBeenOpenedBeforeRecord *ai  = AiListp + Offset;
                    ObjectRecord                      *obj = objFindByTagId(ai->OBJECT_TAG);
                    if (obj && obj->prop && obj->type == PROPDEF_DOOR && (obj->runtime_bitflags & RUNTIMEBITFLAG_BEENOPENED))
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFDoorHasBeenOpenedBeforeRecord);
                    }
                    break;
                }
                case AI_DoorSetLock:
                {
                    AiDoorSetLockRecord *ai  = AiListp + Offset;
                    ObjectRecord        *obj = objFindByTagId(ai->OBJECT_TAG);
                    if (obj && obj->prop && obj->prop->type == PROP_TYPE_DOOR)
                    {
                        DoorRecord *door = (DoorRecord *)obj;
                        u8          bits = ai->LOCK_FLAG;
                        door->keyflags   = door->keyflags | bits;
                    }
                    Offset += sizeof(AiDoorSetLockRecord);
                    break;
                }
                case AI_DoorUnsetLock:
                {
                    AiDoorUnsetLockRecord *ai  = AiListp + Offset;
                    ObjectRecord          *obj = objFindByTagId(ai->OBJECT_TAG);
                    if (obj && obj->prop && obj->prop->type == PROP_TYPE_DOOR)
                    {
                        DoorRecord *door = (DoorRecord *)obj;
                        u8          bits = ai->LOCK_FLAG;
                        door->keyflags &= ~bits;
                    }
                    Offset += sizeof(AiDoorUnsetLockRecord);
                    break;
                }
                case AI_IFDoorLockEqual:
                {
                    AiIFDoorLockEqualRecord *ai   = AiListp + Offset;
                    ObjectRecord            *obj  = objFindByTagId(ai->OBJECT_TAG);
                    bool                     pass = FALSE;
                    if (obj && obj->prop && obj->prop->type == PROP_TYPE_DOOR)
                    {
                        DoorRecord *door = (DoorRecord *)obj;
                        s32         bits = ai->LOCK_FLAG;
                        if ((door->keyflags & bits) == bits)
                        {
                            pass = TRUE;
                        }
                    }
                    if (pass)
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFDoorLockEqualRecord);
                    }
                    break;
                }
                case AI_IFObjectiveNumComplete:
                {
                    AiIFObjectiveNumCompleteRecord *ai = AiListp + Offset;
                    /*  additional PD code for dificulty filtering
                     == OBJECTIVE_COMPLETE && objectivelvlGetSelectedDifficultyBits(ai->val[0]) & (1 << lvlGetSelectedDifficulty()))  *
                    */
                    if (objectiveGetCount() > ai->OBJ_NUM && OBJECTIVESTATUS_COMPLETE == objectiveGetStatus_WEAK(ai->OBJ_NUM * 1, ai->OBJ_NUM))
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFObjectiveNumCompleteRecord);
                    }
                    break;
                }
                case AI_TRYUnknown6e:
                {
                    AiTRYUnknown6eRecord *ai = AiListp + Offset;
                    if (check_2328_preset_set_with_method(ChrEntityp, ai->val))
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiTRYUnknown6eRecord);
                    }
                    break;
                }
                case AI_TRYUnknown6f:
                {
                    AiTRYUnknown6fRecord *ai = AiListp + Offset;
                    if (sub_GAME_7F033AAC(ChrEntityp, ai->val))
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiTRYUnknown6fRecord);
                    }
                    break;
                }

                case AI_IFMyNumArghsLessThan:
                {
                    AiIFMyNumArghsLessThanRecord *ai = AiListp + Offset;
                    if (ai->val > chrGetNumArghs(ChrEntityp)) // order matter
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFMyNumArghsLessThanRecord);
                    }
                    break;
                }
                case AI_IFMyNumArghsGreaterThan:
                {
                    AiIFMyNumArghsGreaterThanRecord *ai = AiListp + Offset;
                    if (ai->val < chrGetNumArghs(ChrEntityp)) // order matter
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFMyNumArghsGreaterThanRecord);
                    }
                    break;
                }
                case AI_IFMyNumCloseArghsLessThan:
                {
                    AiIFMyNumCloseArghsLessThanRecord *ai = AiListp + Offset;
                    if (ai->val > chrGetNumCloseArghs(ChrEntityp))
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFMyNumCloseArghsLessThanRecord);
                    }
                    break;
                }
                case AI_IFMyNumCloseArghsGreaterThan:
                {
                    AiIFMyNumCloseArghsGreaterThanRecord *ai = AiListp + Offset;
                    if (ai->val < chrGetNumCloseArghs(ChrEntityp))
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFMyNumCloseArghsGreaterThanRecord);
                    }
                    break;
                }
                case AI_IFChrHealthLessThan:
                {
                    AiIFChrHealthLessThanRecord *ai    = AiListp + Offset;
                    f32                          value = (ai->HEALTH) * 0.1f;
                    ChrRecord                   *chr   = chrFindById(ChrEntityp, ai->CHR_NUM);

                    if (chr && ((chr->maxdamage - chr->damage) < value))
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFChrHealthLessThanRecord);
                    }
                    break;
                }
                case AI_IFChrHealthGreaterThan:
                {
                    AiIFChrHealthGreaterThanRecord *ai    = AiListp + Offset;
                    f32                             value = (ai->HEALTH) * 0.1f;
                    ChrRecord                      *chr   = chrFindById(ChrEntityp, ai->CHR_NUM);

                    if (chr && ((chr->maxdamage - chr->damage) > value))
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFChrHealthGreaterThanRecord);
                    }
                    break;
                }
                case AI_IFChrWasDamagedSinceLastCheck:
                {
                    AiIFChrWasDamagedSinceLastCheckRecord *ai  = AiListp + Offset;
                    ChrRecord                             *chr = chrFindById(ChrEntityp, ai->CHR_NUM);
                    if (chr && (chr->chrflags & CHRFLAG_WAS_DAMAGED))
                    {
                        chr->chrflags &= ~CHRFLAG_WAS_DAMAGED; // disable flag
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFChrWasDamagedSinceLastCheckRecord);
                    }
                    break;
                }
                case AI_IFBondHealthLessThan:
                {
                    AiIFBondHealthLessThanRecord *ai  = AiListp + Offset;
                    float                         val = (ai->HEALTH) / 255.0f;
                    if (val > currentPlayerGetHealth())
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFBondHealthLessThanRecord);
                    }
                    break;
                }
                case AI_IFBondHealthGreaterThan:
                {
                    AiIFBondHealthGreaterThanRecord *ai  = AiListp + Offset;
                    float                            val = (ai->HEALTH) / 255.0f;
                    if (val < currentPlayerGetHealth())
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFBondHealthGreaterThanRecord);
                    }
                    break;
                }
                case AI_IFGameDifficultyLessThan:
                {
                    AiIFGameDifficultyLessThanRecord *ai = AiListp + Offset;
                    if (ai->DIFICULTY_ID > lvlGetSelectedDifficulty())
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFGameDifficultyLessThanRecord);
                    }
                    break;
                }
                case AI_IFGameDifficultyGreaterThan:
                {
                    AiIFGameDifficultyGreaterThanRecord *ai = AiListp + Offset;
                    if (ai->DIFICULTY_ID < lvlGetSelectedDifficulty())
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFGameDifficultyGreaterThanRecord);
                    }
                    break;
                }
                case AI_IFMissionTimeLessThan:
                {
                    AiIFMissionTimeLessThanRecord *ai     = AiListp + Offset;
                    f32                            target = ntohs(ai->SECONDS);
                    if (target > lvlGetCurrentMultiPlayerSec())
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFMissionTimeLessThanRecord);
                    }
                    break;
                }
                case AI_IFMissionTimeGreaterThan:
                {
                    AiIFMissionTimeGreaterThanRecord *ai     = AiListp + Offset;
                    f32                               target = ntohs(ai->SECONDS);
                    if (target < lvlGetCurrentMultiPlayerSec())
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFMissionTimeGreaterThanRecord);
                    }
                    break;
                }
                case AI_IFSystemPowerTimeLessThan:
                {
                    AiIFSystemPowerTimeLessThanRecord *ai     = AiListp + Offset;
                    f32                                target = ntohs(ai->MINUTES) * CHRAI_TICKRATE_F;
                    if (target > lvlGetCurrentMultiPlayerMin())
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFSystemPowerTimeLessThanRecord);
                    }
                    break;
                }
                case AI_IFSystemPowerTimeGreaterThan:
                {
                    AiIFSystemPowerTimeGreaterThanRecord *ai     = AiListp + Offset;
                    f32                                   target = ntohs(ai->MINUTES) * CHRAI_TICKRATE_F;
                    if (target < lvlGetCurrentMultiPlayerMin())
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFSystemPowerTimeGreaterThanRecord);
                    }
                    break;
                }
                case AI_IFLevelIdLessThan:
                {
                    AiIFLevelIdLessThanRecord *ai = AiListp + Offset;
                    if (ai->LEVEL_ID > bossGetStageNum())
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFLevelIdLessThanRecord);
                    }
                    break;
                }
                case AI_IFLevelIdGreaterThan:
                {
                    AiIFLevelIdGreaterThanRecord *ai = AiListp + Offset;
                    if (ai->LEVEL_ID < bossGetStageNum())
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFLevelIdGreaterThanRecord);
                    }
                    break;
                }
                case AI_SetMyMorale:
                {
                    AiSetMyMoraleRecord *ai = AiListp + Offset;
                    ChrEntityp->morale      = ai->val;
    #ifdef ENABLE_LOG
                    osSyncPrintf("MORALE IS NOW %d \n", ChrEntityp->morale);
    #endif
                    Offset += sizeof(AiSetMyMoraleRecord);
                    break;
                }
                case AI_AddToMyMorale:
                {
                    AiAddToMyMoraleRecord *ai = AiListp + Offset;
                    if (255 - ai->val < ChrEntityp->morale) // clamp to 255
                    {
                        ChrEntityp->morale = 255; // max
                    }
                    else
                    {
                        ChrEntityp->morale += ai->val;
                    }
    #ifdef ENABLE_LOG
                    osSyncPrintf("MORALE IS NOW %d \n", ChrEntityp->morale);
    #endif

                    Offset += sizeof(AiAddToMyMoraleRecord);
                    break;
                }
                case AI_SubtractFromMyMorale:
                {
                    AiSubtractFromMyMoraleRecord *ai = AiListp + Offset;
                    if (ai->val > ChrEntityp->morale) // clamp to 0
                    {
                        ChrEntityp->morale = 0;
                    }
                    else
                    {
                        ChrEntityp->morale -= ai->val;
                    }
    #ifdef ENABLE_LOG
                    osSyncPrintf("MORALE IS NOW %d \n", ChrEntityp->morale);
    #endif
                    Offset += sizeof(AiSubtractFromMyMoraleRecord);
                    break;
                }
                case AI_IFMyMoraleLessThan:
                {
                    AiIFMyMoraleLessThanRecord *ai = AiListp + Offset;
                    if (ai->val > ChrEntityp->morale)
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFMyMoraleLessThanRecord);
                    }
                    break;
                }
                case AI_IFMyMoraleLessThanRandom:
                {
                    AiIFMyMoraleLessThanRandomRecord *ai = AiListp + Offset;
                    if (ChrEntityp->morale < ChrEntityp->random)
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFMyMoraleLessThanRandomRecord);
                    }
                    break;
                }
                case AI_SetMyAlertness:
                {
                    AiSetMyAlertnessRecord *ai = AiListp + Offset;
                    ChrEntityp->alertness      = ai->val;
    #ifdef ENABLE_LOG
                    osSyncPrintf("AI_PRINT(void) Alertness =  %d!\n", ChrEntityp->alertness);
    #endif
                    Offset += sizeof(AiSetMyAlertnessRecord);
                    break;
                }
                case AI_AddToMyAlertness:
                {
                    AiAddToMyAlertnessRecord *ai = AiListp + Offset;
                    if (255 - ai->val < ChrEntityp->alertness) // clamp to 255
                    {
                        ChrEntityp->alertness = 255; // max
                    }
                    else
                    {
                        ChrEntityp->alertness += ai->val;
                    }
                    Offset += sizeof(AiAddToMyAlertnessRecord);
                    break;
                }
                case AI_SubtractFromMyAlertness:
                {
                    AiSubtractFromMyAlertnessRecord *ai = AiListp + Offset;
                    if (ai->val > ChrEntityp->alertness) // clamp to 0
                    {
                        ChrEntityp->alertness = 0;
                    }
                    else
                    {
                        ChrEntityp->alertness -= ai->val;
                    }
                    Offset += sizeof(AiSubtractFromMyAlertnessRecord);
                    break;
                }
                case AI_IFMyAlertnessLessThan:
                {
                    AiIFMyAlertnessLessThanRecord *ai = AiListp + Offset;
                    if (ai->CHRBYTE > ChrEntityp->alertness)
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFMyAlertnessLessThanRecord);
                    }
                    break;
                }
                case AI_IFMyAlertnessLessThanRandom:
                {
                    AiIFMyAlertnessLessThanRandomRecord *ai = AiListp + Offset;
                    if (ChrEntityp->alertness < ChrEntityp->random)
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFMyAlertnessLessThanRandomRecord);
                    }
                    break;
                }
                case AI_SetMyHearingScale:
                {
                    AiSetMyHearingScaleRecord *ai       = AiListp + Offset;
                    f32                        distance = ntohs(ai->HEARING_SCALE) / 1000.0f;
                    ChrEntityp->hearingscale            = distance;
                    Offset += sizeof(AiSetMyHearingScaleRecord);
                    break;
                }
                case AI_SetMyVisionRange:
                {
                    AiSetMyVisionRangeRecord *ai = AiListp + Offset;
                    ChrEntityp->visionrange      = (ai->VISION_RANGE);
                    Offset += sizeof(AiSetMyVisionRangeRecord);
                    break;
                }
                case AI_SetMyGrenadeProbability:
                {
                    AiSetMyGrenadeProbabilityRecord *ai = AiListp + Offset;
                    ChrEntityp->grenadeprob             = ai->GRENADE_PROB;
                    Offset += sizeof(AiSetMyGrenadeProbabilityRecord);
                    break;
                }
                case AI_SetMyChrNum:
                {
                    AiSetMyChrNumRecord *ai = AiListp + Offset;
                    ChrEntityp->chrnum      = ai->CHR_NUM;
                    Offset += sizeof(AiSetMyChrNumRecord);
                    break;
                }
                case AI_SetMyHealthTotal:
                {
                    AiSetMyHealthTotalRecord *ai     = AiListp + Offset;
                    f32                       amount = ntohs(ai->HEALTH) * 0.1f;
                    chrSetMaxDamage(ChrEntityp, amount);
                    Offset += sizeof(AiSetMyHealthTotalRecord);
                    break;
                }
                case AI_SetMyArmour:
                {
                    AiSetMyArmourRecord *ai     = AiListp + Offset;
                    f32                  amount = ntohs(ai->AMOUNT) * 0.1f; /*if (cheatIsActive(CHEAT_ENEMYSHIELDS)) { amount = amount < 8 ? 8 : amount; } */
                    chrAddHealth(ChrEntityp, amount);
                    Offset += sizeof(AiSetMyArmourRecord);
                    break;
                }
                case AI_SetMySpeedRating:
                {
                    AiSetMySpeedRatingRecord *ai = AiListp + Offset;
    #ifdef DEBUG
                    /*
                        ".\\ported\\chrai.c", 2258, "Assertion failed: ai->val>=0"
                        ".\\ported\\chrai.c", 2259, "Assertion failed: ai->val<=100"
                     */
                    assert(ai->val >= 0);
                    assert(ai->val <= 100);
    #endif
                    ChrEntityp->speedrating = ai->val;
                    Offset += sizeof(AiSetMySpeedRatingRecord);
                    break;
                }
                case AI_SetMyArghRating:
                {
                    AiSetMyArghRatingRecord *ai = AiListp + Offset;
    #ifdef DEBUG
                    /*
                        ".\\ported\\chrai.c", 2268, "Assertion failed: ai->val>=0"
                        ".\\ported\\chrai.c", 2269, "Assertion failed: ai->val<=100"
                    */
                    assert(ai->val >= 0);
                    assert(ai->val <= 100);
    #endif
                    ChrEntityp->arghrating = ai->val;
                    Offset += sizeof(AiSetMyArghRatingRecord);
                    break;
                }
                case AI_SetMyAccuracyRating:
                {
                    AiSetMyAccuracyRatingRecord *ai = AiListp + Offset;
                    ChrEntityp->accuracyrating      = ai->val;
                    Offset += sizeof(AiSetMyAccuracyRatingRecord);
                    break;
                }
                case AI_SetMyFlags2:
                {
                    AiSetMyFlags2Record *ai = AiListp + Offset;
                    chrSetFlags2(ChrEntityp, ai->BITS);
                    Offset += sizeof(AiSetMyFlags2Record);
                    break;
                }
                case AI_UnsetMyFlags2:
                {
                    AiUnsetMyFlags2Record *ai = AiListp + Offset;
                    chrUnsetFlags2(ChrEntityp, ai->BITS);
                    Offset += sizeof(AiUnsetMyFlags2Record);
                    break;
                }
                case AI_IFMyFlags2Has:
                {
                    AiIFMyFlags2HasRecord *ai = AiListp + Offset;
                    if (chrHasFlags2(ChrEntityp, ai->BITS))
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFMyFlags2HasRecord);
                    }
                    break;
                }
                case AI_SetChrBitfield:
                {
                    AiSetChrBitfieldRecord *ai = AiListp + Offset;
                    chrSetFlags2ById(ChrEntityp, ai->CHR_NUM, ai->BITS);
                    Offset += sizeof(AiSetChrBitfieldRecord);
                    break;
                }
                case AI_UnsetChrBitfield:
                {
                    AiUnsetChrBitfieldRecord *ai = AiListp + Offset;
                    chrUnsetFlags2ById(ChrEntityp, ai->CHR_NUM, ai->BITS);
                    Offset += sizeof(AiUnsetChrBitfieldRecord);
                    break;
                }
                case AI_IFChrBitfieldHas:
                {
                    AiIFChrBitfieldHasRecord *ai = AiListp + Offset;
                    if (chrHasFlags2ById(ChrEntityp, ai->CHR_NUM, ai->BITS))
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFChrBitfieldHasRecord);
                    }
                    break;
                }
                case AI_SetObjectiveBitfield:
                {
                    AiSetObjectiveBitfieldRecord *ai    = AiListp + Offset;
                    s32                           flags = ntohl(ai->BITFIELD);
                    chrSetStageFlags(ChrEntityp, flags);
                    Offset += sizeof(AiSetObjectiveBitfieldRecord);
                    break;
                }
                case AI_UnsetObjectiveBitfield:
                {
                    AiUnsetObjectiveBitfieldRecord *ai    = AiListp + Offset;
                    s32                             flags = ntohl(ai->BITFIELD);
                    chrUnsetStageFlags(ChrEntityp, flags);
                    Offset += sizeof(AiUnsetObjectiveBitfieldRecord);
                    break;
                }
                case AI_IFObjectiveBitfieldHas:
                {
                    AiIFObjectiveBitfieldHasRecord *ai    = AiListp + Offset;
                    s32                             flags = ntohl(ai->BITS);
                    if (chrHasStageFlag(ChrEntityp, flags)) /* PD && ai->val[4] == 1) || (!chrHasStageFlag(ChrEntityp, flags) && ai->val[4] == 0  * */
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFObjectiveBitfieldHasRecord);
                    }
                    break;
                }
                case AI_SetMychrflags:
                {
                    AiSetMychrflagsRecord *ai    = AiListp + Offset;
                    CHRFLAG                flags = ntohl(ai->CHRFLAGS);
                    ChrEntityp->chrflags |= flags;
                    Offset += sizeof(AiSetMychrflagsRecord);
                    break;
                }
                case AI_UnsetMychrflags:
                {
                    AiUnsetMychrflagsRecord *ai    = AiListp + Offset;
                    CHRFLAG                  flags = ntohl(ai->CHRFLAGS);
                    ChrEntityp->chrflags &= ~flags;
                    Offset += sizeof(AiUnsetMychrflagsRecord);
                    break;
                }
                case AI_IFMychrflagsHas:
                {
                    AiIFMychrflagsHasRecord *ai    = AiListp + Offset;
                    CHRFLAG                  flags = ntohl(ai->CHRFLAGS);
                    if ((ChrEntityp->chrflags & flags) == flags)
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFMychrflagsHasRecord);
                    }
                    break;
                }
                case AI_SetChrchrflags:
                {
                    AiSetChrchrflagsRecord *ai    = AiListp + Offset;
                    CHRFLAG                 flags = ntohl(ai->CHRFLAGS);
                    ChrRecord              *chr   = chrFindById(ChrEntityp, ai->CHR_NUM);
                    if (chr)
                    {
                        chr->chrflags |= flags;
                    }
                    Offset += sizeof(AiSetChrchrflagsRecord);
                    break;
                }
                case AI_UnsetChrchrflags:
                {
                    AiUnsetChrchrflagsRecord *ai    = AiListp + Offset;
                    CHRFLAG                   flags = ntohl(ai->CHRFLAGS);
                    ChrRecord                *chr   = chrFindById(ChrEntityp, ai->CHR_NUM);
                    if (chr)
                    {
                        chr->chrflags &= ~flags;
                    }
                    Offset += sizeof(AiUnsetChrchrflagsRecord);
                    break;
                }
                case AI_IFChrchrflagsHas:
                {
                    AiIFChrchrflagsHasRecord *ai    = AiListp + Offset;
                    CHRFLAG                   flags = ntohl(ai->CHRFLAGS);
                    ChrRecord                *chr   = chrFindById(ChrEntityp, ai->CHR_NUM);
                    if (chr && (chr->chrflags & flags) == flags)
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFChrchrflagsHasRecord);
                    }
                    break;
                }
                case AI_SetObjectFlags:
                {
                    AiSetObjectFlagsRecord *ai    = AiListp + Offset;
                    s32                     flags = ntohl(ai->BITFIELD);
                    ObjectRecord           *obj   = objFindByTagId(ai->OBJECT_TAG);
                    if (obj && obj->prop)
                    {
                        obj->flags |= flags;
                    }
                    Offset += sizeof(AiSetObjectFlagsRecord);
                    break;
                }
                case AI_UnsetObjectFlags:
                {
                    AiUnsetObjectFlagsRecord *ai    = AiListp + Offset;
                    s32                       flags = ntohl(ai->BITFIELD);
                    ObjectRecord             *obj   = objFindByTagId(ai->OBJECT_TAG);
                    if (obj && obj->prop)
                    {
                        obj->flags &= ~flags;
                    }
                    Offset += sizeof(AiUnsetObjectFlagsRecord);
                    break;
                }
                case AI_IFObjectFlagsHas:
                {
                    AiIFObjectFlagsHasRecord *ai    = AiListp + Offset;
                    s32                       flags = ntohl(ai->BITS);
                    ObjectRecord             *obj   = objFindByTagId(ai->OBJECT_TAG);
                    if (obj && obj->prop && (obj->flags & flags) == flags)
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFObjectFlagsHasRecord);
                    }
                    break;
                }
                case AI_SetObjectFlags2:
                {
                    AiSetObjectFlags2Record *ai    = AiListp + Offset;
                    s32                      flags = ntohl(ai->BITS);
                    ObjectRecord            *obj   = objFindByTagId(ai->OBJECT_TAG);
                    if (obj && obj->prop)
                    {
                        obj->flags2 |= flags;
                    }
                    Offset += sizeof(AiSetObjectFlags2Record);
                    break;
                }
                case AI_UnsetObjectFlags2:
                {
                    AiUnsetObjectFlags2Record *ai    = AiListp + Offset;
                    s32                        flags = ntohl(ai->BITS);
                    ObjectRecord              *obj   = objFindByTagId(ai->OBJECT_TAG);
                    if (obj && obj->prop)
                    {
                        obj->flags2 &= ~flags;
                    }
                    Offset += sizeof(AiUnsetObjectFlags2Record);
                    break;
                }
                case AI_IFObjectFlags2Has:
                {
                    AiIFObjectFlags2HasRecord *ai    = AiListp + Offset;
                    s32                        flags = ntohl(ai->BITFIELD);
                    ObjectRecord              *obj   = objFindByTagId(ai->OBJECT_TAG);
                    if (obj && obj->prop && ((obj->flags2 & flags) == flags))
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFObjectFlags2HasRecord);
                    }
                    break;
                }
                case AI_SetMyChrPreset:
                {
                    AiSetMyChrPresetRecord *ai = AiListp + Offset;
                    chrSetChrPreset(ChrEntityp, ai->PRESET);
                    Offset += sizeof(AiSetMyChrPresetRecord);
                    break;
                }
                case AI_SetChrChrPreset:
                {
                    AiSetChrChrPresetRecord *ai = AiListp + Offset;
                    chrSetChrPreset2(ChrEntityp, ai->CHR_NUM, ai->PRESET);
                    Offset += sizeof(AiSetChrChrPresetRecord);
                    break;
                }
                case AI_SetMyPadPreset:
                {
                    AiSetMyPadPresetRecord *ai     = AiListp + Offset;
                    u16                     pad_id = ntohs(ai->PAD_PRESET);
                    if (ChrEntityp)
                    {
    #ifdef ENABLE_LOG
                        if (pad_id == PAD_PRESET1 && ChrEntityp->padpreset1 == PAD_PRESET1)
                        {
                            osSyncPrintf("RUSS : Pad is bollox -> Num=%d (%d) - PAD_PRESET1=%d\n", pad_id, ChrEntityp->padpreset1, PAD_PRESET1);
                        }
    #endif
                        chrSetPadPreset(ChrEntityp, pad_id);
                    }
                    else if (AircraftEntityp)
                    {
                        AircraftEntityp->pad = pad_id;
                    }
                    Offset += sizeof(AiSetMyPadPresetRecord);
                    break;
                }
                case AI_SetChrPadPreset:
                {
                    AiSetChrPadPresetRecord *ai     = AiListp + Offset;
                    u16                      pad_id = ntohs(ai->PAD_PRESET);
                    chrSetPadPresetByChrnum(ChrEntityp, ai->CHR_NUM, pad_id);
                    Offset += sizeof(AiSetChrPadPresetRecord);
                    break;
                }
                case AI_PRINT:
                {
    #ifdef ENABLE_LOG
                    AIRecord *ai = AiListp + Offset;
                    osSyncPrintf("AI_PRINT: %s\n", ai->val);
        #ifdef IS_PD
                    if (ChrEntityp)
                    {
                        osSyncPrintf("AI_PRINT(void) [%d] %s\n", ChrEntityp->chrnum, ai->val);
                    }
                    else if (VehichleEntityp)
                    {
                        osSyncPrintf("AI_PRINT(void) [hover vehicle] %s\n", ai->val);
                    }
        #endif
    #endif
                    Offset += chraiitemsize(AiListp, Offset);
                    break;
                }
                case AI_MyTimerStart:
                {
                    chrRestartTimer(ChrEntityp); // Set timer60 to 0 and set flag
                    Offset += sizeof(AiMyTimerStartRecord);
                    break;
                }
                case AI_MyTimerReset:
                {
                    ChrEntityp->timer60 = 0;
                    Offset += sizeof(AiMyTimerResetRecord);
                    break;
                }
                case AI_MyTimerPause:
                {
                    ChrEntityp->hidden &= ~CHRHIDDEN_TIMER_ACTIVE;
                    Offset += sizeof(AiMyTimerPauseRecord);
                    break;
                }
                case AI_MyTimerResume:
                {
                    ChrEntityp->hidden |= CHRHIDDEN_TIMER_ACTIVE;
                    Offset += sizeof(AiMyTimerResumeRecord);
                    break;
                }
                case AI_IFMyTimerIsNotRunning:
                {
                    AiIFMyTimerIsNotRunningRecord *ai = AiListp + Offset;
                    if (((ChrEntityp->hidden & CHRHIDDEN_TIMER_ACTIVE) == 0))
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFMyTimerIsNotRunningRecord);
                    }
                    break;
                }
                case AI_IFMyTimerLessThanTicks:
                {
                    AiIFMyTimerLessThanTicksRecord *ai   = AiListp + Offset;
                    f32                             valf = ((unsigned)CharArrayTo24(((unsigned char *)(&(ai->TICKS))), 0)) / CHRAI_TICKRATE_F;

                    if (chrGetTimer(ChrEntityp) < valf)
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFMyTimerLessThanTicksRecord);
                    }
                    break;
                }
                case AI_IFMyTimerGreaterThanTicks:
                {
                    AiIFMyTimerGreaterThanTicksRecord *ai   = AiListp + Offset;
                    f32                                valf = ((unsigned)CharArrayTo24(((unsigned char *)(&(ai->TICKS))), 0)) / CHRAI_TICKRATE_F;
                    if (chrGetTimer(ChrEntityp) > valf)
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFMyTimerGreaterThanTicksRecord);
                    }
                    break;
                }
                case AI_HudCountdownShow:
                {
                    countdownTimerSetVisible(1, TRUE);
                    Offset += sizeof(AiHudCountdownShowRecord);
                    break;
                }
                case AI_HudCountdownHide:
                {
                    countdownTimerSetVisible(1, FALSE);
                    Offset += sizeof(AiHudCountdownHideRecord);
                    break;
                }
                case AI_HudCountdownSet:
                {
                    AiHudCountdownSetRecord *ai      = AiListp + Offset;
                    f32                      seconds = ntohs(ai->SECONDS);
                    countdownTimerSetValue(seconds * CHRAI_TICKRATE_F);
                    Offset += sizeof(AiHudCountdownSetRecord);
                    break;
                }
                case AI_HudCountdownStop:
                {
                    countdownTimerSetRunning(FALSE);
                    Offset += sizeof(AiHudCountdownStopRecord);
                    break;
                }
                case AI_HudCountdownStart:
                {
                    countdownTimerSetRunning(TRUE);
                    Offset += sizeof(AiHudCountdownStartRecord);
                    break;
                }
                case AI_IFHudCountdownIsNotRunning:
                {
                    AiIFHudCountdownIsNotRunningRecord *ai = AiListp + Offset;
                    if (!countdownTimerIsRunning())
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFHudCountdownIsNotRunningRecord);
                    }
                    break;
                }
                case AI_IFHudCountdownLessThan:
                {
                    AiIFHudCountdownLessThanRecord *ai    = AiListp + Offset;
                    f32                             value = ntohs(ai->SECONDS);
                    if (countdownTimerGetValue() < value * CHRAI_TICKRATE_F)
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFHudCountdownLessThanRecord);
                    }
                    break;
                }
                case AI_IFHudCountdownGreaterThan:
                {
                    AiIFHudCountdownGreaterThanRecord *ai    = AiListp + Offset;
                    f32                                value = ntohs(ai->SECONDS);
                    if (countdownTimerGetValue() > value * CHRAI_TICKRATE_F)
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFHudCountdownGreaterThanRecord);
                    }
                    break;
                }
                case AI_TRYSpawningChrAtPad:
                {
                    AiTRYSpawningChrAtPadRecord *ai       = AiListp + Offset;
                    u16                          pad      = ntohs(ai->PAD);
                    CHRFLAG                      flags    = ntohl(ai->BITFIELD);
                    u16                          ailistid = ntohs(ai->AI_LIST_ID);
                    AIRecord                    *ailist   = ailistFindById(ailistid);
    #ifdef ENABLE_LOG
                    if (flags & 32)
                    {
                        osSyncPrintf("ai_createchrheadthenjumpf : Flag set CHRSTART_FORCENOBLOOD\n");
                    }
    #endif
                    if (chrSpawnAtPad(ChrEntityp, ai->BODY_NUM, ai->HEAD_NUM, pad, ailist, flags))
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiTRYSpawningChrAtPadRecord);
                    }
                    break;
                }
                case AI_TRYSpawningChrNextToChr:
                {
                    AiTRYSpawningChrNextToChrRecord *ai       = AiListp + Offset;
                    CHRFLAG                          flags    = ntohl(ai->BITFIELD);
                    u16                              ailistid = ntohs(ai->AI_LIST_ID);
                    AIRecord                        *ailist   = ailistFindById(ailistid);
                    if (chrSpawnAtChr(ChrEntityp, ai->BODY_NUM, ai->HEAD_NUM, ai->CHR_NUM_TARGET, ailist, flags))
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiTRYSpawningChrNextToChrRecord);
                    }
                    break;
                }
                case AI_TRYGiveMeItem:
                {
                    AiTRYGiveMeItemRecord *ai    = AiListp + Offset;
                    s32                    flags = ntohl(ai->PROPFLAG);
                    s32                    model = ntohs(ai->PROP_NUM);
                    PropRecord            *prop  = NULL;

                    if (ChrEntityp && ChrEntityp->prop && ChrEntityp->model)
                    {
                        /*  more nice PD code that might be usefull in future
                        if (cheatIsActive(CHEAT_MARQUIS))
                        {
                            flags &= ~0x10000000;
                            flags |= 0x20000000;
                            prop = chrGiveWeapon(ChrEntityp, model, ai->val[2], flags);
                        }
                        else
                        */
                        if (cheatIsActive(28)) // CHEAT_ENEMYROCKETS
                        {
                            switch (ai->ITEM_NUM) // ITEM_IDS
                            {
                                case ITEM_KNIFE:
                                case ITEM_THROWKNIFE:
                                case ITEM_WPPK:
                                case ITEM_WPPKSIL:
                                case ITEM_TT33:
                                case ITEM_SKORPION:
                                case ITEM_AK47:
                                case ITEM_UZI:
                                case ITEM_MP5K:
                                case ITEM_MP5KSIL:
                                case ITEM_SPECTRE:
                                case ITEM_M16:
                                case ITEM_FNP90:
                                case ITEM_SHOTGUN:
                                case ITEM_AUTOSHOT:
                                case ITEM_SNIPERRIFLE:
                                case ITEM_RUGER:
                                case ITEM_GOLDENGUN:
                                case ITEM_SILVERWPPK:
                                case ITEM_GOLDWPPK:
                                case ITEM_LASER:
                                case ITEM_WATCHLASER:
                                case ITEM_REMOTEMINE:
                                case ITEM_TRIGGER:
                                case ITEM_TASER:

                                    prop = chrGiveWeapon(ChrEntityp, PROP_CHRROCKETLAUNCH, ITEM_ROCKETLAUNCH, flags);
                                    //! Bug, No Break! relying on chrGiveWeapon checking weapon already given
                                case ITEM_TIMEDMINE:
                                case ITEM_PROXIMITYMINE:
                                default:
                                    prop = chrGiveWeapon(ChrEntityp, model, ai->ITEM_NUM, flags);
                                    break;
                            }
                        }
                        else
                        {
                            prop = chrGiveWeapon(ChrEntityp, model, ai->ITEM_NUM, flags);
                        }
                    }
                    if (prop)
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiTRYGiveMeItemRecord);
                    }
                    break;
                }
                case AI_TRYGiveMeHat:
                {
                    AiTRYGiveMeHatRecord *ai       = AiListp + Offset;
                    s32                   flags    = ntohl(ai->PROP_BITFIELD);
                    s32                   modelnum = ntohs(ai->PROP_NUM);
                    bool                  ok       = FALSE;
                    if (ChrEntityp && ChrEntityp->prop && ChrEntityp->model)
                    {
                        ok = hatCreateForChr(ChrEntityp, modelnum, flags);
                    }
                    if (ok)
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiTRYGiveMeHatRecord);
                    }
                    break;
                }
                case AI_TRYCloningChr:
                {
                    AiTRYCloningChrRecord *ai       = AiListp + Offset;
                    // int zero                        = 0; //on stack in xbla, but matches without
                    u16                    ailistid = ntohs(ai->AI_LIST_ID);
                    u8                    *ailist   = ailistFindById((u16)ailistid);
                    ChrRecord             *chr      = chrFindById(ChrEntityp, ai->CHR_NUM);
                    bool                   pass     = FALSE; // 564
                    int                    chrnum;
                    PropRecord            *srcweaponLprop   = NULL;
                    PropRecord            *srcweaponRprop   = NULL;
                    PropRecord            *cloneweaponRprop = NULL;
                    PropRecord            *cloneweaponLprop = NULL;
                    PropRecord            *cloneprop        = NULL;
                    ChrRecord             *clone            = NULL; // 536
                    WeaponObjRecord       *srcweaponL       = NULL;
                    WeaponObjRecord       *cloneweaponL     = NULL; // 528
                    WeaponObjRecord       *cloneweaponR     = NULL; // 524
                    WeaponObjRecord       *srcweaponR       = NULL;
                    PropRecord            *hatprop;
                    ObjectRecord          *hatobj;
                    // bool tryhat;
                    if (chr && (chr->chrflags & CHRFLAG_CLONE))
                    {
                        cloneprop = chrSpawnAtChr(ChrEntityp, chr->bodynum, -1, chr->chrnum, ailist, 0);
                        if (cloneprop)
                        {
                            clone  = cloneprop->chr;
                            chrnum = chr->chrnum + 10000;
                            if (!chrFindById(ChrEntityp, chrnum))
                            {
                                clone->chrnum = chrnum;
                            }
                            // chrSetChrnum(clone, getLowestUnusedChrId());
                            // chr->chrdup = clone->chrnum;
                            srcweaponRprop = chrGetEquippedWeaponProp(chr, GUNRIGHT);
                            if (srcweaponRprop)
                            {
                                srcweaponR       = srcweaponRprop->weapon;
                                cloneweaponRprop = chrGiveWeapon(clone, srcweaponR->obj, srcweaponR->weaponnum, 0);
                                if (cloneweaponRprop)
                                {
                                    cloneweaponR = cloneweaponRprop->weapon;
                                }
                            }

                            srcweaponLprop = chrGetEquippedWeaponProp(chr, GUNLEFT);
                            if (srcweaponLprop)
                            {
                                srcweaponL       = srcweaponLprop->weapon;
                                cloneweaponLprop = chrGiveWeapon(clone, srcweaponL->obj, srcweaponL->weaponnum, 0x10000000);
                                if (cloneweaponLprop)
                                {
                                    cloneweaponL = cloneweaponLprop->weapon;
                                }
                            }

                            if (srcweaponL && srcweaponR && cloneweaponL && cloneweaponR && srcweaponR == srcweaponL->dualweapon && srcweaponL == srcweaponR->dualweapon)
                            {
                                propweaponSetDual(cloneweaponL, cloneweaponR);
                            }
                            {
                                hatprop = chr->handle_positiondata_hat;
                                if (hatprop)
                                {
                                    hatobj = hatprop->obj;

                                    hatCreateForChr(clone, hatobj->obj, 0);
                                }
                            }
                            /* PD extras */
                            // clone->morale     = chr->morale;
                            // clone->alertness  = chr->alertness;
                            // clone->padpreset1 = chr->padpreset1;
                            pass = TRUE;
                        }
                    }
                    if (pass)
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiTRYCloningChrRecord);
                    }
                    break;
                }
                case AI_TextPrintBottom:
                {
                    AiTextPrintBottomRecord *ai   = AiListp + Offset;
                    char                    *text = langGet(ntohs(ai->txt));
    #ifdef ENABLE_LOG
                    osSyncPrintf("USING HUD MESSAGE Stringy = %d, ai->txt = %d\n", text, ntohs(ai->txt));
    #endif
    #ifdef BUGFIX_R1
                    jp_hudmsgBottomShow(text);
    #else
                    hudmsgBottomShow(text);
    #endif
                    Offset += sizeof(AiTextPrintBottomRecord);
                    break;
                }
                case AI_TextPrintTop:
                {
                    AiTextPrintTopRecord *ai   = AiListp + Offset;
                    char                 *text = langGet(ntohs(ai->txt));

    #ifdef ENABLE_LOG
                    osSyncPrintf("ptop =  %f \n", text);
                    osSyncPrintf("USING HUD MESSAGE Stringy = %d, ai->txt = %d\n", text, ntohs(ai->txt));

    #endif

                    hudmsgTopShow(text);
                    Offset += sizeof(AiTextPrintTopRecord);
                    break;
                }
                case AI_SfxPlay:
                {
                    AiSfxPlayRecord *ai       = AiListp + Offset;
                    s16              audio_id = ntohs(ai->SOUND_NUM);
                    audioPlayFromProp((s8)ai->CHANNEL_NUM, audio_id);
                    Offset += sizeof(AiSfxPlayRecord);
                    break;
                }

                case AI_SfxStopChannel:
                {
                    AiSfxStopChannelRecord *ai = AiListp + Offset;
                    sub_GAME_7F0349BC(ai->CHANNEL_NUM);
                    Offset += sizeof(AiSfxStopChannelRecord);
                    break;
                }
                case AI_SfxSetChannelVolume:
                {
                    AiSfxSetChannelVolumeRecord *ai    = AiListp + Offset;
                    s16                          vol   = ntohs(ai->TARGET_VOLUME);
                    u16                          sfxID = ntohs(ai->sfxID);
                    if (ai->slotID >= 0 && ai->slotID < 8)
                    {
    #ifdef VERSION_EU
                        sfx_related[ai->slotID].sfxID = (sfxID * 50) / 60;
    #else
                        sfx_related[ai->slotID].sfxID = sfxID;
    #endif
                        sfx_related[ai->slotID].Volume = vol;
                        sfx_related[ai->slotID].pos    = NULL;
                        sfx_related[ai->slotID].Obj    = NULL;
                        if (sfxID == 0)
                        {
                            audioPlayFromProp2(ai->slotID);
                        }
                    }
                    /*
                      "AI : Bad sound setup command\n"
                    */
                    Offset += sizeof(AiSfxSetChannelVolumeRecord);
                    break;
                }
                case AI_SfxFadeChannelVolume:
                {
                    AiSfxFadeChannelVolumeRecord *ai    = AiListp + Offset;
                    f32                           vol   = ntohs(ai->TARGET_VOLUME);
                    u16                           sfxID = ntohs(ai->sfxID);
                    /*
                        "fadeTo\n"
                     */
                    if (ai->slotID >= 0 && ai->slotID < 8)
                    {
    #ifdef VERSION_EU
                        sfx_related[ai->slotID].sfxID = (sfxID * 50) / 60;
    #else
                        sfx_related[ai->slotID].sfxID = sfxID;
    #endif
                        sfx_related[ai->slotID].Volume = sub_GAME_7F0539B8(vol);
                        sfx_related[ai->slotID].pos    = NULL;
                        sfx_related[ai->slotID].Obj    = NULL;
                        if (sfxID == 0)
                        {
                            audioPlayFromProp2(ai->slotID);
                        }
                    }
                    Offset += sizeof(AiSfxFadeChannelVolumeRecord);
                    break;
                }
                case AI_SfxEmitFromObject:
                {
                    AiSfxEmitFromObjectRecord *ai    = AiListp + Offset;
                    ObjectRecord              *obj   = objFindByTagId(ai->OBJECT_TAG);
                    u16                        sfxID = ntohs(ai->sfxID);
                    if (ai->slotID >= 0 && ai->slotID < 8 && obj)
                    {
    #ifdef VERSION_EU
                        sfx_related[ai->slotID].sfxID = (sfxID * 50) / 60;
    #else
                        sfx_related[ai->slotID].sfxID = sfxID;
    #endif
                        sfx_related[ai->slotID].pos = NULL;
                        sfx_related[ai->slotID].Obj = obj;
                        if (sfxID == 0)
                        {
                            audioPlayFromProp2(ai->slotID);
                        }
                    }
                    Offset += sizeof(AiSfxEmitFromObjectRecord);
                    break;
                }
                case AI_SfxEmitFromPad:
                {
                    AiSfxEmitFromPadRecord *ai     = AiListp + Offset;
                    u16                     padnum = ntohs(ai->PAD);
                    PadRecord              *pad;
                    u16                     sfxID = ntohs(ai->sfxID);
                    if (isNotBoundPad(padnum))
                    {
                        pad = &g_CurrentSetup.pads[padnum];
                    }
                    else
                    {
                        pad = (PadRecord *)&g_CurrentSetup.boundpads[getBoundPadNum(padnum)];
                    }
                    if (ai->slotID >= 0 && ai->slotID < 8 && pad)
                    {
    #ifdef VERSION_EU
                        sfx_related[ai->slotID].sfxID = (sfxID * 50) / 60;
    #else
                        sfx_related[ai->slotID].sfxID = sfxID;
    #endif
                        sfx_related[ai->slotID].pos = pad;
                        sfx_related[ai->slotID].Obj = NULL;
                        if (sfxID == 0)
                        {
                            audioPlayFromProp2(ai->slotID);
                        }
                    }
                    Offset += sizeof(AiSfxEmitFromPadRecord);
                    break;
                }

                case AI_IFSfxChannelVolumeLessThan:
                {
                    AiIFSfxChannelVolumeLessThanRecord *ai  = AiListp + Offset;
                    s16                                 vol = ntohs(ai->VOLUME);
                    /*
                     * "ai_ifmusicqueueemptyjumpf : %s, State=%x (getlvleveltime60=%f)\n"
                     */

                    if ((ai->slotID >= 0) && (ai->slotID < 8) && (sfx_related[ai->slotID].Volume2 < vol))
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFSfxChannelVolumeLessThanRecord);
                    }
                    break;
                }
                case AI_VehicleStartPath:
                {
                    AiVehicleStartPathRecord *ai   = AiListp + Offset;
                    PathRecord               *path = pathFindById(ai->PATH_NUM);
                    if (VehichleEntityp)
                    {
                        VehichleEntityp->path     = path;
                        VehichleEntityp->nextstep = 0;
                    }
                    Offset += sizeof(AiVehicleStartPathRecord);
                    break;
                }
                case AI_VehicleSpeed:
                {
                    AiVehicleSpeedRecord *ai        = AiListp + Offset;
                    f32                   speedtime = ntohs(ai->ACCELERATION_TIME60);
                    f32                   speedaim  = ntohs(ai->TOP_SPEED) * 100.0f / 15360.0f;
                    if (VehichleEntityp)
                    {
                        VehichleEntityp->speedaim    = speedaim;
                        VehichleEntityp->speedtime60 = speedtime;
                    }
                    Offset += sizeof(AiVehicleSpeedRecord);
                    break;
                }
                case AI_AircraftRotorSpeed:
                {
                    AiAircraftRotorSpeedRecord *ai        = AiListp + Offset;
                    f32                         speedtime = ntohs(ai->ACCELERATION_TIME60);
                    f32                         speedaim  = ntohs(ai->ROTOR_SPEED) * M_TAU_F / 3600.0f;
                    if (AircraftEntityp)
                    {
                        AircraftEntityp->rotaryspeedaim  = speedaim;
                        AircraftEntityp->rotaryspeedtime = speedtime;
                    }
                    Offset += sizeof(AiAircraftRotorSpeedRecord);
                    break;
                }
                case AI_IFCameraIsInIntro:
                {
                    AiIFCameraIsInIntroRecord *ai = AiListp + Offset;
                    if ((bondviewGetCameraMode() == 1) || (bondviewGetCameraMode() == 2))
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFCameraIsInIntroRecord);
                    }
                    break;
                }
                case AI_IFCameraIsInBondSwirl:
                {
                    AiIFCameraIsInBondSwirlRecord *ai = AiListp + Offset;
                    if (bondviewGetCameraMode() == 3)
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFCameraIsInBondSwirlRecord);
                    }
                    break;
                }
                case AI_TvChangeScreenBank:
                {
                    AiTvChangeScreenBankRecord *ai  = AiListp + Offset;
                    ObjectRecord               *obj = objFindByTagId(ai->OBJECT_TAG);
                    if (obj && obj->prop)
                    {
                        if (obj->type == PROPDEF_MONITOR)
                        {
                            MonitorObjRecord *sm = (MonitorObjRecord *)obj;
                            monitorSetImageByNum(&sm->Monitor.cmdlist, ai->SCREEN_BANK);
                        }
                        else if (obj->type == PROPDEF_MULTI_MONITOR)
                        {
                            u8 slot = ai->SCREEN_INDEX;
                            if (slot < 4)
                            {
                                MultiMonitorObjRecord *mm = (MultiMonitorObjRecord *)obj; // need new size here 0x74 (116) + 0x80 (so monitor is obj + 74)
                                monitorSetImageByNum(&mm->Monitor[slot].cmdlist, ai->SCREEN_BANK);
                            }
                        }
                    }
                    Offset += sizeof(AiTvChangeScreenBankRecord);
                    break;
                }
                case AI_IFBondInTank: // canonical name
                {
                    AiIFBondInTankRecord *ai = AiListp + Offset;
    #ifdef ENABLE_LOG
                    osSyncPrintf("ai_ifbondintank\n");
    #endif
                    if (isBondInTank() == TRUE)
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFBondInTankRecord);
                    }
                    break;
                }
                case AI_EndLevel: // canonical name
                {
                    /*"aiEndLevel" */
                    if (cameraBufferToggle)
                    {
                        if (cameraFrameCounter2 == FALSE)
                        {
                            cameraFrameCounter2 = TRUE;
                        }
                    }
                    else
                    {
                        bossReturnTitleStage();
                    }
                    Offset += sizeof(AiEndLevelRecord);
                    break;
                }
                case AI_CameraReturnToBond:
                {
                    bondviewSetCameraMode(CAMERAMODE_FP_NOINPUT);
                    Offset += sizeof(AiCameraReturnToBondRecord);
                    break;
                }
                case AI_CameraLookAtBondFromPad:
                {
                    AiCameraLookAtBondFromPadRecord *ai     = AiListp + Offset;
                    u16                              padnum = ntohs(ai->PAD);
                    if (isNotBoundPad(padnum))
                    {
                        g_CameraLookAtBondPad = &g_CurrentSetup.pads[padnum];
                    }
                    else
                    {
                        g_CameraLookAtBondPad = (PadRecord *)&g_CurrentSetup.boundpads[getBoundPadNum(padnum)];
                    }
                    bondviewSetCameraMode(CAMERAMODE_POSEND);
                    Offset += sizeof(AiCameraLookAtBondFromPadRecord);
                    break;
                }
                case AI_CameraSwitch:
                {
                    AiCameraSwitchRecord *ai  = AiListp + Offset;
                    TagObjectRecord      *tag = sub_GAME_7F057080(ai->OBJECT_TAG);
                    if (tag)
                    {
                        int TagIndex = tagGetCommandIndex(tag); // get index
                        if (TagIndex >= 0)
                        {
                            CutsceneRecord *cdef = setupGetPtrToCommandByIndex(tag->OffsetToObj + TagIndex); // get obj

    #ifdef ENABLE_LOG
                            /*".\\ported\\chrai.c", 0xc2b, "Assertion failed: cdef->type==PROPDEF_CAMERAPOS") */
                            assert(cdef->type == PROPDEF_CAMERAPOS);
    #endif
                            g_CameraLookAtBondPad = NULL;
                            gBondViewCutscene       = cdef;
                            dword_CODE_bss_80079A18 = ntohs(ai->LOOK_AT_BOND_FLAG);
                            dword_CODE_bss_80079A1C = ntohs(ai->UNUSED_FLAG);
                            bondviewSetCameraMode(CAMERAMODE_POSEND);
                        }
                    }
                    Offset += AI_CameraSwitch_LENGTH;
                    break;
                }
                case AI_IFBondYPosLessThan:
                {
                    AiIFBondYPosLessThanRecord *ai      = AiListp + Offset;
                    f32                         bondpos = (s16)ntohs(ai->Y_POS);
                    if (getCurrentPlayerProp()->pos.y < bondpos)
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFBondYPosLessThanRecord);
                    }
                    break;
                }
                case AI_BondDisableControl:
                {
                    AiBondDisableControlRecord *ai = AiListp + Offset;
                    gunSetSightVisible(GUNSIGHTREASON_NOCONTROL, FALSE);
                    gunSetGunAmmoVisible(GUNAMMOREASON_NOCONTROL, FALSE);
                    if (!(PLAYERFLAG_NOCONTROL & ai->val))
                    {
                        hudmsgsSetOff(PLAYERFLAG_NOCONTROL);
                    }
                    if (!(ai->val & PLAYERFLAG_LOCKCONTROLS))
                    {
                        bondviewSetUpperTextDisplayFlag(PLAYERFLAG_NOCONTROL);
                    }
                    if (!(ai->val & PLAYERFLAG_NOTIMER))
                    {
                        countdownTimerSetVisible(16, FALSE);
                    }
                    is_timer_active = FALSE;
                    Offset += sizeof(AiBondDisableControlRecord);
                    break;
                }
                case AI_BondEnableControl:
                {
    #ifdef ENABLE_LOG
                    osSyncPrintf("AI_BONDENABLECONTROL\n");
    #endif
                    gunSetSightVisible(GUNSIGHTREASON_NOCONTROL, TRUE);
                    gunSetGunAmmoVisible(GUNAMMOREASON_NOCONTROL, TRUE);
                    hudmsgsSetOn(PLAYERFLAG_NOCONTROL);
                    bondviewClearUpperTextDisplayFlag(2);
                    countdownTimerSetVisible(16, TRUE);
                    is_timer_active = TRUE;
                    Offset += sizeof(AiBondEnableControlRecord);
                    break;
                }
                case AI_TRYTeleportingChrToPad:
                {
                    AiTRYTeleportingChrToPadRecord *ai     = AiListp + Offset;
                    s32                             padnum = ntohs(ai->PAD);
                    ChrRecord                      *chr    = chrFindById(ChrEntityp, ai->CHR_NUM);
                    bool                            pass   = FALSE;
                    PadRecord                      *pad;
                    f32                             FacingDirection;
                    coord3d                         pos;
                    StandTile                      *stan;

                    if (chr)
                    {
                        padnum = chrResolvePadId(ChrEntityp, padnum);
                        if (isNotBoundPad(padnum))
                        {
                            pad = &g_CurrentSetup.pads[padnum];
                        }
                        else
                        {
                            pad = (PadRecord *)&g_CurrentSetup.boundpads[getBoundPadNum(padnum)];
                        }

                        FacingDirection = atan2f(pad->look.x, pad->look.z);
                        pos.x           = pad->pos.x;
                        pos.y           = pad->pos.y;
                        pos.z           = pad->pos.z;
                        // pos  = pad->pos; <-uses lw instead of lwc1
                        stan            = pad->stan;
                        sub_GAME_7F03D058(chr->prop, FALSE);

                        if (chrAdjustPosForSpawn(&pos, &stan, FacingDirection, TRUE))
                        {
                            {
                                chr->prop->pos.x = pos.x;
                                chr->prop->pos.y = pos.y;
                                chr->prop->pos.z = pos.z;
                            }
                            // chr->prop->pos  = pos;
                            chr->prop->stan = stan;
                            chr->chrflags   = chr->chrflags | CHRFLAG_INIT;
                            setsubroty(chr->model, FacingDirection);
                            setsuboffset(chr->model, &pos);
                            chrDetectRooms(chr);
                            if (chr->prop == g_CurrentPlayer->prop)
                            {
                                g_CurrentPlayer->field_488.collision_position.x = pos.x;
                                g_CurrentPlayer->field_488.collision_position.y = pos.y;
                                g_CurrentPlayer->field_488.collision_position.z = pos.z;

                                // g_CurrentPlayer->pos = pos;
                                g_CurrentPlayer->field_488.current_tile_ptr = stan;
                            }
                            pass = TRUE;
                        }
                        sub_GAME_7F03D058(chr->prop, TRUE);
                    }
                    if (pass)
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiTRYTeleportingChrToPadRecord);
                    }
                    break;
                }
                case AI_ScreenFadeToBlack:
                {
                    if (stop_time_flag != 2)
                    {
                        currentPlayerSetFadeColour(0, 0, 0, 0);
                        currentPlayerSetFadeFrac(CHRAI_TICKRATE_F, 1);
                    }
                    Offset += sizeof(AiScreenFadeToBlackRecord);
                    break;
                }
                case AI_ScreenFadeFromBlack:
                {
                    if (stop_time_flag != 2)
                    {
                        currentPlayerSetFadeColour(0, 0, 0, 1);
                        currentPlayerSetFadeFrac(CHRAI_TICKRATE_F, 0);
                    }
                    Offset += sizeof(AiScreenFadeFromBlackRecord);
                    break;
                }
                case AI_IFScreenFadeCompleted:
                {
                    AiIFScreenFadeCompletedRecord *ai = AiListp + Offset;
                    if (g_CurrentPlayer->colourfadetimemax60 < 0)
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFScreenFadeCompletedRecord);
                    }
                    break;
                }
                case AI_HideAllChrs:
                {
                    s32 num;
                    for (num = get_numguards() - 1; num >= 0; num--)
                    {
                        if (g_ChrSlots[num].model != NULL)
                        {
                            g_ChrSlots[num].chrflags |= CHRFLAG_HIDDEN;
                        }
                    }
                    Offset += sizeof(AiHideAllChrsRecord);
                    break;
                }
                case AI_ShowAllChrs:
                {
                    s32 num;
                    for (num = get_numguards() - 1; num >= 0; num--)
                    {
                        g_ChrSlots[num].chrflags &= ~CHRFLAG_HIDDEN;
                    }

                    Offset += sizeof(AiShowAllChrsRecord);
                    break;
                }
                case AI_DoorOpenInstant:
                {
                    AiDoorOpenInstantRecord *ai   = AiListp + Offset;
                    DoorRecord              *door = objFindByTagId(ai->OBJECT_TAG);
                    if (door && door->prop)
                    {
                        // DoorRecord *door   = (DoorRecord *)obj;
                        door->speed        = 0;
                        door->openPosition = door->maxFrac;
                        door->openedTime   = g_GlobalTimer;
                        door->openstate    = DOORSTATE_STATIONARY;
                        doorUpdateBbox(door);
                        doorActivatePortal(door); // doorActivatePortal
                        door7F053B10(door);
                    }
                    Offset += sizeof(AiDoorOpenInstantRecord);
                    break;
                }
                case AI_ChrRemoveItemInHand:
                {
                    AiChrRemoveItemInHandRecord *ai  = AiListp + Offset;
                    ChrRecord                   *chr = chrFindById(ChrEntityp, ai->CHR_NUM);
                    if (chr)
                    {
                        chrSetWeaponFlag4(chr, ai->HAND_INDEX);
                    }
                    Offset += sizeof(AiChrRemoveItemInHandRecord);
                    break;
                }
                case AI_IfNumberOfActivePlayersLessThan:
                {
                    AiIfNumberOfActivePlayersLessThanRecord *ai = AiListp + Offset;
                    if (getPlayerCount() < ai->NUMBER)
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIfNumberOfActivePlayersLessThanRecord);
                    }
                    break;
                }
                case AI_IFBondItemTotalAmmoLessThan:
                {
                    AiIFBondItemTotalAmmoLessThanRecord *ai = AiListp + Offset;
                    if (currentPlayerGetAmmoCount(ai->ITEM_NUM) < ai->AMMO_TOTAL)
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFBondItemTotalAmmoLessThanRecord);
                    }
                    break;
                }
                case AI_BondEquipItem:
                {
                    AiBondEquipItemRecord *ai = AiListp + Offset;
                    currentPlayerEquipWeaponWrapper(GUNRIGHT, ai->ITEM_NUM);
                    currentPlayerEquipWeaponWrapper(GUNLEFT, 0);
                    Offset += sizeof(AiBondEquipItemRecord);
                    break;
                }
                case AI_BondEquipItemCinema:
                {
                    AiBondEquipItemCinemaRecord *ai = AiListp + Offset;
                    currentPlayerUnEquipWeaponWrapper(GUNRIGHT, ai->ITEM_NUM);
                    currentPlayerUnEquipWeaponWrapper(GUNLEFT, 0);
                    Offset += sizeof(AiBondEquipItemCinemaRecord);
                    break;
                }
                case AI_BondSetLockedVelocity:
                {
                    /*
                    g_Vars.currentplayer->bondforcespeed.x = (s8)ai->val[1];
                    g_Vars.currentplayer->bondforcespeed.y = 0;
                    g_Vars.currentplayer->bondforcespeed.z = (s8)ai->val[2];
                    */
                    AiBondSetLockedVelocityRecord *ai = AiListp + Offset;
                    g_ForceBondMoveOffset.x           = ai->X_SPEED60;
                    g_ForceBondMoveOffset.y           = 0;
                    g_ForceBondMoveOffset.z           = ai->Z_SPEED60;
                    Offset += sizeof(AiBondSetLockedVelocityRecord);
                    break;
                }
                case AI_IFObjectInRoomWithPad:
                {
                    AiIFObjectInRoomWithPadRecord *ai     = AiListp + Offset;
                    u16                            padnum = ntohs(ai->PAD);
                    PadRecord                     *pad;
                    ObjectRecord                  *obj = objFindByTagId(ai->OBJECT_TAG);

                    if (isNotBoundPad(padnum))
                    {
                        pad = &g_CurrentSetup.pads[padnum * 1]; // needs a mult by 1 to correct s0/v1
                    }
                    else
                    {
                        pad = (PadRecord *)&g_CurrentSetup.boundpads[getBoundPadNum(padnum)];
                    }

                    if (pad->stan && obj && obj->prop && (pad->stan->room == obj->prop->stan->room))
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFObjectInRoomWithPadRecord);
                    }
                    break;
                }
                case AI_SwitchSky:
                { // SWITCHENVIRONMENT
                    fogSwitchToSolosky2(1.0);
                    Offset += sizeof(AiSwitchSkyRecord);
                    break;
                }
                case AI_TriggerFadeAndExitLevelOnButtonPress:
                {
                    if (stop_time_flag == FALSE)
                    {
                        stop_time_flag = TRUE;
                    }
                    Offset += sizeof(AiTriggerFadeAndExitLevelOnButtonPressRecord);
                    break;
                }
                case AI_IFBondIsDead:
                {
                    AiIFBondIsDeadRecord *ai = AiListp + Offset;
                    if (g_CurrentPlayer->bonddead)
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFBondIsDeadRecord);
                    }
                    break;
                }
                case AI_BondDisableDamageAndPickups:
                {
                    g_PlayerInvincible = TRUE;
                    Offset += sizeof(AiBondDisableDamageAndPickupsRecord);
                    break;
                }
                case AI_BondHideWeapons:
                {
                    /*"remove guntype %d\n" */
                    remove_item_in_hand(GUNRIGHT);
                    remove_item_in_hand(GUNLEFT);
                    Offset += sizeof(AiBondHideWeaponsRecord);
                    break;
                }
                case AI_CameraOrbitPad: // sp order from xbla
                {
                    AiCameraOrbitPadRecord *ai = AiListp + Offset;
                    s32                     padnum;
                    s32                     speed60;
                    s32                     camDististance;
                    s32                     targetHeight;
                    s32                     camHeight;
                    s32                     start;
                    camDististance          = ntohs(ai->LAT_DISTANCE);
                    camHeight               = (s16)ntohs(ai->VERT_DISTANCE);
                    speed60                 = (s16)ntohs(ai->ORBIT_SPEED60);
                    padnum                  = ntohs(ai->PAD);
                    targetHeight            = (s16)ntohs(ai->Y_POS_OFFSET);
                    start                   = ntohs(ai->INITIAL_ROTATION);
                    g_CameraLookAtBondPad = NULL;
                    gBondViewCutscene       = NULL;
                    flt_CODE_bss_80079A00   = (start * M_TAU_F) / M_U16_MAX_VALUE_F;   /*unit direction 0 - 1 (increments are 0.000001) */
                    flt_CODE_bss_80079A04   = (speed60 * M_TAU_F) / M_U16_MAX_VALUE_F; /*how many increments per frame */
                    flt_CODE_bss_80079A08   = camDististance;
                    flt_CODE_bss_80079A0C   = camHeight;
                    flt_CODE_bss_80079A10   = targetHeight;
                    dword_CODE_bss_80079A14 = padnum;
                    bondviewSetCameraMode(CAMERAMODE_POSEND);
                    Offset += sizeof(AiCameraOrbitPadRecord);
                    break;
                }
                case AI_CreditsRoll:
                {
                    credits_state = TRUE;
                    Offset += sizeof(AiCreditsRollRecord);
                    break;
                }
                case AI_IFCreditsHasCompleted:
                {
                    AiIFCreditsHasCompletedRecord *ai = AiListp + Offset;
                    if (credits_state == 2)
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFCreditsHasCompletedRecord);
                    }
                    break;
                }
                case AI_IFObjectiveAllCompleted:
                {
                    AiIFObjectiveAllCompletedRecord *ai = AiListp + Offset;
                    // bool a = objectiveIsAllComplete();
                    if (objectiveIsAllComplete())
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFObjectiveAllCompletedRecord);
                    }
                    break;
                }
                case AI_IFFolderActorIsEqual:
                {
                    AiIFFolderActorIsEqualRecord *ai = AiListp + Offset;
                    if (fileGetBondForCurrentFolder() == ai->BOND_ACTOR_INDEX)
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFFolderActorIsEqualRecord);
                    }
                    break;
                }
                case AI_IFBondDamageAndPickupsDisabled:
                {
                    AiIFBondDamageAndPickupsDisabledRecord *ai = AiListp + Offset;
                    if (g_PlayerInvincible)
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFBondDamageAndPickupsDisabledRecord);
                    }
                    break;
                }
                case AI_MusicPlaySlot:
                {
                    AiMusicPlaySlotRecord *ai = AiListp + Offset;
                    Offset += sizeof(AiMusicPlaySlotRecord);
                    musicPlaySlot(ai->MUSIC_SLOT, ai->SECONDS_STOPPED_DURATION, ai->SECONDS_TOTAL_DURATION);
    #ifdef ENABLE_LOG
                    osSyncPrintf("ai: enery tune on (%d, %d, %d)\n", ai->MUSIC_SLOT, ai->SECONDS_STOPPED_DURATION, ai->SECONDS_TOTAL_DURATION);
    #endif
                    break;
                }
                case AI_MusicStopSlot:
                {
                    AiMusicStopSlotRecord *ai = AiListp + Offset;
                    Offset += sizeof(AiMusicStopSlotRecord);
                    musicStopSlot(ai->MUSIC_SLOT);
    #ifdef ENABLE_LOG
                    osSyncPrintf("ai: enery tune off (%d)\n", ai->MUSIC_SLOT);
    #endif
                    break;
                }
                case AI_TriggerExplosionsAroundBond:
                {
                    SurroundWithExplosions(0);
                    Offset += sizeof(AiTriggerExplosionsAroundBondRecord);
                    break;
                }
                case AI_IFKilledCiviliansGreaterThan:
                {
                    AiIFKilledCiviliansGreaterThanRecord *ai = AiListp + Offset;
                    if (ai->CIVILIANS_KILLED < get_civilian_casualties())
                    {
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFKilledCiviliansGreaterThanRecord);
                    }
                    break;
                }
                case AI_IFChrWasShotSinceLastCheck:
                {
                    AiIFChrWasShotSinceLastCheckRecord *ai  = AiListp + Offset;
                    ChrRecord                          *chr = chrFindById(ChrEntityp, ai->CHR_NUM);
                    if (chr && chr->chrflags & CHRFLAG_WAS_HIT)
                    {
                        chr->chrflags &= ~CHRFLAG_WAS_HIT;
                        Offset = chraiGoToLabel(AiListp, Offset, ai->GOTOLABEL);
                    }
                    else
                    {
                        Offset += sizeof(AiIFChrWasShotSinceLastCheckRecord);
                    }
                    break;
                }
                case AI_BondKilledInAction:
                {
                    g_isBondKIA = TRUE;
                    Offset += sizeof(AiBondKilledInActionRecord);
                    break;
                }
                case AI_RaiseArms:
                {
                    chrTrySurprisedSurrender(ChrEntityp);
                    Offset += sizeof(AiRaiseArmsRecord);
                    break;
                }
                case AI_GasLeakAndFadeFog:
                {
                    coord3d emitPos = New_Coord3d();
                    init_trigger_toxic_gas_effect(&emitPos);
                    Offset += sizeof(AiGasLeakAndFadeFogRecord);
                    break;
                }
                case AI_ObjectRocketLaunch:
                {
                    AiObjectRocketLaunchRecord *ai  = AiListp + Offset;
                    ObjectRecord               *obj = objFindByTagId(ai->OBJECT_TAG);

                    if (obj && obj->prop)
                    {
                        sub_GAME_7F03FDA8(obj->prop);

                        if (obj->runtime_bitflags & RUNTIMEBITFLAG_HASPROJECTILE)
                        {
                            obj->projectile->flags |= 0x601;
                            projectileSetSticky(obj->prop);
                            matrix_4x4_set_identity(&obj->projectile->mtx);
                            obj->projectile->speed.x = 0;
                            obj->projectile->speed.y = 0.016666666f; // step height?
                            obj->projectile->speed.z = 0;
                            obj->projectile->unk10.x = 0;
                            obj->projectile->unk10.y = 0.29166666f; // direction to move?
                            obj->projectile->unk10.z = 0;
                        }
                    }
                    Offset += sizeof(AiObjectRocketLaunchRecord);
                    break;
                } //============================================================================================================
#endif
                default:
                    /*
                     * No Command found, advance ailist by 1.
                     * This is attempting to handle situations where the command
                     * type is invalid by passing over them and continuing
                     * execution.
                     * chraiitemsize returns 1 which is pointless really
                     * could have done it here without a jump
                     *
                     * Outcome:crash
                     */
                    {
                        Offset += chraiitemsize(AiListp, Offset);
                    }
            } // switch
        } // for
    } // Has ailist
#ifdef ENABLE_LOG
    osSyncPrintf("SERIOUS AI ERROR!!!!!! Null ailist!\n");
#endif
} // ai()
