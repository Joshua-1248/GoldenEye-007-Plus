#include <ultra64.h>
#include <math.h>
#include <memp.h>
#include <music.h>
#include <snd.h>
#include "game/mp_weapon.h"
#include "game/front.h"
#include "game/bondview_r.h"
#include "bg.h"
#include "bondview_r.h"
#include "chr.h"
#include "chrai.h"
#include "chraction.h"
#include "propobj.h"
#include "inititemslots.h"
#include "initobjects.h"
#include "initpathtablesomething.h"
#include "limits.h"
#include "loadobjectmodel.h"
#include "lv.h"
#include "language.h"
#include "math_atan2f.h"
#include "matrixmath.h"
#include "mp_weapon.h"
#include "ob.h"
#include "objective.h"
#include "objective_status.h"
#include "objecthandler.h"
#include "options.h"
#include "player.h"
#include "prop.h"
#include "stan.h"
#include "model.h"
#ifdef GE_MODDED_CHEATS
#include "mirroredlevels.h"
#include "levelmodifiers.h"
#include "mpbots.h"
#endif
#include "token.h"
#ifdef GE_MAP_MAKER
#include "mapmaker.h"
#endif

/**
 * EU .bss 0x80068480
*/
ITEM_IDS lastmpweaponnum;

// redeclare with the element count so ARRAYCOUNT works in proplvreset2
extern ItemModelFileRecord PitemZ_entries[341];

// forward declarations

s32 load_proptype(PROPDEF_TYPE type);
s32 getMaxNumRooms(void);
s32 chrpropRayIntersectsRoomBbox(s32 room, coord3d *start, coord3d *dir);
void sub_GAME_7F001BD4(struct BoundPadRecord *pad, struct coord3d *arg1);
void domakedefaultobj(s32 arg0, ObjectRecord *arg1, s32 cmdindex);
void weaponAssignToHome(s32 arg0, WeaponObjRecord* weapon, s32 cmdindex);
void setupHat(s32 arg0, ObjectRecord* hat, s32 cmdindex);
void setupKey(s32 arg0, ObjectRecord* key, s32 cmdindex);
void setupCctv(s32 arg0, CCTVRecord *arg1, s32 cmdindex);
void setupAutogun(s32 stageID, AutogunRecord *autogun, s32 cmdindex);
void setupHangingMonitors(s32 arg0, ObjectRecord* rack, s32 cmdindex);
void setupSingleMonitor(s32 stageID, MonitorObjRecord *monitor, s32 cmdindex);
void setupMultiMonitor(s32 stageID, MultiMonitorObjRecord* monitor, s32 cmdindex);
void sub_GAME_7F00324C(struct BoundPadRecord *arg0, s32 *arg1, s32 *arg2, struct coord3d *arg3, struct coord3d *arg4);
void setupDoor(s32 arg0, struct DoorRecord *door, s32 arg2);


#ifdef GE_MODDED_CHEATS

/* R27S_R3_COMPACT_PATCH_TABLES_SAFE: exact R2 edits, compact run encoding. */
extern s32 gptr_stan;
static void lmPatchByteRun(u8 *b,u32 o,s32 n,u32 d,u8 a,u8 z,s32 on){u8 v=on?z:a;while(n--){b[o]=v;o+=d;}}
static void lmPatchHalfRun(u8 *b,u32 o,s32 n,u32 d,u16 a,u16 z,s32 on){u16 v=on?z:a;while(n--){b[o]=(u8)(v>>8);b[o+1]=(u8)v;o+=d;}}
s32 propLevelModifierApplyBetaSetupPatches(s32 levelid,s32 facilityDoors,s32 facilityTanks,s32 surfaceDoors,s32 bunkerDoors,s32 frigateKeepClear,s32 frigateRemoved)
{
    u8 *base=(u8 *)g_CurrentSetup.propDefs; s32 i;
    if(base==NULL)return FALSE;
    /* R27S_R4_NO_TABLE_RODATA: same exact writes, no static const lookup tables. */
    if(levelid==LEVELID_FACILITY){
        lmPatchByteRun(base,0x0149,9,0x100,0x9f,0x8f,facilityDoors);
        lmPatchByteRun(base,0x0a49,16,0x100,0x9b,0x9a,facilityDoors);
        for(i=0;i<4;i++){
            base[0x3873+i*0x120]=facilityTanks?(u8)(0x10+i*2):(u8)(0x11+i*2);
            base[0x38c8+i*0x120]=facilityTanks?0xa3:0xc3;
            base[0x3839+i*0x120]=facilityTanks?0xf0:0x53;
        }
        for(i=0;i<8;i++){
            base[0x889d+i*0x80]=facilityTanks?0x75:0x60;
            base[0x889f+i*0x80]=facilityTanks?(u8)(0x11+(i&3)*2):(u8)(0x27+i);
        }
        for(i=0;i<4;i++)base[0x8ad4+i*0x80]=facilityTanks?0xa3:0xc3;
        return TRUE;
    }
    if(levelid==LEVELID_SURFACE){lmPatchHalfRun(base,0x5148,8,0x100,0x00a6,0x00a7,surfaceDoors);return TRUE;}
    if(levelid==LEVELID_BUNKER1){lmPatchHalfRun(base,0x35a8,8,0x100,0x008a,0x0089,bunkerDoors);lmPatchByteRun(base,0x3da8,2,0x100,0x00,0x87,bunkerDoors);return TRUE;}
    if(levelid==LEVELID_FRIGATE){
        lmPatchHalfRun(base,0x4fd8,2,0x200,0x0098,0x0099,frigateKeepClear);
        if(g_CurrentSetup.pads!=NULL&&gptr_stan!=0){
            g_CurrentSetup.pads[70].stan=(void *)((u8 *)gptr_stan+(frigateRemoved?0xdd64:0xbc94));
            g_CurrentSetup.pads[71].stan=(void *)((u8 *)gptr_stan+(frigateRemoved?0xda04:0xbb54));
            g_CurrentSetup.pads[74].stan=(void *)((u8 *)gptr_stan+(frigateRemoved?0xd774:0xb4ac));
            g_CurrentSetup.pads[85].stan=(void *)((u8 *)gptr_stan+(frigateRemoved?0xd3b4:0x90a4));
            g_CurrentSetup.pads[86].stan=(void *)((u8 *)gptr_stan+(frigateRemoved?0xd594:0xadec));
            g_CurrentSetup.pads[89].stan=(void *)((u8 *)gptr_stan+(frigateRemoved?0xb6ac:0xa2dc));
        }
        return TRUE;
    }
    return TRUE;
}


/* R27Q_DAM_DOCK_RESTORATION
 *
 * The old GameShark speedboat restoration rewrote an already-live Dam prop:
 * model 0x003e / pad 359 -> PROP_SPEEDBOAT (0x0122) / pad 111 (0x006f),
 * then patched pad 111's NULL STAN pointer and manually corrected runtime Y.
 *
 * Do not reproduce those absolute-address writes.  Capture the pristine setup
 * donor before proplvreset2 mutates it, repair the preserved unused pad through
 * an existing symbolic Dam pad/STAN, then instantiate an additional object
 * through the normal prop path.  The original dock prop is never touched.
 *
 * The historical dock-door restoration similarly moved two existing doors
 * from pads 65/68 to 66/69.  Here we clone the two pristine DoorRecords and
 * instantiate two additional doors, preserving the retail pair as well.
 */
static ObjectRecord g_LevelModifierDamBoatTemplate;
static ObjectRecord g_LevelModifierDamBoatRuntime;
/* R27R_DRIVABLE_SPEEDBOAT */
static ObjectRecord g_LevelModifierDamDrivableBoatRuntime;
static s32 g_LevelModifierDamDrivableBoatDriver = -1;
static f32 g_LevelModifierDamDrivableBoatHeading = M_PI_F;
static f32 g_LevelModifierDamDrivableBoatSpeed = 0.0f;
static f32 g_LevelModifierDamDrivableBoatThrottle = 0.0f;
static f32 g_LevelModifierDamDrivableBoatSteering = 0.0f;
static ALSoundState *g_LevelModifierDamDrivableBoatSfx[2] = {NULL, NULL};
static f32 g_LevelModifierDamDrivableBoatEnterBlend = 1.0f;
static f32 g_LevelModifierDamDrivableBoatEnterStartYaw = 0.0f;
static coord3d g_LevelModifierDamDrivableBoatEnterStartPos;
static f32 g_LevelModifierDamDrivableBoatSteeringApplied = 0.0f;
static s32 g_LevelModifierDamDrivableBoatUseWaterEnvelope = FALSE;

#define DAM_DRIVABLE_BOAT_START_X        448.0f
#define DAM_DRIVABLE_BOAT_START_Y       -768.0f
#define DAM_DRIVABLE_BOAT_START_Z      -7200.0f
#define DAM_DRIVABLE_BOAT_START_HEADING  M_PI_F
#define DAM_DRIVABLE_BOAT_MAX_SPEED       25.0f
#define DAM_DRIVABLE_BOAT_ACCEL            0.40f
#define DAM_DRIVABLE_BOAT_DECEL            0.78f
#define DAM_DRIVABLE_BOAT_TURN_DEG          1.65f
#define DAM_DRIVABLE_BOAT_DRIVER_Y_OFFSET  -12.0f
#define DAM_DRIVABLE_BOAT_ENTRY_RADIUS    360.0f
#define DAM_DRIVABLE_BOAT_DECK_HALF_X      105.0f
#define DAM_DRIVABLE_BOAT_DECK_HALF_Z     210.0f

#define DAM_DRIVABLE_BOAT_COLLISION_Y_OFFSET 48.0f
#define DAM_DRIVABLE_BOAT_ENTRY_Y_RANGE      220.0f
#define DAM_DRIVABLE_BOAT_MODEL_YAW_OFFSET      M_PI_F
#define DAM_DRIVABLE_BOAT_DECK_BOARD_MARGIN       95.0f
#define DAM_DRIVABLE_BOAT_LAND_PROBE_RADIUS       55.0f
#define DAM_DRIVABLE_BOAT_LAND_PROBE_FORWARD      95.0f
#define DAM_DRIVABLE_BOAT_LAND_STEP_UP           450.0f
#define DAM_DRIVABLE_BOAT_LAND_STEP_DOWN         350.0f
#define DAM_DRIVABLE_BOAT_ENTER_TICKS             45.0f
/* R27R_R5_SMOOTH_BOAT_WALK_VOLUME */
#define DAM_DRIVABLE_BOAT_DECK_CENTER_Y_OFFSET  8.0f
#define DAM_DRIVABLE_BOAT_DECK_END_Y_OFFSET     55.0f
#define DAM_DRIVABLE_BOAT_DECK_RAISE_START_Z   115.0f
#define DAM_DRIVABLE_BOAT_DECK_RAISE_FULL_Z    178.0f
#define DAM_DRIVABLE_BOAT_DECK_TAPER_START_Z   135.0f
#define DAM_DRIVABLE_BOAT_DECK_TIP_HALF_X       72.0f
#define DAM_DRIVABLE_BOAT_DECK_SEAM_MARGIN      84.0f
/* R27R_R11_RECTANGULAR_HULL_LONGER_COVERAGE */
#define DAM_DRIVABLE_BOAT_HULL_HALF_X            88.0f
#define DAM_DRIVABLE_BOAT_HULL_HALF_Z            220.0f
/* R27R_R11_R1_RESTORE_BOAT_MOVEMENT */
/* R27R_R7_RECTANGULAR_STAN_COVERAGE_Y856 */
#define DAM_DRIVABLE_BOAT_TRANSITION_HALF_X     (DAM_DRIVABLE_BOAT_DECK_HALF_X + DAM_DRIVABLE_BOAT_DECK_SEAM_MARGIN)
#define DAM_DRIVABLE_BOAT_TRANSITION_HALF_Z     (DAM_DRIVABLE_BOAT_DECK_HALF_Z + DAM_DRIVABLE_BOAT_DECK_SEAM_MARGIN)
/* R27R_R9_CLIP_BRIDGE_SMOOTH_ENTRY_STEERING */
#define DAM_DRIVABLE_BOAT_CLIP_RELAX_EXTRA      18.0f
#define DAM_DRIVABLE_BOAT_STEER_RESPONSE          0.060f
/* R27R_R16_STATIONARY_TURN_SLOWER_ACCEL_LOWER_DRIVER */
#define DAM_DRIVABLE_BOAT_STATIONARY_TURN_SCALE  0.10f
/* R27R_R10_LOCK_BOAT_TO_DAM_WATER_ROOM23 */
/* R27R_R12_DISABLE_ROOM23_MOVEMENT_GATE */
#define DAM_DRIVABLE_BOAT_WATER_ROOM              0x23
#define DAM_DRIVABLE_BOAT_ROOM_PROBE_Y_OFFSET   4096.0f



/* R27R_R6_STAN_GATED_TRANSITION_VOLUME */
#define DAM_DRIVABLE_BOAT_STAN_TOUCH_RADIUS      96.0f
#define DAM_DRIVABLE_BOAT_STAN_SAMPLE_RADIUS     14.0f
#define DAM_DRIVABLE_BOAT_STAN_SAMPLE_STEP       24.0f
#define DAM_DRIVABLE_BOAT_STAN_SAMPLE_RINGS       4

#define DAM_DRIVABLE_BOAT_DECK_LAND_RADIUS      16.0f
#define DAM_DRIVABLE_BOAT_DECK_LAND_MIN_Y       -676.0f

static DoorRecord g_LevelModifierDamDoorTemplate[2];
static DoorRecord g_LevelModifierDamDoorRuntime[2];
static s32 g_LevelModifierDamBoatCommandIndex = -1;
static s32 g_LevelModifierDamDoorCommandIndex[2] = {-1, -1};
static s32 g_LevelModifierDamRestorePrepared = FALSE;

static f32 propLevelModifierDamDrivableBoatAbs(f32 v)
{
    return v < 0.0f ? -v : v;
}

/* R27R_R21_R3_RESTORE_ABS_HELPER_LINK_FIX */
void propLevelModifierDamDrivableBoatStopAudio(void)
{
    s32 i;

    for (i = 0; i < 2; i++)
    {
        if (g_LevelModifierDamDrivableBoatSfx[i] != NULL)
        {
            sndCreatePostEvent(g_LevelModifierDamDrivableBoatSfx[i], 8, 0);

            if (sndGetPlayingState(g_LevelModifierDamDrivableBoatSfx[i]) != AL_STOPPED)
            {
                sndDeactivate(g_LevelModifierDamDrivableBoatSfx[i]);
            }

            g_LevelModifierDamDrivableBoatSfx[i] = NULL;
        }
    }
}

static void propLevelModifierDamDrivableBoatUpdateAudio(void)
{
    f32 utilization;
    s32 movingVolume;

    if (g_LevelModifierDamDrivableBoatDriver < 0)
    {
        propLevelModifierDamDrivableBoatStopAudio();
        return;
    }

    /* Temporary R27R audio: mirror the Tank sound arrangement until the
     * existing trainidl/speedbt bank entries are identified. */
    if (g_LevelModifierDamDrivableBoatSfx[0] == NULL
        || sndGetPlayingState(g_LevelModifierDamDrivableBoatSfx[0]) == AL_STOPPED)
    {
        g_LevelModifierDamDrivableBoatSfx[0] = NULL;

        if (lvlGetControlsLockedFlag() == 0)
        {
            sndPlaySfx((struct ALBankAlt_s *)g_musicSfxBufferPtr,
                TRUCK_RUN_SFX, &g_LevelModifierDamDrivableBoatSfx[0]);
        }
    }

    utilization = propLevelModifierDamDrivableBoatAbs(
        g_LevelModifierDamDrivableBoatSpeed) / DAM_DRIVABLE_BOAT_MAX_SPEED;

    if (utilization > 1.0f)
        utilization = 1.0f;

    if (g_LevelModifierDamDrivableBoatSfx[0] != NULL)
    {
        s32 idleVolume = (s32)(25000.0f + utilization * 7767.0f);
        if (idleVolume > 0x7fff)
            idleVolume = 0x7fff;
        sndCreatePostEvent(g_LevelModifierDamDrivableBoatSfx[0], 8, idleVolume);
    }

    if (utilization > 0.015f)
    {
        if (g_LevelModifierDamDrivableBoatSfx[1] == NULL
            || sndGetPlayingState(g_LevelModifierDamDrivableBoatSfx[1]) == AL_STOPPED)
        {
            g_LevelModifierDamDrivableBoatSfx[1] = NULL;

            if (lvlGetControlsLockedFlag() == 0)
            {
                sndPlaySfx((struct ALBankAlt_s *)g_musicSfxBufferPtr,
                    TANK_SFX, &g_LevelModifierDamDrivableBoatSfx[1]);
            }
        }

        movingVolume = (s32)(12000.0f + utilization * 20767.0f);
        if (movingVolume > 0x7fff)
            movingVolume = 0x7fff;

        if (g_LevelModifierDamDrivableBoatSfx[1] != NULL)
            sndCreatePostEvent(g_LevelModifierDamDrivableBoatSfx[1], 8, movingVolume);
    }
    else if (g_LevelModifierDamDrivableBoatSfx[1] != NULL)
    {
        if (sndGetPlayingState(g_LevelModifierDamDrivableBoatSfx[1]) != AL_STOPPED)
            sndDeactivate(g_LevelModifierDamDrivableBoatSfx[1]);
        g_LevelModifierDamDrivableBoatSfx[1] = NULL;
    }
}

static s32 propLevelModifierDamDrivableBoatBgSegmentBlocked(
    const coord3d *from, const coord3d *to)
{
    coord3d dir;
    coord3d scaledstart;
    coord3d worldhit;
    HitThing hit;
    f32 bgscale;
    f32 invscale;
    f32 len2;
    f32 frac;
    s32 room;

    dir.x = to->x - from->x;
    dir.y = to->y - from->y;
    dir.z = to->z - from->z;

    len2 = dir.x * dir.x + dir.y * dir.y + dir.z * dir.z;

    if (len2 <= 0.000001f)
        return FALSE;

    bgscale = get_room_data_float1() * bgGetLevelVisibilityScale();
    invscale = get_room_data_float2();

    scaledstart.x = from->x * bgscale;
    scaledstart.y = from->y * bgscale;
    scaledstart.z = from->z * bgscale;

    for (room = 1; room < getMaxNumRooms(); room++)
    {
        if (chrpropRayIntersectsRoomBbox(room, &scaledstart, &dir)
            && bgTestBulletHitBackground((coord3d *)from, (coord3d *)to,
                room, &hit))
        {
            worldhit.x = hit.hitpos.x * invscale;
            worldhit.y = hit.hitpos.y * invscale;
            worldhit.z = hit.hitpos.z * invscale;

            frac = ((worldhit.x - from->x) * dir.x
                  + (worldhit.y - from->y) * dir.y
                  + (worldhit.z - from->z) * dir.z) / len2;

            if (frac >= 0.035f && frac <= 1.0005f)
                return TRUE;
        }
    }

    return FALSE;
}

static s32 propLevelModifierDamDrivableBoatWaterSurfacePresent(
    const coord3d *center)
{
    coord3d from;
    coord3d to;

    from = *center;
    to = *center;

    from.y = DAM_DRIVABLE_BOAT_START_Y + 36.0f;
    to.y = DAM_DRIVABLE_BOAT_START_Y - 36.0f;

    return propLevelModifierDamDrivableBoatBgSegmentBlocked(&from, &to);
}

static s32 propLevelModifierDamDrivableBoatLandUnderHull(
    const coord3d *center)
{
    coord3d probe;
    StandTile *tile;
    f32 groundY;

    probe = *center;
    probe.y = DAM_DRIVABLE_BOAT_START_Y + 500.0f;

    tile = stanFindGroundAtCyl(
        &probe, DAM_DRIVABLE_BOAT_LAND_PROBE_RADIUS, NULL, &groundY);

    if (tile == NULL)
        return FALSE;

    return groundY > DAM_DRIVABLE_BOAT_START_Y + 30.0f;
}

static void propLevelModifierDamDrivableBoatHullPoint(
    coord3d *out, const coord3d *center, f32 heading,
    f32 localx, f32 localz, f32 yoffset)
{
    Mtxf rot;
    coord3d local;

    local.x = localx;
    local.y = 0.0f;
    local.z = localz;

    matrix_4x4_set_rotation_around_y(
        M_TAU_F - heading + DAM_DRIVABLE_BOAT_MODEL_YAW_OFFSET, &rot);
    mtx4RotateVecInPlace(&rot, &local);

    out->x = center->x + local.x;
    out->y = DAM_DRIVABLE_BOAT_START_Y + yoffset;
    out->z = center->z + local.z;
}

static s32 propLevelModifierDamDrivableBoatInWaterRoom(
    const coord3d *center)
{
    coord3d probe;
    StandTile *tile;
    f32 groundY;
    u8 rooms[2];

    if (center == NULL)
        return FALSE;

    rooms[0] = DAM_DRIVABLE_BOAT_WATER_ROOM;
    rooms[1] = 0xff;

    probe = *center;
    probe.y = DAM_DRIVABLE_BOAT_START_Y + 512.0f;

    /*
     * Query the authored lake room directly. An unfiltered high-Y lookup can
     * pick the dock or mountain STAN above the water and trap the boat at its
     * spawn even though Room 0x23 water is valid underneath.
     */
    tile = stanFindGroundAtCyl(&probe, 8.0f, rooms, &groundY);

    if (tile == NULL)
        return FALSE;

    return tile->room == DAM_DRIVABLE_BOAT_WATER_ROOM;
}

static s32 propLevelModifierDamDrivableBoatHullBlocked(
    const coord3d *oldpos, f32 oldheading,
    const coord3d *newpos, f32 newheading)
{
    ObjectRecord *obj = &g_LevelModifierDamDrivableBoatRuntime;
    f32 xmin;
    f32 xmax;
    f32 zmin;
    f32 zmax;
    f32 midx;
    f32 midz;
    f32 lx[9];
    f32 lz[9];
    f32 yoffsets[3];
    coord3d from;
    coord3d to;
    s32 i;
    s32 h;

    if (obj->prop == NULL || obj->model == NULL)
        return TRUE;

    if (g_LevelModifierDamDrivableBoatUseWaterEnvelope
        && !propLevelModifierDamDrivableBoatWaterSurfacePresent(newpos))
    {
        return TRUE;
    }

    if (propLevelModifierDamDrivableBoatLandUnderHull(newpos))
        return TRUE;

    /*
     * R11: use a fixed boat-local rectangle instead of deriving the hard
     * collision footprint from the speedboat model bbox. This makes the
     * vehicle's driving collision predictable and symmetric.
     */
    xmin = -DAM_DRIVABLE_BOAT_HULL_HALF_X;
    xmax =  DAM_DRIVABLE_BOAT_HULL_HALF_X;
    zmin = -DAM_DRIVABLE_BOAT_HULL_HALF_Z;
    zmax =  DAM_DRIVABLE_BOAT_HULL_HALF_Z;
    midx = 0.0f;
    midz = 0.0f;

    /* Center, four corners, and four edge midpoints. */
    lx[0] = midx; lz[0] = midz;
    lx[1] = xmin; lz[1] = zmin;
    lx[2] = xmax; lz[2] = zmin;
    lx[3] = xmax; lz[3] = zmax;
    lx[4] = xmin; lz[4] = zmax;
    lx[5] = midx; lz[5] = zmin;
    lx[6] = xmax; lz[6] = midz;
    lx[7] = midx; lz[7] = zmax;
    lx[8] = xmin; lz[8] = midz;

    yoffsets[0] = 12.0f;
    yoffsets[1] = 64.0f;
    yoffsets[2] = 132.0f;

    for (h = 0; h < 3; h++)
    {
        for (i = 0; i < 9; i++)
        {
            propLevelModifierDamDrivableBoatHullPoint(
                &from, oldpos, oldheading, lx[i], lz[i], yoffsets[h]);
            propLevelModifierDamDrivableBoatHullPoint(
                &to, newpos, newheading, lx[i], lz[i], yoffsets[h]);

            if (propLevelModifierDamDrivableBoatBgSegmentBlocked(
                    &from, &to))
            {
                return TRUE;
            }
        }
    }

    return FALSE;
}

static void propLevelModifierDamDrivableBoatApplyTransform(void)
{
    Mtxf mtx;
    ObjectRecord *obj = &g_LevelModifierDamDrivableBoatRuntime;

    if (obj->prop == NULL || obj->model == NULL)
        return;

    matrix_4x4_set_rotation_around_y(
        M_TAU_F - g_LevelModifierDamDrivableBoatHeading
            + DAM_DRIVABLE_BOAT_MODEL_YAW_OFFSET,
        &mtx);
    matrix_scalar_multiply(obj->model->scale, &mtx);
    matrix_4x4_copy(&mtx, &obj->mtx);

    obj->runtime_pos = obj->prop->pos;
    setupUpdateObjectRoomPosition(obj);
    chrobjCollisionRelated(obj);
}

static void propLevelModifierDamDrivableBoatPlaceDriver(void)
{
    coord3d bbmin;
    coord3d bbmax;
    coord3d target;
    coord3d placed;
    coord3d *p;
    f32 remain;
    f32 eyeOffset;

    if (!propLevelModifierDamDrivableBoatCurrentPlayerDriving()
        || g_CurrentPlayer == NULL
        || g_CurrentPlayer->prop == NULL
        || g_LevelModifierDamDrivableBoatRuntime.prop == NULL)
    {
        return;
    }

    p = &g_LevelModifierDamDrivableBoatRuntime.prop->pos;

    /*
     * R27R_R15_ENTRY_VERTICAL_REFERENCE_FIX
     *
     * collision_position.y is Bond's first-person eye/collision height.
     * field_70 is his authoritative body/floor/base Y. R9 accidentally
     * interpolated from eye-space Y to the boat's base/driver Y, then stored
     * that mixed value back into field_70. The normal view code subsequently
     * added Bond's eye height again, producing the "Bond rises up" feeling
     * during embark.
     *
     * Preserve the live eye-height offset, interpolate only the base Y, then
     * rebuild collision_position.y from base + eye offset.
     */
    eyeOffset = g_CurrentPlayer->field_488.collision_position.y
        - g_CurrentPlayer->field_70;

    target.x = p->x;
    target.y = p->y + DAM_DRIVABLE_BOAT_DRIVER_Y_OFFSET;
    target.z = p->z;
    placed = target;

    if (g_LevelModifierDamDrivableBoatEnterBlend < 1.0f)
    {
        remain = (cosf(g_LevelModifierDamDrivableBoatEnterBlend
            * M_PI_F) + 1.0f) * 0.5f;

        placed.x =
            remain * g_LevelModifierDamDrivableBoatEnterStartPos.x
            + (1.0f - remain) * target.x;
        placed.y =
            remain * g_LevelModifierDamDrivableBoatEnterStartPos.y
            + (1.0f - remain) * target.y;
        placed.z =
            remain * g_LevelModifierDamDrivableBoatEnterStartPos.z
            + (1.0f - remain) * target.z;
    }

    g_CurrentPlayer->field_70 = placed.y;
    g_CurrentPlayer->stanHeight = placed.y;
    g_CurrentPlayer->field_7C = 0.0f;

    g_CurrentPlayer->field_488.collision_position.x = placed.x;
    g_CurrentPlayer->field_488.collision_position.y =
        placed.y + eyeOffset;
    g_CurrentPlayer->field_488.collision_position.z = placed.z;

    g_CurrentPlayer->prop->pos =
        g_CurrentPlayer->field_488.collision_position;

    bbmin.x = placed.x - 40.0f;
    bbmin.y = placed.y - 40.0f;
    bbmin.z = placed.z - 40.0f;
    bbmax.x = placed.x + 40.0f;
    bbmax.y = placed.y + 100.0f;
    bbmax.z = placed.z + 40.0f;

    chrpropDeregisterRooms(g_CurrentPlayer->prop);
    chrpropUpdateRoomList(g_CurrentPlayer->prop, &bbmin, &bbmax, 50.0f);
    chrpropRegisterRooms(g_CurrentPlayer->prop);
}

static void propLevelModifierFreeDamDrivableBoat(void)
{
    propLevelModifierDamDrivableBoatStopAudio();
    g_LevelModifierDamDrivableBoatDriver = -1;
    g_LevelModifierDamDrivableBoatSpeed = 0.0f;
    g_LevelModifierDamDrivableBoatThrottle = 0.0f;
    g_LevelModifierDamDrivableBoatSteering = 0.0f;
    g_LevelModifierDamDrivableBoatEnterBlend = 1.0f;
    g_LevelModifierDamDrivableBoatEnterStartYaw = 0.0f;
    g_LevelModifierDamDrivableBoatUseWaterEnvelope = FALSE;

    if (g_LevelModifierDamDrivableBoatRuntime.prop != NULL)
        objFreePermanently(&g_LevelModifierDamDrivableBoatRuntime, TRUE);

    g_LevelModifierDamDrivableBoatRuntime.prop = NULL;
}

static void propLevelModifierFreeDamDoor(s32 index)
{
    if (index >= 0 && index < 2 && g_LevelModifierDamDoorRuntime[index].prop != NULL)
        objFreePermanently((ObjectRecord *)&g_LevelModifierDamDoorRuntime[index], TRUE);
}

static void propLevelModifierFreeDamBoat(void)
{
    if (g_LevelModifierDamBoatRuntime.prop != NULL)
        objFreePermanently(&g_LevelModifierDamBoatRuntime, TRUE);
}

s32 propLevelModifierPrepareDamRestorations(enum LEVELID stageId)
{
    PropDefHeaderRecord *phead;
    s32 pdefIndex;
    s32 foundBoat;
    s32 foundDoor0;
    s32 foundDoor1;
    ObjectRecord *obj;
    DoorRecord *door;

    g_LevelModifierDamRestorePrepared = FALSE;
    g_LevelModifierDamBoatRuntime.prop = NULL;
    g_LevelModifierDamDrivableBoatRuntime.prop = NULL;
    g_LevelModifierDamDrivableBoatDriver = -1;
    g_LevelModifierDamDrivableBoatHeading = DAM_DRIVABLE_BOAT_START_HEADING;
    g_LevelModifierDamDrivableBoatSpeed = 0.0f;
    g_LevelModifierDamDrivableBoatThrottle = 0.0f;
    g_LevelModifierDamDrivableBoatSteering = 0.0f;
    propLevelModifierDamDrivableBoatStopAudio();
    g_LevelModifierDamDoorRuntime[0].prop = NULL;
    g_LevelModifierDamDoorRuntime[1].prop = NULL;
    g_LevelModifierDamBoatCommandIndex = -1;
    g_LevelModifierDamDoorCommandIndex[0] = -1;
    g_LevelModifierDamDoorCommandIndex[1] = -1;

    if (stageId != LEVELID_DAM
        || g_CurrentSetup.propDefs == NULL
        || g_CurrentSetup.pads == NULL)
    {
        return FALSE;
    }

    foundBoat = FALSE;
    foundDoor0 = FALSE;
    foundDoor1 = FALSE;
    phead = g_CurrentSetup.propDefs;
    pdefIndex = 0;

    while (phead->type != PROPDEF_END)
    {
        if (phead->type == PROPDEF_PROP)
        {
            obj = (ObjectRecord *)phead;

            if (!foundBoat
                && obj->obj == PROP_OIL_DRUM7
                && obj->pad == 359)
            {
                g_LevelModifierDamBoatTemplate = *obj;
                g_LevelModifierDamBoatCommandIndex = pdefIndex;
                foundBoat = TRUE;
            }
        }
        else if (phead->type == PROPDEF_DOOR)
        {
            door = (DoorRecord *)phead;

            if (!foundDoor0
                && door->obj == PROP_GAS_PLANT_MET1_DO1
                && door->pad == 65)
            {
                g_LevelModifierDamDoorTemplate[0] = *door;
                g_LevelModifierDamDoorCommandIndex[0] = pdefIndex;
                foundDoor0 = TRUE;
            }
            else if (!foundDoor1
                && door->obj == PROP_GAS_PLANT_MET1_DO1
                && door->pad == 68)
            {
                g_LevelModifierDamDoorTemplate[1] = *door;
                g_LevelModifierDamDoorCommandIndex[1] = pdefIndex;
                foundDoor1 = TRUE;
            }
        }

        phead = (PropDefHeaderRecord *)(((u32 *)phead) + sizepropdef(phead));
        pdefIndex++;
    }

    if (!foundBoat || !foundDoor0 || !foundDoor1)
        return FALSE;

    /*
     * Dam pad 111 is the intact authored boat placement:
     *   pos  = (424, -130, -1626)
     *   look ~= (0, 0, -1)
     * Its plink string is intentionally empty in retail, leaving stan == NULL.
     * The old cheat wrote the live STAN pointer used by the dock tile. Pads
     * 64/65/66/236 all resolve to that same tile; copy pad 64's resolved STAN
     * symbolically instead of hard-coding the old 0x801CD0F8 address.
     */
    if (g_CurrentSetup.pads[64].stan == NULL)
        return FALSE;

    g_CurrentSetup.pads[111].stan = g_CurrentSetup.pads[64].stan;

    /* R27R_R14_R1_STATIC_PROP_RUNTIME_Y768
     *
     * Keep pad 111's authored position/orientation intact. The restored
     * static speedboat's exact Y is applied to its LIVE ObjectRecord after
     * domakedefaultobj(), so X/Z/orientation still come from the original
     * authored placement while runtime Y matches the drivable boat.
     */

    /* Preserve every other authored field from the donor records. */
    g_LevelModifierDamBoatTemplate.obj = PROP_SPEEDBOAT;
    g_LevelModifierDamBoatTemplate.pad = 111;

    g_LevelModifierDamDoorTemplate[0].pad = 66;
    g_LevelModifierDamDoorTemplate[1].pad = 69;

    /* R27Q_R6_DAM_RESTORED_DOOR_PORTAL_VISIBILITY
     *
     * The retail donor doors at pads 65/68 use CULL_BEHIND_DOOR so their
     * portal is disabled while fully closed.  At the restored lower dock
     * placements (66/69), that culls the exterior-side doorway/wall geometry
     * until the door begins opening.  Keep the restored clones registered
     * across both rooms, but leave their portals visible at all times.
     *
     * Original retail doors are not modified.
     */
    g_LevelModifierDamDoorTemplate[0].flags &= ~PROPFLAG_CULL_BEHIND_DOOR;
    g_LevelModifierDamDoorTemplate[0].flags |= PROPFLAG_NO_PORTAL_CLOSE;
    g_LevelModifierDamDoorTemplate[1].flags &= ~PROPFLAG_CULL_BEHIND_DOOR;
    g_LevelModifierDamDoorTemplate[1].flags |= PROPFLAG_NO_PORTAL_CLOSE;

    g_LevelModifierDamRestorePrepared = TRUE;
    return TRUE;
}

s32 propLevelModifierSetDamDoors(s32 enabled)
{
    if (!g_LevelModifierDamRestorePrepared)
        return FALSE;

    if (!enabled)
    {
        propLevelModifierFreeDamDoor(0);
        propLevelModifierFreeDamDoor(1);
        return TRUE;
    }

    if (g_LevelModifierDamDoorRuntime[0].prop != NULL
        && g_LevelModifierDamDoorRuntime[1].prop != NULL)
    {
        return TRUE;
    }

    propLevelModifierFreeDamDoor(0);
    propLevelModifierFreeDamDoor(1);

    g_LevelModifierDamDoorRuntime[0] = g_LevelModifierDamDoorTemplate[0];
    setupDoor(LEVELID_DAM,
        &g_LevelModifierDamDoorRuntime[0],
        g_LevelModifierDamDoorCommandIndex[0]);

    if (g_LevelModifierDamDoorRuntime[0].prop == NULL)
        return FALSE;

    g_LevelModifierDamDoorRuntime[1] = g_LevelModifierDamDoorTemplate[1];
    setupDoor(LEVELID_DAM,
        &g_LevelModifierDamDoorRuntime[1],
        g_LevelModifierDamDoorCommandIndex[1]);

    if (g_LevelModifierDamDoorRuntime[1].prop == NULL)
    {
        propLevelModifierFreeDamDoor(0);
        return FALSE;
    }

    return TRUE;
}

s32 propLevelModifierSetDamSpeedboat(s32 enabled)
{
    if (!g_LevelModifierDamRestorePrepared)
        return FALSE;

    if (!enabled)
    {
        propLevelModifierFreeDamBoat();
        return TRUE;
    }

    if (g_LevelModifierDamBoatRuntime.prop != NULL)
        return TRUE;

    g_LevelModifierDamBoatRuntime = g_LevelModifierDamBoatTemplate;

    /*
     * Let the normal setup path preserve the authored pad-111 X/Z/orientation.
     * Then override ONLY the live object's Y. This is the same coordinate
     * domain used by the drivable speedboat's prop->pos, so -768 is exact.
     */
    domakedefaultobj(LEVELID_DAM,
        &g_LevelModifierDamBoatRuntime,
        g_LevelModifierDamBoatCommandIndex);

    if (g_LevelModifierDamBoatRuntime.prop == NULL)
        return FALSE;

    g_LevelModifierDamBoatRuntime.prop->pos.y = -768.0f;

    /*
     * Keep ObjectRecord runtime_pos and room/collision bookkeeping in sync
     * with the live PropRecord. The previous R14 only changed pad data and
     * therefore did not force the rendered/runtime boat to this Y.
     */
    g_LevelModifierDamBoatRuntime.runtime_pos =
        g_LevelModifierDamBoatRuntime.prop->pos;
    setupUpdateObjectRoomPosition(&g_LevelModifierDamBoatRuntime);
    chrobjCollisionRelated(&g_LevelModifierDamBoatRuntime);

    return TRUE;
}

s32 propLevelModifierActivateDamDrivableSpeedboat(void)
{
    if (!g_LevelModifierDamRestorePrepared)
        return FALSE;

    if (g_LevelModifierDamDrivableBoatRuntime.prop != NULL)
        return TRUE;

    g_LevelModifierDamDrivableBoatRuntime = g_LevelModifierDamBoatTemplate;

    g_LevelModifierDamDrivableBoatRuntime.flags |= PROPFLAG_INVINCIBLE;
    g_LevelModifierDamDrivableBoatRuntime.flags2 |=
        PROPFLAG2_00004000 | PROPFLAG2_00200000;

    domakedefaultobj(LEVELID_DAM,
        &g_LevelModifierDamDrivableBoatRuntime,
        g_LevelModifierDamBoatCommandIndex);

    if (g_LevelModifierDamDrivableBoatRuntime.prop == NULL)
        return FALSE;

    g_LevelModifierDamDrivableBoatRuntime.flags |= PROPFLAG_INVINCIBLE;
    g_LevelModifierDamDrivableBoatRuntime.flags2 |=
        PROPFLAG2_00004000 | PROPFLAG2_00200000;

    g_LevelModifierDamDrivableBoatRuntime.state |= PROPSTATE_20;

    g_LevelModifierDamDrivableBoatHeading = DAM_DRIVABLE_BOAT_START_HEADING;
    g_LevelModifierDamDrivableBoatSpeed = 0.0f;
    g_LevelModifierDamDrivableBoatThrottle = 0.0f;
    g_LevelModifierDamDrivableBoatSteering = 0.0f;
    g_LevelModifierDamDrivableBoatDriver = -1;
    g_LevelModifierDamDrivableBoatEnterBlend = 1.0f;
    g_LevelModifierDamDrivableBoatEnterStartYaw = 0.0f;

    g_LevelModifierDamDrivableBoatRuntime.prop->pos.x = DAM_DRIVABLE_BOAT_START_X;
    g_LevelModifierDamDrivableBoatRuntime.prop->pos.y = DAM_DRIVABLE_BOAT_START_Y;
    g_LevelModifierDamDrivableBoatRuntime.prop->pos.z = DAM_DRIVABLE_BOAT_START_Z;

    propLevelModifierDamDrivableBoatApplyTransform();

    g_LevelModifierDamDrivableBoatUseWaterEnvelope =
        propLevelModifierDamDrivableBoatWaterSurfacePresent(
            &g_LevelModifierDamDrivableBoatRuntime.prop->pos);

    return TRUE;
}

s32 propLevelModifierDamDrivableBoatCurrentPlayerDriving(void)
{
    return g_LevelModifierDamDrivableBoatRuntime.prop != NULL
        && g_LevelModifierDamDrivableBoatDriver == get_cur_playernum();
}

s32 propLevelModifierDamDrivableBoatEntering(void)
{
    return propLevelModifierDamDrivableBoatCurrentPlayerDriving()
        && g_LevelModifierDamDrivableBoatEnterBlend < 1.0f;
}


s32 propLevelModifierDamDrivableBoatCanCurrentPlayerEnter(void)
{
    f32 dx;
    f32 dy;
    f32 dz;

    if (g_LevelModifierDamDrivableBoatRuntime.prop == NULL
        || g_LevelModifierDamDrivableBoatDriver >= 0
        || g_CurrentPlayer == NULL)
    {
        return FALSE;
    }

    dx = g_CurrentPlayer->field_488.collision_position.x
        - g_LevelModifierDamDrivableBoatRuntime.prop->pos.x;
    dy = g_CurrentPlayer->field_488.collision_position.y
        - (g_LevelModifierDamDrivableBoatRuntime.prop->pos.y
            + DAM_DRIVABLE_BOAT_DRIVER_Y_OFFSET);
    dz = g_CurrentPlayer->field_488.collision_position.z
        - g_LevelModifierDamDrivableBoatRuntime.prop->pos.z;

    if (propLevelModifierDamDrivableBoatAbs(dy)
        > DAM_DRIVABLE_BOAT_ENTRY_Y_RANGE)
    {
        return FALSE;
    }

    return (dx * dx + dz * dz)
        <= DAM_DRIVABLE_BOAT_ENTRY_RADIUS * DAM_DRIVABLE_BOAT_ENTRY_RADIUS;
}

void propLevelModifierDamDrivableBoatEnterCurrentPlayer(void)
{
    if (!propLevelModifierDamDrivableBoatCanCurrentPlayerEnter())
        return;

    propLevelModifierDamDrivableBoatStopAudio();

    g_LevelModifierDamDrivableBoatDriver = get_cur_playernum();
    g_LevelModifierDamDrivableBoatSpeed = 0.0f;
    g_LevelModifierDamDrivableBoatThrottle = 0.0f;
    g_LevelModifierDamDrivableBoatSteering = 0.0f;
    g_LevelModifierDamDrivableBoatSteeringApplied = 0.0f;
    g_LevelModifierDamDrivableBoatEnterBlend = 0.0f;
    g_LevelModifierDamDrivableBoatEnterStartYaw = g_CurrentPlayer->vv_theta;
    g_LevelModifierDamDrivableBoatEnterStartPos =
        g_CurrentPlayer->field_488.collision_position;

    /*
     * X/Z begin at Bond's collision position, but Y must begin at his
     * body/floor/base Y. Do not mix first-person eye Y with driver-floor Y.
     */
    g_LevelModifierDamDrivableBoatEnterStartPos.y =
        g_CurrentPlayer->field_70;

    g_CurrentPlayer->speedsideways = 0.0f;
    g_CurrentPlayer->speedforwards = 0.0f;
    g_CurrentPlayer->speedtheta = 0.0f;
    g_CurrentPlayer->crouchpos = CROUCH_STAND;
    g_CurrentPlayer->field_7C = 0.0f;

    /* Do not place the driver here. Tick() performs the same 45-tick
     * cosine-style position blend used for the entry yaw. */
    propLevelModifierDamDrivableBoatUpdateAudio();
}

void propLevelModifierDamDrivableBoatExitCurrentPlayer(void)
{
    if (!propLevelModifierDamDrivableBoatCurrentPlayerDriving())
        return;

    g_LevelModifierDamDrivableBoatDriver = -1;
    g_LevelModifierDamDrivableBoatSpeed = 0.0f;
    g_LevelModifierDamDrivableBoatThrottle = 0.0f;
    g_LevelModifierDamDrivableBoatSteering = 0.0f;
    g_LevelModifierDamDrivableBoatSteeringApplied = 0.0f;
    g_LevelModifierDamDrivableBoatEnterBlend = 1.0f;

    g_CurrentPlayer->speedsideways = 0.0f;
    g_CurrentPlayer->speedforwards = 0.0f;
    g_CurrentPlayer->speedtheta = 0.0f;

    /* Bond remains at the helm/deck position. The object is latched and is
     * deliberately not destroyed when the driver exits. */
    propLevelModifierDamDrivableBoatStopAudio();
}

void propLevelModifierDamDrivableBoatSetControls(f32 throttle, f32 steering)
{
    if (!propLevelModifierDamDrivableBoatCurrentPlayerDriving())
        return;

    if (throttle > 1.0f) throttle = 1.0f;
    if (throttle < -1.0f) throttle = -1.0f;
    if (steering > 1.0f) steering = 1.0f;
    if (steering < -1.0f) steering = -1.0f;

    g_LevelModifierDamDrivableBoatThrottle = throttle;
    g_LevelModifierDamDrivableBoatSteering = steering;
}

f32 propLevelModifierDamDrivableBoatDriverY(void)
{
    if (g_LevelModifierDamDrivableBoatRuntime.prop == NULL)
        return 0.0f;

    return g_LevelModifierDamDrivableBoatRuntime.prop->pos.y
        + DAM_DRIVABLE_BOAT_DRIVER_Y_OFFSET;
}

static void propLevelModifierDamDrivableBoatWorldToLocal(
    const coord3d *world, f32 *localx, f32 *localz)
{
    f32 dx;
    f32 dz;
    f32 c;
    f32 s;

    dx = world->x - g_LevelModifierDamDrivableBoatRuntime.prop->pos.x;
    dz = world->z - g_LevelModifierDamDrivableBoatRuntime.prop->pos.z;

    c = cosf(g_LevelModifierDamDrivableBoatHeading
        + DAM_DRIVABLE_BOAT_MODEL_YAW_OFFSET);
    s = sinf(g_LevelModifierDamDrivableBoatHeading
        + DAM_DRIVABLE_BOAT_MODEL_YAW_OFFSET);

    *localx = dx * c + dz * s;
    *localz = -dx * s + dz * c;
}

static void propLevelModifierDamDrivableBoatLocalToWorld(
    f32 localx, f32 localz, coord3d *world)
{
    f32 c;
    f32 s;

    c = cosf(g_LevelModifierDamDrivableBoatHeading
        + DAM_DRIVABLE_BOAT_MODEL_YAW_OFFSET);
    s = sinf(g_LevelModifierDamDrivableBoatHeading
        + DAM_DRIVABLE_BOAT_MODEL_YAW_OFFSET);

    world->x = g_LevelModifierDamDrivableBoatRuntime.prop->pos.x
        + localx * c - localz * s;
    world->z = g_LevelModifierDamDrivableBoatRuntime.prop->pos.z
        + localx * s + localz * c;
}

static s32 propLevelModifierDamDrivableBoatTryLandTransfer(
    const coord3d *candidate, const coord3d *moveOffset)
{
    StandTile *tile;
    f32 groundY;
    f32 currentFloor;

    (void)moveOffset;

    if (g_CurrentPlayer == NULL)
        return FALSE;

    currentFloor = g_CurrentPlayer->field_70;

    if (!propLevelModifierDamDrivableBoatWorldLandAt(
            candidate, &tile, &groundY))
    {
        return FALSE;
    }

    if (groundY > currentFloor + DAM_DRIVABLE_BOAT_LAND_STEP_UP
        || groundY < currentFloor - DAM_DRIVABLE_BOAT_LAND_STEP_DOWN)
    {
        return FALSE;
    }

    g_CurrentPlayer->field_488.collision_position.x = candidate->x;
    g_CurrentPlayer->field_488.collision_position.z = candidate->z;
    g_CurrentPlayer->field_488.current_tile_ptr = tile;
    g_CurrentPlayer->field_488.current_tile_ptr_for_portals = tile;
    g_CurrentPlayer->prop->stan = tile;
    g_CurrentPlayer->stanHeight = groundY;

    return TRUE;
}

/* R27R_R21_R2_REVERT_TO_PRE_STAN_Y_BASELINE_R16 */
static s32 propLevelModifierDamDrivableBoatDeckFloorAtLocal(
    f32 localx, f32 localz, f32 margin, f32 *floorY)
{
    f32 absx;
    f32 absz;
    f32 shapez;
    f32 halfx;
    f32 t;
    f32 smooth;
    f32 yoff;

    if (g_LevelModifierDamDrivableBoatRuntime.prop == NULL)
        return FALSE;

    absx = propLevelModifierDamDrivableBoatAbs(localx);
    absz = propLevelModifierDamDrivableBoatAbs(localz);

    if (absz > DAM_DRIVABLE_BOAT_DECK_HALF_Z + margin)
        return FALSE;

    shapez = absz;

    if (shapez > DAM_DRIVABLE_BOAT_DECK_HALF_Z)
        shapez = DAM_DRIVABLE_BOAT_DECK_HALF_Z;

    halfx = DAM_DRIVABLE_BOAT_DECK_HALF_X;

    if (shapez > DAM_DRIVABLE_BOAT_DECK_TAPER_START_Z)
    {
        t = (shapez - DAM_DRIVABLE_BOAT_DECK_TAPER_START_Z)
            / (DAM_DRIVABLE_BOAT_DECK_HALF_Z
                - DAM_DRIVABLE_BOAT_DECK_TAPER_START_Z);

        if (t > 1.0f)
            t = 1.0f;

        halfx += (DAM_DRIVABLE_BOAT_DECK_TIP_HALF_X
            - DAM_DRIVABLE_BOAT_DECK_HALF_X) * t;
    }

    if (absx > halfx + margin)
        return FALSE;

    yoff = DAM_DRIVABLE_BOAT_DECK_CENTER_Y_OFFSET;

    if (shapez > DAM_DRIVABLE_BOAT_DECK_RAISE_START_Z)
    {
        t = (shapez - DAM_DRIVABLE_BOAT_DECK_RAISE_START_Z)
            / (DAM_DRIVABLE_BOAT_DECK_RAISE_FULL_Z
                - DAM_DRIVABLE_BOAT_DECK_RAISE_START_Z);

        if (t < 0.0f)
            t = 0.0f;
        if (t > 1.0f)
            t = 1.0f;

        smooth = t * t * (3.0f - 2.0f * t);

        yoff += (DAM_DRIVABLE_BOAT_DECK_END_Y_OFFSET
            - DAM_DRIVABLE_BOAT_DECK_CENTER_Y_OFFSET) * smooth;
    }

    if (floorY != NULL)
        *floorY = g_LevelModifierDamDrivableBoatRuntime.prop->pos.y + yoff;

    return TRUE;
}

static s32 propLevelModifierDamDrivableBoatDeckFloorAtWorld(
    const coord3d *world, f32 margin, f32 *floorY)
{
    f32 localx;
    f32 localz;

    if (world == NULL)
        return FALSE;

    propLevelModifierDamDrivableBoatWorldToLocal(world, &localx, &localz);

    return propLevelModifierDamDrivableBoatDeckFloorAtLocal(
        localx, localz, margin, floorY);
}

static s32 propLevelModifierDamDrivableBoatWorldLandAt(
    const coord3d *world, StandTile **tileOut, f32 *groundYOut)
{
    coord3d probe;
    StandTile *tile;
    f32 groundY;

    if (world == NULL || g_CurrentPlayer == NULL)
        return FALSE;

    probe = *world;
    probe.y = g_CurrentPlayer->field_70 + DAM_DRIVABLE_BOAT_LAND_STEP_UP;

    tile = stanFindGroundAtCyl(
        &probe, DAM_DRIVABLE_BOAT_DECK_LAND_RADIUS, NULL, &groundY);

    if (tile == NULL || groundY <= DAM_DRIVABLE_BOAT_DECK_LAND_MIN_Y)
        return FALSE;

    if (tileOut != NULL)
        *tileOut = tile;

    if (groundYOut != NULL)
        *groundYOut = groundY;

    return TRUE;
}

static void propLevelModifierDamDrivableBoatSetPlayerBaseY(f32 baseY)
{
    f32 eyeOffset;

    if (g_CurrentPlayer == NULL)
        return;

    eyeOffset = g_CurrentPlayer->field_488.collision_position.y
        - g_CurrentPlayer->field_70;

    g_CurrentPlayer->field_70 = baseY;
    g_CurrentPlayer->stanHeight = baseY;
    g_CurrentPlayer->field_7C = 0.0f;
    g_CurrentPlayer->field_488.collision_position.y =
        baseY + eyeOffset;

    if (g_CurrentPlayer->prop != NULL)
        g_CurrentPlayer->prop->pos.y =
            g_CurrentPlayer->field_488.collision_position.y;
}


static s32 propLevelModifierDamDrivableBoatNearbyValidStan(
    const coord3d *world, f32 *groundYOut)
{
    coord3d probe;
    StandTile *tile;
    f32 groundY;
    f32 radius;
    f32 diag;
    s32 ring;
    s32 sample;
    f32 ox[8];
    f32 oz[8];

    if (world == NULL || g_CurrentPlayer == NULL)
        return FALSE;

    for (ring = 1; ring <= DAM_DRIVABLE_BOAT_STAN_SAMPLE_RINGS; ring++)
    {
        radius = DAM_DRIVABLE_BOAT_STAN_SAMPLE_STEP * ring;
        diag = radius * 0.70710678f;

        ox[0] =  radius; oz[0] = 0.0f;
        ox[1] = -radius; oz[1] = 0.0f;
        ox[2] = 0.0f;    oz[2] =  radius;
        ox[3] = 0.0f;    oz[3] = -radius;
        ox[4] =  diag;    oz[4] =  diag;
        ox[5] = -diag;    oz[5] =  diag;
        ox[6] =  diag;    oz[6] = -diag;
        ox[7] = -diag;    oz[7] = -diag;

        for (sample = 0; sample < 8; sample++)
        {
            probe = *world;
            probe.x += ox[sample];
            probe.z += oz[sample];
            probe.y = g_CurrentPlayer->field_70
                + DAM_DRIVABLE_BOAT_LAND_STEP_UP;

            tile = stanFindGroundAtCyl(
                &probe, DAM_DRIVABLE_BOAT_STAN_SAMPLE_RADIUS,
                NULL, &groundY);

            if (tile != NULL
                && groundY > DAM_DRIVABLE_BOAT_DECK_LAND_MIN_Y)
            {
                if (groundYOut != NULL)
                    *groundYOut = groundY;

                return TRUE;
            }
        }
    }

    return FALSE;
}

static s32 propLevelModifierDamDrivableBoatTransitionRectAtWorld(
    const coord3d *world, f32 *floorY)
{
    f32 localx;
    f32 localz;
    f32 samplez;
    f32 t;
    f32 smooth;
    f32 yoff;

    if (world == NULL || g_LevelModifierDamDrivableBoatRuntime.prop == NULL)
        return FALSE;

    propLevelModifierDamDrivableBoatWorldToLocal(world, &localx, &localz);

    if (propLevelModifierDamDrivableBoatAbs(localx)
            > DAM_DRIVABLE_BOAT_TRANSITION_HALF_X
        || propLevelModifierDamDrivableBoatAbs(localz)
            > DAM_DRIVABLE_BOAT_TRANSITION_HALF_Z)
    {
        return FALSE;
    }

    /*
     * The transition volume is rectangular, but its support Y follows the
     * boat's longitudinal deck profile. Clamp Z to the real hull length so
     * the extra rectangle beyond bow/stern does not invent taller geometry.
     */
    samplez = propLevelModifierDamDrivableBoatAbs(localz);

    if (samplez > DAM_DRIVABLE_BOAT_DECK_HALF_Z)
        samplez = DAM_DRIVABLE_BOAT_DECK_HALF_Z;

    yoff = DAM_DRIVABLE_BOAT_DECK_CENTER_Y_OFFSET;

    if (samplez > DAM_DRIVABLE_BOAT_DECK_RAISE_START_Z)
    {
        t = (samplez - DAM_DRIVABLE_BOAT_DECK_RAISE_START_Z)
            / (DAM_DRIVABLE_BOAT_DECK_RAISE_FULL_Z
                - DAM_DRIVABLE_BOAT_DECK_RAISE_START_Z);

        if (t < 0.0f)
            t = 0.0f;
        if (t > 1.0f)
            t = 1.0f;

        smooth = t * t * (3.0f - 2.0f * t);

        yoff += (DAM_DRIVABLE_BOAT_DECK_END_Y_OFFSET
            - DAM_DRIVABLE_BOAT_DECK_CENTER_Y_OFFSET) * smooth;
    }

    if (floorY != NULL)
        *floorY = g_LevelModifierDamDrivableBoatRuntime.prop->pos.y + yoff;

    return TRUE;
}

static s32 propLevelModifierDamDrivableBoatTransitionClipHaloAtWorld(
    const coord3d *world, f32 *floorY)
{
    coord3d clamped;
    f32 localx;
    f32 localz;
    f32 extra;

    if (world == NULL
        || g_CurrentPlayer == NULL
        || g_LevelModifierDamDrivableBoatRuntime.prop == NULL)
    {
        return FALSE;
    }

    extra = g_CurrentPlayer->field_488.collision_radius
        + DAM_DRIVABLE_BOAT_CLIP_RELAX_EXTRA;

    if (extra < 30.0f)
        extra = 30.0f;
    if (extra > 72.0f)
        extra = 72.0f;

    propLevelModifierDamDrivableBoatWorldToLocal(world, &localx, &localz);

    if (propLevelModifierDamDrivableBoatAbs(localx)
            > DAM_DRIVABLE_BOAT_TRANSITION_HALF_X + extra
        || propLevelModifierDamDrivableBoatAbs(localz)
            > DAM_DRIVABLE_BOAT_TRANSITION_HALF_Z + extra)
    {
        return FALSE;
    }

    if (localx > DAM_DRIVABLE_BOAT_TRANSITION_HALF_X)
        localx = DAM_DRIVABLE_BOAT_TRANSITION_HALF_X;
    if (localx < -DAM_DRIVABLE_BOAT_TRANSITION_HALF_X)
        localx = -DAM_DRIVABLE_BOAT_TRANSITION_HALF_X;
    if (localz > DAM_DRIVABLE_BOAT_TRANSITION_HALF_Z)
        localz = DAM_DRIVABLE_BOAT_TRANSITION_HALF_Z;
    if (localz < -DAM_DRIVABLE_BOAT_TRANSITION_HALF_Z)
        localz = -DAM_DRIVABLE_BOAT_TRANSITION_HALF_Z;

    clamped = *world;
    propLevelModifierDamDrivableBoatLocalToWorld(
        localx, localz, &clamped);

    return propLevelModifierDamDrivableBoatTransitionRectAtWorld(
        &clamped, floorY);
}

s32 propLevelModifierDamDrivableBoatGetWalkFloor(f32 *floorY)
{
    coord3d pos;
    f32 deckY;
    f32 nearbyGroundY;
    s32 inTransitionRect;
    s32 inClipHalo;

    if (g_LevelModifierDamDrivableBoatRuntime.prop == NULL
        || g_CurrentPlayer == NULL
        || propLevelModifierDamDrivableBoatCurrentPlayerDriving())
    {
        return FALSE;
    }

    pos = g_CurrentPlayer->field_488.collision_position;

    if (propLevelModifierDamDrivableBoatDeckFloorAtWorld(
            &pos, 0.0f, &deckY))
    {
        if (floorY != NULL)
            *floorY = deckY;
        return TRUE;
    }

    inTransitionRect =
        propLevelModifierDamDrivableBoatTransitionRectAtWorld(
            &pos, &deckY);

    inClipHalo = FALSE;

    if (!inTransitionRect)
    {
        inClipHalo =
            propLevelModifierDamDrivableBoatTransitionClipHaloAtWorld(
                &pos, &deckY);
    }

    if (!inTransitionRect && !inClipHalo)
        return FALSE;

    if (propLevelModifierDamDrivableBoatWorldLandAt(&pos, NULL, NULL))
        return FALSE;

    if (!propLevelModifierDamDrivableBoatNearbyValidStan(
            &pos, &nearbyGroundY))
    {
        return FALSE;
    }

    if (floorY != NULL)
        *floorY = deckY;

    return TRUE;
}

/* R27R_R13_DECK_EDGE_WALL_SLIDE
 *
 * When a diagonal movement step would leave the boat into unsupported water,
 * do not consume the entire step. Preserve whichever boat-local axis can
 * still move on the real deck. This gives Bond a wall-slide effect along the
 * bow/stern/sides instead of feeling glued in place at the edge.
 *
 * No speed/velocity fields are cleared here; only the blocked outward
 * component is discarded for this frame.
 */
static s32 propLevelModifierDamDrivableBoatTryDeckWallSlide(
    const coord3d *current, const coord3d *moveOffset)
{
    coord3d full;
    coord3d xcandidate;
    coord3d zcandidate;
    f32 currentLocalX;
    f32 currentLocalZ;
    f32 fullLocalX;
    f32 fullLocalZ;
    f32 dx;
    f32 dz;
    f32 xFloor;
    f32 zFloor;
    s32 xValid;
    s32 zValid;

    if (current == NULL || moveOffset == NULL)
        return FALSE;

    full = *current;
    full.x += moveOffset->x;
    full.z += moveOffset->z;

    propLevelModifierDamDrivableBoatWorldToLocal(
        current, &currentLocalX, &currentLocalZ);
    propLevelModifierDamDrivableBoatWorldToLocal(
        &full, &fullLocalX, &fullLocalZ);

    dx = fullLocalX - currentLocalX;
    dz = fullLocalZ - currentLocalZ;

    xcandidate = *current;
    zcandidate = *current;

    propLevelModifierDamDrivableBoatLocalToWorld(
        currentLocalX + dx, currentLocalZ, &xcandidate);
    propLevelModifierDamDrivableBoatLocalToWorld(
        currentLocalX, currentLocalZ + dz, &zcandidate);

    xValid = propLevelModifierDamDrivableBoatDeckFloorAtWorld(
        &xcandidate, 0.0f, &xFloor);
    zValid = propLevelModifierDamDrivableBoatDeckFloorAtWorld(
        &zcandidate, 0.0f, &zFloor);

    if (!xValid && !zValid)
        return FALSE;

    if (xValid && (!zValid
        || propLevelModifierDamDrivableBoatAbs(dx)
            >= propLevelModifierDamDrivableBoatAbs(dz)))
    {
        g_CurrentPlayer->field_488.collision_position.x = xcandidate.x;
        g_CurrentPlayer->field_488.collision_position.z = xcandidate.z;
        g_CurrentPlayer->stanHeight = xFloor;
        return TRUE;
    }

    g_CurrentPlayer->field_488.collision_position.x = zcandidate.x;
    g_CurrentPlayer->field_488.collision_position.z = zcandidate.z;
    g_CurrentPlayer->stanHeight = zFloor;
    return TRUE;
}

s32 propLevelModifierDamDrivableBoatHandleFootMovement(
    struct coord3d *moveOffset)
{
    coord3d current;
    coord3d candidate;
    f32 currentFloor;
    f32 candidateFloor;
    f32 nearbyGroundY;
    s32 currentOnDeck;
    s32 currentInShell;
    s32 currentInClipHalo;
    s32 candidateOnDeck;
    s32 candidateInShell;
    s32 candidateInClipHalo;
    s32 candidateTouchesStan;

    if (g_LevelModifierDamDrivableBoatRuntime.prop == NULL
        || g_CurrentPlayer == NULL
        || moveOffset == NULL
        || propLevelModifierDamDrivableBoatCurrentPlayerDriving())
    {
        return FALSE;
    }

    current = g_CurrentPlayer->field_488.collision_position;
    candidate = current;
    candidate.x += moveOffset->x;
    candidate.z += moveOffset->z;

    currentOnDeck = propLevelModifierDamDrivableBoatDeckFloorAtWorld(
        &current, 0.0f, &currentFloor);
    currentInShell = propLevelModifierDamDrivableBoatTransitionRectAtWorld(
        &current, &currentFloor);
    currentInClipHalo =
        propLevelModifierDamDrivableBoatTransitionClipHaloAtWorld(
            &current, &currentFloor);

    candidateOnDeck = propLevelModifierDamDrivableBoatDeckFloorAtWorld(
        &candidate, 0.0f, &candidateFloor);
    candidateInShell = propLevelModifierDamDrivableBoatTransitionRectAtWorld(
        &candidate, &candidateFloor);
    candidateInClipHalo =
        propLevelModifierDamDrivableBoatTransitionClipHaloAtWorld(
            &candidate, &candidateFloor);

    if (candidateOnDeck)
    {
        g_CurrentPlayer->field_488.collision_position.x = candidate.x;
        g_CurrentPlayer->field_488.collision_position.z = candidate.z;
        g_CurrentPlayer->stanHeight = candidateFloor;
        return TRUE;
    }

    if (candidateInShell)
    {
        if (propLevelModifierDamDrivableBoatTryLandTransfer(
                &candidate, moveOffset))
        {
            return TRUE;
        }

        candidateTouchesStan =
            propLevelModifierDamDrivableBoatNearbyValidStan(
                &candidate, &nearbyGroundY);

        if (candidateTouchesStan)
        {
            g_CurrentPlayer->field_488.collision_position.x = candidate.x;
            g_CurrentPlayer->field_488.collision_position.z = candidate.z;
            g_CurrentPlayer->stanHeight = candidateFloor;
            return TRUE;
        }

        if (currentOnDeck
            && propLevelModifierDamDrivableBoatTryDeckWallSlide(
                &current, moveOffset))
        {
            return TRUE;
        }

        if (currentOnDeck || currentInShell || currentInClipHalo)
            return TRUE;

        return FALSE;
    }

    if (candidateInClipHalo)
    {
        if (propLevelModifierDamDrivableBoatTryLandTransfer(
                &candidate, moveOffset))
        {
            return TRUE;
        }

        candidateTouchesStan =
            propLevelModifierDamDrivableBoatNearbyValidStan(
                &candidate, &nearbyGroundY);

        if (candidateTouchesStan)
        {
            g_CurrentPlayer->field_488.collision_position.x = candidate.x;
            g_CurrentPlayer->field_488.collision_position.z = candidate.z;
            g_CurrentPlayer->stanHeight = candidateFloor;
            return TRUE;
        }

        if (currentOnDeck
            && propLevelModifierDamDrivableBoatTryDeckWallSlide(
                &current, moveOffset))
        {
            return TRUE;
        }

        if (currentOnDeck || currentInShell || currentInClipHalo)
            return TRUE;

        return FALSE;
    }

    if (currentOnDeck || currentInShell || currentInClipHalo)
    {
        if (propLevelModifierDamDrivableBoatTryLandTransfer(
                &candidate, moveOffset))
        {
            return TRUE;
        }

        if (currentOnDeck
            && propLevelModifierDamDrivableBoatTryDeckWallSlide(
                &current, moveOffset))
        {
            return TRUE;
        }

        return TRUE;
    }

    return FALSE;
}

s32 propLevelModifierDamDrivableBoatCurrentPlayerSupported(void)
{
    coord3d pos;
    f32 deckY;

    if (g_LevelModifierDamDrivableBoatRuntime.prop == NULL
        || g_CurrentPlayer == NULL)
    {
        return FALSE;
    }

    pos = g_CurrentPlayer->field_488.collision_position;

    if (!propLevelModifierDamDrivableBoatDeckFloorAtWorld(
            &pos, DAM_DRIVABLE_BOAT_DECK_SEAM_MARGIN, &deckY))
    {
        return FALSE;
    }

    return propLevelModifierDamDrivableBoatAbs(
        g_CurrentPlayer->field_70 - deckY) <= 160.0f;
}

void propLevelModifierDamDrivableBoatTick(void)
{
    f32 target;
    f32 step;
    f32 oldHeading;
    f32 candidateHeading;
    f32 headingdeg;
    f32 oldHeadingDeg;
    f32 headingDeltaDeg;
    f32 speedScale;
    f32 steeringTarget;
    f32 steeringStep;
    f32 remain;
    f32 angleDiff;
    f32 targetForBlend;
    coord3d oldpos;
    coord3d candidatePos;
    ObjectRecord *obj;

    if (!propLevelModifierDamDrivableBoatCurrentPlayerDriving())
    {
        propLevelModifierDamDrivableBoatStopAudio();
        return;
    }

    if (g_CurrentPlayer->bonddead)
    {
        propLevelModifierDamDrivableBoatExitCurrentPlayer();
        return;
    }

    obj = &g_LevelModifierDamDrivableBoatRuntime;

    if (obj->prop == NULL)
    {
        propLevelModifierDamDrivableBoatStopAudio();
        return;
    }

    if (propLevelModifierDamDrivableBoatEntering())
        target = 0.0f;
    else
        target = g_LevelModifierDamDrivableBoatThrottle
            * DAM_DRIVABLE_BOAT_MAX_SPEED;

    if (g_LevelModifierDamDrivableBoatSpeed < target)
    {
        step = DAM_DRIVABLE_BOAT_ACCEL * g_GlobalTimerDelta;
        g_LevelModifierDamDrivableBoatSpeed += step;

        if (g_LevelModifierDamDrivableBoatSpeed > target)
            g_LevelModifierDamDrivableBoatSpeed = target;
    }
    else if (g_LevelModifierDamDrivableBoatSpeed > target)
    {
        step = DAM_DRIVABLE_BOAT_DECEL * g_GlobalTimerDelta;
        g_LevelModifierDamDrivableBoatSpeed -= step;

        if (g_LevelModifierDamDrivableBoatSpeed < target)
            g_LevelModifierDamDrivableBoatSpeed = target;
    }

    oldpos = obj->prop->pos;
    candidatePos = oldpos;
    oldHeading = g_LevelModifierDamDrivableBoatHeading;
    candidateHeading = oldHeading;

    steeringTarget = g_LevelModifierDamDrivableBoatSteering;

    if (propLevelModifierDamDrivableBoatEntering())
        steeringTarget = 0.0f;

    steeringStep =
        DAM_DRIVABLE_BOAT_STEER_RESPONSE * g_GlobalTimerDelta;

    if (g_LevelModifierDamDrivableBoatSteeringApplied < steeringTarget)
    {
        g_LevelModifierDamDrivableBoatSteeringApplied += steeringStep;

        if (g_LevelModifierDamDrivableBoatSteeringApplied > steeringTarget)
            g_LevelModifierDamDrivableBoatSteeringApplied = steeringTarget;
    }
    else if (g_LevelModifierDamDrivableBoatSteeringApplied > steeringTarget)
    {
        g_LevelModifierDamDrivableBoatSteeringApplied -= steeringStep;

        if (g_LevelModifierDamDrivableBoatSteeringApplied < steeringTarget)
            g_LevelModifierDamDrivableBoatSteeringApplied = steeringTarget;
    }

    speedScale = propLevelModifierDamDrivableBoatAbs(
        g_LevelModifierDamDrivableBoatSpeed)
        / DAM_DRIVABLE_BOAT_MAX_SPEED;

    if (propLevelModifierDamDrivableBoatAbs(
            g_LevelModifierDamDrivableBoatSpeed) <= 0.05f)
    {
        /*
         * R16: the rudder/helm may slowly rotate the boat in place. This is
         * deliberately much weaker than the moving minimum-turn authority.
         */
        speedScale = DAM_DRIVABLE_BOAT_STATIONARY_TURN_SCALE;
    }
    else if (speedScale < 0.25f)
    {
        speedScale = 0.25f;
    }

    candidateHeading +=
        g_LevelModifierDamDrivableBoatSteeringApplied
        * DegToRad1Fact(DAM_DRIVABLE_BOAT_TURN_DEG)
        * g_GlobalTimerDelta * speedScale;

    while (candidateHeading >= M_TAU_F)
        candidateHeading -= M_TAU_F;
    while (candidateHeading < 0.0f)
        candidateHeading += M_TAU_F;

    candidatePos.x += g_LevelModifierDamDrivableBoatSpeed
        * sinf(M_TAU_F - candidateHeading)
        * g_GlobalTimerDelta;
    candidatePos.z += g_LevelModifierDamDrivableBoatSpeed
        * cosf(M_TAU_F - candidateHeading)
        * g_GlobalTimerDelta;
    candidatePos.y = DAM_DRIVABLE_BOAT_START_Y;

    /*
     * R27R R12: do NOT gate vehicle movement on STAN room 0x23.
     * The known Dam lake Room 0x23 is a BG room; treating it as guaranteed
     * STAN support can return NULL/another STAN room and reject every movement
     * tick. Keep the existing water envelope + hard hull/background collision
     * as the movement authority until a true BG-room boundary test is added.
     */
    if (!propLevelModifierDamDrivableBoatHullBlocked(
            &oldpos, oldHeading, &candidatePos, candidateHeading))
    {
        g_LevelModifierDamDrivableBoatHeading = candidateHeading;
        obj->prop->pos = candidatePos;
    }
    else
    {
        g_LevelModifierDamDrivableBoatSpeed = 0.0f;
        g_LevelModifierDamDrivableBoatThrottle = 0.0f;
    }

    obj->prop->pos.y = DAM_DRIVABLE_BOAT_START_Y;
    propLevelModifierDamDrivableBoatApplyTransform();

    headingdeg = g_LevelModifierDamDrivableBoatHeading * 360.0f / M_TAU_F;
    oldHeadingDeg = oldHeading * 360.0f / M_TAU_F;

    while (headingdeg < 0.0f) headingdeg += 360.0f;
    while (headingdeg >= 360.0f) headingdeg -= 360.0f;

    if (g_LevelModifierDamDrivableBoatEnterBlend < 1.0f)
    {
        g_LevelModifierDamDrivableBoatEnterBlend +=
            g_GlobalTimerDelta / DAM_DRIVABLE_BOAT_ENTER_TICKS;

        if (g_LevelModifierDamDrivableBoatEnterBlend > 1.0f)
            g_LevelModifierDamDrivableBoatEnterBlend = 1.0f;

        remain = (cosf(g_LevelModifierDamDrivableBoatEnterBlend
            * M_PI_F) + 1.0f) * 0.5f;

        targetForBlend = headingdeg;
        angleDiff = targetForBlend - g_LevelModifierDamDrivableBoatEnterStartYaw;

        if (angleDiff > 180.0f)
            targetForBlend -= 360.0f;
        else if (angleDiff < -180.0f)
            targetForBlend += 360.0f;

        g_CurrentPlayer->vv_theta =
            remain * g_LevelModifierDamDrivableBoatEnterStartYaw
            + (1.0f - remain) * targetForBlend;
    }
    else
    {
        headingDeltaDeg = headingdeg - oldHeadingDeg;

        if (headingDeltaDeg > 180.0f) headingDeltaDeg -= 360.0f;
        if (headingDeltaDeg < -180.0f) headingDeltaDeg += 360.0f;

        g_CurrentPlayer->vv_theta += headingDeltaDeg;
    }

    while (g_CurrentPlayer->vv_theta < 0.0f)
        g_CurrentPlayer->vv_theta += 360.0f;
    while (g_CurrentPlayer->vv_theta >= 360.0f)
        g_CurrentPlayer->vv_theta -= 360.0f;

    g_CurrentPlayer->speedtheta = 0.0f;
    g_CurrentPlayer->vv_costheta =
        cosf(g_CurrentPlayer->vv_theta * DegToRad1Fact(1));
    g_CurrentPlayer->vv_sintheta =
        sinf(g_CurrentPlayer->vv_theta * DegToRad1Fact(1));
    g_CurrentPlayer->field_488.theta_transform.x =
        -g_CurrentPlayer->vv_sintheta;
    g_CurrentPlayer->field_488.theta_transform.y = 0.0f;
    g_CurrentPlayer->field_488.theta_transform.z =
        g_CurrentPlayer->vv_costheta;

    propLevelModifierDamDrivableBoatPlaceDriver();
    propLevelModifierDamDrivableBoatUpdateAudio();
}

void propLevelModifierCleanupDamRestorations(void)
{
    propLevelModifierFreeDamDoor(0);
    propLevelModifierFreeDamDoor(1);
    propLevelModifierFreeDamBoat();
    propLevelModifierFreeDamDrivableBoat();
    g_LevelModifierDamRestorePrepared = FALSE;
}

/* V90: Zoinkity Citadel multiplayer runtime pad/intro handoff.
 *
 * The 2005 restoration loaded its Citadel setup normally, then redirected
 * g_CurrentSetup.intro and g_CurrentSetup.pads to runtime-ready data embedded
 * in Citadel.bin.  Keep that architecture here, but resolve STAN links by name
 * instead of using hard-coded RDRAM addresses. */
typedef struct CitadelMpPadTemplate
{
    coord3d pos;
    coord3d up;
    coord3d look;
    char *stanName;
} CitadelMpPadTemplate;

static CitadelMpPadTemplate g_CitadelMpPadTemplates[48] = {
    { {906.790894f, 161.348511f, 1997.19531f}, {0.0f, 1.0f, 0.0f}, {0.493941993f, 0.0f, 0.869494975f}, "p1298d1" },
    { {878.068115f, 161.348511f, -717.105042f}, {0.0f, 1.0f, 0.0f}, {0.992645919f, 0.0f, -0.121054374f}, "p1093e6" },
    { {2002.39856f, 7.11361074f, -1291.46204f}, {0.0f, 1.0f, 0.0f}, {0.5f, 0.0f, -0.866024971f}, "p1105f" },
    { {-2492.71069f, 170.023163f, -1003.17767f}, {0.0f, 1.0f, 0.0f}, {0.572181582f, 0.0f, -0.820126951f}, "p805c3" },
    { {-2267.35669f, 32.9636765f, 1376.95813f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, "p468a5" },
    { {-1787.55786f, 32.9636765f, 2882.29956f}, {0.0f, 1.0f, 0.0f}, {1.0f, -1.0f, 0.0f}, "p1769h6" },
    { {1307.17456f, -431.99765f, -124.999413f}, {0.0f, 1.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, "p503b4" },
    { {-499.99765f, 32.9636765f, 506.937347f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, "p691a6" },
    { {-2565.19287f, 16.1580276f, -1535.79736f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, "p905f4" },
    { {-1980.42651f, 16.1580276f, -1162.20691f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, "p905f4" },
    { {1920.66833f, 6.85920048f, 1915.74792f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, "p1293d" },
    { {-499.107727f, 0.0f, -3132.66724f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, "p46a" },
    { {3163.41626f, 0.0f, 494.881683f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, "p65a1" },
    { {-516.28125f, 0.0f, 4155.96973f}, {0.0f, -1.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, "p20a1" },
    { {-2316.1311f, 6.86361074f, 2316.71069f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, "p1716h2" },
    { {-502.721375f, -1093.74219f, 857.248779f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, "p604e4" },
    { {-4218.17725f, 161.348511f, 539.127441f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, "p35a1" },
    { {1023.03278f, 277.271088f, -2076.71069f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, "p1135f6" },
    { {1057.43726f, 285.757355f, -791.223999f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, "p1153g2" },
    { {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, "" },
    { {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, "" },
    { {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, "" },
    { {-507.094971f, 607.977661f, 206.372772f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, "p671a1" },
    { {-434.576263f, -525.977661f, 504.648529f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, "p527c1" },
    { {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, "" },
    { {-1724.32751f, -826.600098f, 492.59964f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, "p564d2" },
    { {-458.913391f, -839.614746f, -789.778931f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, "p650f6" },
    { {-2908.90234f, 16.1319084f, 2892.12964f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, "p1742h4" },
    { {-1813.0061f, 16.1319084f, 1771.46204f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, "p1690h" },
    { {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, "" },
    { {-1261.87158f, -516.715149f, 502.696533f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, "p527c1" },
    { {-191.65593f, 616.715149f, 765.200073f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, "p699b" },
    { {-783.129578f, 616.715149f, 767.465149f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, "p699b" },
    { {-2006.63965f, 1017.4845f, -1424.5686f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, "p784b6" },
    { {-3152.94873f, 565.056885f, -534.309814f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, "p752a6" },
    { {-2977.32886f, 821.056885f, -1762.97974f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, "p772b3" },
    { {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, "" },
    { {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, "" },
    { {1657.72632f, 16.1473484f, 2920.46606f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, "p1288c7" },
    { {725.393677f, 16.1473484f, 2783.31396f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, "p1303d2" },
    { {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, "" },
    { {-2510.2439f, 7.59738064f, -1140.32751f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, "p905f4" },
    { {702.163757f, -826.600098f, 506.239532f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, "p556d" },
    { {587.225586f, 286.110535f, -2076.12964f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, "p1135f6" },
    { {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, "" },
    { {1022.50305f, 286.110535f, -1667.46204f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, "p1135f6" },
    { {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, "" },
    { {519.080505f, -516.715149f, 526.453552f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, "p527c1" },
};

static PadRecord g_CitadelMpPads[49];

static s32 g_CitadelMpIntro[] = {
    0x00000000, 0x0000000B, 0x00000000,
    0x00000000, 0x0000000C, 0x00000000,
    0x00000000, 0x0000000D, 0x00000000,
    0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000001, 0x00000000,
    0x00000000, 0x00000010, 0x00000000,
    0x00000000, 0x00000003, 0x00000000,
    0x00000000, 0x00000004, 0x00000000,
    0x00000000, 0x00000005, 0x00000000,
    0x00000000, 0x00000006, 0x00000000,
    0x00000000, 0x00000007, 0x00000000,
    0x00000001, 0x00000001, 0xFFFFFFFF, 0x00000000,
    0x00000005, 0x00000002,
    0x00000009
};

static void citadelApplyMultiplayerSetupOverrides(enum LEVELID stageId)
{
    s32 i;

    if (stageId != LEVELID_CITADEL
            || gamemode != GAMEMODE_MULTI
            || get_scenario() == SCENARIO_COOP)
    {
        return;
    }

    for (i = 0; i < 48; i++)
    {
        g_CitadelMpPads[i].pos = g_CitadelMpPadTemplates[i].pos;
        g_CitadelMpPads[i].up = g_CitadelMpPadTemplates[i].up;
        g_CitadelMpPads[i].look = g_CitadelMpPadTemplates[i].look;
        g_CitadelMpPads[i].plink = g_CitadelMpPadTemplates[i].stanName;

        if (g_CitadelMpPadTemplates[i].stanName[0] != '\0')
        {
            g_CitadelMpPads[i].stan = (StandTile *)stanMatchTileName(g_CitadelMpPadTemplates[i].stanName);
        }
        else
        {
            g_CitadelMpPads[i].stan = NULL;
        }
    }

    /* Keep the pad table iterable by normal Plus systems such as Mirrored
     * Levels, while preserving all 48 indices from Zoinkity's runtime table. */
    g_CitadelMpPads[48].pos.x = 0.0f;
    g_CitadelMpPads[48].pos.y = 0.0f;
    g_CitadelMpPads[48].pos.z = 0.0f;
    g_CitadelMpPads[48].plink = NULL;
    g_CitadelMpPads[48].stan = NULL;

    g_CurrentSetup.intro = g_CitadelMpIntro;
    g_CurrentSetup.pads = g_CitadelMpPads;
}
#endif


#ifdef GE_MODDED_CHEATS
void modSyncBodyArmorPickups(void)
{
    PropDefHeaderRecord *pdef = (PropDefHeaderRecord *)g_CurrentSetup.propDefs;

    while (pdef != NULL && pdef->type != PROPDEF_END)
    {
        if (pdef->type == PROPDEF_ARMOUR)
        {
            ObjectRecord *obj = (ObjectRecord *)pdef;

            /* Collected solo armour has obj->prop == NULL. Regenerating MP
             * armour retains its prop with a positive timetoregen. */
            if (obj->prop != NULL)
            {
                if (g_ModDisableBodyArmorEnabled)
                {
                    chrpropDisable(obj->prop);
                }
                else if (obj->prop->timetoregen <= 0)
                {
                    chrpropEnable(obj->prop);

                    if (obj->prop->rooms[0] == 0xff)
                        sub_GAME_7F03E134(obj->prop);
                }
            }
        }

        pdef = &pdef[sizepropdef(pdef)];
    }
}
#endif

s32 load_proptype(PROPDEF_TYPE type)
{
    PropDefHeaderRecord *propdef = (PropDefHeaderRecord *) g_CurrentSetup.propDefs;
    s32 count = 0;

    if (propdef != NULL)
    {
        while (propdef->type != PROPDEF_END)
        {
            if (propdef->type == (type & 0xFF))
            {
                count ++;
            }
            propdef = &propdef[sizepropdef((PropDefHeaderRecord* ) propdef)];
        }
    }
    return count;
}


/**
 * perfect dark padGetCentre (pad.c)
 *
 * NTSC address 0x7F001BD4.
*/
void sub_GAME_7F001BD4(struct BoundPadRecord *pad, struct coord3d *arg1)
{
    struct coord3d normal;
    f32 scale;
    struct bbox bb;
    f32 temp;

    bb.zmax = pad->bbox.xmin;
    bb.zmin = pad->bbox.xmax;
    bb.ymax = pad->bbox.ymin;
    bb.ymin = pad->bbox.ymax;
    bb.xmax = pad->bbox.zmin;
    bb.xmin = pad->bbox.zmax;

    normal.f[0] = (pad->up.f[1] * pad->look.f[2]) - (pad->up.f[2] * pad->look.f[1]);
    normal.f[1] = (pad->up.f[2] * pad->look.f[0]) - (pad->up.f[0] * pad->look.f[2]);
    normal.f[2] = (pad->up.f[0] * pad->look.f[1]) - (pad->up.f[1] * pad->look.f[0]);

    temp = (normal.f[0] * normal.f[0]) + (normal.f[1] * normal.f[1]) + (normal.f[2] * normal.f[2]);
    scale = 1.0f / sqrtf(temp);

    normal.f[0] *= scale;
    normal.f[1] *= scale;
    normal.f[2] *= scale;

    arg1->f[0] = pad->pos.f[0] + (
			(bb.zmax + bb.zmin) * normal.f[0] +
			(bb.ymax + bb.ymin) * pad->up.f[0] +
			(bb.xmax + bb.xmin) * pad->look.f[0]) * 0.5f;

	arg1->f[1] = pad->pos.f[1] + (
			(bb.zmax + bb.zmin) * normal.f[1] +
			(bb.ymax + bb.ymin) * pad->up.f[1] +
			(bb.xmax + bb.xmin) * pad->look.f[1]) * 0.5f;

	arg1->f[2] = pad->pos.f[2] + (
			(bb.zmax + bb.zmin) * normal.f[2] +
			(bb.ymax + bb.ymin) * pad->up.f[2] +
			(bb.xmax + bb.xmin) * pad->look.f[2]) * 0.5f;

}

/**
 * NTSC address 0x7F001D9C.
*/
void domakedefaultobj(s32 arg0, ObjectRecord *arg1, s32 cmdindex)
{
    s32 padding;
    s32 spF0;
    f32 var_f0;
    struct coord3d spE0;
    struct StandTile *spDC;
    struct coord3d spD0;
    StandTile *spCC;
    Mtxf sp8C;
    struct coord3d sp80;
    struct PropRecord *var_v0;
    f32 sp78;
    s32 sp74;
    struct BoundPadRecord *var_s0;
    ChrRecord *sp6C;
    ModelRoData_BoundingBoxRecord *temp_v0_3;
    struct PadRecord *sp64;
    struct PropRecord *sp60;
    s32 padding2;
    f32 sp58;
    f32 sp54;
    f32 sp50;
    s32 padding3;
    f32 sp48;

    spF0 = arg1->obj;
    var_s0 = NULL;

    modelLoad(spF0);

    sp78 = arg1->extrascale * 0.00390625f;

    arg1->damage = *(s32*)&arg1->damage / 65536.0f;

#ifdef GE_MODDED_CHEATS
    if (gamemode == GAMEMODE_MULTI && get_scenario() != SCENARIO_COOP)
#else
    if (getPlayerCount() >= 2)
#endif
    {
        sp74 = 1;

        if ((get_scenario() == SCENARIO_TLD) && (arg1->obj == PROP_FLAG))
        {
            sp74 = 0;
        }
        else if ((get_scenario() == SCENARIO_MWTGG) && (arg1->obj == PROP_CHRGOLDEN))
        {
            sp74 = 0;
        }

        if (sp74 != 0)
        {
            arg1->state |= PROPSTATE_RESPAWN; // respawn enabled
        }
    }

    if (arg1->flags & PROPFLAG_INSIDEANOTHEROBJ)
    {
        if (arg1->type == PROP_TYPE_SMOKE)
        {
            sub_GAME_7F051DD8(arg1, PitemZ_entries[spF0].header);
        }
        else
        {
            objInitWithModelDef(arg1, PitemZ_entries[spF0].header);
        }

        modelSetScale(arg1->model, arg1->model->scale * sp78);
    }
    else if (arg1->flags & PROPFLAG_ASSIGNEDTOCHR)
    {
        sp6C = chrFindByLiteralId(arg1->pad);

        if ((sp6C != NULL) && (sp6C->prop != NULL) && (sp6C->model != NULL))
        {
            if (arg1->type == 8)
            {
                var_v0 = sub_GAME_7F051DD8(arg1, PitemZ_entries[spF0].header);
            }
            else
            {
                var_v0 = objInitWithModelDef(arg1, PitemZ_entries[spF0].header);
            }

            modelSetScale(arg1->model, arg1->model->scale * sp78);
            chrpropReparent(var_v0, sp6C->prop);
        }
        #ifdef DEBUG
        else
        {
            osSyncPrintf("domakedefaultobj: no chr number %d for obj number %d!\n",arg1->pad,cmdindex + 1);
        }
        #endif
    }
    else
    {
        if (isNotBoundPad(arg1->pad))
        {
            sp64 = &g_CurrentSetup.pads[arg1->pad];

            matrix_4x4_set_basis_and_position_target(&sp8C, 0.0f, 0.0f, 0.0f, -sp64->look.f[0], -sp64->look.f[1], -sp64->look.f[2], sp64->up.f[0], sp64->up.f[1], sp64->up.f[2]);

            spD0.f[0] = sp64->pos.f[0];
            spD0.f[1] = sp64->pos.f[1];
            spD0.f[2] = sp64->pos.f[2];

            if (arg1->flags & PROPFLAG_ONSCREEN)
            {
                sp80.f[0] = sp64->pos.f[0];
                sp80.f[1] = sp64->pos.f[1];
                sp80.f[2] = sp64->pos.f[2];
            }
            else
            {
                // same as above?

                sp80.f[0] = sp64->pos.f[0];
                sp80.f[1] = sp64->pos.f[1];
                sp80.f[2] = sp64->pos.f[2];
            }

            spCC = sp64->stan;
        }
        else
        {
            var_s0 = &g_CurrentSetup.boundpads[getBoundPadNum(arg1->pad)];

            matrix_4x4_set_basis_and_position_target(&sp8C, 0.0f, 0.0f, 0.0f, -var_s0->look.f[0], -var_s0->look.f[1], -var_s0->look.f[2], var_s0->up.f[0], var_s0->up.f[1], var_s0->up.f[2]);

            if (!(arg1->flags2 & PROPFLAG2_00000001))
            {
                sub_GAME_7F001BD4(var_s0, &spD0);

                sp80.f[0] = spD0.f[0] + (var_s0->up.f[0] * ((var_s0->bbox.ymin - var_s0->bbox.ymax) * 0.5f));
                sp80.f[1] = spD0.f[1] + (var_s0->up.f[1] * ((var_s0->bbox.ymin - var_s0->bbox.ymax) * 0.5f));
                sp80.f[2] = spD0.f[2] + (var_s0->up.f[2] * ((var_s0->bbox.ymin - var_s0->bbox.ymax) * 0.5f));

                spCC = var_s0->stan;

                if (walkTilesBetweenPoints_NoCallback(&spCC, var_s0->pos.f[0], var_s0->pos.f[2], spD0.f[0], spD0.f[2]) == 0)
                {
                    spD0.f[0] = var_s0->pos.f[0];
                    spD0.f[1] = var_s0->pos.f[1];
                    spD0.f[2] = var_s0->pos.f[2];

                    spCC = var_s0->stan;

                    if (!(arg1->flags & PROPFLAG_ONSCREEN) && !(arg1->flags & PROPFLAG_00001000))
                    {
                        // removed
                        #ifdef DEBUG
                            osSyncPrintf("object number %d not positioned correctly!\n",cmdindex + 1);
                        #endif
                    }
                }
            }
            else
            {
                spD0.f[0] = var_s0->pos.f[0];
                spD0.f[1] = var_s0->pos.f[1];
                spD0.f[2] = var_s0->pos.f[2];

                spCC = var_s0->stan;

                sub_GAME_7F001BD4(var_s0, &sp80);

                sp80.f[0] += (var_s0->bbox.ymin - var_s0->bbox.ymax) * 0.5f * var_s0->up.f[0];
                sp80.f[1] += (var_s0->bbox.ymin - var_s0->bbox.ymax) * 0.5f * var_s0->up.f[1];
                sp80.f[2] += (var_s0->bbox.ymin - var_s0->bbox.ymax) * 0.5f * var_s0->up.f[2];
            }
        }

        if (getposstan(&spD0, spCC, 0.0f, &spE0, &spDC) != 0)
        {
            if (arg1->type == PROP_TYPE_SMOKE)
            {
                sp60 = sub_GAME_7F051DD8(arg1, PitemZ_entries[spF0].header);
            }
            else
            {
                sp60 = objInitWithAutoModel(arg1);
            }

            if (var_s0 != NULL)
            {
                temp_v0_3 = chrobjGetBboxFromObjectRecord(arg1);
                if (temp_v0_3 != NULL)
                {
                    sp58 = 1.0f;
                    sp54 = 1.0f;
                    sp50 = 1.0f;

                    if (arg1->flags & (PROPFLAG_00000010 | PROPFLAG_00000020))
                    {
                        if (temp_v0_3->Bounds.xmin < temp_v0_3->Bounds.xmax)
                        {
                            if (arg1->flags & PROPFLAG_ONSCREEN)
                            {
                                sp58 = (var_s0->bbox.xmax - var_s0->bbox.xmin) / ((temp_v0_3->Bounds.xmax - temp_v0_3->Bounds.xmin) * arg1->model->scale);
                            }
                            else
                            {
                                sp58 = (var_s0->bbox.xmax - var_s0->bbox.xmin) / ((temp_v0_3->Bounds.xmax - temp_v0_3->Bounds.xmin) * arg1->model->scale);
                            }
                        }
                    }

                    if (arg1->flags & (PROPFLAG_00000010 | PROPFLAG_00000040))
                    {
                        if (temp_v0_3->Bounds.ymin < temp_v0_3->Bounds.ymax)
                        {
                            if (arg1->flags & PROPFLAG_ONSCREEN)
                            {
                                sp50 = (var_s0->bbox.zmax - var_s0->bbox.zmin) / ((temp_v0_3->Bounds.ymax - temp_v0_3->Bounds.ymin) * arg1->model->scale);
                            }
                            else
                            {
                                sp54 = (var_s0->bbox.ymax - var_s0->bbox.ymin) / ((temp_v0_3->Bounds.ymax - temp_v0_3->Bounds.ymin) * arg1->model->scale);
                            }
                        }
                    }

                    if (arg1->flags & (PROPFLAG_00000010 | PROPFLAG_00000080))
                    {
                        if (temp_v0_3->Bounds.zmin < temp_v0_3->Bounds.zmax)
                        {
                            if (arg1->flags & PROPFLAG_ONSCREEN)
                            {
                                sp54 = (var_s0->bbox.ymax - var_s0->bbox.ymin) / ((temp_v0_3->Bounds.zmax - temp_v0_3->Bounds.zmin) * arg1->model->scale);
                            }
                            else
                            {
                                sp50 = (var_s0->bbox.zmax - var_s0->bbox.zmin) / ((temp_v0_3->Bounds.zmax - temp_v0_3->Bounds.zmin) * arg1->model->scale);
                            }
                        }
                    }

                    var_f0 = sp58;

                    if (sp54 < var_f0)
                    {
                        var_f0 = sp54;
                    }

                    if (sp50 < var_f0)
                    {
                        var_f0 = sp50;
                    }

                    sp48 = sp58;

                    if (sp58 < sp54)
                    {
                        sp48 = sp54;
                    }

                    if (sp48 < sp50)
                    {
                        sp48 = sp50;
                    }

                    if (arg1->flags & PROPFLAG_00000010)
                    {
                        sp50 = var_f0;
                        sp54 = var_f0;
                        sp58 = var_f0;
                    }
                    else
                    {
                        if (!(arg1->flags & PROPFLAG_00000020))
                        {
                            if (arg1->flags & PROPFLAG_ONSCREEN)
                            {
                                if (temp_v0_3->Bounds.xmax == temp_v0_3->Bounds.xmin)
                                {
                                    sp58 = sp48;
                                }
                            }
                            else if (temp_v0_3->Bounds.xmax == temp_v0_3->Bounds.xmin)
                            {
                                sp58 = sp48;
                            }
                        }

                        if (!(arg1->flags & PROPFLAG_00000040))
                        {
                            if (arg1->flags & PROPFLAG_ONSCREEN)
                            {
                                if (temp_v0_3->Bounds.ymax == temp_v0_3->Bounds.ymin)
                                {
                                    sp50 = sp48;
                                }
                            }
                            else if (temp_v0_3->Bounds.ymax == temp_v0_3->Bounds.ymin)
                            {
                                sp54 = sp48;
                            }
                        }

                        if (!(arg1->flags & PROPFLAG_00000080))
                        {
                            if (arg1->flags & PROPFLAG_ONSCREEN)
                            {
                                if (temp_v0_3->Bounds.zmax == temp_v0_3->Bounds.zmin)
                                {
                                    sp54 = sp48;
                                }
                            }
                            else if (temp_v0_3->Bounds.zmax == temp_v0_3->Bounds.zmin)
                            {
                                sp50 = sp48;
                            }
                        }
                    }

                    sp58 /= sp48;
                    sp54 /= sp48;
                    sp50 /= sp48;

                    if ((sp58 <= 0.000001f) || (sp54 <= 0.000001f) || (sp50 <= 0.000001f))
                    {
                        #ifdef DEBUG
                        osSyncPrintf("Scale warning: object number %d has a small scale: %f,%f,%f\n",cmdindex +1, sp58,sp54,sp50);
                        #endif
                        sp50 = 1.0f;
                        sp54 = 1.0f;
                        sp58 = 1.0f;
                    }

                    matrix_column_1_scalar_multiply(sp58, sp8C.m[0]);
                    matrix_column_2_scalar_multiply(sp54, sp8C.m[0]);
                    matrix_column_3_scalar_multiply_2(sp50, sp8C.m[0]);

                    modelSetScale(arg1->model, arg1->model->scale * sp48);
                }
            }

            modelSetScale(arg1->model, arg1->model->scale * sp78);
            matrix_scalar_multiply(arg1->model->scale, sp8C.m[0]);

            if (arg1->flags & PROPFLAG_ONSCREEN)
            {
                sub_GAME_7F040BA0(arg1, &spE0, &sp8C, spDC, &sp80);
            }
            else
            {
                sub_GAME_7F04088C(arg1, &spE0, &sp8C, spDC, &sp80);
            }

            setupUpdateObjectRoomPosition(arg1);
            chrpropActivate(sp60);
            chrpropEnable(sp60);
        }
        #ifdef DEBUG
        else
        {
            osSyncPrintf("domakedefaultobj: prop obj number %d not reset!\n",cmdindex + 1);
        }
        #endif
    }
}

/**
 * NTSC address 0x7F002738.
 * PAL address 0x7F002738.
*/
void weaponAssignToHome(s32 arg0, WeaponObjRecord* weapon, s32 cmdindex)
{
    s32 padding;
    bool hastoken;
    ChrRecord* chr;
    bool giveweapon;
    s32 temp_a0;
    struct s_mp_weapon_set* weapon_set;

    if ((weapon->flags & PROPFLAG_ASSIGNEDTOCHR))
    {
        chr = chrFindByLiteralId(weapon->pad);

        if (chr && chr->prop && chr->model)
        {
            if (cheatIsActive(CHEAT_ENEMY_ROCKETS))
            {
                switch ((s8)weapon->weaponnum)
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
                        weapon->weaponnum = ITEM_ROCKETLAUNCH;
                        weapon->obj = PROP_CHRROCKETLAUNCH;
                        weapon->extrascale = 256;
                        break;
                }
            }

            weaponLoadProjectileModels((s8)weapon->weaponnum);
            sub_GAME_7F052030(weapon, chr);
        }
        #ifdef DEBUG
        else
        {
            osSyncPrintf("domakeweaponobj: no chr number %d for obj number %d!\n",weapon->pad, cmdindex + 1);
        }
        #endif
    }
    else
    {
        hastoken = 1;
        giveweapon = 1;

#ifdef GE_MODDED_CHEATS
        if (gamemode == GAMEMODE_MULTI && get_scenario() != SCENARIO_COOP)
#else
        if (getPlayerCount() >= 2)
#endif
        {
            lastmpweaponnum = -1;

            switch ((u8)weapon->weaponnum)
            {
                case ITEM_UNARMED + 0xF0:
                case ITEM_FIST + 0xF0:
                case ITEM_KNIFE + 0xF0:
                case ITEM_THROWKNIFE + 0xF0:
                case ITEM_WPPK + 0xF0:
                case ITEM_WPPKSIL + 0xF0:
                case ITEM_TT33 + 0xF0:
                case ITEM_SKORPION + 0xF0:
                    weapon_set = getPtrMPWeaponSetData();

                    temp_a0 = (u8)weapon->weaponnum - 0xF0;
                    lastmpweaponnum = temp_a0;

                    weapon->weaponnum = weapon_set[temp_a0].itemID;
                    weapon->obj = weapon_set[temp_a0].propID;
#if defined(VERSION_EU)
                    weapon->extrascale = (weapon_set[temp_a0].size16);
#else
                    weapon->extrascale = (weapon_set[temp_a0].size * 256.0f);
#endif

                    giveweapon = weapon_set[temp_a0].allowpickup;

                    break;

                case ITEM_TOKEN:

                    hastoken = 1;
                    giveweapon = 1;

                    if (get_scenario() != SCENARIO_TLD)
                    {
                        giveweapon = 0;
                    }
                    break;
            }
        }

        if ((weapon->weaponnum != ITEM_UNARMED) && giveweapon)
        {
            weaponLoadProjectileModels(weapon->weaponnum);
            domakedefaultobj(arg0, (struct ObjectRecord*)weapon, cmdindex);
        }
    }
}

//i should be object hat
void setupHat(s32 arg0, ObjectRecord* hat, s32 cmdindex)
{
    if (hat->flags & PROPFLAG_ASSIGNEDTOCHR) {
        ChrRecord* chr = chrFindByLiteralId(hat->pad);
        if (chr && chr->prop && chr->model) {
            hatAssignToChr(hat, chr);
        }
        #ifdef DEBUG
        else
        {
            osSyncPrintf("domakehatobj: no chr number %d for obj number %d!\n",hat->pad, cmdindex + 1);
        }
        #endif
    } else {
        domakedefaultobj(arg0, hat, cmdindex);
    }
}

//i should be object key
void setupKey(s32 arg0, ObjectRecord* key, s32 cmdindex)
{
    domakedefaultobj(arg0, key, cmdindex);
}


/**
 * NTSC address 0x7F002A3C.
*/
void setupCctv(s32 arg0, CCTVRecord *arg1, s32 cmdindex)
{
    struct coord3d *temp_a2;
    struct PadRecord *sp50;
    struct coord3d sp44;
    Mtxf *sp3C;

    domakedefaultobj(arg0, (struct ObjectRecord*)arg1, cmdindex);

    if (arg1->pad >= 0)
    {
        temp_a2 = (struct coord3d*)arg1->model->obj->Switches[0]->Data;

        if (isNotBoundPad(arg1->pad))
        {
            sp50 = &g_CurrentSetup.pads[arg1->pad];
        }
        else
        {
            sp50 = (struct PadRecord *)&g_CurrentSetup.boundpads[getBoundPadNum(arg1->pad)];
        }

        sp44.f[0] = temp_a2->f[0];
        sp44.f[1] = temp_a2->f[1];
        sp44.f[2] = temp_a2->f[2];

        mtx4RotateVecInPlace(&arg1->mtx, &sp44);

        sp3C = &arg1->unk84;

        sp44.f[0] += arg1->prop->pos.f[0];
        sp44.f[1] += arg1->prop->pos.f[1];
        sp44.f[2] += arg1->prop->pos.f[2];

        matrix_4x4_set_basis_and_position_target(sp3C, 0.0f, 0.0f, 0.0f, sp44.f[0] - sp50->pos.f[0], sp44.f[1] - sp50->pos.f[1], sp44.f[2] - sp50->pos.f[2], 0.0f, 1.0f, 0.0f);
        matrix_scalar_multiply(arg1->model->scale, sp3C->m[0]);

        if (arg1->convert_to_f32 == 0)
        {
            arg1->convert_to_f32 = 1;
            arg1->unkCC = (*(s32*)&arg1->unkCC * M_TAU_F) / 65536.0f;
            arg1->unkD0 = (*(s32*)&arg1->unkD0 * M_TAU_F) / 65536.0f;
            arg1->unkDC = (*(s32*)&arg1->unkDC * M_TAU_F) / 65536.0f;
            arg1->unkE8 = *(s32*)&arg1->unkE8;
        }

        arg1->unkD4 = 0;
        arg1->unkD8 = 0.0f;
        arg1->unkC8 = arg1->unkCC;
        arg1->unkC4 = atan2f(sp44.f[0] - sp50->pos.f[0], sp44.f[2] - sp50->pos.f[2]);
        arg1->timer = 0;
    }
}

void setupAutogun(s32 stageID, AutogunRecord *autogun, s32 cmdindex)
{
    s8 *beam;

    domakedefaultobj(stageID, (ObjectRecord *) autogun, cmdindex);

#ifdef VERSION_EU
    autogun->speed = ((*((s32 *) (&autogun->speed))) * 7.5398226f) / 65536.0f;
    autogun->aimdist = ((*((s32 *) (&autogun->aimdist))) * 100.0f) / 65536.0f;
    autogun->unk88 = ((*((s32 *) (&autogun->unk88))) * M_TAU_F) / 65536.0f;
    autogun->unk8C = ((*((s32 *) (&autogun->unk8C))) * M_TAU_F) / 65536.0f;
#endif

    autogun->unkAC = 0;
    autogun->unkB8 = -1;
    autogun->unkBC = -1;
    autogun->unkC0 = -1;
    autogun->unkC4 = 0;
    autogun->unkC8 = 0;
    autogun->unk90 = 0.0f;
    autogun->unk94 = 0.0f;
    autogun->rot_related = 0.0f;
    autogun->unk9C = 0.0f;
    autogun->unkA0 = 0.0f;
    autogun->unk98 = 0.0f;
    autogun->unkB0 = 0.0f;
    autogun->unkB4 = 0.0f;

#ifndef VERSION_EU
    autogun->speed = ((*((s32 *) (&autogun->speed))) * M_TAU_F) / 65536.0f;
    autogun->aimdist = ((*((s32 *) (&autogun->aimdist))) * 100.0f) / 65536.0f;
    autogun->unk88 = ((*((s32 *) (&autogun->unk88))) * M_TAU_F) / 65536.0f;
    autogun->unk8C = ((*((s32 *) (&autogun->unk8C))) * M_TAU_F) / 65536.0f;
#endif

    beam          = mempAllocBytesInBank(0x30U, MEMPOOL_STAGE);
    autogun->beam = beam;
    *beam = -1;

    autogun->is_active = FALSE;
    autogun->unkD4 = 0.0f;

    if (autogun->padID >= 0)
    {
        s32 stack1;
        f32 xdiff;
        f32 ydiff;
        f32 zdiff;
        PadRecord *pad;
        PropRecord *prop;

        if (autogun->padID < 0x2710)
        {
            if (1);
            pad = &g_CurrentSetup.pads[autogun->padID];
        }
        else
        {
            pad = &g_CurrentSetup.boundpads[getBoundPadNum(autogun->padID)];
        }

        prop = autogun->prop;

        xdiff = pad->pos.x - prop->pos.x;
        ydiff = pad->pos.y - prop->pos.y;
        zdiff = pad->pos.z - prop->pos.z;

        autogun->rot_related = atan2f(xdiff, zdiff);
        autogun->unk98 = atan2f(ydiff, sqrtf((xdiff * xdiff) + (zdiff * zdiff)));
    }
}


//i should be object rack
void setupHangingMonitors(s32 arg0, ObjectRecord* rack, s32 cmdindex)
{
    domakedefaultobj(arg0, rack, cmdindex);
}


void setupSingleMonitor(s32 stageID, MonitorObjRecord *monitor, s32 cmdindex)
{
    MonitorRecord *record;
    s32 unused;
    s32 modelnum;
    ObjectRecord *owner;
    PropRecord *prop;
    f32 scale;

    monitor->Monitor = g_MonitorAnimController;
    record = &monitor->Monitor;
    monitorSetImageByNum(&monitor->Monitor, monitor->ImageNum);

    if (monitor->pad < 0 && (monitor->flags & PROPFLAG_INSIDEANOTHEROBJ) == 0)
    {
        modelnum = monitor->obj;
        owner = (struct ObjectRecord *)setupGetPtrToCommandByIndex(cmdindex + monitor->OwnerOffset);

        modelLoad(modelnum);

        scale = monitor->extrascale * (1.0f / 256.0f);
        monitor->damage = *(s32*)&monitor->damage / M_U16_MAX_VALUE_F;

#ifdef GE_MODDED_CHEATS
        if (gamemode == GAMEMODE_MULTI && get_scenario() != SCENARIO_COOP)
#else
        if (getPlayerCount() >= 2)
#endif
        {
            monitor->state |= PROPSTATE_RESPAWN;
        }

        prop = objInitWithAutoModel((ObjectRecord*)monitor);
        monitor->embedment = embedmentAllocate();

        if (prop && monitor->embedment)
        {
            monitor->runtime_bitflags |= RUNTIMEBITFLAG_EMBEDDED;
            modelSetScale(monitor->model, monitor->model->scale * scale);
            monitor->model->attachedto = owner->model;

            if (monitor->OwnerPart == 0)
            {
                monitor->model->attachedto_objinst = owner->model->obj->Switches[0];
            }
            else if (monitor->OwnerPart == 1)
            {
                monitor->model->attachedto_objinst = owner->model->obj->Switches[1];
            }
            else if (monitor->OwnerPart == 2)
            {
                monitor->model->attachedto_objinst = owner->model->obj->Switches[2];
            }
            else
            {
                monitor->model->attachedto_objinst = owner->model->obj->Switches[3];;
            }

            chrpropReparent(prop, owner->prop);
            matrix_4x4_set_rotation_around_x(0.36651915f, (Mtxf*)&monitor->embedment->matrix);
            matrix_scalar_multiply(monitor->model->scale / owner->model->scale, (f32*)&monitor->embedment->matrix);
        }
    }
    else
    {
        domakedefaultobj(stageID, (ObjectRecord*)monitor, cmdindex);
    }

    if ((monitor->flags & PROPFLAG_MONITOR_RENDERPOSTBG) && monitor->prop)
    {
        monitor->prop->flags |= PROPFLAG_RENDERPOSTBG;
    }
}


void setupMultiMonitor(s32 stageID, MultiMonitorObjRecord* monitor, s32 cmdindex)
{
    monitor->Monitor[0] = g_MonitorAnimController;
    monitorSetImageByNum(&monitor->Monitor[0], monitor->ImageNums[0]);

    monitor->Monitor[1] = g_MonitorAnimController;
    monitorSetImageByNum(&monitor->Monitor[1], monitor->ImageNums[1]);

    monitor->Monitor[2] = g_MonitorAnimController;
    monitorSetImageByNum(&monitor->Monitor[2], monitor->ImageNums[2]);

    monitor->Monitor[3] = g_MonitorAnimController;
    monitorSetImageByNum(&monitor->Monitor[3], monitor->ImageNums[3]);

    domakedefaultobj(stageID, monitor, cmdindex);
}

void sub_GAME_7F00324C(struct BoundPadRecord *arg0, s32 *arg1, s32 *arg2, struct coord3d *arg3, struct coord3d *arg4)
{
    StandTile *sp4C;
    struct coord3d normal;
    s32 padding;
    struct coord3d center;
    StandTile *sp2C;
    f32 scale;

    sub_GAME_7F001BD4(arg0, &center);
    sp2C = (StandTile *)arg0->stan;

    if (walkTilesBetweenPoints_NoCallback(&sp2C, arg0->pos.f[0], arg0->pos.f[2], center.f[0], center.f[2]) == 0)
    {
        sp2C = (StandTile *)arg0->stan;
        center.f[0] = arg0->pos.f[0];
        center.f[1] = arg0->pos.f[1];
        center.f[2] = arg0->pos.f[2];
    }

    normal.f[0] = (arg0->up.f[1] * arg0->look.f[2]) - (arg0->up.f[2] * arg0->look.f[1]);
    normal.f[1] = (arg0->up.f[2] * arg0->look.f[0]) - (arg0->up.f[0] * arg0->look.f[2]);
    normal.f[2] = (arg0->up.f[0] * arg0->look.f[1]) - (arg0->up.f[1] * arg0->look.f[0]);

    scale = 1.0f / sqrtf(((normal.f[0] * normal.f[0]) + (normal.f[1] * normal.f[1])) + (normal.f[2] * normal.f[2]));
    sp4C = sp2C;

    normal.f[0] *= scale;
    normal.f[1] *= scale;
    normal.f[2] *= scale;

    arg3->f[0] = center.f[0] + (normal.f[0] * 50.0f);
    arg3->f[1] = center.f[1];
    arg3->f[2] = center.f[2] + (normal.f[2] * 50.0f);


    walkTilesBetweenPoints_NoCallback(&sp4C, center.f[0], center.f[2], arg3->f[0], arg3->f[2]);

    if (1);

    *arg1 = (s32) sp4C->room;
    sp4C = sp2C;

    arg4->f[0] = center.f[0] - (normal.f[0] * 50.0f);
    arg4->f[1] = center.f[1];
    arg4->f[2] = center.f[2] - (normal.f[2] * 50.0f);

    walkTilesBetweenPoints_NoCallback(&sp4C, center.f[0], center.f[2], arg4->f[0], arg4->f[2]);

    if (1);

    *arg2 = (s32) sp4C->room;

    if (*arg2 == *arg1)
    {
        *arg2 = -1;
    }
}


extern f32 g_DoorScale;
/**
 *
 * NTSC ADDRESS: 7F003480
 * PAL ADDRESS: 7F0033F0
 * Perfect Dark: void setupCreateDoor(struct doorobj *door, s32 cmdindex)
*/
void setupDoor(s32 arg0, struct DoorRecord *door, s32 arg2)
{
    s32 padding; // no sp
    s32 modelnum;
    struct BoundPadRecord *pad;
    StandTile *sp1C8_stan;
    PropRecord *prop;
    struct coord3d sp1B8;
    s32 portalnum; //sp1b4
    s32 sp1B0;
    s32 sp1AC;
    struct coord3d sp1A0;
    struct coord3d sp194;
    struct PortalMetric sp180;
    struct ModelRoData_BoundingBoxRecord *temp_v0;
    struct coord3d sp170;
    StandTile *sp16C;
    Mtxf sp12C;
    f32 temp_f2; // no sp
    ModelFileHeader *sp124;
    struct coord3d sp118;                           /* compiler-managed */
    StandTile *sp114_stan;
    Mtxf spD4;
    struct coord3d spC8;
    Mtxf sp88;
    struct coord3d sp7C;
    struct bbox bb2;
    f32 xscale;
    f32 yscale;
    f32 zscale;
    f32 scale;
    //StandTile *stan;
    u8 *padding2;

    modelnum = door->obj;

    portalnum = -1;
    sp1B0 = -1;
    sp1AC = -1;

    modelLoad(modelnum);

    pad = &(g_CurrentSetup.boundpads[door->pad]);

    if ((door->flags & PROPFLAG_CULL_BEHIND_DOOR) || (door->flags & PROPFLAG_NO_PORTAL_CLOSE))
    {
        sub_GAME_7F00324C(pad, &sp1B0, &sp1AC, &sp1A0, &sp194);

        if ((door->flags & PROPFLAG_CULL_BEHIND_DOOR) && (sp1B0 >= 0) && (sp1AC >= 0))
        {
            portalnum = bgGetPortalBetweenRooms(sp1B0, sp1AC, &sp1A0, &sp194);
        }
    }

    if (g_DoorScale != 1.0f)
    {
        if (portalnum >= 0)
        {
            sub_GAME_7F0B96CC(portalnum, &sp180);
            sp180.min *= get_room_data_float2();

            temp_f2 = (pad->pos.f[0] * sp180.normal.f[0]) + (pad->pos.f[1] * sp180.normal.f[1]) + (pad->pos.f[2] * sp180.normal.f[2]);

            if (g_DoorScale < 1.0f)
            {
                temp_f2 = (temp_f2 - sp180.min) * (1.0f - g_DoorScale);
                sp170.f[0] = pad->pos.f[0] - (sp180.normal.f[0] * temp_f2);
                sp170.f[1] = pad->pos.f[1] - (sp180.normal.f[1] * temp_f2);
                sp170.f[2] = pad->pos.f[2] - (sp180.normal.f[2] * temp_f2);
            }
            else
            {
                temp_f2 = (temp_f2 - sp180.min) * (g_DoorScale - 1.0f);
                sp170.f[0] = pad->pos.f[0] + (sp180.normal.f[0] * temp_f2);
                sp170.f[1] = pad->pos.f[1] + (sp180.normal.f[1] * temp_f2);
                sp170.f[2] = pad->pos.f[2] + (sp180.normal.f[2] * temp_f2);
            }

            sp16C = pad->stan;
            if (walkTilesBetweenPoints_NoCallback(&sp16C, pad->pos.f[0], pad->pos.f[2], sp170.f[0], sp170.f[2]) != 0)
            {
                pad->stan = sp16C;
                pad->pos.f[0] = sp170.f[0];
                pad->pos.f[1] = sp170.f[1];
                pad->pos.f[2] = sp170.f[2];
                pad->bbox.xmin *= g_DoorScale;
                pad->bbox.xmax *= g_DoorScale;
            }
 #ifdef DEBUG
            else
            {
                osSyncPrintf("volume for door object number %d did not have depth changed!\n",arg2 + 1);
            }
            #endif
        }
        else
        {
            pad->bbox.xmin *= g_DoorScale;
            pad->bbox.xmax *= g_DoorScale;
        }
    }

    if (getposstan(&pad->pos, pad->stan, 0.0f, &sp1B8, &sp1C8_stan) != 0)
    {
        matrix_4x4_set_basis_and_position_target(&sp12C, 0, 0, 0, -pad->look.f[0], -pad->look.f[1], -pad->look.f[2], pad->up.f[0], pad->up.f[1], pad->up.f[2]);
        sp124 = PitemZ_entries[modelnum].header;
        sp114_stan = sp1C8_stan;

        bb2.zmax = pad->bbox.xmin;
        bb2.zmin = pad->bbox.xmax; //78
        bb2.ymax = pad->bbox.ymin; //74
        bb2.ymin = pad->bbox.ymax; //70
        bb2.xmax = pad->bbox.zmin; //6c
        bb2.xmin = pad->bbox.zmax; //68

        matrix_4x4_set_rotation_around_x(M_HALF_PI, &spD4);
        matrix_4x4_set_rotation_around_z(M_HALF_PI, &sp88);
        matrix_4x4_multiply_in_place(&sp88, &spD4);
        matrix_4x4_multiply_in_place(&sp12C, &spD4);
        sub_GAME_7F001BD4(pad, &sp118);

        temp_v0 = (struct ModelRoData_BoundingBoxRecord *)sp124->RootNode->Child->Data;

        xscale = (bb2.ymin - bb2.ymax) / (temp_v0->Bounds.xmax - temp_v0->Bounds.xmin);
        yscale = (bb2.xmin - bb2.xmax) / (temp_v0->Bounds.ymax - temp_v0->Bounds.ymin);
        zscale = (bb2.zmin - bb2.zmax) / (temp_v0->Bounds.zmax - temp_v0->Bounds.zmin);

        if ((xscale <= 0.000001f) || (yscale <= 0.000001f) || (zscale <= 0.000001f))
        {
            #ifdef DEBUG
            osSyncPrintf("Scale warning: door object number %d has a small scale: %f,%f,%f\n",arg2 +1, xscale,yscale,zscale);
            #endif
            xscale =
                yscale =
                zscale = 1.0f;
        }

        matrix_column_1_scalar_multiply(xscale, spD4.m[0]);
        matrix_column_2_scalar_multiply(yscale, spD4.m[0]);
        matrix_column_3_scalar_multiply_2(zscale, spD4.m[0]);

        spC8.f[0] = sp118.f[0];
        spC8.f[1] = sp118.f[1];
        spC8.f[2] = sp118.f[2];

        if (!(door->flags2 & 1))
        {
            if (walkTilesBetweenPoints_NoCallback(&sp114_stan, sp1B8.f[0], sp1B8.f[2], sp118.f[0], sp118.f[2]) != 0)
            {
                sp1C8_stan = sp114_stan;
            }
            else
            {
                sp118.f[0] = sp1B8.f[0];
                sp118.f[2] = sp1B8.f[2];

                if (!(door->flags & 0x1000)) // prop flag PROPFLAG_00001000 "Absolute Position"
                {
                    #ifdef DEBUG
                    osSyncPrintf("door object number %d not positioned correctly!\n",arg2 +1);
                    #endif

                }
            }
        }
        else
        {
            sp118.f[0] = sp1B8.f[0];
            sp118.f[1] = sp1B8.f[1];
            sp118.f[2] = sp1B8.f[2];
        }

        if ((door->doorType == DOORTYPE_VERTICAL) || (door->doorType == DOORTYPE_FALLAWAY))
        {
            sp7C.f[0] = pad->look.f[0] * (bb2.xmin - bb2.xmax);
            sp7C.f[1] = pad->look.f[1] * (bb2.xmin - bb2.xmax);
            sp7C.f[2] = pad->look.f[2] * (bb2.xmin - bb2.xmax);
        }
        else
        {
            sp7C.f[0] = pad->up.f[0] * (bb2.ymax - bb2.ymin);
            sp7C.f[1] = pad->up.f[1] * (bb2.ymax - bb2.ymin);
            sp7C.f[2] = pad->up.f[2] * (bb2.ymax - bb2.ymin);
        }

        // These values are stored in the setup files as integers, but at
		// runtime they are floats. Hence reading a "float" as an integer,
		// converting it to a float and writing it back to the same property.
		door->maxFrac = *(s32 *) &door->maxFrac / 65536.0f;
		door->perimFrac = *(s32 *) &door->perimFrac / 65536.0f;
#if defined(VERSION_EU)
        door->accel = (*(s32 *) &door->accel) * 1.2f / 65536.0f;
		door->decel = (*(s32 *) &door->decel) * 1.2f / 65536.0f;
		door->maxSpeed = (*(s32 *) &door->maxSpeed) * 1.2f / 65536.0f;
#else
		door->accel = (*(s32 *) &door->accel) / 65536.0f;
		door->decel = (*(s32 *) &door->decel) / 65536.0f;
		door->maxSpeed = (*(s32 *) &door->maxSpeed) / 65536.0f;
#endif

        prop = doorInit(door, &sp118, &spD4, sp1C8_stan, &sp7C, &spC8);
        if (door->flags & PROPFLAG_CULL_BEHIND_DOOR)
        {
            door->portalNumber = portalnum;
            if ((portalnum >= 0) && (door->openPosition == 0.0f))
            {
                doorDeactivatePortal(door);
            }
            #ifdef DEBUG
            else
            {
                osSyncPrintf("No portal for door object number %d ",arg2 + 1);
            }
            #endif
        }

        prop->rooms[0] = prop->stan->room;
        chrpropRegisterRoom(prop, prop->stan->room);
        prop->rooms[1] = 0xFFU;
        prop->rooms[2] = 0xFFU;

        if ((door->flags & PROPFLAG_CULL_BEHIND_DOOR) || (door->flags & PROPFLAG_NO_PORTAL_CLOSE))
        {
            if (sp1B0 != prop->stan->room)
            {
                if (sp1B0 >= 0)
                {
                    prop->rooms[1] = sp1B0;
                    chrpropRegisterRoom(prop, sp1B0);
                }
            }
            else if (sp1AC >= 0)
            {
                prop->rooms[1] = sp1AC;
                chrpropRegisterRoom(prop, sp1AC);
            }

            if (prop->rooms[1] != 0xff && 1)
            {
                if (!prop->stan->room)
                {
                    #ifdef DEBUG
                        osSyncPrintf("3 rooms for door object number %d\n",arg2 + 1);
                    #endif
                }

            }
            #ifdef DEBUG
            else
            {
                osSyncPrintf("No 2nd room for door object number %d\n",arg2 + 1);
            }
            #endif
        }

        if (door->model != NULL)
        {
            scale = xscale;

            if (scale < yscale)
            {
                scale = yscale;
            }

            if (scale < zscale)
            {
                scale = zscale;
            }

            modelSetScale(door->model, door->model->scale * scale);
        }

        chrpropActivate(prop);
        chrpropEnable(prop);

        if (door->linkedDoorOffset != 0)
        {
            door->linkedDoor = (struct DoorRecord *)setupGetPtrToCommandByIndex(door->linkedDoorOffset + arg2);
        }
    }
    else
    {
        door->prop = NULL;
        #ifdef DEBUG
            osSyncPrintf("proplvreset: prop door object number %d not reset!\n",arg2 + 1);
        #endif
    }

}


// Perfect Dark void setupLoadFiles(s32 stagenum)
void proplvreset2(enum LEVELID stageId)
{
    ItemModelFileRecord *pitem;
    s32 withchrs;
    s32 withobjs;

    withchrs = (((void *) tokenFind(1, "-nochr")) == NULL) && (((void *) tokenFind(1, "-noprop")) == NULL);
    withobjs = (((void *) tokenFind(1, "-noobj")) == NULL) && (((void *) tokenFind(1, "-noprop")) == NULL);
    g_DoorScale = 1.0f;

    /**
     * Mark every prop model as "not resident" so the model loads later in this function actually fetch data. Essentially
     * RootNode is doubling as a loaded flag for function modelLoad().
     * 
     * The last entry in the PitemZ_entries table is a terminator which is why 1 is subtracted from the loop length.
     */
    for (pitem = PitemZ_entries; pitem < &PitemZ_entries[ARRAYCOUNT(PitemZ_entries) - 1]; pitem++)
    {
        pitem->header->RootNode = NULL;
    }

    if ((stageId <= (LEVELID_MAX + 1)) && setup_text_pointers[stageId])
    {
        char strResource[0x100] = ""; // Scratch buffer for synthesizing the setup file's name at runtime.
        s32 numAnimatedObjects = 0;
        s32 numObjects = 0;
        s32 i1 = 0;
        s32 i2 = 0;
        s32 i3 = 0;
        f32 roompos_1;
        s32 i5 = 0;
        s32 i8;
        f32 roompos_2;
        struct stagesetup *local_stage;

        strResource[0] = setup_text_pointers[stageId][0]; // 'U' -> "U"
        strResource[1] = 0; // Terminate so strcat has a valid string.

        /**
         * There are no slots for the mp stages in setup_text_pointers. The name is created
         * by adding "mp_" after the "U" e.g. "Ump_setuparchZ"
         */
#ifdef GE_MODDED_CHEATS
        /* R7: setup family follows the selected game mode, not viewport count.
         * Regular multiplayer must use Ump_* even with one player.  Co-Op must
         * keep the normal campaign Usetup* file even with multiple players. */
        if (gamemode == GAMEMODE_MULTI && get_scenario() != SCENARIO_COOP)
#else
        if (getPlayerCount() >= 2)
#endif
        {
            strcat(strResource, "mp_"); // -> "Ump_"
        }

        strcat(strResource, setup_text_pointers[stageId] + 1); // Add remaining text back U[mp_] + setupxxxZ

        g_ptrStageSetupFile = _fileNameLoadToBank(strResource, FILELOADMETHOD_DEFAULT, 256, MEMPOOL_STAGE);

        local_stage = g_ptrStageSetupFile;
        langLoadToAddr(langGetLangBankIndexFromStagenum(stageId));

        /**
         * The setup file stores every internal reference as a byte offset from the start of the file,
         * so rebase them all onto the RAM copy at local_stage.
         */
        g_CurrentSetup.pathwaypoints = (void *) (((u32) local_stage) + ((u32) local_stage->pathwaypoints));
        g_CurrentSetup.waypointgroups = (void *) (((u32) local_stage) + ((u32) local_stage->waypointgroups));
        g_CurrentSetup.intro = (void *) (((u32) local_stage) + ((u32) local_stage->intro));
        g_CurrentSetup.propDefs = (void *) (((u32) local_stage) + ((u32) local_stage->propDefs));
        g_CurrentSetup.patrolpaths = (void *) (((u32) local_stage) + ((u32) local_stage->patrolpaths));
        g_CurrentSetup.ailists = (void *) (((u32) local_stage) + ((u32) local_stage->ailists));
        g_CurrentSetup.pads = (void *) (((u32) local_stage) + ((u32) local_stage->pads));
        g_CurrentSetup.boundpads = (void *) (((u32) local_stage) + ((u32) local_stage->boundpads));

        // Pad names and bound names are optional. An offset of 0 means absent.
        if (local_stage->padnames != 0)
        {
            g_CurrentSetup.padnames = (void *) (((u32) local_stage) + ((u32) local_stage->padnames));
        }
        else
        {
            g_CurrentSetup.padnames = NULL;
        }

        if (local_stage->boundpadnames != 0)
        {
            g_CurrentSetup.boundpadnames = (void *) (((u32) local_stage) + ((u32) local_stage->boundpadnames));
        }
        else
        {
            g_CurrentSetup.boundpadnames = NULL;
        }

        if (g_CurrentSetup.pathwaypoints)
        {
            for (i1 = 0; g_CurrentSetup.pathwaypoints[i1].padID >= 0; i1++)
            {
                g_CurrentSetup.pathwaypoints[i1].neighbours = (void *) (((u32) g_CurrentSetup.pathwaypoints[i1].neighbours) + ((u32) local_stage));
            }
        }

        if (g_CurrentSetup.waypointgroups)
        {
            for (i2 = 0; g_CurrentSetup.waypointgroups[i2].neighbours; i2++)
            {
                g_CurrentSetup.waypointgroups[i2].neighbours = (void *) (((u32) g_CurrentSetup.waypointgroups[i2].neighbours) + ((u32) local_stage));
                g_CurrentSetup.waypointgroups[i2].waypoints = (void *) (((u32) g_CurrentSetup.waypointgroups[i2].waypoints) + ((u32) local_stage));
            }
        }

        // Convert ailist pointers a.k.a. Action Blocks from file-local to proper pointers
        {
            AIListRecord *ailists = g_CurrentSetup.ailists;
            if (ailists)
            {
                for (i3 = 0; g_CurrentSetup.ailists[i3].ailist != 0; i3++)
                {
                    g_CurrentSetup.ailists[i3].ailist = (void *) (((u32) g_CurrentSetup.ailists[i3].ailist) + ((u32) local_stage));
                }
            }
        }

        if (g_CurrentSetup.patrolpaths)
        {
            for (i3 = 0; g_CurrentSetup.patrolpaths[i3].waypoints != NULL; i3++)
            {
                g_CurrentSetup.patrolpaths[i3].waypoints = (void *) (((u32) g_CurrentSetup.patrolpaths[i3].waypoints) + ((u32) local_stage));

                for (i5 = 0; g_CurrentSetup.patrolpaths[i3].waypoints[i5] >= 0; i5++)
                {
                    // Empty
                }

                g_CurrentSetup.patrolpaths[i3].len = i5;
            }
        }

        if (g_CurrentSetup.pads)
        {
            struct PadRecord *pad;
    
            roompos_1 = get_room_data_float2();
            pad = g_CurrentSetup.pads;

            for (; pad->plink != NULL; pad++)
            {
                pad->plink = (void *) (((u32) local_stage) + ((u32) pad->plink));
                pad->pos.f[0] *= roompos_1;
                pad->pos.f[1] *= roompos_1;
                pad->pos.f[2] *= roompos_1;
        
#ifdef DEBUG
                {
                    s32 sret = init_pathtable_something(pad, pad->plink, &pad->stan);
                    if (sret == 0)
                    {
                        osSyncPrintf("pad number %d has no stan! (%s)\n", (s32)(pad - g_CurrentSetup.pads), pad->plink);
                    }
                    else if (sret == 2)
                    {
                        osSyncPrintf("pad number %d changed stan from %s to %s\n", (s32)(pad - g_CurrentSetup.pads), pad->plink, GetStanName(pad->stan));
                    }
                }
#else
                init_pathtable_something(pad, pad->plink, &pad->stan);
#endif

                if (1);
            }
        }

        if (g_CurrentSetup.boundpads)
        {
            struct BoundPadRecord *vol;

            roompos_2 = get_room_data_float2();
            vol = g_CurrentSetup.boundpads;
            
            for (; vol->plink != NULL; vol++)
            {
                /** Ugly matching hack. 
                *   TODO: investigate if there's a way to get rid of this.
                */
                if ((((u32) local_stage) ^ 0) + ((u32)vol->plink));

                vol->plink = (void *) (((u32) local_stage) + ((u32)vol->plink));
                vol->pos.f[0] *= roompos_2;
                vol->pos.f[1] *= roompos_2;
                vol->pos.f[2] *= roompos_2;
                vol->bbox.xmin *= roompos_2;
                vol->bbox.xmax *= roompos_2;
                vol->bbox.ymin *= roompos_2;
                vol->bbox.ymax *= roompos_2;
                vol->bbox.zmin *= roompos_2;
                vol->bbox.zmax *= roompos_2;

#ifdef DEBUG
                {
                    s32 sret = init_pathtable_something((struct PadRecord *) vol, vol->plink, &vol->stan);
                    if (sret == 0)
                    {
                        osSyncPrintf("vol number %d has no stan! (%s)\n", (s32)(vol - g_CurrentSetup.boundpads), vol->plink);
                    }
                    else if (sret == 2)
                    {
                        osSyncPrintf("vol number %d changed stan from %s to %s\n", (s32)(vol - g_CurrentSetup.boundpads), vol->plink, GetStanName(vol->stan));
                    }
                }
#else
                init_pathtable_something((struct PadRecord *) vol, vol->plink, &vol->stan);
#endif

                if (1);
            }
        }

        if (g_CurrentSetup.padnames)
        {
            for (i1 = 0; g_CurrentSetup.padnames[i1].p; i1++)
            {
                g_CurrentSetup.padnames[i1].p = (void *) (((u32) g_CurrentSetup.padnames[i1].p) + ((u32) local_stage));
            }
        }

        if (g_CurrentSetup.boundpadnames)
        {
            // Required for matching.
            if (g_CurrentSetup.ailists && g_CurrentSetup.ailists);

            for (i1 = 0; g_CurrentSetup.boundpadnames[i1].p; i1++)
            {
                g_CurrentSetup.boundpadnames[i1].p = (void *) (((u32) g_CurrentSetup.boundpadnames[i1].p) + ((u32) local_stage));
            }
        }

        // PD rejoins here

#ifdef GE_MODDED_CHEATS
        /* R27Q: capture pristine Dam donor records and repair preserved pad 111
         * after normal STAN resolution, before any props are instantiated. */
        levelModifiersOnSetupReady(stageId);

        /* V90: reproduce Zoinkity's original Citadel runtime handoff using
         * symbols and STAN names rather than 2005-era absolute RAM hooks. */
        citadelApplyMultiplayerSetupOverrides(stageId);

        /* Mirrored Levels transforms setup pads before any stage props/guards
         * are instantiated, so initial placement is born in mirrored space. */
        mirrorLevelsApplySetupIfNeeded();
#endif

        if (withchrs)
        {
            alloc_init_GUARDdata_entries(load_proptype(PROPDEF_GUARD));
            numAnimatedObjects += load_proptype(PROPDEF_GUARD);
            numObjects += load_proptype(PROPDEF_COLLECTABLE);
            numObjects += load_proptype(PROPDEF_KEY);
            numObjects += load_proptype(PROPDEF_HAT);
        }
        else
        {
            alloc_init_GUARDdata_entries(0); // chrmgrConfigure
        }

        if (withobjs)
        {
            // load std props for all stages
            numObjects += load_proptype(PROPDEF_DOOR);
            numObjects += load_proptype(PROPDEF_CCTV);
            numObjects += load_proptype(PROPDEF_AUTOGUN);
            numObjects += load_proptype(PROPDEF_RACK);
            numObjects += load_proptype(PROPDEF_MONITOR);
            numObjects += load_proptype(PROPDEF_MULTI_MONITOR);
            numObjects += load_proptype(PROPDEF_ARMOUR);
            numObjects += load_proptype(PROPDEF_PROP);
            numObjects += load_proptype(PROPDEF_GLASS);
            numObjects += load_proptype(PROPDEF_TINTED_GLASS);
            numObjects += load_proptype(PROPDEF_SAFE);
            numObjects += load_proptype(PROPDEF_UNK41);
            numObjects += load_proptype(PROPDEF_GAS_RELEASING);
            numObjects += load_proptype(PROPDEF_ALARM);
            numObjects += load_proptype(PROPDEF_MAGAZINE);
            numObjects += load_proptype(PROPDEF_AMMO);
            numObjects += load_proptype(PROPDEF_VEHICHLE);
            numObjects += load_proptype(PROPDEF_TANK);
            numAnimatedObjects += load_proptype(PROPDEF_AIRCRAFT);
        }

#ifdef GE_MODDED_CHEATS
        if (withobjs)
            numObjects += levelModifiersGetReservedObjectCount(stageId);

        /* Reserve one animated-model slot per configured Simulant. P10
         * primes these slots during level-reset allocation so each receives
         * exact-size rwdata that can be reused by live spawn and respawn. */
        if (gamemode == GAMEMODE_MULTI && get_scenario() != SCENARIO_COOP)
            numAnimatedObjects += modMpBotsGetCount();
#endif

        modelmgrAllocateModelSlots(numObjects);
        modelmgrAllocateAnimModelSlots(numAnimatedObjects);

        for (i8 = 0; i8 < getPlayerCount(); i8++)
        {
            set_cur_player(i8);
            alloc_additional_item_slots(load_proptype(PROPDEF_LINK));
        }

        if (g_CurrentSetup.propDefs)
        {
            PropDefHeaderRecord *phead;
            s32 flags;
            s32 pdefIndex;

            // per-difficulty "Don't Load" mask: PROPFLAG2_00000010/20/40 for Agent/Secret/00
            flags = 1 << (lvlGetSelectedDifficulty() + 4);

            /**
             * Complete the skip loading mask started on the line above. Checks for:
             * - don't load on 2 players
             * - don't load on 3 players
             * - don't load on 4 players
             * - don't load in multiplayer
             */
#ifdef GE_MODDED_CHEATS
            if (gamemode == GAMEMODE_MULTI && get_scenario() != SCENARIO_COOP)
#else
            if (getPlayerCount() >= 2)
#endif
            {
                flags |= 1 << (getPlayerCount() + 20);
            }

            phead = g_CurrentSetup.propDefs;
            pdefIndex = 0;

            while ((i1 = phead->type) != PROPDEF_END)
            {
                switch (phead->type)
                {
                    case PROPDEF_GUARD_ATTRIBUTE:
                    {
                        GuardAttributeRecord *pdef_guarda;
                        u8 prob;
                        ChrRecord *chr;
                        pdef_guarda = (GuardAttributeRecord *) phead;
                        prob = (u8) pdef_guarda->GrenadeProb;
                        chr = chrFindByLiteralId(pdef_guarda->chrnum);
                        if ((chr && chr->prop) && chr->model)
                        {
                            chr->grenadeprob = prob;
                        }
#ifdef DEBUG
                        else
                        {
                            osSyncPrintf("grenade prob: no chr number %d for obj number %d! ", pdef_guarda->GrenadeProb, pdefIndex + 1);
                        }
#endif
                        break;
                    }
                    case PROPDEF_GUARD:
                        if (withchrs)
                        {
                            expand_09_characters(stageId, (struct GuardRecord *) phead, pdefIndex);
                        }
                        break;
                    case PROPDEF_DOOR:
                        if (withobjs && (!(((struct DoorRecord *) phead)->flags2 & flags)))
                        {
                            setupDoor(stageId, (struct DoorRecord *) phead, pdefIndex);
                        }
                        break;
                    case PROPDEF_DOOR_SCALE:
                        g_DoorScale = ((struct GlobalDoorScaleRecord *) phead)->Scale / M_U16_MAX_VALUE_F;
                        break;
                    case PROPDEF_COLLECTABLE:
                        if (withchrs && (!(((struct WeaponObjRecord *) phead)->flags2 & flags)))
                        {
                            weaponAssignToHome(stageId, (struct WeaponObjRecord *) phead, pdefIndex);
                        }
                        break;
                    case PROPDEF_KEY:
                        if (withchrs && (!(((struct KeyRecord *) phead)->flags2 & flags)))
                        {
                            setupKey(stageId, (struct ObjectRecord *) phead, pdefIndex);
                        }
                        break;
                    case PROPDEF_HAT:
                        if (withchrs && (!(((struct ObjectRecord *) phead)->flags2 & flags)))
                        {
                            setupHat(stageId, (struct ObjectRecord *) phead, pdefIndex);
                        }
                        break;
                    case PROPDEF_CCTV:
                        if (withobjs && (!(((struct CCTVRecord *) phead)->flags2 & flags)))
                        {
                            setupCctv(stageId, (struct CCTVRecord *) phead, pdefIndex);
                        }
                        break;
                    case PROPDEF_AUTOGUN:
                        if (withobjs && (!(((struct AutogunRecord *) phead)->flags2 & flags)))
                        {
                            setupAutogun(stageId, (struct AutogunRecord *) phead, pdefIndex);
                        }
                        break;
                    case PROPDEF_RACK:
                        if (withobjs && (!(((struct ObjectRecord *) phead)->flags2 & flags)))
                        {
                            setupHangingMonitors(stageId, (struct ObjectRecord *) phead, pdefIndex);
                        }
                        break;
                    case PROPDEF_MONITOR:
                        if (withobjs && (!(((struct MonitorObjRecord *) phead)->flags2 & flags)))
                        {
                            setupSingleMonitor(stageId, (struct MonitorObjRecord *) phead, pdefIndex);
                        }
                        break;
                    case PROPDEF_MULTI_MONITOR:
                        if (withobjs && (!(((struct MultiMonitorObjRecord *) phead)->flags2 & flags)))
                        {
                            setupMultiMonitor(stageId, (struct MultiMonitorObjRecord *) phead, pdefIndex);
                        }
                        break;
                    case PROPDEF_ARMOUR:
                    {
                        struct BodyArmourRecord *pdef_ba = (struct BodyArmourRecord *) phead;
#ifndef VERSION_US
                        if (withobjs && (((pdef_ba->flags2 & flags) == 0) || j_text_trigger)) // JP: armour setup also proceeds when j_text_trigger is set
#else
                        if (withobjs && ((pdef_ba->flags2 & flags) == 0))
#endif
                        {
                            pdef_ba->initialamount = (*((s32 *) (&pdef_ba->initialamount))) / M_U16_MAX_VALUE_F;
                            pdef_ba->amount = pdef_ba->initialamount;
                            domakedefaultobj(stageId, (struct ObjectRecord *) phead, pdefIndex);
#ifdef GE_MODDED_CHEATS
                            if (g_ModDisableBodyArmorEnabled && pdef_ba->prop != NULL)
                                chrpropDisable(pdef_ba->prop);
#endif
                        }
                        break;
                    }
                    case PROPDEF_TINTED_GLASS:
                    {
                        if (withobjs && (!(((struct TintedGlassRecord *) phead)->flags2 & flags)))
                        {
                            if (((struct TintedGlassRecord *) phead)->flags & PROPFLAG_GLASS_HASPORTAL)
                            {
                                if (!(((struct TintedGlassRecord *) phead)->pad < 10000))
                                {
                                    struct coord3d up;
                                    struct coord3d up2;
                                    BoundPadRecord *pad3d;

                                    pad3d = &g_CurrentSetup.boundpads[((struct TintedGlassRecord *) phead)->pad - 10000];
                                    sub_GAME_7F001BD4(pad3d, &up);
                                    up2.x = (10.0f * pad3d->up.x) + up.x;
                                    up2.y = (10.0f * pad3d->up.y) + up.y;
                                    up2.z = (10.0f * pad3d->up.z) + up.z;
                                    up.x -= 10.0f * pad3d->up.x;
                                    up.y -= 10.0f * pad3d->up.y;
                                    up.z -= 10.0f * pad3d->up.z;

                                    ((struct TintedGlassRecord *) phead)->portalnum = sub_GAME_7F0B9E04(&up, &up2);
                                    ((struct TintedGlassRecord *) phead)->unk90 = (*((s32 *) (&((struct TintedGlassRecord *) phead)->unk90))) / M_U16_MAX_VALUE_F;
                                }
                            }
                            domakedefaultobj(stageId, (struct ObjectRecord *) phead, pdefIndex);
                        }
                        break;
                    }
                    case PROPDEF_PROP:
                    case PROPDEF_ALARM:
                    case PROPDEF_MAGAZINE:
                    case PROPDEF_GAS_RELEASING:
                    case PROPDEF_UNK41:
                    case PROPDEF_GLASS:
                    case PROPDEF_SAFE:
                        if (withobjs && (!(((ObjectRecord *) phead)->flags2 & flags)))
                        {
                            domakedefaultobj(stageId, (struct ObjectRecord *) phead, pdefIndex);
                        }
                        break;
                    case PROPDEF_AMMO:
                    {
                        struct MultiAmmoCrateRecord *pdef_macr = (struct MultiAmmoCrateRecord *) phead;
                        s32 ammoqty = 1;
                        s32 i9;

#ifdef GE_MODDED_CHEATS
                        if (gamemode == GAMEMODE_MULTI && get_scenario() != SCENARIO_COOP)
#else
                        if (getPlayerCount() >= 2)
#endif
                        {
                            struct s_mp_weapon_set *mpweapon = &getPtrMPWeaponSetData()[lastmpweaponnum];
                            
                            ammoqty = mpweapon->ammoamount;
                            if (mpweapon->ammotype);
                            pdef_macr->slots[mpweapon->ammotype - 1].quantity = ammoqty;
                        }

                        if (((ammoqty > 0) && withobjs) && (!(pdef_macr->flags2 & flags)))
                        {
                            for (i9 = 0; i9 < AMMOTYPE_GLOBAL_MAX; i9++)
                            {
                                if ((pdef_macr->slots[i9].quantity > 0) && (pdef_macr->slots[i9].modelnum != 0xFFFF))
                                {
                                    modelLoad(pdef_macr->slots[i9].modelnum);
                                }
                            }

                            domakedefaultobj(stageId, (struct ObjectRecord *) pdef_macr, pdefIndex);
                        }
                        break;
                    }
                    case PROPDEF_TANK:
                        if (withobjs && (!(((struct TankRecord *) phead)->flags2 & flags)))
                        {
                            struct TankRecord *pdef_tank = (struct TankRecord *) phead;
                            struct PropRecord *tank_prop;

                            s32 padding;
                            f32 stan_y = 0.0f;
                            s32 paddinggg[4];

                            weaponLoadProjectileModels(ITEM_TANKSHELLS);
                            domakedefaultobj(stageId, (struct ObjectRecord *) pdef_tank, pdefIndex);
                            pdef_tank->turret_vertical_angle = 0.0f;
                            pdef_tank->turret_orientation_angle = 0.0f;
                            pdef_tank->tank_orientation_angle = M_TAU_F - atan2f(pdef_tank->mtx.m[2][0], pdef_tank->mtx.m[2][2]);
                            tank_prop = pdef_tank->prop;

                            if (tank_prop)
                            {
                                stan_y = stanGetPositionYValue(tank_prop->stan, tank_prop->pos.f[0], tank_prop->pos.f[2]);
                            }

                            pdef_tank->stan_y = stan_y;
#ifdef VERSION_EU
                            pdef_tank->unkD0 = stan_y / 0.2004f; // EU-tuned constant
#else
                            pdef_tank->unkD0 = stan_y / 0.17000002f;
#endif
                        }
                        break;
                    case PROPDEF_VEHICHLE:
                        if (withobjs && (!(((struct VehichleRecord *) phead)->flags2 & flags)))
                        {
                            struct VehichleRecord *pdef_veh = (struct VehichleRecord *) phead;

                            domakedefaultobj(stageId, (struct ObjectRecord *) pdef_veh, pdefIndex);

                            if (pdef_veh->model != NULL)
                            {
                                if (pdef_veh->model->obj->Switches[5] != NULL)
                                {
                                    modelGetNodeRwData(pdef_veh->model, pdef_veh->model->obj->Switches[5])->Raw.unk00 = (pdef_veh->flags & 0x10000000) == 0;
                                }
                            }

                            pdef_veh->speed        = 0.0f;
                            pdef_veh->wheelxrot    = 0.0f;
                            pdef_veh->wheelyrot    = 0.0f;
                            pdef_veh->speedaim     = 0.0f;
                            pdef_veh->turnrot60    = 0.0f;
                            pdef_veh->roty         = 0.0f;
                            pdef_veh->speedtime60  = -1.0f;
                            pdef_veh->ailist       = ailistFindById(pdef_veh->ailist);
                            pdef_veh->aioffset     = 0;
                            pdef_veh->aireturnlist = -1;
                            pdef_veh->path         = 0;
                            pdef_veh->nextstep     = 0;
                            pdef_veh->Sound        = 0;
                        }
                        break;
                    case PROPDEF_AIRCRAFT:
                        if (withobjs && (!(((struct AircraftRecord *) phead)->flags2 & flags)))
                        {
                            struct AircraftRecord *pdef_air = (struct AircraftRecord *) phead;

                            domakedefaultobj(stageId, (struct ObjectRecord *) pdef_air, pdefIndex);
                            pdef_air->speed           = 0.0f;
                            pdef_air->speedaim        = 0.0f;
                            pdef_air->rotoryrot       = 0.0f;
                            pdef_air->rotaryspeed     = 0.0f;
                            pdef_air->rotaryspeedaim  = 0.0f;
                            pdef_air->yrot            = 0.0f;
                            pdef_air->speedtime60     = -1.0f;
                            pdef_air->rotaryspeedtime = -1.0f;
                            pdef_air->ailist          = ailistFindById(pdef_air->ailist);
                            pdef_air->aioffset        = 0;
                            pdef_air->aireturnlist    = -1;
                            pdef_air->nextstep        = 0;
                            pdef_air->path            = 0;
                            pdef_air->Sound           = 0;
                        }
                        break;
                    case PROPDEF_TAG:
                    {
                        struct TagObjectRecord *pdef_tag;
                        struct ObjectRecord *taggedobj;

                        pdef_tag = (struct TagObjectRecord *) phead;
                        taggedobj = setupCommandGetObject(stageId, pdefIndex + ((s32) pdef_tag->OffsetToObj));
                        pdef_tag->TaggedObject = taggedobj;

                        if (taggedobj)
                        {
                            taggedobj->runtime_bitflags |= RUNTIMEBITFLAG_TAGGED;
                        }

                        set_parent_cur_tag_entry(pdef_tag);
                        break;
                    }
                    case PROPDEF_RENAME:
                    {
                        struct RenameObjectRecord *pdef_ren;
                        struct ObjectRecord *targetobj;

                        pdef_ren = (struct RenameObjectRecord *) phead;
                        i3 = pdef_ren->TagID + pdefIndex;
                        targetobj = setupCommandGetObject(stageId, i3);
                        pdef_ren->renobj = targetobj;

                        if (targetobj)
                        {
                            targetobj->runtime_bitflags |= RUNTIMEBITFLAG_DESTROYED;
                        }

                        bondinvAddTextOverride((struct textoverride *) pdef_ren);
                        break;
                    }
                    case PROPDEF_WATCH_MENU_OBJECTIVE_TEXT:
                        setup_briefing_text_entry_parent((struct setup_objective_text *) phead);
                        break;
                    case PROPDEF_CAMERAPOS:
                    {
                        struct CutsceneRecord *pdef_cam = (struct CutsceneRecord *) phead;

                        pdef_cam->pos.f[0] = (*((s32 *) (&pdef_cam->pos.f[0]))) / 100.0f;
                        pdef_cam->pos.f[1] = (*((s32 *) (&pdef_cam->pos.f[1]))) / 100.0f;
                        pdef_cam->pos.f[2] = (*((s32 *) (&pdef_cam->pos.f[2]))) / 100.0f;
                        pdef_cam->theta = (*((s32 *) (&pdef_cam->theta))) / M_U16_MAX_VALUE_F;
                        pdef_cam->verta = (*((s32 *) (&pdef_cam->verta))) / M_U16_MAX_VALUE_F;
                        break;
                    }
                    case PROPDEF_OBJECTIVE_START:
                        add_ptr_to_objective((struct objective_entry *) phead);
                        break;
                    case PROPDEF_OBJECTIVE_ENTER_ROOM:
                        set_parent_cur_obj_enter_room((struct criteria_roomentered *) phead);
                        break;
                    case PROPDEF_OBJECTIVE_DEPOSIT_OBJECT_IN_ROOM:
                        set_parent_cur_obj_deposited_in_room((struct criteria_deposit *) phead);
                        break;
                    case PROPDEF_OBJECTIVE_PHOTOGRAPH:
                        set_parent_cur_obj_photograph((struct criteria_picture *) phead);
                        break;
                }

                phead = (PropDefHeaderRecord *) (((u32 *) phead) + sizepropdef(phead));
                pdefIndex++;
            }

            phead = g_CurrentSetup.propDefs;
            pdefIndex = 0;

            while (phead->type != PROPDEF_END)
            {
                switch (phead->type)
                {
                    case PROPDEF_PROP:
                    case PROPDEF_KEY:
                    case PROPDEF_MAGAZINE:
                    case PROPDEF_COLLECTABLE:
                    case PROPDEF_MONITOR:
                    case PROPDEF_AMMO:
                    case PROPDEF_ARMOUR:
                    case PROPDEF_GAS_RELEASING:
                    case PROPDEF_UNK41:
                    case PROPDEF_GLASS:
                    case PROPDEF_SAFE:
                    case PROPDEF_TINTED_GLASS:
                    {
                        struct ObjectRecord *pdef_obj = (struct ObjectRecord *) phead;

                        if (pdef_obj->prop && (pdef_obj->flags & PROPFLAG_INSIDEANOTHEROBJ))
                        {
                            u32 offset = pdef_obj->pad;
                            struct ObjectRecord *inobj = setupCommandGetObject(stageId, offset + pdefIndex);

                            if (inobj && inobj->prop)
                            {
                                pdef_obj->runtime_bitflags |= RUNTIMEBITFLAG_HASOWNER;
                                modelSetScale(pdef_obj->model, pdef_obj->model->scale);
                                chrpropReparent(pdef_obj->prop, inobj->prop);
                            }

#ifdef DEBUG
                            //possibly wrong place
                            else
                            {
                                osSyncPrintf("inobj link not found for object number %d\n", pdefIndex + 1);
                            }
#endif

                        }
                        break;
                    }
                    case PROPDEF_LINK:
                    {
                        struct LinkRecord *pdef_link = (struct LinkRecord *) phead;
                        struct WeaponObjRecord *guna = (struct WeaponObjRecord *) setupGetPtrToCommandByIndex(pdef_link->Index1 + pdefIndex);
                        struct WeaponObjRecord *gunb = (struct WeaponObjRecord *) setupGetPtrToCommandByIndex(pdef_link->Index2 + pdefIndex);

                        if (guna && gunb)
                        {
                            if ((guna->type == PROPDEF_COLLECTABLE) && (gunb->type == PROPDEF_COLLECTABLE))
                            {
                                propweaponSetDual(guna, gunb);
                            }
#ifdef DEBUG
                            else
                            {
                                osSyncPrintf("link type wrong for doublegun object number %d\n", pdefIndex + 1);
                            }
                        }
                        else
                        {
                            osSyncPrintf("link not found for doublegun object number %d\n", pdefIndex + 1);
#endif

                        }

                        break;
                    }
                    case PROPDEF_SWITCH:
                    {
                        struct LinkRecord *pdef_switch;
                        struct ObjectRecord *doorA;
                        struct ObjectRecord *doorB;
                        s32 index1;
                        s32 index2;

                        pdef_switch = (struct LinkRecord *) phead;
                        index1 = pdef_switch->Index1;
                        index2 = pdef_switch->Index2;
                        doorA = (struct ObjectRecord *) setupCommandGetObject(stageId, pdefIndex + index1);
                        doorB = (struct ObjectRecord *) setupGetPtrToCommandByIndex(pdefIndex + index2);

                        if ((((doorA && doorA->prop) && doorB) && (doorB->type == PROPDEF_DOOR)) && doorB->prop)
                        {
                            pdef_switch->first = doorA->prop;
                            pdef_switch->second = doorB->prop;
                            initSetLevelLoadPropSwitch(pdef_switch);
                            doorA->runtime_bitflags |= RUNTIMEBITFLAG_00000001; // linked door
                        }

#ifdef DEBUG
                        else
                        {
                            osSyncPrintf("doorlink object number %d not initialised\n", pdefIndex + 1);
                        }
#endif

                        break;
                    }
                    case PROPDEF_SAFE_ITEM:
                    {
                        s32 index1;
                        struct SafeObjectRecord *pdef_safe;
                        s32 index2;
                        s32 index3;
                        struct ObjectRecord *safe_item;
                        struct SafeRecord *safe;
                        struct DoorRecord *door;

                        pdef_safe = (struct SafeObjectRecord *) phead;
                        index1 = pdef_safe->Index1;
                        index2 = pdef_safe->Index2;
                        index3 = pdef_safe->Index3;
                        safe_item = setupCommandGetObject(stageId, pdefIndex + index1);
                        safe = (struct SafeRecord *) setupCommandGetObject(stageId, pdefIndex + index2);
                        door = (struct DoorRecord *) setupCommandGetObject(stageId, pdefIndex + index3);

                        if (((((((safe_item && safe_item->prop) && safe) && safe->prop) && (safe->type == PROPDEF_SAFE)) && door) && door->prop) && (door->type == PROPDEF_DOOR))
                        {
                            pdef_safe->item = safe_item;
                            pdef_safe->safe = safe;
                            pdef_safe->door = door;
                            initSetLevelLoadPropSafeItem((struct ObjectRecord *) pdef_safe);
                            safe_item->flags2 |= PROPFLAG2_LINKEDTOSAFE;
                            door->flags2 |= PROPFLAG2_LINKEDTOSAFE;
                        }
#ifdef DEBUG
                        else
                        {
                            osSyncPrintf("safelink object number %d not initialised\n", pdefIndex + 1);
                        }
#endif
                        break;
                    }
                    case PROPDEF_LOCK_DOOR:
                    {
                        struct LockDoorRecord *pdef_lock_door;
                        struct DoorRecord *door;
                        struct ObjectRecord *lock;
                        s32 index1;
                        s32 index2;

                        pdef_lock_door = (struct LockDoorRecord *) phead;

                        index1 = pdef_lock_door->Index1;
                        index2 = pdef_lock_door->Index2;

                        door = (struct DoorRecord *) setupCommandGetObject(stageId, pdefIndex + index1);
                        lock = setupCommandGetObject(stageId, pdefIndex + index2);

                        if ((((door && door->prop) && lock) && lock->prop) && (door->type == PROPDEF_DOOR))
                        {
                            pdef_lock_door->door = door;
                            pdef_lock_door->lock = lock;
                            initSetLevelLoadPropLockDoor(pdef_lock_door);
                            door->runtime_bitflags |= RUNTIMEBITFLAG_PADLOCKEDDOOR;
                        }
#ifdef DEBUG
                        else
                        {
                            osSyncPrintf("doorlock object number %d not initialised\n", pdefIndex + 1);
                        }
#endif
                        break;
                    }
                }

                phead = (PropDefHeaderRecord *) (((u32 *) phead) + sizepropdef(phead));
                pdefIndex += 1;
            }
        }

#ifdef GE_MODDED_CHEATS
        if (withobjs)
            levelModifiersOnPropsLoaded(stageId);
#endif
    }
    else
    {
        g_CurrentSetup.pathwaypoints = NULL;
        g_CurrentSetup.waypointgroups = NULL;
        g_CurrentSetup.intro = 0;
        g_CurrentSetup.propDefs = 0;
        g_CurrentSetup.patrolpaths = NULL;
        g_CurrentSetup.ailists = NULL;
        g_CurrentSetup.pads = NULL;
        g_CurrentSetup.boundpads = NULL;
        g_CurrentSetup.padnames = NULL;
        g_CurrentSetup.boundpadnames = NULL;
        alloc_init_GUARDdata_entries(0);
        modelmgrAllocateModelSlots(0);
        modelmgrAllocateAnimModelSlots(0);
    }

    alloc_false_GUARDdata_to_exec_global_action();
}
