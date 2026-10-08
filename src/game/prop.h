#ifndef _PROP_H_
#define _PROP_H_
#include <ultra64.h>

extern const u32 only_read_by_stageload[];

void proplvreset2(enum LEVELID stageId);


#ifdef GE_MODDED_CHEATS
/* R27Q_DAM_DOCK_RESTORATION */
s32 propLevelModifierPrepareDamRestorations(enum LEVELID stageId);
s32 propLevelModifierSetDamDoors(s32 enabled);
s32 propLevelModifierSetDamSpeedboat(s32 enabled);
/* R27R_DRIVABLE_SPEEDBOAT */
s32 propLevelModifierActivateDamDrivableSpeedboat(void);
s32 propLevelModifierDamDrivableBoatCurrentPlayerDriving(void);
s32 propLevelModifierDamDrivableBoatEntering(void);
s32 propLevelModifierDamDrivableBoatCanCurrentPlayerEnter(void);
void propLevelModifierDamDrivableBoatEnterCurrentPlayer(void);
void propLevelModifierDamDrivableBoatExitCurrentPlayer(void);
void propLevelModifierDamDrivableBoatSetControls(f32 throttle, f32 steering);
void propLevelModifierDamDrivableBoatTick(void);
f32 propLevelModifierDamDrivableBoatDriverY(void);
s32 propLevelModifierDamDrivableBoatCurrentPlayerSupported(void);
void propLevelModifierDamDrivableBoatStopAudio(void);
s32 propLevelModifierDamDrivableBoatHandleFootMovement(struct coord3d *moveOffset);
s32 propLevelModifierDamDrivableBoatGetWalkFloor(f32 *floorY);
void propLevelModifierCleanupDamRestorations(void);
#endif

#endif
