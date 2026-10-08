#!/usr/bin/env python3
from pathlib import Path
import sys
import re

R = Path(__file__).resolve().parents[1]
LM = (R/"src/game/levelmodifiers.c").read_text(errors="replace")
P  = (R/"src/game/prop.c").read_text(errors="replace")
B  = (R/"src/game/bondview2.c").read_text(errors="replace")
MK = (R/"Makefile").read_text(errors="replace")

checks=[]
def ck(name, cond):
    cond=bool(cond)
    checks.append((name,cond))
    print(("[PASS] " if cond else "[FAIL] ")+name)

ck("Dam exposes three modifiers including Drivable Speedboat",
   "return g_LevelModifierDamRuntimeAvailable ? 3 : 0;" in LM and
   '"Drivable Speedboat"' in LM)

ck("Drivable Speedboat is latched ACTIVATE/ACTIVE in-game",
   "levelModifiersGetDamDrivableSpeedboatPolicy" in LM and
   "return LEVELMOD_POLICY_LATCHED;" in
       LM[LM.find("levelModifiersGetDamDrivableSpeedboatPolicy"):
          LM.find("levelModifiersGetCurrentStageModifierName")] and
   'g_LevelModifierDamDrivableSpeedboatActive ? "ACTIVE" : "ACTIVATE"' in LM)

ck("active Drivable Speedboat is locked against toggle-off",
   "index == 2" in LM and
   "g_LevelModifierDamDrivableSpeedboatActive" in LM and
   "return levelModifiersActivateDamDrivableSpeedboat();" in LM)

ck("frontend preload still permits pre-stage On/Off selection",
   "g_LevelModifierDamDrivableSpeedboatPreload" in LM and
   "levelModifiersSetDamDrivableSpeedboatPreload" in LM)

ck("one additional Dam object slot is reserved",
   "return 4; /* two restored doors + static speedboat + drivable speedboat */" in LM)

ck("drivable boat is an additional PROP_SPEEDBOAT clone",
   "g_LevelModifierDamDrivableBoatRuntime = g_LevelModifierDamBoatTemplate;" in P and
   "propLevelModifierActivateDamDrivableSpeedboat" in P)

ck("requested spawn transform is exact",
   "DAM_DRIVABLE_BOAT_START_X        448.0f" in P and
   "DAM_DRIVABLE_BOAT_START_Y       -768.0f" in P and
   "DAM_DRIVABLE_BOAT_START_Z      -7200.0f" in P and
   "DAM_DRIVABLE_BOAT_START_HEADING  M_PI_F" in P)

ck("boat top speed is 25.0 and above retail Tank max",
   "DAM_DRIVABLE_BOAT_MAX_SPEED       25.0f" in P)

ck("Tank B enter/exit interaction is reused for boat",
   "propLevelModifierDamDrivableBoatCanCurrentPlayerEnter()" in B and
   "propLevelModifierDamDrivableBoatEnterCurrentPlayer();" in B and
   "propLevelModifierDamDrivableBoatExitCurrentPlayer();" in B)

ck("boat uses Tank-family temporary audio with independent handles",
   "TRUCK_RUN_SFX" in P and
   "TANK_SFX" in P and
   "g_LevelModifierDamDrivableBoatSfx[2]" in P and
   "TRUCK_START_SFX, NULL" not in P)

ck("pause and stage cleanup stop boat audio",
   "propLevelModifierDamDrivableBoatStopAudio();" in B and
   "propLevelModifierFreeDamDrivableBoat();" in P)

ck("driving bypasses ordinary on-foot translation",
   "propLevelModifierDamDrivableBoatSetControls(" in B and
   "propLevelModifierDamDrivableBoatTick();" in B)

ck("driver/deck Y support prevents immediate gravity snap",
   "propLevelModifierDamDrivableBoatCurrentPlayerSupported()" in B and
   "propLevelModifierDamDrivableBoatDriverY()" in B)

prereq = next((x for x in MK.splitlines() if x.startswith("prerequisites:")), "")
ck("R27R audit is mandatory",
   "r27r-drivable-speedboat-audit:" in MK and
   "r27r-drivable-speedboat-audit" in prereq)


ck("dock-side boarding radius and Y tolerance are widened",
   any("DAM_DRIVABLE_BOAT_ENTRY_RADIUS" in line and "360.0f" in line
       for line in P.splitlines()) and
   any("DAM_DRIVABLE_BOAT_ENTRY_Y_RANGE" in line and "220.0f" in line
       for line in P.splitlines()))

ck("driver yaw follows hull heading after eased entry",
   "headingDeltaDeg = headingdeg - oldHeadingDeg;" in P and
   "g_CurrentPlayer->vv_theta += headingDeltaDeg;" in P and
   "g_CurrentPlayer->field_488.theta_transform.x" in P)

ck("all boat loop SFX are tracked and hard-stopped on exit",
   "sndCreatePostEvent(g_LevelModifierDamDrivableBoatSfx[i], 8, 0);" in P and
   "TRUCK_START_SFX, NULL" not in P)

ck("drivable speedboat is invincible to gunfire and explosions",
   "PROPFLAG_INVINCIBLE" in P and
   "PROPFLAG2_00004000 | PROPFLAG2_00200000" in P)

ck("boat hull movement is swept against real background geometry",
   "propLevelModifierDamDrivableBoatHullBlocked" in P and
   "bgTestBulletHitBackground" in P and
   "chrpropRayIntersectsRoomBbox" in P)


ck("R4 flips only the speedboat model forward axis",
   "DAM_DRIVABLE_BOAT_MODEL_YAW_OFFSET" in P and
   "M_TAU_F - g_LevelModifierDamDrivableBoatHeading" in P)

ck("R4 disables stock boat object prism for player boarding",
   "g_LevelModifierDamDrivableBoatRuntime.state |= PROPSTATE_20;" in P)

ck("R4 has custom land-to-deck and deck-to-STAN movement handoff",
   "propLevelModifierDamDrivableBoatHandleFootMovement" in P and
   "propLevelModifierDamDrivableBoatTryLandTransfer" in P and
   "stanFindGroundAtCyl" in P)

ck("R4 hooks custom boat foot movement before ordinary Bond collision",
   "propLevelModifierDamDrivableBoatHandleFootMovement(&move_offset)" in B)

ck("R4 uses Tank-style eased entry yaw instead of instant snap",
   "g_LevelModifierDamDrivableBoatEnterBlend" in P and
   "DAM_DRIVABLE_BOAT_ENTER_TICKS" in P and
   "cosf(g_LevelModifierDamDrivableBoatEnterBlend" in P)

ck("R4/R11 uses separate water envelope plus multi-height hard hull collision",
   "g_LevelModifierDamDrivableBoatUseWaterEnvelope" in P and
   "propLevelModifierDamDrivableBoatWaterSurfacePresent" in P and
   "f32 yoffsets[3];" in P and
   "for (h = 0; h < 3; h++)" in P and
   "for (i = 0; i < 9; i++)" in P and
   "propLevelModifierDamDrivableBoatBgSegmentBlocked" in P)

ck("R4 keeps Room 81 STAN while providing portal visibility anchor",
   "current_tile_ptr->room == 0x51" in B and
   "g_CurrentSetup.pads[111].stan" in B)




ck("R5 boarding keeps exact requested XZ without clamp teleport",
   "clampedX" not in P and "clampedZ" not in P and
   "collision_position.x = candidate.x;" in P and
   "collision_position.z = candidate.z;" in P)

ck("R5/R6 uses explicit boat-local transition coverage",
   "DAM_DRIVABLE_BOAT_DECK_SEAM_MARGIN" in P and
   "candidateInShell" in P and
   "propLevelModifierDamDrivableBoatDeckFloorAtWorld" in P)

ck("R5/R6 land handoff keeps exact candidate position",
   "propLevelModifierDamDrivableBoatTryLandTransfer" in P and
   "collision_position.x = candidate->x;" in P and
   "collision_position.z = candidate->z;" in P)

ck("R5 feeds boat floor into native Bond gravity integration",
   "propLevelModifierDamDrivableBoatGetWalkFloor" in B and
   "r27rBoatWalkFloorActive" in B and
   "use_stanHeight = 1;" in B and
   "R27R_R5_SMOOTH_BOAT_WALK_VOLUME" in B)

ck("R5 walking handler no longer zeros vertical velocity or snaps field70",
   "g_CurrentPlayer->field_7C = 0.0f;" not in P[P.find("s32 propLevelModifierDamDrivableBoatHandleFootMovement("):P.find("s32 propLevelModifierDamDrivableBoatCurrentPlayerSupported(")] and
   "g_CurrentPlayer->field_70 =" not in P[P.find("s32 propLevelModifierDamDrivableBoatHandleFootMovement("):P.find("s32 propLevelModifierDamDrivableBoatCurrentPlayerSupported(")])


ck("R6/R11 transition shell remains larger than R5",
   "DAM_DRIVABLE_BOAT_DECK_SEAM_MARGIN      84.0f" in P)


ck("R6 direct world STAN handoff retains exact candidate XZ",
   "propLevelModifierDamDrivableBoatWorldLandAt" in P and
   "collision_position.x = candidate->x;" in P and
   "collision_position.z = candidate->z;" in P)


ck("R6 bow and stern retain raised +55 deck profile",
   "DAM_DRIVABLE_BOAT_DECK_END_Y_OFFSET     55.0f" in P)


ck("R7 transition coverage is rectangular in boat-local space",
   "propLevelModifierDamDrivableBoatTransitionRectAtWorld" in P and
   "DAM_DRIVABLE_BOAT_TRANSITION_HALF_X" in P and
   "DAM_DRIVABLE_BOAT_TRANSITION_HALF_Z" in P and
   "> DAM_DRIVABLE_BOAT_TRANSITION_HALF_X" in P and
   "> DAM_DRIVABLE_BOAT_TRANSITION_HALF_Z" in P)

ck("R7 rectangular coverage is larger than the real deck",
   "(DAM_DRIVABLE_BOAT_DECK_HALF_X + DAM_DRIVABLE_BOAT_DECK_SEAM_MARGIN)" in P and
   "(DAM_DRIVABLE_BOAT_DECK_HALF_Z + DAM_DRIVABLE_BOAT_DECK_SEAM_MARGIN)" in P)

ck("R7 movement and walk-floor both use rectangular transition coverage",
   "propLevelModifierDamDrivableBoatTransitionRectAtWorld(\n            &pos, &deckY)" in
       P[P.find("s32 propLevelModifierDamDrivableBoatGetWalkFloor(f32 *floorY)"):
         P.find("s32 propLevelModifierDamDrivableBoatHandleFootMovement(")] and
   P[P.find("s32 propLevelModifierDamDrivableBoatHandleFootMovement("):
     P.find("s32 propLevelModifierDamDrivableBoatCurrentPlayerSupported(void)")].count(
         "propLevelModifierDamDrivableBoatTransitionRectAtWorld(") >= 2)

ck("R7/R8 drivable boat Y is -768",
   "DAM_DRIVABLE_BOAT_START_Y       -768.0f" in P)

ck("R7 preserves R6 water rejection threshold while lowering boat",
   "DAM_DRIVABLE_BOAT_DECK_LAND_MIN_Y       -676.0f" in P and
   "groundY > DAM_DRIVABLE_BOAT_DECK_LAND_MIN_Y" in P)


ck("R9 clipping-relaxation halo follows Bond collision radius",
   "DAM_DRIVABLE_BOAT_CLIP_RELAX_EXTRA" in P and
   "field_488.collision_radius" in P and
   "propLevelModifierDamDrivableBoatTransitionClipHaloAtWorld" in P)


ck("R9 boat entry stores exact starting position and does not snap immediately",
   "g_LevelModifierDamDrivableBoatEnterStartPos =" in P and
   "Do not place the driver here." in P)

ck("R9 boat entry position uses 45-tick cosine interpolation",
   "g_LevelModifierDamDrivableBoatEnterStartPos.x" in P and
   "cosf(g_LevelModifierDamDrivableBoatEnterBlend" in P and
   "DAM_DRIVABLE_BOAT_ENTER_TICKS" in P)

ck("R9 entry Y is preserved instead of snapped to final driver Y",
   "propLevelModifierDamDrivableBoatEntering()" in B and
   "cosine-interpolated entry Y" in B)

ck("R9 steering input is smoothed before hull yaw",
   "g_LevelModifierDamDrivableBoatSteeringApplied" in P and
   "DAM_DRIVABLE_BOAT_STEER_RESPONSE" in P and
   "steeringTarget = g_LevelModifierDamDrivableBoatSteering;" in P)

ck("R9 keeps the boat stationary while Bond is entering",
   "if (propLevelModifierDamDrivableBoatEntering())" in P and
   "target = 0.0f;" in P)

ck("R9 preserves final boat Y -768",
   "DAM_DRIVABLE_BOAT_START_Y       -768.0f" in P)


ck("R10/R12 retains Room-23 data but no longer hard-gates movement with STAN",
   "DAM_DRIVABLE_BOAT_WATER_ROOM              0x23" in P and
   "propLevelModifierDamDrivableBoatInWaterRoom" in P and
   "R27R R12: do NOT gate vehicle movement on STAN room 0x23." in P)

ck("R10/R12 Room-23 helper remains available but dormant",
   "rooms[0] = DAM_DRIVABLE_BOAT_WATER_ROOM;" in P and
   "stanFindGroundAtCyl(&probe, 8.0f, rooms, &groundY)" in P)

ck("R12 hard hull collision is movement authority without STAN Room-23 gate",
   "R27R R12: do NOT gate vehicle movement on STAN room 0x23." in P and
   "if (!propLevelModifierDamDrivableBoatHullBlocked(" in P)

ck("R10 retains final boat Y -768",
   "DAM_DRIVABLE_BOAT_START_Y       -768.0f" in P)


ck("R11/R11R1 hard hull collision is explicit rectangular boat-local geometry",
   "DAM_DRIVABLE_BOAT_HULL_HALF_X            88.0f" in P and
   "DAM_DRIVABLE_BOAT_HULL_HALF_Z            220.0f" in P and
   "xmin = -DAM_DRIVABLE_BOAT_HULL_HALF_X;" in P and
   "zmax =  DAM_DRIVABLE_BOAT_HULL_HALF_Z;" in P)

ck("R11/R11R1 rectangular hull remains slightly longer than the 210-unit deck half-length",
   "DAM_DRIVABLE_BOAT_HULL_HALF_Z            220.0f" in P and
   "DAM_DRIVABLE_BOAT_DECK_HALF_Z" in P)

ck("R11 hard hull still samples corners edge midpoints and center at three heights",
   "f32 lx[9];" in P and
   "f32 lz[9];" in P and
   "f32 yoffsets[3];" in P and
   "for (h = 0; h < 3; h++)" in P and
   "for (i = 0; i < 9; i++)" in P)

ck("R11 rectangular transition coverage margin is 84",
   "DAM_DRIVABLE_BOAT_DECK_SEAM_MARGIN      84.0f" in P)

ck("R11 preserves final boat Y -768",
   "DAM_DRIVABLE_BOAT_START_Y       -768.0f" in P)


ck("R11R1 restores dock clearance while keeping rectangular hull",
   "DAM_DRIVABLE_BOAT_HULL_HALF_X            88.0f" in P and
   "DAM_DRIVABLE_BOAT_HULL_HALF_Z            220.0f" in P)

ck("R11R1 preserves larger 84-unit player transition coverage",
   "DAM_DRIVABLE_BOAT_DECK_SEAM_MARGIN      84.0f" in P)

ck("R11R1 preserves final boat Y -768",
   "DAM_DRIVABLE_BOAT_START_Y       -768.0f" in P)


ck("R12 removes Room-23 STAN gating from Tick movement acceptance",
   "R27R R12: do NOT gate vehicle movement on STAN room 0x23." in P and
   "if (!propLevelModifierDamDrivableBoatHullBlocked(" in P)

ck("R12 preserves rectangular hull and expanded transition coverage",
   "DAM_DRIVABLE_BOAT_HULL_HALF_X            88.0f" in P and
   "DAM_DRIVABLE_BOAT_HULL_HALF_Z            220.0f" in P and
   "DAM_DRIVABLE_BOAT_DECK_SEAM_MARGIN      84.0f" in P)

ck("R12 preserves final boat Y -768",
   "DAM_DRIVABLE_BOAT_START_Y       -768.0f" in P)


ck("R13 adds boat-local deck edge wall-slide",
   "propLevelModifierDamDrivableBoatTryDeckWallSlide" in P and
   "currentLocalX" in P and
   "currentLocalZ" in P and
   "fullLocalX" in P and
   "fullLocalZ" in P)

ck("R13 tests X-only and Z-only deck movement instead of clamping",
   "xcandidate" in P and
   "zcandidate" in P and
   "xValid = propLevelModifierDamDrivableBoatDeckFloorAtWorld" in P and
   "zValid = propLevelModifierDamDrivableBoatDeckFloorAtWorld" in P)


ck("R13 does not zero player movement velocity at deck edges",
   "No speed/velocity fields are cleared here" in P)



ck("R15 embark Y interpolation uses Bond base Y rather than eye Y",
   "g_LevelModifierDamDrivableBoatEnterStartPos.y =" in P and
   "g_CurrentPlayer->field_70;" in P and
   "Do not mix first-person eye Y with driver-floor Y." in P)

ck("R15 preserves eye-height offset while interpolating base position",
   "eyeOffset = g_CurrentPlayer->field_488.collision_position.y" in P and
   "- g_CurrentPlayer->field_70;" in P and
   "placed.y + eyeOffset" in P)


ck("R15 keeps cosine entry duration and XZ interpolation",
   "DAM_DRIVABLE_BOAT_ENTER_TICKS" in P and
   "cosf(g_LevelModifierDamDrivableBoatEnterBlend" in P and
   "g_LevelModifierDamDrivableBoatEnterStartPos.x" in P and
   "g_LevelModifierDamDrivableBoatEnterStartPos.z" in P)


ck("R16 allows slow stationary boat turning",
   "DAM_DRIVABLE_BOAT_STATIONARY_TURN_SCALE  0.10f" in P and
   "speedScale = DAM_DRIVABLE_BOAT_STATIONARY_TURN_SCALE;" in P and
   "g_LevelModifierDamDrivableBoatSteeringApplied" in P)

ck("R16 preserves stronger minimum turn authority once moving",
   "else if (speedScale < 0.25f)" in P and
   "speedScale = 0.25f;" in P)

ck("R16 slows boat acceleration for heavier water feel",
   re.search(r"#define\s+DAM_DRIVABLE_BOAT_ACCEL\s+0\.40f", P) is not None)

ck("R16 lowers only the embarked driver base below the walking deck",
   re.search(r"#define\s+DAM_DRIVABLE_BOAT_DRIVER_Y_OFFSET\s+-12\.0f", P) is not None and
   "DAM_DRIVABLE_BOAT_DECK_CENTER_Y_OFFSET  8.0f" in P)

ck("R16 preserves drivable boat waterline Y -768",
   "DAM_DRIVABLE_BOAT_START_Y       -768.0f" in P)






























ck("R5/R6/R16 keeps central deck low while driver height is independently tuned",
   re.search(r"#define\s+DAM_DRIVABLE_BOAT_DECK_CENTER_Y_OFFSET\s+8\.0f", P) is not None and
   re.search(r"#define\s+DAM_DRIVABLE_BOAT_DECK_END_Y_OFFSET\s+55\.0f", P) is not None and
   re.search(r"#define\s+DAM_DRIVABLE_BOAT_DRIVER_Y_OFFSET\s+-12\.0f", P) is not None)

ck("R5 uses shaped tapered deck support with smooth raised ends",
   "propLevelModifierDamDrivableBoatDeckFloorAtLocal" in P and
   "DAM_DRIVABLE_BOAT_DECK_TIP_HALF_X" in P and
   "smooth = t * t * (3.0f - 2.0f * t);" in P)

ck("R6/R16 central wooden deck stays +8 while embarked driver is lowered to -12",
   re.search(r"#define\s+DAM_DRIVABLE_BOAT_DECK_CENTER_Y_OFFSET\s+8\.0f", P) is not None and
   re.search(r"#define\s+DAM_DRIVABLE_BOAT_DRIVER_Y_OFFSET\s+-12\.0f", P) is not None)

ck("R15/R16 keeps corrected entry reference with lower driver base",
   re.search(r"#define\s+DAM_DRIVABLE_BOAT_DRIVER_Y_OFFSET\s+-12\.0f", P) is not None and
   "g_LevelModifierDamDrivableBoatEnterStartPos.y =" in P)

ck("R6/R9 transition coverage is gated by nearby valid above-water STAN",
   "propLevelModifierDamDrivableBoatNearbyValidStan" in P and
   "groundY > DAM_DRIVABLE_BOAT_DECK_LAND_MIN_Y" in P and
   "candidateTouchesStan" in P and
   "candidateInShell" in P)

ck("R9/R13 clipping relaxation remains gated by valid above-water STAN",
   "candidateInClipHalo" in P and
   "candidateTouchesStan" in P and
   "propLevelModifierDamDrivableBoatNearbyValidStan" in P and
   "if (candidateTouchesStan)" in P and
   "propLevelModifierDamDrivableBoatTryDeckWallSlide" in P)

ck("R13 keeps tangential movement at unsupported water edges",
   P.count("propLevelModifierDamDrivableBoatTryDeckWallSlide(") >= 4)

ck("R13 preserves water walk-off blocking",
   "candidateTouchesStan" in P and
   "candidateInClipHalo" in P)

ck("R21 rollback removes R17-R20 STAN/Y ownership machinery",
   "R27R_R21_R2_REVERT_TO_PRE_STAN_Y_BASELINE_R16" in P and
   "DAM_DRIVABLE_BOAT_FOOT_OWNER_STAN" not in P and
   "g_LevelModifierDamDrivableBoatFootOwner" not in P and
   "candidateStickyStan" not in P and
   "candidateExactStan" not in P)


ck("R21R3 original boat Abs helper is defined exactly once",
   P.count("static f32 propLevelModifierDamDrivableBoatAbs(f32 v)") == 1 and
   "return v < 0.0f ? -v : v;" in P)

bad=[n for n,ok in checks if not ok]
print()
print(f"R27R DRIVABLE SPEEDBOAT AUDIT: {'PASS' if not bad else 'FAIL'} ({len(checks)-len(bad)}/{len(checks)})")
if bad:
    for n in bad: print(" - "+n)
    sys.exit(1)
