#include <ultra64.h>
#include <bondconstants.h>
#include <fr.h>
#include <joy.h>
#include <math.h>
#include <music.h>
#include <random.h>
#include <string.h>
#include "options.h"
#include "file.h"
#include "file2.h"
#include "front.h"
#include "gun.h"
#include "image.h"
#include "othermodemicrocode.h"
#include "libultra/io/controller.h"
#include "lv.h"
#include "dyn.h"
#include "mapmaker.h"
#include "memp.h"
#include "bondview.h"
#include "bg.h"
#include "bgfog.h"
#include "player.h"
#ifdef GE_MODDED_CHEATS
#include "mirroredlevels.h"
#endif

#ifdef GE_MAP_MAKER

#define MM_CELL_SIZE       160
#define MM_LAYER_HEIGHT    120
#define MM_FLOOR_HEIGHT      1
#define MM_WALL_HEIGHT     120
#define MM_WALL_THICKNESS    1
#define MM_GRID_MIN       (-128)
#define MM_GRID_MAX         127
#define MM_GRID_NEAR_RADIUS  12
#define MM_GRID_MID_RADIUS   32
#define MM_GRID_FAR_RADIUS   64
#define MM_EDITOR_DRAW_RADIUS 10400.0f
#define MM_NATIVE_DRAW_RADIUS 4200.0f
#define MM_MARKER_DRAW_RADIUS 3200.0f
#define MM_COLLISION_COARSE_RADIUS 180.0f
#define MM_RENDER_VTX_RESERVE 4096
#define MM_RENDER_GFX_RESERVE 384
#define MM_STICK_THRESHOLD  42
#define MM_MOVE_REPEAT       8
#define MM_GRID_LINE_WIDTH   3
#define MM_PLAYER_RADIUS     24.0f
#define MM_PLAYER_EYE_HEIGHT 62.0f
#define MM_PLAYER_STEP       30.0f
#define MM_PLAYER_SPEED      7.0f
#define MM_PLAYER_GRAVITY    2.2f
#define MM_PLAYER_TERMINAL   28.0f
#define MM_SAVE_BYTES        2560
#define MM_PAK_MAX_PAGES      120
#define MM_PAK_MAX_BYTES      (MM_PAK_MAX_PAGES * BLOCKSIZE * PFS_ONE_PAGE)
#define MM_MUSIC_RANDOM       (-1)
#define MM_TEXTURE_POOL_BYTES 0x28000
#define MM_FREE_FLY_SPEED     12.0f
#define MM_FREE_FLY_VERTICAL  10.0f
#define MM_FREE_LOOK_SPEED    0.035f
#define MM_SAVE_PAGES        (MM_SAVE_BYTES / (BLOCKSIZE * PFS_ONE_PAGE))
#define MM_PFS_COMPANY       0x4745
#define MM_PFS_GAME_CODE     0x47454d50u
#define MM_PFS_EOF           1

/*
 * Map Maker bulk storage must NOT live in the retail .bss arena. GoldenEye's
 * MEMPOOL_TOTAL begins at _bssSegmentEnd and ends at the fixed stack block; a
 * ~230 KiB editor BSS allocation silently steals that much stage heap from
 * every normal level even when the editor is never opened. In physical Plus
 * builds Map Maker instead borrows the Expansion-Pak weapon-texture scratch
 * while the front end owns the machine. Stage loading may reuse that window,
 * so a magic word detects invalidation and resets the resident editor map on
 * the next entry rather than trusting overwritten data.
 */
#define MAPMAKER_SCRATCH_MAGIC 0x4d4d5331u /* 'MMS1' */
extern u8 _mapMakerScratchStart[];
extern u8 _mapMakerScratchEnd[];

typedef struct MapMakerScratch {
    u32 magic;
    u32 pad;
    MapMakerModuleInstance modules[MAPMAKER_MAX_MODULES];
    MapMakerAdvancedMesh advanced_meshes[MAPMAKER_ADV_MAX_MESHES];
    MapMakerAdvancedLoop advanced_loops[MAPMAKER_ADV_MAX_LOOPS];
    MapMakerAdvancedVertex advanced_vertices[MAPMAKER_ADV_MAX_VERTICES];
    MapMakerAdvancedPortal advanced_portals[MAPMAKER_ADV_MAX_PORTALS];
    MapMakerEntityInstance entities[MAPMAKER_MAX_ENTITIES];
    MapMakerPlayerStart player_starts[MAPMAKER_MAX_PLAYER_STARTS];
    u64 texture_buffer[MM_TEXTURE_POOL_BYTES / sizeof(u64)];
} MapMakerScratch;
typedef char MapMakerScratchFitsBorrowedWindow[(sizeof(MapMakerScratch) <= 0x40000) ? 1 : -1];

#define g_MapMakerScratch ((MapMakerScratch *)_mapMakerScratchStart)
#define g_MapMakerModules (g_MapMakerScratch->modules)
#define g_MapMakerAdvancedMeshes (g_MapMakerScratch->advanced_meshes)
#define g_MapMakerAdvancedLoops (g_MapMakerScratch->advanced_loops)
#define g_MapMakerAdvancedVertices (g_MapMakerScratch->advanced_vertices)
#define g_MapMakerAdvancedPortals (g_MapMakerScratch->advanced_portals)
#define g_MapMakerEntities (g_MapMakerScratch->entities)
#define g_MapMakerPlayerStarts (g_MapMakerScratch->player_starts)
#define g_MapMakerTextureBuffer (g_MapMakerScratch->texture_buffer)
static s32 g_MapMakerModuleCount;
static s32 g_MapMakerCursorX;
static s32 g_MapMakerCursorY;
static s32 g_MapMakerCursorZ;
static s32 g_MapMakerModuleType;
static s32 g_MapMakerRotation;
static s32 g_MapMakerMoveRepeat;
static f32 g_MapMakerCameraYaw;
static f32 g_MapMakerCameraPitch;
static f32 g_MapMakerCameraDistance;
static s32 g_MapMakerMenuOpen;
static s32 g_MapMakerMenuChoice;
static s32 g_MapMakerMusicTrack;
static s32 g_MapMakerMusicPreviewPending;
static s32 g_MapMakerMusicPreviewDelay;
static s32 g_MapMakerPlaytest;
static s32 g_MapMakerTool;
static s32 g_MapMakerFreeView;
static s32 g_MapMakerMaterialSlot;
static u16 g_MapMakerMaterialTexture[MAPMAKER_MATERIAL_SLOTS];
static s32 g_MapMakerSelectedModule;
static s32 g_MapMakerSelectedSurface;
static f32 g_MapMakerFreeCameraX;
static f32 g_MapMakerFreeCameraY;
static f32 g_MapMakerFreeCameraZ;
static f32 g_MapMakerPlayerX;
static f32 g_MapMakerPlayerY;
static f32 g_MapMakerPlayerZ;
static f32 g_MapMakerPlayerYaw;
static f32 g_MapMakerPlayerPitch;
static f32 g_MapMakerPlayerVelY;
static s32 g_MapMakerEditorMode;
static s32 g_MapMakerAdvancedMeshCount;
static s32 g_MapMakerAdvancedLoopCount;
static s32 g_MapMakerAdvancedVertexCount;
static s32 g_MapMakerAdvancedMeshType;
static s32 g_MapMakerAdvancedTool;
static s32 g_MapMakerAdvancedAxis;
static s32 g_MapMakerAdvancedRoom;
static s32 g_MapMakerAdvancedPortalCount;
static s32 g_MapMakerAdvancedSelectedPortal;
static s32 g_MapMakerAdvancedSelectedMesh;
static s32 g_MapMakerAdvancedSelectedVertex;
static s32 g_MapMakerAdvancedDraggingVertex;
static MapMakerAdvancedVertex g_MapMakerAdvancedDragOriginal;
static MapMakerAdvancedVertex g_MapMakerAdvancedTransformPivot;
static f32 g_MapMakerAdvancedDragPlanePoint[3];
static f32 g_MapMakerAdvancedDragPlaneNormal[3];

typedef union MapMakerSaveBuffer {
    u64 align;
    u8 bytes[MM_SAVE_BYTES];
} MapMakerSaveBuffer;

static MapMakerSaveBuffer g_MapMakerSessionSave;
static MapMakerSaveBuffer g_MapMakerPakIoBuffer;
static u8 g_MapMakerPakVisitedPages[128];
static u8 g_MapMakerPakPageList[MM_PAK_MAX_PAGES];
static s32 g_MapMakerPakPageCount;
static s32 g_MapMakerLastSaveBytes;
static s32 g_MapMakerSessionSaveValid;
static s32 g_MapMakerStorageBusy;
static char g_MapMakerStorageStatus[96];
static s32 g_MapMakerInitialized;
static s32 g_MapMakerDirty;
static char g_MapMakerCurrentName[24];
static struct texpool g_MapMakerSavedDefaultTexturePool;
static s32 g_MapMakerFrontendPoolCaptured;
static s32 g_MapMakerPaintDragModule = -1;
static s32 g_MapMakerPaintDragSurface = -1;
static s32 g_MapMakerTexturePoolRecyclePending;
static u16 g_MapMakerLastTextureId = MAPMAKER_TEXTURE_NONE;
static sImageTableEntry g_MapMakerLastTextureImage;
static s32 g_MapMakerLastTextureWidth;
static s32 g_MapMakerLastTextureHeight;
static s32 g_MapMakerBuildDragX = 9999;
static s32 g_MapMakerBuildDragY = 9999;
static s32 g_MapMakerBuildDragZ = 9999;
static s32 g_MapMakerBuildDragType = -1;
static s32 g_MapMakerBuildDragRotation = -1;
static s32 g_MapMakerTextureFlip;
static s32 g_MapMakerEntityCount;
static s32 g_MapMakerPlayerStartCount;
static s32 g_MapMakerPlayerStartSlot;
static s32 g_MapMakerEntityType;
static s32 g_MapMakerEntityObjectId;
static s32 g_MapMakerEntityAuxId;
static s32 g_MapMakerPickupType;
static s32 g_MapMakerPickupSubtype;
static s32 g_MapMakerPickupQuantity;
static s32 g_MapMakerSnapEnabled;
static s32 g_MapMakerGridSize;
static s32 g_MapMakerPlacementEdge;
static s32 g_MapMakerEditorFog;
static s32 g_MapMakerArbitraryRotation;
static f32 g_MapMakerEntityCursorX;
static f32 g_MapMakerEntityCursorY;
static f32 g_MapMakerEntityCursorZ;

/* Native-test state deliberately lives in normal small BSS; the authored bulk
 * data is snapshotted to g_MapMakerSessionSave before stage load and rebuilt
 * into MEMPOOL_STAGE after the frontend scratch window has been reclaimed. */
static s32 g_MapMakerNativeTestPending;
static s32 g_MapMakerNativeTestEntered;
static s32 g_MapMakerNativeReturning;
static s32 g_MapMakerNativeTestRuntimeReady;
static s32 g_MapMakerNativeOriginValid;
static s32 g_MapMakerRestoreSessionOnInit;
static MapMakerModuleInstance *g_MapMakerNativeModules;
static s32 g_MapMakerNativeModuleCount;
static MapMakerPlayerStart *g_MapMakerNativeStarts;
static s32 g_MapMakerNativeStartCount;
static f32 g_MapMakerNativeVelY[4];
static f32 g_MapMakerNativeOriginX;
static f32 g_MapMakerNativeOriginY;
static f32 g_MapMakerNativeOriginZ;

extern s32 g_musicXTrack1CurrentTrackNum;
extern s16 random_tracks[];
extern OSMesgQueue g_ContInputMessageQueue;

#define MM_MENU_RESUME        0
#define MM_MENU_FIRSTPERSON   1
#define MM_MENU_NATIVETEST    2
#define MM_MENU_TOOL          3
#define MM_MENU_VIEW          4
#define MM_MENU_MATERIAL      5
#define MM_MENU_SAVE          6
#define MM_MENU_LOAD          7
#define MM_MENU_MUSIC         8
#define MM_MENU_SNAP          9
#define MM_MENU_GRID          10
#define MM_MENU_PLACE         11
#define MM_MENU_FLIP          12
#define MM_MENU_FOG           13
#define MM_MENU_ARBITRARY     14
#define MM_MENU_EXIT          15
#define MM_MENU_COUNT         16
#define MM_TEXTURE_LAST (MAX_TEXTURES - 2)
#define MM_MUSIC_FIRST M_SHORT_SOLO_DEATH
#define MM_MUSIC_LAST  M_END_SOMETHING

/* GoldenEye setup records use PROPDEF_TYPE IDs 00..2F.  The Setup Editor
 * exposes these IDs directly, so Map Maker does the same instead of hiding
 * them behind the old eight-entry prototype enum.  Pickup-class setup records
 * are browsed through the separate Pickup tool but retain their native IDs. */
static const char *g_MapMakerSetupTypeNames[PROPDEF_TINTED_GLASS + 1] = {
    "Nothing / Invalid",          /* 00 */
    "Door",                       /* 01 */
    "Door Scale",                 /* 02 */
    "Standard Object",            /* 03 */
    "Key",                        /* 04 */
    "Alarm",                      /* 05 */
    "CCTV Camera",                /* 06 */
    "Ammo Magazine",              /* 07 */
    "Weapon / Item",              /* 08 */
    "Guard / Character",          /* 09 */
    "Single Monitor",             /* 0A */
    "Multi Monitor",              /* 0B */
    "Monitor Rack",               /* 0C */
    "Drone Gun / Autogun",        /* 0D */
    "Interlink Pickups",          /* 0E */
    "Invalid 0F",                 /* 0F */
    "Invalid 10",                 /* 10 */
    "Hat",                        /* 11 */
    "Guard Attribute",            /* 12 */
    "Switch / Link",              /* 13 */
    "Ammo Crate",                 /* 14 */
    "Body Armour",                /* 15 */
    "Tag",                        /* 16 */
    "Objective Start",            /* 17 */
    "Objective End",              /* 18 */
    "Objective: Destroy",         /* 19 */
    "Objective: Complete If",     /* 1A */
    "Objective: Fail If",         /* 1B */
    "Objective: Collect",         /* 1C */
    "Objective: Deposit",         /* 1D */
    "Objective: Photograph",      /* 1E */
    "Objective: Invalid",         /* 1F */
    "Objective: Enter Room",      /* 20 */
    "Objective: Deposit In Room", /* 21 */
    "Objective: Key Analyzer",    /* 22 */
    "Watch Objective Text",       /* 23 */
    "Gas-Releasing Object",       /* 24 */
    "Rename",                     /* 25 */
    "Lock Door",                  /* 26 */
    "Wheeled Vehicle",            /* 27 */
    "Aircraft",                   /* 28 */
    "Invalid 29",                 /* 29 */
    "Glass",                      /* 2A */
    "Safe",                       /* 2B */
    "Safe Item Link",             /* 2C */
    "Tank",                       /* 2D */
    "Camera Position",            /* 2E */
    "Security / Tinted Glass"     /* 2F */
};

static const char *g_MapMakerAmmoTypeNames[AMMOTYPE_MAX] = {
    "None", "9mm", "9mm (2)", "Rifle", "Shotgun", "Grenade",
    "Rockets", "Remote Mine", "Proximity Mine", "Timed Mine", "Knife",
    "Grenade Round", "Magnum", "Golden Gun", "Darts", "Explosive Pen",
    "Bomb Case", "Flare", "Piton", "Dynamite", "Bug", "Micro Camera",
    "GoldenEye Key", "Plastique", "Watch Laser", "Watch Magnet", "Unknown",
    "Camera", "Tank Shells", "Token"
};

static s32 mapmakerSetupTypeIsPickup(s32 type)
{
    return type == PROPDEF_KEY
        || type == PROPDEF_MAGAZINE
        || type == PROPDEF_COLLECTABLE
        || type == PROPDEF_AMMO
        || type == PROPDEF_ARMOUR;
}

static const char *mapmakerSetupTypeName(s32 type)
{
    if (type < 0 || type > PROPDEF_TINTED_GLASS) return "Unknown";
    return g_MapMakerSetupTypeNames[type];
}

static const char *mapmakerSetupTypeClassName(s32 type)
{
    if (mapmakerSetupTypeIsPickup(type)) return "Pickup";
    if (type >= PROPDEF_OBJECTIVE_START && type <= PROPDEF_WATCH_MENU_OBJECTIVE_TEXT) return "Objective";
    if (type == PROPDEF_DOOR_SCALE || type == PROPDEF_LINK || type == PROPDEF_GUARD_ATTRIBUTE
            || type == PROPDEF_SWITCH || type == PROPDEF_TAG || type == PROPDEF_RENAME
            || type == PROPDEF_LOCK_DOOR || type == PROPDEF_SAFE_ITEM)
        return "Modifier";
    if (type == PROPDEF_NOTHING || type == PROPDEF_DEBRIS || type == PROPDEF_UNK16 || type == PROPDEF_UNK41)
        return "Invalid";
    return "Entity";
}

static s32 mapmakerPickupSetupType(s32 pickup)
{
    switch (pickup)
    {
    case MAPPICKUP_AMMO_MAGAZINE: return PROPDEF_MAGAZINE;
    case MAPPICKUP_AMMO_CRATE: return PROPDEF_AMMO;
    case MAPPICKUP_KEY: return PROPDEF_KEY;
    case MAPPICKUP_ARMOUR: return PROPDEF_ARMOUR;
    default: return PROPDEF_COLLECTABLE;
    }
}

static const char *mapmakerPickupTypeName(s32 pickup)
{
    switch (pickup)
    {
    case MAPPICKUP_AMMO_MAGAZINE: return "Ammo Magazine";
    case MAPPICKUP_AMMO_CRATE: return "Ammo Crate";
    case MAPPICKUP_KEY: return "Key";
    case MAPPICKUP_ARMOUR: return "Body Armour";
    default: return "Weapon / Item";
    }
}

static s32 mapmakerEntityTypeUsesPropId(s32 type)
{
    switch (type)
    {
    case PROPDEF_DOOR:
    case PROPDEF_PROP:
    case PROPDEF_ALARM:
    case PROPDEF_CCTV:
    case PROPDEF_MONITOR:
    case PROPDEF_MULTI_MONITOR:
    case PROPDEF_RACK:
    case PROPDEF_AUTOGUN:
    case PROPDEF_HAT:
    case PROPDEF_GAS_RELEASING:
    case PROPDEF_VEHICHLE:
    case PROPDEF_AIRCRAFT:
    case PROPDEF_GLASS:
    case PROPDEF_SAFE:
    case PROPDEF_TANK:
    case PROPDEF_TINTED_GLASS:
        return 1;
    default:
        return 0;
    }
}

static s32 mapmakerEntityIdMax(s32 type)
{
    if (type == PROPDEF_GUARD) return BODIES_MAX - 1;
    if (mapmakerEntityTypeUsesPropId(type)) return PROP_MAX - 1;
    return 0xffff;
}

static void mapmakerChangeEntityType(s32 delta)
{
    s32 type = g_MapMakerEntityType;
    s32 guard = 0;

    do
    {
        type += delta < 0 ? -1 : 1;
        if (type < PROPDEF_NOTHING) type = PROPDEF_TINTED_GLASS;
        if (type > PROPDEF_TINTED_GLASS) type = PROPDEF_NOTHING;
        guard++;
    } while (mapmakerSetupTypeIsPickup(type) && guard <= PROPDEF_TINTED_GLASS + 1);

    g_MapMakerEntityType = type;
    if (g_MapMakerEntityObjectId > mapmakerEntityIdMax(type))
        g_MapMakerEntityObjectId = mapmakerEntityIdMax(type);
}

static void mapmakerChangeEntityObjectId(s32 delta)
{
    s32 maxid = mapmakerEntityIdMax(g_MapMakerEntityType);
    s32 value = g_MapMakerEntityObjectId + delta;
    if (value < 0) value = maxid;
    if (value > maxid) value = 0;
    g_MapMakerEntityObjectId = value;
}

static void mapmakerResetPickupSelection(void)
{
    switch (g_MapMakerPickupType)
    {
    case MAPPICKUP_AMMO_MAGAZINE:
    case MAPPICKUP_AMMO_CRATE:
        g_MapMakerPickupSubtype = AMMO_9MM;
        g_MapMakerPickupQuantity = 30;
        break;
    case MAPPICKUP_KEY:
        g_MapMakerPickupSubtype = 0;
        g_MapMakerPickupQuantity = 1;
        break;
    case MAPPICKUP_ARMOUR:
        g_MapMakerPickupSubtype = 0;
        g_MapMakerPickupQuantity = 100;
        break;
    default:
        g_MapMakerPickupSubtype = ITEM_WPPK;
        g_MapMakerPickupQuantity = 1;
        break;
    }
}

static void mapmakerChangePickupType(s32 delta)
{
    g_MapMakerPickupType += delta < 0 ? -1 : 1;
    if (g_MapMakerPickupType < 0) g_MapMakerPickupType = MAPPICKUP_TYPE_COUNT - 1;
    if (g_MapMakerPickupType >= MAPPICKUP_TYPE_COUNT) g_MapMakerPickupType = 0;
    mapmakerResetPickupSelection();
}

static void mapmakerChangePickupSubtype(s32 delta)
{
    s32 min = 0;
    s32 max = 0;
    s32 value;

    switch (g_MapMakerPickupType)
    {
    case MAPPICKUP_WEAPON_ITEM:
        min = ITEM_UNARMED;
        max = ITEM_IDS_MAX - 1;
        break;
    case MAPPICKUP_AMMO_MAGAZINE:
        min = AMMO_9MM;
        max = AMMOTYPE_MAX - 1;
        break;
    case MAPPICKUP_AMMO_CRATE:
        min = AMMO_9MM;
        max = AMMOTYPE_GLOBAL_MAX - 1;
        break;
    case MAPPICKUP_KEY:
        min = 0;
        max = 255;
        break;
    default:
        return;
    }

    value = g_MapMakerPickupSubtype + (delta < 0 ? -1 : 1);
    if (value < min) value = max;
    if (value > max) value = min;
    g_MapMakerPickupSubtype = value;
}

static void mapmakerChangePickupQuantity(s32 delta)
{
    s32 min;
    s32 max;

    if (g_MapMakerPickupType == MAPPICKUP_AMMO_CRATE)
    {
        min = 1;
        max = 999;
    }
    else if (g_MapMakerPickupType == MAPPICKUP_ARMOUR)
    {
        min = 0;
        max = 100;
    }
    else
    {
        return;
    }

    g_MapMakerPickupQuantity += delta;
    if (g_MapMakerPickupQuantity < min) g_MapMakerPickupQuantity = min;
    if (g_MapMakerPickupQuantity > max) g_MapMakerPickupQuantity = max;
}

static const char *g_MapMakerMusicNames[MM_MUSIC_LAST + 1] = {
    "None",
    "Solo Death Short",
    "Intro",
    "Train",
    "Depot",
    "Multiplayer Theme",
    "Citadel",
    "Facility",
    "Control",
    "Dam",
    "Frigate",
    "Archives",
    "Silo",
    "Multiplayer Theme 2",
    "Streets",
    "Bunker I",
    "Bunker II",
    "Statue",
    "Elevator - Control",
    "Cradle",
    "Unknown 1",
    "Elevator - WC",
    "Egyptian",
    "Folders",
    "Watch",
    "Aztec",
    "Water Caverns",
    "Solo Death",
    "Surface II",
    "Train X",
    "Unknown 2",
    "Facility X",
    "Depot X",
    "Control X",
    "Water Caverns X",
    "Dam X",
    "Frigate X",
    "Archives X",
    "Silo X",
    "Egyptian X",
    "Streets X",
    "Bunker I X",
    "Bunker II X",
    "Jungle X",
    "Intro Swoosh",
    "Statue X",
    "Aztec X",
    "Egypt X",
    "Cradle X",
    "Cuba",
    "Runway",
    "Runway Plane",
    "Multiplayer Theme 3",
    "Wind",
    "Guitar Gliss",
    "Jungle",
    "Runway X",
    "Surface I",
    "Multiplayer Death",
    "Surface II X",
    "Surface II End",
    "Statue Part",
    "End Sequence"

};

static OSPfs g_MapMakerPfs;
static const u8 g_MapMakerPakName[PFS_FILE_NAME_LEN] = {
    'G','E','P','L','U','S',' ','M','A','P',0,0,0,0,0,0
};
static const u8 g_MapMakerPakExt[PFS_FILE_EXT_LEN] = {'B','0','2',0};

void mapmakerSetEditorMode(s32 mode)
{
    g_MapMakerEditorMode = mode == MAPMAKER_EDITOR_ADVANCED
        ? MAPMAKER_EDITOR_ADVANCED : MAPMAKER_EDITOR_BASIC;
}

s32 mapmakerGetEditorMode(void)
{
    return g_MapMakerEditorMode;
}

const char *mapmakerGetEditorModeName(void)
{
    return g_MapMakerEditorMode == MAPMAKER_EDITOR_ADVANCED ? "ADVANCED" : "BASIC";
}

const char *mapmakerAdvancedGetMeshTypeName(void)
{
    switch (g_MapMakerAdvancedMeshType)
    {
    case MAPADV_MESH_CUBE: return "Cube";
    case MAPADV_MESH_CIRCLE: return "Circle";
    case MAPADV_MESH_UV_SPHERE: return "UV Sphere";
    case MAPADV_MESH_ICO_SPHERE: return "ICO Sphere";
    case MAPADV_MESH_CYLINDER: return "Cylinder";
    case MAPADV_MESH_CONE: return "Cone";
    case MAPADV_MESH_TORUS: return "Torus";
    default: return "Plane";
    }
}

const char *mapmakerAdvancedGetToolName(void)
{
    static const char *names[MAPADV_TOOL_COUNT] = {
        "Create", "Move", "Rotate", "Scale", "Vertex", "Room", "Portal", "Depth",
        "Build", "Texture", "Entity", "Pickup", "Player Start"
    };
    if (g_MapMakerAdvancedTool < 0 || g_MapMakerAdvancedTool >= MAPADV_TOOL_COUNT) return "Create";
    return names[g_MapMakerAdvancedTool];
}

const char *mapmakerAdvancedGetAxisName(void)
{
    static const char *names[MAPADV_AXIS_COUNT] = {"X", "Y", "Z", "All"};
    if (g_MapMakerAdvancedAxis < 0 || g_MapMakerAdvancedAxis >= MAPADV_AXIS_COUNT) return "All";
    return names[g_MapMakerAdvancedAxis];
}

s32 mapmakerAdvancedGetPortalCount(void) { return g_MapMakerAdvancedPortalCount; }
s32 mapmakerAdvancedGetRoom(void) { return g_MapMakerAdvancedRoom; }

s32 mapmakerAdvancedGetMeshCount(void)
{
    s32 i;
    s32 count = 0;
    for (i = 0; i < g_MapMakerAdvancedMeshCount; i++)
        if ((g_MapMakerAdvancedMeshes[i].flags & 0x8000) == 0) count++;
    return count;
}
s32 mapmakerAdvancedGetSelectedVertex(void) { return g_MapMakerAdvancedSelectedVertex; }
s32 mapmakerAdvancedIsDraggingVertex(void) { return g_MapMakerAdvancedDraggingVertex; }

static void mapmakerMarkDirty(void)
{
    g_MapMakerDirty = 1;
    strcpy(g_MapMakerCurrentName, "Untitled");
}

static u32 mapmakerCrc32(const u8 *data, s32 len)
{
    u32 crc = 0xffffffffu;
    s32 i;
    s32 bit;

    for (i = 0; i < len; i++)
    {
        crc ^= data[i];
        for (bit = 0; bit < 8; bit++)
        {
            crc = (crc >> 1) ^ (0xedb88320u & (0u - (crc & 1u)));
        }
    }
    return ~crc;
}

static s32 mapmakerValidateModule(const MapMakerModuleInstance *module)
{
    s32 i;

    if (module->grid_x < MM_GRID_MIN || module->grid_x > MM_GRID_MAX
            || module->grid_z < MM_GRID_MIN || module->grid_z > MM_GRID_MAX
            || module->grid_y < MAPMAKER_MIN_LAYER || module->grid_y > MAPMAKER_MAX_LAYER
            || module->type >= MAPMODULE_COUNT || module->rotation > 3)
    {
        return 0;
    }

    for (i = 0; i < MAPMAKER_SURFACE_COUNT; i++)
    {
        if (module->surface_texture[i] != MAPMAKER_TEXTURE_NONE
                && module->surface_texture[i] > MM_TEXTURE_LAST)
        {
            return 0;
        }
    }

    return 1;
}

static s32 mapmakerValidateEntity(const MapMakerEntityInstance *entity)
{
    if (entity->type > PROPDEF_TINTED_GLASS || entity->rotation > 3)
        return 0;

    if (entity->type == PROPDEF_GUARD && entity->object_id >= BODIES_MAX)
        return 0;
    if (mapmakerEntityTypeUsesPropId(entity->type) && entity->object_id >= PROP_MAX)
        return 0;

    switch (entity->type)
    {
    case PROPDEF_COLLECTABLE:
        return entity->subtype >= ITEM_UNARMED && entity->subtype < ITEM_IDS_MAX;
    case PROPDEF_MAGAZINE:
        return entity->subtype >= AMMO_9MM && entity->subtype < AMMOTYPE_MAX;
    case PROPDEF_AMMO:
        return entity->subtype >= AMMO_9MM && entity->subtype < AMMOTYPE_GLOBAL_MAX
            && entity->quantity >= 1 && entity->quantity <= 999;
    case PROPDEF_KEY:
        return entity->subtype >= 0 && entity->subtype <= 255;
    case PROPDEF_ARMOUR:
        return entity->quantity <= 100;
    default:
        return 1;
    }
}

static s32 mapmakerValidatePlayerStart(const MapMakerPlayerStart *start)
{
    if (start->rotation > 3 || start->slot >= MAPMAKER_MAX_PLAYER_STARTS)
        return 0;
    return 1;
}

static s32 mapmakerValidateLegacySaveBlob(const u8 *buffer)
{
    GeMapHeader header;
    const u8 *payload = buffer + sizeof(GeMapHeader);
    s32 materialbytes = sizeof(g_MapMakerMaterialTexture);
    s32 modulebytes;
    s32 entitybytes;
    s32 entitycount;
    s32 startcount;
    s32 startbytes;
    s32 expected;
    s32 i;

    /* Never cast an arbitrary Pak byte buffer to GeMapHeader on MIPS.  A Pak
     * block is only byte-aligned by contract and an unaligned u32 load would
     * raise an address exception on real hardware. */
    memcpy(&header, buffer, sizeof(header));

    if (header.magic != GEMAP_MAGIC
            || (header.version != 5 && header.version != 6 && header.version != 7)
            || header.header_size != sizeof(GeMapHeader)
            || header.module_count > MAPMAKER_MAX_MODULES)
    {
        return 0;
    }

    /* GEMAP v5 used object_count as a palette-count sentinel and did not save
     * editor entities.  v6 makes object_count the real placement count while
     * keeping the same fixed Controller Pak file size. */
    if (header.version == 5)
    {
        if (header.object_count != MAPMAKER_MATERIAL_SLOTS) return 0;
        entitycount = 0;
    }
    else
    {
        if (header.object_count > MAPMAKER_MAX_ENTITIES) return 0;
        entitycount = header.object_count;
    }

    startcount = header.version >= 7 ? (s32)(header.flags >> 16) : 0;
    if (startcount < 0 || startcount > MAPMAKER_MAX_PLAYER_STARTS) return 0;

    modulebytes = header.module_count * sizeof(MapMakerModuleInstance);
    entitybytes = entitycount * sizeof(MapMakerEntityInstance);
    startbytes = startcount * sizeof(MapMakerPlayerStart);
    expected = materialbytes + modulebytes + entitybytes + startbytes;

    if (header.payload_size != (u32)expected
            || expected < 0
            || sizeof(GeMapHeader) + expected > MM_SAVE_BYTES
            || header.payload_crc != mapmakerCrc32(payload, expected))
    {
        return 0;
    }

    for (i = 0; i < MAPMAKER_MATERIAL_SLOTS; i++)
    {
        u16 texture;
        memcpy(&texture, payload + i * sizeof(u16), sizeof(texture));
        if (texture > MM_TEXTURE_LAST)
            return 0;
    }

    for (i = 0; i < header.module_count; i++)
    {
        MapMakerModuleInstance module;
        memcpy(&module, payload + materialbytes + i * sizeof(module), sizeof(module));
        if (!mapmakerValidateModule(&module))
            return 0;
    }

    for (i = 0; i < entitycount; i++)
    {
        MapMakerEntityInstance entity;
        memcpy(&entity, payload + materialbytes + modulebytes + i * sizeof(entity), sizeof(entity));
        if (!mapmakerValidateEntity(&entity))
            return 0;
    }

    for (i = 0; i < startcount; i++)
    {
        MapMakerPlayerStart start;
        memcpy(&start, payload + materialbytes + modulebytes + entitybytes + i * sizeof(start), sizeof(start));
        if (!mapmakerValidatePlayerStart(&start)) return 0;
    }

    return 1;
}

#define MM_V8_TEX_RAW 0xfeu
#define MM_V8_MAX_TEXTURE_PALETTE 254

typedef struct GeMapV8Meta {
    u16 texture_count;
    u16 advanced_mesh_count;
    u16 advanced_loop_count;
    u16 advanced_vertex_count;
    u16 portal_count;
    u16 grid_size;
    u8 start_count;
    s8 music_track;
    u8 editor_mode;
    u8 reserved;
} GeMapV8Meta;

static s32 mapmakerStreamWrite(u8 **cursor, u8 *end, const void *src, s32 bytes)
{
    if (bytes < 0 || *cursor > end || bytes > end - *cursor) return 0;
    memcpy(*cursor, src, bytes);
    *cursor += bytes;
    return 1;
}

static s32 mapmakerStreamRead(const u8 **cursor, const u8 *end, void *dst, s32 bytes)
{
    if (bytes < 0 || *cursor > end || bytes > end - *cursor) return 0;
    memcpy(dst, *cursor, bytes);
    *cursor += bytes;
    return 1;
}

static s32 mapmakerV8PaletteIndex(const u16 *palette, s32 count, u16 texture)
{
    s32 i;
    for (i = 0; i < count; i++) if (palette[i] == texture) return i;
    return -1;
}

static s32 mapmakerValidateV8SaveBlob(const u8 *buffer, s32 available)
{
    GeMapHeader header;
    GeMapV8Meta meta;
    const u8 *cursor;
    const u8 *end;
    u16 palette[MM_V8_MAX_TEXTURE_PALETTE];
    s32 i;

    if (available < (s32)(sizeof(GeMapHeader) + sizeof(GeMapV8Meta))) return 0;
    memcpy(&header, buffer, sizeof(header));
    if (header.magic != GEMAP_MAGIC || header.version != GEMAP_VERSION
            || header.header_size != sizeof(GeMapHeader)
            || header.module_count > MAPMAKER_MAX_MODULES
            || header.object_count > MAPMAKER_MAX_ENTITIES
            || header.payload_size > (u32)(available - sizeof(GeMapHeader))) return 0;

    cursor = buffer + sizeof(GeMapHeader);
    end = cursor + header.payload_size;
    if (header.payload_crc != mapmakerCrc32(cursor, header.payload_size)) return 0;
    if (!mapmakerStreamRead(&cursor, end, &meta, sizeof(meta))) return 0;
    if (meta.texture_count > MM_V8_MAX_TEXTURE_PALETTE
            || meta.start_count > MAPMAKER_MAX_PLAYER_STARTS
            || meta.advanced_mesh_count > MAPMAKER_ADV_MAX_MESHES
            || meta.advanced_loop_count > MAPMAKER_ADV_MAX_LOOPS
            || meta.advanced_vertex_count > MAPMAKER_ADV_MAX_VERTICES
            || meta.portal_count > MAPMAKER_ADV_MAX_PORTALS
            || meta.grid_size < 20 || meta.grid_size > 640
            || (meta.music_track != MM_MUSIC_RANDOM
                && (meta.music_track < MM_MUSIC_FIRST || meta.music_track > MM_MUSIC_LAST))
            || meta.editor_mode > MAPMAKER_EDITOR_ADVANCED) return 0;

    for (i = 0; i < MAPMAKER_MATERIAL_SLOTS; i++)
    {
        u16 texture;
        if (!mapmakerStreamRead(&cursor, end, &texture, sizeof(texture)) || texture > MM_TEXTURE_LAST) return 0;
    }
    for (i = 0; i < meta.texture_count; i++)
    {
        if (!mapmakerStreamRead(&cursor, end, &palette[i], sizeof(u16)) || palette[i] > MM_TEXTURE_LAST) return 0;
    }

    for (i = 0; i < header.module_count; i++)
    {
        MapMakerModuleInstance module;
        s8 gx, gy, gz;
        u8 typerot;
        u8 mask;
        s32 surface;
        memset(&module, 0, sizeof(module));
        for (surface = 0; surface < MAPMAKER_SURFACE_COUNT; surface++) module.surface_texture[surface] = MAPMAKER_TEXTURE_NONE;
        if (!mapmakerStreamRead(&cursor, end, &gx, 1)
                || !mapmakerStreamRead(&cursor, end, &gy, 1)
                || !mapmakerStreamRead(&cursor, end, &gz, 1)
                || !mapmakerStreamRead(&cursor, end, &typerot, 1)
                || !mapmakerStreamRead(&cursor, end, &module.flags, sizeof(module.flags))
                || !mapmakerStreamRead(&cursor, end, &mask, 1)) return 0;
        module.grid_x = gx; module.grid_y = gy; module.grid_z = gz;
        module.type = typerot & 7; module.rotation = (typerot >> 3) & 3;
        if (mask & ~0x3f) return 0;
        for (surface = 0; surface < MAPMAKER_SURFACE_COUNT; surface++)
        {
            if (mask & (1 << surface))
            {
                u8 token;
                if (!mapmakerStreamRead(&cursor, end, &token, 1)) return 0;
                if (token == MM_V8_TEX_RAW)
                {
                    if (!mapmakerStreamRead(&cursor, end, &module.surface_texture[surface], sizeof(u16))
                            || module.surface_texture[surface] > MM_TEXTURE_LAST) return 0;
                }
                else
                {
                    if (token >= meta.texture_count) return 0;
                    module.surface_texture[surface] = palette[token];
                }
            }
        }
        if (!mapmakerValidateModule(&module)) return 0;
    }

    for (i = 0; i < header.object_count; i++)
    {
        MapMakerEntityInstance entity;
        if (!mapmakerStreamRead(&cursor, end, &entity, sizeof(entity)) || !mapmakerValidateEntity(&entity)) return 0;
    }
    for (i = 0; i < meta.start_count; i++)
    {
        MapMakerPlayerStart start;
        if (!mapmakerStreamRead(&cursor, end, &start, sizeof(start)) || !mapmakerValidatePlayerStart(&start)) return 0;
    }
    for (i = 0; i < meta.advanced_mesh_count; i++)
    {
        MapMakerAdvancedMesh mesh;
        if (!mapmakerStreamRead(&cursor, end, &mesh, sizeof(mesh))
                || mesh.type >= MAPADV_MESH_TYPE_COUNT || mesh.material >= MAPMAKER_MATERIAL_SLOTS
                || mesh.first_loop + mesh.loop_count > meta.advanced_loop_count) return 0;
    }
    for (i = 0; i < meta.advanced_loop_count; i++)
    {
        MapMakerAdvancedLoop loop;
        if (!mapmakerStreamRead(&cursor, end, &loop, sizeof(loop))
                || loop.vertex_count < 3 || loop.vertex_count > MAPMAKER_ADV_MAX_POINTS_PER_LOOP
                || loop.first_vertex + loop.vertex_count > meta.advanced_vertex_count) return 0;
    }
    for (i = 0; i < meta.advanced_vertex_count; i++)
    {
        MapMakerAdvancedVertex vertex;
        if (!mapmakerStreamRead(&cursor, end, &vertex, sizeof(vertex))) return 0;
    }
    for (i = 0; i < meta.portal_count; i++)
    {
        MapMakerAdvancedPortal portal;
        if (!mapmakerStreamRead(&cursor, end, &portal, sizeof(portal))
                || portal.axis > 1 || portal.half_width <= 0 || portal.half_height <= 0) return 0;
    }
    return cursor == end;
}

static s32 mapmakerValidateSaveBlob(const u8 *buffer, s32 available)
{
    GeMapHeader header;
    if (buffer == NULL || available < (s32)sizeof(header)) return 0;
    memcpy(&header, buffer, sizeof(header));
    if (header.version == 5 || header.version == 6 || header.version == 7)
        return available >= MM_SAVE_BYTES && mapmakerValidateLegacySaveBlob(buffer);
    if (header.version == GEMAP_VERSION)
        return mapmakerValidateV8SaveBlob(buffer, available);
    return 0;
}

static s32 mapmakerBuildSaveBlob(u8 *buffer, s32 capacity)
{
    GeMapHeader header;
    GeMapV8Meta meta;
    u16 palette[MM_V8_MAX_TEXTURE_PALETTE];
    s32 palettecount = 0;
    u8 *payload;
    u8 *cursor;
    u8 *end;
    s32 i;

    if (buffer == NULL || capacity < (s32)(sizeof(header) + sizeof(meta))
            || g_MapMakerModuleCount < 0 || g_MapMakerModuleCount > MAPMAKER_MAX_MODULES
            || g_MapMakerEntityCount < 0 || g_MapMakerEntityCount > MAPMAKER_MAX_ENTITIES
            || g_MapMakerPlayerStartCount < 0 || g_MapMakerPlayerStartCount > MAPMAKER_MAX_PLAYER_STARTS
            || g_MapMakerAdvancedMeshCount < 0 || g_MapMakerAdvancedMeshCount > MAPMAKER_ADV_MAX_MESHES
            || g_MapMakerAdvancedLoopCount < 0 || g_MapMakerAdvancedLoopCount > MAPMAKER_ADV_MAX_LOOPS
            || g_MapMakerAdvancedVertexCount < 0 || g_MapMakerAdvancedVertexCount > MAPMAKER_ADV_MAX_VERTICES
            || g_MapMakerAdvancedPortalCount < 0 || g_MapMakerAdvancedPortalCount > MAPMAKER_ADV_MAX_PORTALS)
        return 0;

    for (i = 0; i < g_MapMakerModuleCount; i++)
    {
        s32 surface;
        if (!mapmakerValidateModule(&g_MapMakerModules[i])) return 0;
        for (surface = 0; surface < MAPMAKER_SURFACE_COUNT; surface++)
        {
            u16 texture = g_MapMakerModules[i].surface_texture[surface];
            if (texture != MAPMAKER_TEXTURE_NONE && mapmakerV8PaletteIndex(palette, palettecount, texture) < 0
                    && palettecount < MM_V8_MAX_TEXTURE_PALETTE)
                palette[palettecount++] = texture;
        }
    }
    for (i = 0; i < g_MapMakerEntityCount; i++) if (!mapmakerValidateEntity(&g_MapMakerEntities[i])) return 0;
    for (i = 0; i < g_MapMakerPlayerStartCount; i++) if (!mapmakerValidatePlayerStart(&g_MapMakerPlayerStarts[i])) return 0;

    memset(buffer, 0, capacity);
    memset(&header, 0, sizeof(header));
    memset(&meta, 0, sizeof(meta));
    payload = buffer + sizeof(header);
    cursor = payload;
    end = buffer + capacity;
    meta.texture_count = (u16)palettecount;
    meta.advanced_mesh_count = (u16)g_MapMakerAdvancedMeshCount;
    meta.advanced_loop_count = (u16)g_MapMakerAdvancedLoopCount;
    meta.advanced_vertex_count = (u16)g_MapMakerAdvancedVertexCount;
    meta.portal_count = (u16)g_MapMakerAdvancedPortalCount;
    meta.grid_size = (u16)g_MapMakerGridSize;
    meta.start_count = (u8)g_MapMakerPlayerStartCount;
    meta.music_track = (s8)g_MapMakerMusicTrack;
    meta.editor_mode = (u8)g_MapMakerEditorMode;
    if (!mapmakerStreamWrite(&cursor, end, &meta, sizeof(meta))
            || !mapmakerStreamWrite(&cursor, end, g_MapMakerMaterialTexture, sizeof(g_MapMakerMaterialTexture))
            || !mapmakerStreamWrite(&cursor, end, palette, palettecount * sizeof(u16))) goto too_large;

    for (i = 0; i < g_MapMakerModuleCount; i++)
    {
        MapMakerModuleInstance *module = &g_MapMakerModules[i];
        s8 gx = (s8)module->grid_x;
        s8 gy = (s8)module->grid_y;
        s8 gz = (s8)module->grid_z;
        u8 typerot = (u8)((module->type & 7) | ((module->rotation & 3) << 3));
        u8 mask = 0;
        s32 surface;
        for (surface = 0; surface < MAPMAKER_SURFACE_COUNT; surface++)
            if (module->surface_texture[surface] != MAPMAKER_TEXTURE_NONE) mask |= 1 << surface;
        if (!mapmakerStreamWrite(&cursor, end, &gx, 1)
                || !mapmakerStreamWrite(&cursor, end, &gy, 1)
                || !mapmakerStreamWrite(&cursor, end, &gz, 1)
                || !mapmakerStreamWrite(&cursor, end, &typerot, 1)
                || !mapmakerStreamWrite(&cursor, end, &module->flags, sizeof(module->flags))
                || !mapmakerStreamWrite(&cursor, end, &mask, 1)) goto too_large;
        for (surface = 0; surface < MAPMAKER_SURFACE_COUNT; surface++)
        {
            if (mask & (1 << surface))
            {
                u16 texture = module->surface_texture[surface];
                s32 index = mapmakerV8PaletteIndex(palette, palettecount, texture);
                if (index >= 0)
                {
                    u8 token = (u8)index;
                    if (!mapmakerStreamWrite(&cursor, end, &token, 1)) goto too_large;
                }
                else
                {
                    u8 token = MM_V8_TEX_RAW;
                    if (!mapmakerStreamWrite(&cursor, end, &token, 1)
                            || !mapmakerStreamWrite(&cursor, end, &texture, sizeof(texture))) goto too_large;
                }
            }
        }
    }

    if (!mapmakerStreamWrite(&cursor, end, g_MapMakerEntities, g_MapMakerEntityCount * sizeof(MapMakerEntityInstance))
            || !mapmakerStreamWrite(&cursor, end, g_MapMakerPlayerStarts, g_MapMakerPlayerStartCount * sizeof(MapMakerPlayerStart))
            || !mapmakerStreamWrite(&cursor, end, g_MapMakerAdvancedMeshes, g_MapMakerAdvancedMeshCount * sizeof(MapMakerAdvancedMesh))
            || !mapmakerStreamWrite(&cursor, end, g_MapMakerAdvancedLoops, g_MapMakerAdvancedLoopCount * sizeof(MapMakerAdvancedLoop))
            || !mapmakerStreamWrite(&cursor, end, g_MapMakerAdvancedVertices, g_MapMakerAdvancedVertexCount * sizeof(MapMakerAdvancedVertex))
            || !mapmakerStreamWrite(&cursor, end, g_MapMakerAdvancedPortals, g_MapMakerAdvancedPortalCount * sizeof(MapMakerAdvancedPortal))) goto too_large;

    header.magic = GEMAP_MAGIC;
    header.version = GEMAP_VERSION;
    header.header_size = sizeof(GeMapHeader);
    header.flags = 1;
    header.module_count = (u16)g_MapMakerModuleCount;
    header.object_count = (u16)g_MapMakerEntityCount;
    header.payload_size = (u32)(cursor - payload);
    header.payload_crc = mapmakerCrc32(payload, header.payload_size);
    strncpy(header.name, g_MapMakerCurrentName[0] ? g_MapMakerCurrentName : "Untitled", sizeof(header.name) - 1);
    memcpy(buffer, &header, sizeof(header));
    g_MapMakerLastSaveBytes = (s32)sizeof(header) + (s32)header.payload_size;
    if (!mapmakerValidateSaveBlob(buffer, g_MapMakerLastSaveBytes)) return 0;
    return 1;

too_large:
    strcpy(g_MapMakerStorageStatus, "Map exceeds physical Controller Pak capacity after packing");
    return 0;
}

static s32 mapmakerApplyLegacySaveBlob(const u8 *buffer)
{
    GeMapHeader header;
    const u8 *payload = buffer + sizeof(GeMapHeader);
    s32 materialbytes = sizeof(g_MapMakerMaterialTexture);
    s32 modulebytes;
    s32 entitycount;
    s32 entitybytes;
    s32 startcount;
    s32 startbytes;

    if (!mapmakerValidateLegacySaveBlob(buffer))
        return 0;

    memcpy(&header, buffer, sizeof(header));
    modulebytes = header.module_count * sizeof(MapMakerModuleInstance);
    entitycount = header.version >= 6 ? header.object_count : 0;
    entitybytes = entitycount * sizeof(MapMakerEntityInstance);
    startcount = header.version >= 7 ? (s32)(header.flags >> 16) : 0;
    startbytes = startcount * sizeof(MapMakerPlayerStart);

    /* The current map is mutated only after the whole file, CRC, palette,
     * module records and placement records have passed validation. */
    memcpy(g_MapMakerMaterialTexture, payload, materialbytes);
    g_MapMakerModuleCount = header.module_count;
    memcpy(g_MapMakerModules, payload + materialbytes, modulebytes);
    g_MapMakerEntityCount = entitycount;
    if (entitybytes > 0)
        memcpy(g_MapMakerEntities, payload + materialbytes + modulebytes, entitybytes);
    g_MapMakerPlayerStartCount = startcount;
    if (startbytes > 0)
        memcpy(g_MapMakerPlayerStarts, payload + materialbytes + modulebytes + entitybytes, startbytes);
    g_MapMakerAdvancedMeshCount = 0;
    g_MapMakerAdvancedLoopCount = 0;
    g_MapMakerAdvancedVertexCount = 0;
    g_MapMakerAdvancedPortalCount = 0;
    g_MapMakerEditorMode = MAPMAKER_EDITOR_BASIC;
    memset(g_MapMakerCurrentName, 0, sizeof(g_MapMakerCurrentName));
    memcpy(g_MapMakerCurrentName, header.name, sizeof(header.name));
    g_MapMakerCurrentName[sizeof(g_MapMakerCurrentName) - 1] = '\0';
    if (!g_MapMakerCurrentName[0])
        strcpy(g_MapMakerCurrentName, "Untitled");
    g_MapMakerDirty = 0;
    g_MapMakerInitialized = 1;
    g_MapMakerSelectedModule = -1;
    return 1;
}


static s32 mapmakerApplyV8SaveBlob(const u8 *buffer, s32 available)
{
    GeMapHeader header;
    GeMapV8Meta meta;
    const u8 *cursor;
    const u8 *end;
    u16 palette[MM_V8_MAX_TEXTURE_PALETTE];
    s32 i;

    if (!mapmakerValidateV8SaveBlob(buffer, available)) return 0;
    memcpy(&header, buffer, sizeof(header));
    cursor = buffer + sizeof(GeMapHeader);
    end = cursor + header.payload_size;
    if (!mapmakerStreamRead(&cursor, end, &meta, sizeof(meta))) return 0;

    if (!mapmakerStreamRead(&cursor, end, g_MapMakerMaterialTexture, sizeof(g_MapMakerMaterialTexture))) return 0;
    if (!mapmakerStreamRead(&cursor, end, palette, meta.texture_count * sizeof(u16))) return 0;

    g_MapMakerModuleCount = header.module_count;
    for (i = 0; i < g_MapMakerModuleCount; i++)
    {
        MapMakerModuleInstance *module = &g_MapMakerModules[i];
        s8 gx, gy, gz;
        u8 typerot;
        u8 mask;
        s32 surface;

        memset(module, 0, sizeof(*module));
        for (surface = 0; surface < MAPMAKER_SURFACE_COUNT; surface++)
            module->surface_texture[surface] = MAPMAKER_TEXTURE_NONE;

        if (!mapmakerStreamRead(&cursor, end, &gx, 1)
                || !mapmakerStreamRead(&cursor, end, &gy, 1)
                || !mapmakerStreamRead(&cursor, end, &gz, 1)
                || !mapmakerStreamRead(&cursor, end, &typerot, 1)
                || !mapmakerStreamRead(&cursor, end, &module->flags, sizeof(module->flags))
                || !mapmakerStreamRead(&cursor, end, &mask, 1)) return 0;

        module->grid_x = gx;
        module->grid_y = gy;
        module->grid_z = gz;
        module->type = typerot & 7;
        module->rotation = (typerot >> 3) & 3;

        for (surface = 0; surface < MAPMAKER_SURFACE_COUNT; surface++)
        {
            if (mask & (1 << surface))
            {
                u8 token;
                if (!mapmakerStreamRead(&cursor, end, &token, 1)) return 0;
                if (token == MM_V8_TEX_RAW)
                {
                    if (!mapmakerStreamRead(&cursor, end, &module->surface_texture[surface], sizeof(u16))) return 0;
                }
                else
                {
                    if (token >= meta.texture_count) return 0;
                    module->surface_texture[surface] = palette[token];
                }
            }
        }
    }

    g_MapMakerEntityCount = header.object_count;
    if (!mapmakerStreamRead(&cursor, end, g_MapMakerEntities,
            g_MapMakerEntityCount * sizeof(MapMakerEntityInstance))) return 0;

    g_MapMakerPlayerStartCount = meta.start_count;
    if (!mapmakerStreamRead(&cursor, end, g_MapMakerPlayerStarts,
            g_MapMakerPlayerStartCount * sizeof(MapMakerPlayerStart))) return 0;

    g_MapMakerAdvancedMeshCount = meta.advanced_mesh_count;
    if (!mapmakerStreamRead(&cursor, end, g_MapMakerAdvancedMeshes,
            g_MapMakerAdvancedMeshCount * sizeof(MapMakerAdvancedMesh))) return 0;
    g_MapMakerAdvancedLoopCount = meta.advanced_loop_count;
    if (!mapmakerStreamRead(&cursor, end, g_MapMakerAdvancedLoops,
            g_MapMakerAdvancedLoopCount * sizeof(MapMakerAdvancedLoop))) return 0;
    g_MapMakerAdvancedVertexCount = meta.advanced_vertex_count;
    if (!mapmakerStreamRead(&cursor, end, g_MapMakerAdvancedVertices,
            g_MapMakerAdvancedVertexCount * sizeof(MapMakerAdvancedVertex))) return 0;
    g_MapMakerAdvancedPortalCount = meta.portal_count;
    if (!mapmakerStreamRead(&cursor, end, g_MapMakerAdvancedPortals,
            g_MapMakerAdvancedPortalCount * sizeof(MapMakerAdvancedPortal))) return 0;

    if (cursor != end) return 0;

    g_MapMakerGridSize = meta.grid_size;
    g_MapMakerMusicTrack = meta.music_track;
    g_MapMakerEditorMode = meta.editor_mode;
    memset(g_MapMakerCurrentName, 0, sizeof(g_MapMakerCurrentName));
    memcpy(g_MapMakerCurrentName, header.name, sizeof(header.name));
    g_MapMakerCurrentName[sizeof(g_MapMakerCurrentName) - 1] = '\0';
    if (!g_MapMakerCurrentName[0]) strcpy(g_MapMakerCurrentName, "Untitled");

    g_MapMakerSelectedModule = -1;
    g_MapMakerSelectedSurface = -1;
    g_MapMakerAdvancedSelectedMesh = -1;
    g_MapMakerAdvancedSelectedVertex = -1;
    g_MapMakerAdvancedSelectedPortal = -1;
    g_MapMakerAdvancedDraggingVertex = 0;
    g_MapMakerDirty = 0;
    g_MapMakerInitialized = 1;
    return 1;
}

static s32 mapmakerApplySaveBlob(const u8 *buffer, s32 available)
{
    GeMapHeader header;

    if (buffer == NULL || available < (s32)sizeof(header)) return 0;
    memcpy(&header, buffer, sizeof(header));
    if (header.version == 5 || header.version == 6 || header.version == 7)
        return available >= MM_SAVE_BYTES && mapmakerApplyLegacySaveBlob(buffer);
    if (header.version == GEMAP_VERSION)
        return mapmakerApplyV8SaveBlob(buffer, available);
    return 0;
}

static void mapmakerResetPrivateTexturePool(void);

static s32 mapmakerPakFindFile(OSPfs *pfs, s32 *file_no)
{
    __OSDir dir;
    s32 i;
    s32 j;
    s32 ret;

    *file_no = -1;
    if (pfs->activebank != 0)
    {
        pfs->activebank = 0;
        ret = __osPfsSelectBank(pfs);
        if (ret != 0) return ret;
    }

    for (i = 0; i < pfs->dir_size; i++)
    {
        ret = __osContRamRead(pfs->queue, pfs->channel, pfs->dir_table + i, (u8 *)&dir);
        if (ret != 0) return ret;
        if (dir.company_code == MM_PFS_COMPANY && dir.game_code == MM_PFS_GAME_CODE)
        {
            for (j = 0; j < PFS_FILE_NAME_LEN; j++)
                if (dir.game_name[j] != g_MapMakerPakName[j]) break;
            if (j != PFS_FILE_NAME_LEN) continue;
            for (j = 0; j < PFS_FILE_EXT_LEN; j++)
                if (dir.ext_name[j] != g_MapMakerPakExt[j]) break;
            if (j == PFS_FILE_EXT_LEN)
            {
                *file_no = i;
                return 0;
            }
        }
    }
    return PFS_ERR_INVALID;
}

static s32 mapmakerPakAllocateFile(OSPfs *pfs, s32 *file_no, s32 pagecount)
{
    __OSInode inode;
    __OSDir dir;
    s32 freepages[MM_PAK_MAX_PAGES];
    s32 freecount = 0;
    s32 dirslot = -1;
    s32 i;
    s32 ret;

    if (pfs->banks != 1 || pagecount < 1 || pagecount > MM_PAK_MAX_PAGES)
        return PFS_ERR_INVALID;

    for (i = 0; i < pfs->dir_size; i++)
    {
        ret = __osContRamRead(pfs->queue, pfs->channel, pfs->dir_table + i, (u8 *)&dir);
        if (ret != 0) return ret;
        if (dir.company_code == 0 && dir.game_code == 0)
        {
            dirslot = i;
            break;
        }
    }
    if (dirslot < 0) return PFS_DIR_FULL;

    ret = __osPfsRWInode(pfs, &inode, PFS_READ, 0);
    if (ret != 0) return ret;

    for (i = pfs->inode_start_page; i < 128 && freecount < pagecount; i++)
    {
        if (inode.inode_page[i].ipage == 3)
            freepages[freecount++] = i;
    }
    if (freecount < pagecount) return PFS_DATA_FULL;

    for (i = 0; i < pagecount - 1; i++)
    {
        inode.inode_page[freepages[i]].inode_t.bank = 0;
        inode.inode_page[freepages[i]].inode_t.page = freepages[i + 1];
    }
    inode.inode_page[freepages[pagecount - 1]].ipage = MM_PFS_EOF;

    ret = __osPfsRWInode(pfs, &inode, PFS_WRITE, 0);
    if (ret != 0) return ret;

    memset(&dir, 0, sizeof(dir));
    dir.game_code = MM_PFS_GAME_CODE;
    dir.company_code = MM_PFS_COMPANY;
    dir.start_page.inode_t.bank = 0;
    dir.start_page.inode_t.page = freepages[0];
    memcpy(dir.game_name, g_MapMakerPakName, PFS_FILE_NAME_LEN);
    memcpy(dir.ext_name, g_MapMakerPakExt, PFS_FILE_EXT_LEN);

    ret = __osContRamWrite(pfs->queue, pfs->channel, pfs->dir_table + dirslot, (u8 *)&dir, FALSE);
    if (ret == 0) *file_no = dirslot;
    return ret;
}

static s32 mapmakerPakGetPageList(OSPfs *pfs, s32 file_no, __OSDir *dirout)
{
    __OSDir dir;
    __OSInode inode;
    __OSInodeUnit page;
    s32 ret;

    if (file_no < 0 || file_no >= pfs->dir_size || pfs->banks != 1)
        return PFS_ERR_INVALID;

    if (pfs->activebank != 0)
    {
        pfs->activebank = 0;
        ret = __osPfsSelectBank(pfs);
        if (ret != 0) return ret;
    }

    ret = __osContRamRead(pfs->queue, pfs->channel, pfs->dir_table + file_no, (u8 *)&dir);
    if (ret != 0) return ret;
    if (dir.company_code != MM_PFS_COMPANY || dir.game_code != MM_PFS_GAME_CODE)
        return PFS_ERR_INVALID;

    ret = __osPfsRWInode(pfs, &inode, PFS_READ, 0);
    if (ret != 0) return ret;

    memset(g_MapMakerPakVisitedPages, 0, sizeof(g_MapMakerPakVisitedPages));
    g_MapMakerPakPageCount = 0;
    page = dir.start_page;

    for (;;)
    {
        s32 pageindex = page.inode_t.page;
        __OSInodeUnit next;

        if (g_MapMakerPakPageCount >= MM_PAK_MAX_PAGES
                || page.inode_t.bank != 0
                || pageindex < pfs->inode_start_page || pageindex >= 128
                || g_MapMakerPakVisitedPages[pageindex])
            return PFS_ERR_INCONSISTENT;

        g_MapMakerPakVisitedPages[pageindex] = 1;
        g_MapMakerPakPageList[g_MapMakerPakPageCount++] = (u8)pageindex;
        next = inode.inode_page[pageindex];
        if (next.ipage == MM_PFS_EOF) break;
        if (next.ipage == 3) return PFS_ERR_INCONSISTENT;
        page = next;
    }

    if (dirout != NULL) memcpy(dirout, &dir, sizeof(dir));
    return g_MapMakerPakPageCount > 0 ? 0 : PFS_ERR_INCONSISTENT;
}

static s32 mapmakerPakResizeFile(OSPfs *pfs, s32 file_no, s32 pagecount)
{
    __OSDir dir;
    __OSInode inode;
    s32 freepages[MM_PAK_MAX_PAGES];
    s32 oldcount;
    s32 need;
    s32 freecount = 0;
    s32 i;
    s32 ret;

    if (pagecount < 1 || pagecount > MM_PAK_MAX_PAGES) return PFS_ERR_INVALID;
    ret = mapmakerPakGetPageList(pfs, file_no, &dir);
    if (ret != 0) return ret;
    oldcount = g_MapMakerPakPageCount;
    if (oldcount == pagecount) return 0;

    ret = __osPfsRWInode(pfs, &inode, PFS_READ, 0);
    if (ret != 0) return ret;

    if (pagecount > oldcount)
    {
        need = pagecount - oldcount;
        for (i = pfs->inode_start_page; i < 128 && freecount < need; i++)
        {
            if (inode.inode_page[i].ipage == 3)
                freepages[freecount++] = i;
        }
        if (freecount < need) return PFS_DATA_FULL;

        inode.inode_page[g_MapMakerPakPageList[oldcount - 1]].inode_t.bank = 0;
        inode.inode_page[g_MapMakerPakPageList[oldcount - 1]].inode_t.page = freepages[0];
        for (i = 0; i < need - 1; i++)
        {
            inode.inode_page[freepages[i]].inode_t.bank = 0;
            inode.inode_page[freepages[i]].inode_t.page = freepages[i + 1];
        }
        inode.inode_page[freepages[need - 1]].ipage = MM_PFS_EOF;
    }
    else
    {
        inode.inode_page[g_MapMakerPakPageList[pagecount - 1]].ipage = MM_PFS_EOF;
        for (i = pagecount; i < oldcount; i++)
            inode.inode_page[g_MapMakerPakPageList[i]].ipage = 3;
    }

    return __osPfsRWInode(pfs, &inode, PFS_WRITE, 0);
}

static s32 mapmakerPakReadWrite(OSPfs *pfs, s32 file_no, u8 flag, u8 *buffer, s32 bytes)
{
    __OSDir dir;
    s32 page;
    s32 block;
    s32 offset = 0;
    s32 pagebytes = BLOCKSIZE * PFS_ONE_PAGE;
    s32 pagecount;
    s32 ret;

    if (buffer == NULL || (flag != PFS_READ && flag != PFS_WRITE)
            || bytes <= 0 || (bytes % pagebytes) != 0)
        return PFS_ERR_INVALID;

    ret = mapmakerPakGetPageList(pfs, file_no, &dir);
    if (ret != 0) return ret;
    if (flag == PFS_READ && (dir.status & DIR_STATUS_OCCUPIED) == 0)
        return PFS_ERR_BAD_DATA;

    pagecount = bytes / pagebytes;
    if (pagecount < 1 || pagecount > g_MapMakerPakPageCount)
        return PFS_ERR_INCONSISTENT;

    for (page = 0; page < pagecount; page++)
    {
        for (block = 0; block < PFS_ONE_PAGE; block++)
        {
            u16 address = g_MapMakerPakPageList[page] * PFS_ONE_PAGE + block;
            if (flag == PFS_READ)
                ret = __osContRamRead(pfs->queue, pfs->channel, address, buffer + offset);
            else
                ret = __osContRamWrite(pfs->queue, pfs->channel, address, buffer + offset, FALSE);
            if (ret != 0) return ret;
            offset += BLOCKSIZE;
        }
    }

    if (flag == PFS_WRITE && (dir.status & DIR_STATUS_OCCUPIED) == 0)
    {
        dir.status |= DIR_STATUS_OCCUPIED;
        ret = __osContRamWrite(pfs->queue, pfs->channel, pfs->dir_table + file_no, (u8 *)&dir, FALSE);
        if (ret != 0) return ret;
    }
    return 0;
}

static s32 mapmakerPakOpen(s32 *file_no, s32 create, s32 pagecount)
{
    s32 ret;

    memset(&g_MapMakerPfs, 0, sizeof(g_MapMakerPfs));
    ret = osPfsInit(&g_ContInputMessageQueue, &g_MapMakerPfs, PLAYER_1);
    if (ret != 0 && ret != PFS_ERR_INCONSISTENT) return ret;
    if (g_MapMakerPfs.queue == NULL || g_MapMakerPfs.channel != PLAYER_1
            || g_MapMakerPfs.banks != 1 || g_MapMakerPfs.dir_size <= 0
            || g_MapMakerPfs.dir_size > 16 || g_MapMakerPfs.inode_start_page <= 0
            || g_MapMakerPfs.inode_start_page >= 128)
        return PFS_ERR_DEVICE;

    ret = mapmakerPakFindFile(&g_MapMakerPfs, file_no);
    if (ret == 0)
    {
        if (!create) return 0;
        return mapmakerPakResizeFile(&g_MapMakerPfs, *file_no, pagecount);
    }
    if (!create || ret != PFS_ERR_INVALID) return ret;
    return mapmakerPakAllocateFile(&g_MapMakerPfs, file_no, pagecount);
}

static s32 mapmakerSaveMap(void)
{
    u8 *buffer = (u8 *)g_MapMakerTextureBuffer;
    GeMapHeader expected;
    GeMapHeader verified;
    s32 pagebytes = BLOCKSIZE * PFS_ONE_PAGE;
    s32 roundedbytes;
    s32 pagecount;
    s32 file_no = -1;
    s32 ret;
    s32 verifyret = PFS_ERR_INVALID;

    if (g_MapMakerStorageBusy)
    {
        strcpy(g_MapMakerStorageStatus, "Storage busy - try again");
        return 0;
    }
    g_MapMakerStorageBusy = 1;

    if (!mapmakerBuildSaveBlob(buffer, MM_PAK_MAX_BYTES))
    {
        if (!g_MapMakerStorageStatus[0])
            strcpy(g_MapMakerStorageStatus, "Save refused: current map failed validation");
        g_MapMakerStorageBusy = 0;
        return 0;
    }

    roundedbytes = (g_MapMakerLastSaveBytes + pagebytes - 1) & ~(pagebytes - 1);
    pagecount = roundedbytes / pagebytes;
    if (pagecount < 1 || pagecount > MM_PAK_MAX_PAGES)
    {
        strcpy(g_MapMakerStorageStatus, "Packed map exceeds physical Controller Pak capacity");
        g_MapMakerStorageBusy = 0;
        return 0;
    }
    memcpy(&expected, buffer, sizeof(expected));

    if (g_MapMakerLastSaveBytes <= MM_SAVE_BYTES)
    {
        memset(g_MapMakerSessionSave.bytes, 0, MM_SAVE_BYTES);
        memcpy(g_MapMakerSessionSave.bytes, buffer, g_MapMakerLastSaveBytes);
        g_MapMakerSessionSaveValid = 1;
    }
    else
    {
        g_MapMakerSessionSaveValid = 0;
    }

    joyDisablePoll();
    ret = mapmakerPakOpen(&file_no, 1, pagecount);
    if (ret == 0)
        ret = mapmakerPakReadWrite(&g_MapMakerPfs, file_no, PFS_WRITE, buffer, roundedbytes);
    if (ret == 0)
    {
        memset(buffer, 0, roundedbytes);
        verifyret = mapmakerPakReadWrite(&g_MapMakerPfs, file_no, PFS_READ, buffer, roundedbytes);
    }
    joyEnablePoll();

    if (ret == 0 && verifyret == 0
            && mapmakerValidateSaveBlob(buffer, roundedbytes))
    {
        memcpy(&verified, buffer, sizeof(verified));
        if (memcmp(&expected, &verified, sizeof(expected)) == 0)
        {
            strcpy(g_MapMakerStorageStatus, "Saved + CRC verified: Controller Pak 1");
            g_MapMakerDirty = 0;
            g_MapMakerStorageBusy = 0;
            mapmakerResetPrivateTexturePool();
            return 1;
        }
    }

    if (ret == PFS_DATA_FULL)
        strcpy(g_MapMakerStorageStatus, "Controller Pak has too little free space for packed map");
    else if (ret == 0)
        strcpy(g_MapMakerStorageStatus, "Pak verify failed; current map remains in RAM");
    else
        strcpy(g_MapMakerStorageStatus, "Controller Pak unavailable; current map remains in RAM");
    g_MapMakerStorageBusy = 0;
    mapmakerResetPrivateTexturePool();
    return 0;
}

static s32 mapmakerLoadMapInternal(s32 allowSessionFallback)
{
    u8 *buffer = (u8 *)g_MapMakerTextureBuffer;
    s32 pagebytes = BLOCKSIZE * PFS_ONE_PAGE;
    s32 file_no = -1;
    s32 bytes = 0;
    s32 ret;

    if (g_MapMakerStorageBusy)
    {
        strcpy(g_MapMakerStorageStatus, "Storage busy - try again");
        return 0;
    }

    g_MapMakerStorageBusy = 1;
    joyDisablePoll();
    ret = mapmakerPakOpen(&file_no, 0, 0);
    if (ret == 0)
    {
        ret = mapmakerPakGetPageList(&g_MapMakerPfs, file_no, NULL);
        if (ret == 0)
        {
            bytes = g_MapMakerPakPageCount * pagebytes;
            if (bytes <= 0 || bytes > MM_PAK_MAX_BYTES)
                ret = PFS_ERR_INCONSISTENT;
            else
            {
                memset(buffer, 0, bytes);
                ret = mapmakerPakReadWrite(&g_MapMakerPfs, file_no, PFS_READ, buffer, bytes);
            }
        }
    }
    joyEnablePoll();

    if (ret == 0 && mapmakerValidateSaveBlob(buffer, bytes)
            && mapmakerApplySaveBlob(buffer, bytes))
    {
        if (bytes <= MM_SAVE_BYTES)
        {
            memset(g_MapMakerSessionSave.bytes, 0, MM_SAVE_BYTES);
            memcpy(g_MapMakerSessionSave.bytes, buffer, bytes);
            g_MapMakerSessionSaveValid = 1;
        }
        else
        {
            g_MapMakerSessionSaveValid = 0;
        }
        strcpy(g_MapMakerStorageStatus, "Loaded + CRC checked: Controller Pak 1");
        g_MapMakerStorageBusy = 0;
        mapmakerResetPrivateTexturePool();
        return 1;
    }

    if (allowSessionFallback && g_MapMakerSessionSaveValid
            && mapmakerValidateSaveBlob(g_MapMakerSessionSave.bytes, MM_SAVE_BYTES)
            && mapmakerApplySaveBlob(g_MapMakerSessionSave.bytes, MM_SAVE_BYTES))
    {
        strcpy(g_MapMakerStorageStatus, "Pak load unavailable/invalid - loaded safe session copy");
        g_MapMakerStorageBusy = 0;
        mapmakerResetPrivateTexturePool();
        return 1;
    }

    if (allowSessionFallback)
        strcpy(g_MapMakerStorageStatus, "Load refused: no valid map save; current map unchanged");
    else
        strcpy(g_MapMakerStorageStatus, "Controller Pak map unavailable/invalid; current map unchanged");
    g_MapMakerStorageBusy = 0;
    mapmakerResetPrivateTexturePool();
    return 0;
}

static s32 mapmakerLoadMap(void)
{
    return mapmakerLoadMapInternal(1);
}



static s32 mapmakerScratchIsValid(void);
static void mapmakerWorldToModuleLocal(const MapMakerModuleInstance *module, f32 worldx, f32 worldz, f32 *lx, f32 *lz);
#ifdef GE_MODDED_CHEATS
static s32 mapmakerNativeMirrorRotation(s32 rotation);
#endif
static void mapmakerSetSurfaceFlags(MapMakerModuleInstance *module, s32 surface, u16 flags);
static void mapmakerRotateOffset(s32 x, s32 z, s32 rotation, s32 *outx, s32 *outz);

typedef struct MapMakerControlAxes {
    f32 forward;
    f32 strafe;
    f32 yaw;
    f32 pitch;
} MapMakerControlAxes;

/* The frontend Options page writes the selected control type directly to the
 * active folder save.  Map Maker runs from the frontend, before the normal
 * stage-load path copies that save setting into g_CurrentPlayer, so reading
 * cur_player_get_control_type() here can report the stale default (1.1). */
extern save_data *fileGetSaveForFoldernum(s32 foldernum);

static s32 mapmakerGetConfiguredControlType(void)
{
    save_data *save = fileGetSaveForFoldernum(selected_folder_num);
    s32 type;

    if (save != NULL)
    {
        type = (save->options & OPTION_CONTROLTYPE) >> 8;
        if (type >= CONTROLLER_CONFIG_HONEY && type <= CONTROLLER_CONFIG_GOODHEAD)
            return type;
    }

    return cur_player_get_control_type();
}

static s32 mapmakerGetConfiguredInvertLook(void)
{
    save_data *save = fileGetSaveForFoldernum(selected_folder_num);
    if (save != NULL)
        return (save->options & OPTION_INVERTLOOK) != 0;
    return get_cur_player_look_vertical_inverted() != 0;
}

static f32 mapmakerSafeStick(s32 value)
{
    if (value < -5) return (f32)(value + 5) / 75.0f;
    if (value > 5) return (f32)(value - 5) / 75.0f;
    return 0.0f;
}

static void mapmakerReadControlAxes(MapMakerControlAxes *axes, s32 includeDpad)
{
    s32 control = mapmakerGetConfiguredControlType();
    f32 sx1 = mapmakerSafeStick(joyGetStickX(PLAYER_1));
    f32 sy1 = mapmakerSafeStick(joyGetStickY(PLAYER_1));
    u32 buttons = joyGetButtons(PLAYER_1, 0xffff);
    u32 left = L_CBUTTONS;
    u32 right = R_CBUTTONS;
    u32 up = U_CBUTTONS;
    u32 down = D_CBUTTONS;

    axes->forward = 0.0f;
    axes->strafe = 0.0f;
    axes->yaw = 0.0f;
    axes->pitch = 0.0f;

    if (includeDpad)
    {
        left |= L_JPAD;
        right |= R_JPAD;
        up |= U_JPAD;
        down |= D_JPAD;
    }

    if (control >= CONTROLLER_CONFIG_PLENTY && control <= CONTROLLER_CONFIG_GOODHEAD
            && joyGetControllerCount() >= 2)
    {
        f32 sx2 = mapmakerSafeStick(joyGetStickX(PLAYER_2));
        f32 sy2 = mapmakerSafeStick(joyGetStickY(PLAYER_2));

        if (control == CONTROLLER_CONFIG_PLENTY || control == CONTROLLER_CONFIG_DOMINO)
        {
            /* 2.1 / 2.3: controller 1 is Walk/Turn, controller 2 is Strafe/Look. */
            axes->forward = sy1;
            axes->yaw = sx1;
            axes->strafe = sx2;
            axes->pitch = sy2;
        }
        else
        {
            /* 2.2 / 2.4: controller 1 is Turn/Look, controller 2 is Strafe/Walk. */
            axes->yaw = sx1;
            axes->pitch = sy1;
            axes->strafe = sx2;
            axes->forward = sy2;
        }

        /* GoldenEye option 0 is Reverse and 1 is Upright.  Our camera pitch is
         * positive-up, so Reverse maps stick-up to looking down. */
        if (mapmakerGetConfiguredInvertLook() == 0)
            axes->pitch = -axes->pitch;
        return;
    }

    if (control == CONTROLLER_CONFIG_SOLITARE || control == CONTROLLER_CONFIG_GOODNIGHT)
    {
        /* 1.2 / 1.4: stick is Turn/Look, C buttons are Move/Strafe. */
        axes->yaw = sx1;
        axes->pitch = sy1;
        if (buttons & up) axes->forward += 1.0f;
        if (buttons & down) axes->forward -= 1.0f;
        if (buttons & right) axes->strafe += 1.0f;
        if (buttons & left) axes->strafe -= 1.0f;
        if (mapmakerGetConfiguredInvertLook() == 0)
            axes->pitch = -axes->pitch;
    }
    else
    {
        /* 1.1 / 1.3: stick is Walk/Turn, C buttons are Strafe/Look. */
        axes->forward = sy1;
        axes->yaw = sx1;
        if (buttons & right) axes->strafe += 1.0f;
        if (buttons & left) axes->strafe -= 1.0f;
        if (buttons & down) axes->pitch += 1.0f;
        if (buttons & up) axes->pitch -= 1.0f;
        if (mapmakerGetConfiguredInvertLook() != 0)
            axes->pitch = -axes->pitch;
    }
}

static void mapmakerReadEditorAxes(MapMakerControlAxes *axes)
{
    f32 sx = mapmakerSafeStick(joyGetStickX(PLAYER_1));
    f32 sy = mapmakerSafeStick(joyGetStickY(PLAYER_1));
    u32 buttons = joyGetButtons(PLAYER_1, U_CBUTTONS | D_CBUTTONS | L_CBUTTONS | R_CBUTTONS);

    /* SnapMap-style editor controls are intentionally fixed and simple:
     * analog stick always looks; C buttons always move.  GoldenEye gameplay
     * controls remain untouched and are used by the eventual native test mode. */
    axes->yaw = sx;
    axes->pitch = sy;
    if (mapmakerGetConfiguredInvertLook() == 0)
        axes->pitch = -axes->pitch;
    axes->forward = 0.0f;
    axes->strafe = 0.0f;
    if (buttons & U_CBUTTONS) axes->forward += 1.0f;
    if (buttons & D_CBUTTONS) axes->forward -= 1.0f;
    if (buttons & R_CBUTTONS) axes->strafe += 1.0f;
    if (buttons & L_CBUTTONS) axes->strafe -= 1.0f;
}

static void mapmakerGetCamera(f32 *camx, f32 *camy, f32 *camz,
        f32 *dirx, f32 *diry, f32 *dirz)
{
    f32 cp;

    if (g_MapMakerPlaytest)
    {
        cp = cosf(g_MapMakerPlayerPitch);
        *camx = g_MapMakerPlayerX;
        *camy = g_MapMakerPlayerY;
        *camz = g_MapMakerPlayerZ;
        *dirx = sinf(g_MapMakerPlayerYaw) * cp;
        *diry = sinf(g_MapMakerPlayerPitch);
        *dirz = cosf(g_MapMakerPlayerYaw) * cp;
    }
    else if (g_MapMakerFreeView)
    {
        cp = cosf(g_MapMakerCameraPitch);
        *camx = g_MapMakerFreeCameraX;
        *camy = g_MapMakerFreeCameraY;
        *camz = g_MapMakerFreeCameraZ;
        *dirx = sinf(g_MapMakerCameraYaw) * cp;
        *diry = sinf(g_MapMakerCameraPitch);
        *dirz = cosf(g_MapMakerCameraYaw) * cp;
    }
    else
    {
        f32 targetx = g_MapMakerCursorX * MM_CELL_SIZE;
        f32 targety = g_MapMakerCursorY * MM_LAYER_HEIGHT + 40.0f;
        f32 targetz = g_MapMakerCursorZ * MM_CELL_SIZE;
        cp = cosf(g_MapMakerCameraPitch);
        *camx = targetx + sinf(g_MapMakerCameraYaw) * cp * g_MapMakerCameraDistance;
        *camy = targety + sinf(g_MapMakerCameraPitch) * g_MapMakerCameraDistance;
        *camz = targetz + cosf(g_MapMakerCameraYaw) * cp * g_MapMakerCameraDistance;
        *dirx = targetx - *camx;
        *diry = targety - *camy;
        *dirz = targetz - *camz;
        {
            f32 len = sqrtf(*dirx * *dirx + *diry * *diry + *dirz * *dirz);
            if (len > 0.001f)
            {
                *dirx /= len;
                *diry /= len;
                *dirz /= len;
            }
        }
    }
}

static s32 mapmakerRoundCell(f32 value)
{
    f32 scaled = value / MM_CELL_SIZE;
    if (scaled >= 0.0f) return (s32)(scaled + 0.5f);
    return (s32)(scaled - 0.5f);
}

static void mapmakerUpdateFreeCursor(void)
{
    f32 camx;
    f32 camy;
    f32 camz;
    f32 dirx;
    f32 diry;
    f32 dirz;
    f32 plane = g_MapMakerCursorY * MM_LAYER_HEIGHT + 4.0f;
    f32 t;

    mapmakerGetCamera(&camx, &camy, &camz, &dirx, &diry, &dirz);
    if (diry > -0.001f && diry < 0.001f) return;

    t = (plane - camy) / diry;
    if (t <= 20.0f || t > 6000.0f) return;

    g_MapMakerEntityCursorX = camx + dirx * t;
    g_MapMakerEntityCursorY = plane;
    g_MapMakerEntityCursorZ = camz + dirz * t;
    if (g_MapMakerSnapEnabled)
    {
        f32 step = (f32)g_MapMakerGridSize;
        f32 scaledx = g_MapMakerEntityCursorX / step;
        f32 scaledz = g_MapMakerEntityCursorZ / step;
        s32 cellx = (s32)(scaledx + (scaledx >= 0.0f ? 0.5f : -0.5f));
        s32 cellz = (s32)(scaledz + (scaledz >= 0.0f ? 0.5f : -0.5f));
        g_MapMakerEntityCursorX = (f32)cellx * step;
        g_MapMakerEntityCursorZ = (f32)cellz * step;
    }
    g_MapMakerCursorX = mapmakerRoundCell(camx + dirx * t);
    g_MapMakerCursorZ = mapmakerRoundCell(camz + dirz * t);
    if (g_MapMakerCursorX < MM_GRID_MIN) g_MapMakerCursorX = MM_GRID_MIN;
    if (g_MapMakerCursorX > MM_GRID_MAX) g_MapMakerCursorX = MM_GRID_MAX;
    if (g_MapMakerCursorZ < MM_GRID_MIN) g_MapMakerCursorZ = MM_GRID_MIN;
    if (g_MapMakerCursorZ > MM_GRID_MAX) g_MapMakerCursorZ = MM_GRID_MAX;
}

static void mapmakerWorldVectorToModuleLocal(s32 rotation, f32 x, f32 z, f32 *lx, f32 *lz)
{
    switch (rotation & 3)
    {
    case 1: *lx = -z; *lz = x; break;
    case 2: *lx = -x; *lz = -z; break;
    case 3: *lx = z; *lz = -x; break;
    default:*lx = x; *lz = z; break;
    }
}

static s32 mapmakerRayBoxLocal(f32 ox, f32 oy, f32 oz, f32 dx, f32 dy, f32 dz,
        f32 minx, f32 miny, f32 minz, f32 maxx, f32 maxy, f32 maxz,
        f32 *outt, s32 *outsurface)
{
    f32 tmin = -1000000.0f;
    f32 tmax = 1000000.0f;
    f32 t1;
    f32 t2;
    s32 face = MAPSURFACE_TOP;
    s32 f1;

#define MM_RAY_AXIS(origin, direction, lo, hi, face_lo, face_hi) \
    if ((direction) > -0.0001f && (direction) < 0.0001f) { \
        if ((origin) < (lo) || (origin) > (hi)) return 0; \
    } else { \
        s32 nearface = (face_lo); \
        t1 = ((lo) - (origin)) / (direction); \
        t2 = ((hi) - (origin)) / (direction); \
        if (t1 > t2) { \
            f32 tmp = t1; t1 = t2; t2 = tmp; \
            nearface = (face_hi); \
        } \
        f1 = nearface; \
        if (t1 > tmin) { tmin = t1; face = f1; } \
        if (t2 < tmax) tmax = t2; \
        if (tmin > tmax) return 0; \
    }

    MM_RAY_AXIS(ox, dx, minx, maxx, MAPSURFACE_WEST, MAPSURFACE_EAST);
    MM_RAY_AXIS(oy, dy, miny, maxy, MAPSURFACE_BOTTOM, MAPSURFACE_TOP);
    MM_RAY_AXIS(oz, dz, minz, maxz, MAPSURFACE_NORTH, MAPSURFACE_SOUTH);
#undef MM_RAY_AXIS

    if (tmax < 0.0f) return 0;
    if (tmin < 0.0f) tmin = tmax;
    if (tmin < 0.0f) return 0;
    *outt = tmin;
    *outsurface = face;
    return 1;
}

static void mapmakerConsiderModuleBox(const MapMakerModuleInstance *module, s32 moduleindex,
        f32 ox, f32 oy, f32 oz, f32 dx, f32 dy, f32 dz,
        f32 minx, f32 miny, f32 minz, f32 maxx, f32 maxy, f32 maxz,
        f32 *bestt, s32 *bestmodule, s32 *bestsurface)
{
    f32 t;
    s32 surface;
    if (mapmakerRayBoxLocal(ox, oy, oz, dx, dy, dz, minx, miny, minz, maxx, maxy, maxz, &t, &surface)
            && t < *bestt)
    {
        *bestt = t;
        *bestmodule = moduleindex;
        *bestsurface = surface;
    }
}

static void mapmakerUpdateSurfaceSelection(void)
{
    f32 camx;
    f32 camy;
    f32 camz;
    f32 dirx;
    f32 diry;
    f32 dirz;
    f32 bestt = 1000000.0f;
    s32 bestmodule = -1;
    s32 bestsurface = -1;
    s32 i;

    mapmakerGetCamera(&camx, &camy, &camz, &dirx, &diry, &dirz);

    for (i = 0; i < g_MapMakerModuleCount; i++)
    {
        const MapMakerModuleInstance *module = &g_MapMakerModules[i];
        f32 ox;
        f32 oz;
        f32 dx;
        f32 dz;
        f32 oy = camy - module->grid_y * MM_LAYER_HEIGHT;
        f32 xlocal;
        f32 zlocal;

        mapmakerWorldToModuleLocal(module, camx, camz, &xlocal, &zlocal);
        ox = xlocal;
        oz = zlocal;
        mapmakerWorldVectorToModuleLocal(module->rotation, dirx, dirz, &dx, &dz);

        if (module->type == MAPMODULE_FLOOR)
        {
            mapmakerConsiderModuleBox(module, i, ox, oy, oz, dx, diry, dz,
                    -80, 0, -80, 80, MM_FLOOR_HEIGHT, 80, &bestt, &bestmodule, &bestsurface);
        }
        else if (module->type == MAPMODULE_WALL)
        {
            mapmakerConsiderModuleBox(module, i, ox, oy, oz, dx, diry, dz,
                    -80, 0, -8, 80, MM_WALL_HEIGHT, 8, &bestt, &bestmodule, &bestsurface);
        }
        else if (module->type == MAPMODULE_CORNER)
        {
            mapmakerConsiderModuleBox(module, i, ox, oy, oz, dx, diry, dz,
                    -80, 0, -80, 80, MM_WALL_HEIGHT, -64, &bestt, &bestmodule, &bestsurface);
            mapmakerConsiderModuleBox(module, i, ox, oy, oz, dx, diry, dz,
                    -80, 0, -80, -64, MM_WALL_HEIGHT, 80, &bestt, &bestmodule, &bestsurface);
        }
        else if (module->type == MAPMODULE_DOORWAY)
        {
            mapmakerConsiderModuleBox(module, i, ox, oy, oz, dx, diry, dz,
                    -77, 0, -8, -43, 92, 8, &bestt, &bestmodule, &bestsurface);
            mapmakerConsiderModuleBox(module, i, ox, oy, oz, dx, diry, dz,
                    43, 0, -8, 77, 92, 8, &bestt, &bestmodule, &bestsurface);
            mapmakerConsiderModuleBox(module, i, ox, oy, oz, dx, diry, dz,
                    -80, 92, -8, 80, 120, 8, &bestt, &bestmodule, &bestsurface);
        }
        else
        {
            mapmakerConsiderModuleBox(module, i, ox, oy, oz, dx, diry, dz,
                    -80, 0, -80, 80, 80, 80, &bestt, &bestmodule, &bestsurface);
        }
    }

    g_MapMakerSelectedModule = bestmodule;
    g_MapMakerSelectedSurface = bestsurface;
}

static void mapmakerPaintSelectedSurface(s32 erase)
{
    MapMakerModuleInstance *module;
    if (g_MapMakerSelectedModule < 0 || g_MapMakerSelectedModule >= g_MapMakerModuleCount
            || g_MapMakerSelectedSurface < 0 || g_MapMakerSelectedSurface >= MAPMAKER_SURFACE_COUNT)
    {
        strcpy(g_MapMakerStorageStatus, "Texture: aim at a module surface first");
        return;
    }

    module = &g_MapMakerModules[g_MapMakerSelectedModule];
    module->surface_texture[g_MapMakerSelectedSurface] = erase ? MAPMAKER_TEXTURE_NONE : g_MapMakerMaterialTexture[g_MapMakerMaterialSlot];
    if (!erase) mapmakerSetSurfaceFlags(module, g_MapMakerSelectedSurface, (u16)g_MapMakerTextureFlip);
    mapmakerMarkDirty();
    if (erase)
        strcpy(g_MapMakerStorageStatus, "Texture removed from selected surface");
    else
        strcpy(g_MapMakerStorageStatus, "Texture applied to selected surface");
}

static void mapmakerGetModuleOrigin(const MapMakerModuleInstance *module, f32 *x, f32 *y, f32 *z)
{
    s32 ox = 0;
    s32 oz = 0;
    *x = module->grid_x * MM_CELL_SIZE;
    *y = module->grid_y * MM_LAYER_HEIGHT;
    *z = module->grid_z * MM_CELL_SIZE;

    /* Wall-like modules can be centre-anchored or edge-anchored.  Edge mode
     * uses rotation to choose the cell edge; centre mode preserves the older
     * through-the-middle placement. */
    if ((module->type == MAPMODULE_WALL || module->type == MAPMODULE_CORNER || module->type == MAPMODULE_DOORWAY)
            && (module->flags & MAPMODULE_FLAG_EDGE_ANCHOR))
    {
        mapmakerRotateOffset(0, -MM_CELL_SIZE / 2, module->rotation, &ox, &oz);
        *x += ox;
        *z += oz;
    }
}

static u16 mapmakerGetSurfaceFlags(const MapMakerModuleInstance *module, s32 surface)
{
    if (module == NULL || surface < 0 || surface >= MAPMAKER_SURFACE_COUNT) return 0;
    return (module->flags >> (surface * 2)) & 3;
}

static void mapmakerSetSurfaceFlags(MapMakerModuleInstance *module, s32 surface, u16 flags)
{
    u16 mask;
    if (module == NULL || surface < 0 || surface >= MAPMAKER_SURFACE_COUNT) return;
    mask = (u16)(3u << (surface * 2));
    module->flags = (u16)((module->flags & ~mask) | ((flags & 3u) << (surface * 2)));
}

static void mapmakerRotateSelectedTexture(void)
{
    MapMakerModuleInstance *module;
    u16 next;
    if (g_MapMakerSelectedModule < 0 || g_MapMakerSelectedModule >= g_MapMakerModuleCount
            || g_MapMakerSelectedSurface < 0 || g_MapMakerSelectedSurface >= MAPMAKER_SURFACE_COUNT)
    {
        g_MapMakerTextureFlip = (g_MapMakerTextureFlip + 1) & 3;
        strcpy(g_MapMakerStorageStatus, "Texture orientation changed for next paint");
        return;
    }
    module = &g_MapMakerModules[g_MapMakerSelectedModule];
    next = (mapmakerGetSurfaceFlags(module, g_MapMakerSelectedSurface) + 1) & 3;
    mapmakerSetSurfaceFlags(module, g_MapMakerSelectedSurface, next);
    g_MapMakerTextureFlip = next;
    mapmakerMarkDirty();
    strcpy(g_MapMakerStorageStatus, "Rotated highlighted texture 90 degrees");
}

static void mapmakerWorldToModuleLocal(const MapMakerModuleInstance *module, f32 worldx, f32 worldz, f32 *lx, f32 *lz)
{
    f32 originx;
    f32 originy;
    f32 originz;
    f32 dx;
    f32 dz;
    mapmakerGetModuleOrigin(module, &originx, &originy, &originz);
    dx = worldx - originx;
    dz = worldz - originz;

    switch (module->rotation & 3)
    {
    case 1: *lx = -dz; *lz = dx; break;
    case 2: *lx = -dx; *lz = -dz; break;
    case 3: *lx = dz; *lz = -dx; break;
    default:*lx = dx; *lz = dz; break;
    }
}

static s32 mapmakerGetFloorHeightFrom(const MapMakerModuleInstance *modules, s32 modulecount,
        f32 x, f32 z, f32 referencey, f32 *floory)
{
    s32 i;
    s32 found = 0;
    f32 best = -32768.0f;

    for (i = 0; i < modulecount; i++)
    {
        const MapMakerModuleInstance *module = &modules[i];
        f32 lx;
        f32 lz;
        f32 surface;
        f32 coarsex = x - module->grid_x * MM_CELL_SIZE;
        f32 coarsez = z - module->grid_z * MM_CELL_SIZE;

        /* A module origin can move by at most half a cell for edge anchoring.
         * Reject distant cells before the rotation/local-space transform. */
        if (coarsex < -MM_COLLISION_COARSE_RADIUS || coarsex > MM_COLLISION_COARSE_RADIUS
                || coarsez < -MM_COLLISION_COARSE_RADIUS || coarsez > MM_COLLISION_COARSE_RADIUS) continue;
        mapmakerWorldToModuleLocal(module, x, z, &lx, &lz);
        if (lx < -80.0f || lx > 80.0f || lz < -80.0f || lz > 80.0f) continue;

        surface = module->grid_y * MM_LAYER_HEIGHT + MM_FLOOR_HEIGHT;
        if (module->type == MAPMODULE_RAMP)
            surface = module->grid_y * MM_LAYER_HEIGHT + ((lz + 80.0f) * (MM_LAYER_HEIGHT / 160.0f));

        if (surface <= referencey + MM_PLAYER_STEP && surface > best)
        {
            best = surface;
            found = 1;
        }
    }

    if (found) *floory = best;
    return found;
}

static s32 mapmakerGetFloorHeight(f32 x, f32 z, f32 referencey, f32 *floory)
{
    return mapmakerGetFloorHeightFrom(g_MapMakerModules, g_MapMakerModuleCount, x, z, referencey, floory);
}

static s32 mapmakerPointInRect(f32 x, f32 z, f32 cx, f32 cz, f32 hx, f32 hz)
{
    return x > cx - hx - MM_PLAYER_RADIUS && x < cx + hx + MM_PLAYER_RADIUS
        && z > cz - hz - MM_PLAYER_RADIUS && z < cz + hz + MM_PLAYER_RADIUS;
}

static s32 mapmakerPlayerBlockedBy(const MapMakerModuleInstance *modules, s32 modulecount,
        f32 x, f32 z, f32 eyeY)
{
    s32 i;
    f32 feet = eyeY - MM_PLAYER_EYE_HEIGHT;

    for (i = 0; i < modulecount; i++)
    {
        const MapMakerModuleInstance *module = &modules[i];
        f32 lx;
        f32 lz;
        f32 basey = module->grid_y * MM_LAYER_HEIGHT;
        f32 coarsex;
        f32 coarsez;

        if (feet > basey + MM_WALL_HEIGHT || eyeY < basey) continue;
        coarsex = x - module->grid_x * MM_CELL_SIZE;
        coarsez = z - module->grid_z * MM_CELL_SIZE;
        if (coarsex < -MM_COLLISION_COARSE_RADIUS || coarsex > MM_COLLISION_COARSE_RADIUS
                || coarsez < -MM_COLLISION_COARSE_RADIUS || coarsez > MM_COLLISION_COARSE_RADIUS) continue;
        mapmakerWorldToModuleLocal(module, x, z, &lx, &lz);

        if (module->type == MAPMODULE_WALL)
        {
            if (mapmakerPointInRect(lx, lz, 0, 0, 80.0f, 8.0f)) return 1;
        }
        else if (module->type == MAPMODULE_CORNER)
        {
            if (mapmakerPointInRect(lx, lz, 0, -72.0f, 80.0f, 8.0f)
                    || mapmakerPointInRect(lx, lz, -72.0f, 0, 8.0f, 80.0f)) return 1;
        }
        else if (module->type == MAPMODULE_DOORWAY)
        {
            if (mapmakerPointInRect(lx, lz, -60.0f, 0, 17.0f, 8.0f)
                    || mapmakerPointInRect(lx, lz, 60.0f, 0, 17.0f, 8.0f)) return 1;
        }
    }
    return 0;
}

static s32 mapmakerPlayerBlocked(f32 x, f32 z, f32 eyeY)
{
    return mapmakerPlayerBlockedBy(g_MapMakerModules, g_MapMakerModuleCount, x, z, eyeY);
}

static s32 mapmakerBeginPlaytest(void)
{
    f32 floor;
    s32 i;

    if (g_MapMakerModuleCount <= 0)
    {
        strcpy(g_MapMakerStorageStatus, "First Person View: place at least one module first");
        return 0;
    }

    g_MapMakerPlayerX = g_MapMakerCursorX * MM_CELL_SIZE;
    g_MapMakerPlayerZ = g_MapMakerCursorZ * MM_CELL_SIZE;
    if (!mapmakerGetFloorHeight(g_MapMakerPlayerX, g_MapMakerPlayerZ, 32767.0f, &floor))
    {
        for (i = 0; i < g_MapMakerModuleCount; i++)
        {
            if (g_MapMakerModules[i].type == MAPMODULE_FLOOR || g_MapMakerModules[i].type == MAPMODULE_RAMP)
            {
                g_MapMakerPlayerX = g_MapMakerModules[i].grid_x * MM_CELL_SIZE;
                g_MapMakerPlayerZ = g_MapMakerModules[i].grid_z * MM_CELL_SIZE;
                break;
            }
        }
        if (!mapmakerGetFloorHeight(g_MapMakerPlayerX, g_MapMakerPlayerZ, 32767.0f, &floor))
            floor = g_MapMakerModules[0].grid_y * MM_LAYER_HEIGHT + MM_FLOOR_HEIGHT;
    }

    g_MapMakerPlayerY = floor + MM_PLAYER_EYE_HEIGHT;
    g_MapMakerPlayerYaw = g_MapMakerFreeView ? g_MapMakerCameraYaw : g_MapMakerCameraYaw + 3.14159265f;
    g_MapMakerPlayerPitch = g_MapMakerFreeView ? g_MapMakerCameraPitch : 0.0f;
    g_MapMakerPlayerVelY = 0.0f;
    g_MapMakerPlaytest = 1;
    g_MapMakerMenuOpen = 0;
    strcpy(g_MapMakerStorageStatus, "First Person View active");
    return 1;
}

static s32 mapmakerRequestNativeTest(void)
{
    if (g_MapMakerEditorMode != MAPMAKER_EDITOR_BASIC)
    {
        strcpy(g_MapMakerStorageStatus, "Native Test currently supports Basic maps only");
        return 0;
    }
    if (g_MapMakerModuleCount <= 0)
    {
        strcpy(g_MapMakerStorageStatus, "Native Test: place at least one module first");
        return 0;
    }
    if (g_MapMakerPlayerStartCount <= 0)
    {
        strcpy(g_MapMakerStorageStatus, "Native Test: place a Player Start first");
        return 0;
    }
    /* Native Test consumes the resident Expansion-Pak editor state directly.
     * It must never be gated by the much smaller Controller Pak encoding. */
    if (!mapmakerScratchIsValid())
    {
        strcpy(g_MapMakerStorageStatus, "Native Test: resident map scratch is invalid");
        return 0;
    }

    g_MapMakerNativeTestPending = 1;
    g_MapMakerNativeTestEntered = 0;
    g_MapMakerNativeReturning = 0;
    g_MapMakerNativeTestRuntimeReady = 0;
    g_MapMakerNativeOriginValid = 0;
    strcpy(g_MapMakerStorageStatus, "Launching Native Test Mode");
    return 1;
}

static void mapmakerTickPlaytest(void)
{
    u32 pressed = joyGetButtonsPressedThisFrame(PLAYER_1, 0xffff);
    MapMakerControlAxes axes;
    f32 forwardx;
    f32 forwardz;
    f32 rightx;
    f32 rightz;
    f32 newx;
    f32 newz;
    f32 floor;

    /* Forge-style fast switch: START returns straight to Edit Mode at the
     * player's current position instead of bouncing through another menu. */
    if (pressed & START_BUTTON)
    {
        g_MapMakerPlaytest = 0;
        g_MapMakerFreeView = 1;
        g_MapMakerFreeCameraX = g_MapMakerPlayerX;
        g_MapMakerFreeCameraY = g_MapMakerPlayerY;
        g_MapMakerFreeCameraZ = g_MapMakerPlayerZ;
        g_MapMakerCameraYaw = g_MapMakerPlayerYaw;
        g_MapMakerCameraPitch = g_MapMakerPlayerPitch;
        strcpy(g_MapMakerStorageStatus, "Edit Mode - START opens Map Maker menu");
        return;
    }

    /* Read the same movement layout selected for Bond (1.1-1.4 / 2.1-2.4),
     * then convert its positive-right yaw into Map Maker camera coordinates. */
    mapmakerReadControlAxes(&axes, 1);
    g_MapMakerPlayerYaw -= axes.yaw * MM_FREE_LOOK_SPEED * g_ClockTimer;
    g_MapMakerPlayerPitch += axes.pitch * 0.025f * g_ClockTimer;
    if (g_MapMakerPlayerPitch < -1.5707963f) g_MapMakerPlayerPitch = -1.5707963f;
    if (g_MapMakerPlayerPitch > 1.5707963f) g_MapMakerPlayerPitch = 1.5707963f;

    forwardx = sinf(g_MapMakerPlayerYaw);
    forwardz = cosf(g_MapMakerPlayerYaw);
    rightx = -cosf(g_MapMakerPlayerYaw);
    rightz = sinf(g_MapMakerPlayerYaw);
    newx = g_MapMakerPlayerX + (forwardx * axes.forward + rightx * axes.strafe) * MM_PLAYER_SPEED * g_ClockTimer;
    newz = g_MapMakerPlayerZ + (forwardz * axes.forward + rightz * axes.strafe) * MM_PLAYER_SPEED * g_ClockTimer;

    if (!mapmakerPlayerBlocked(newx, g_MapMakerPlayerZ, g_MapMakerPlayerY)) g_MapMakerPlayerX = newx;
    if (!mapmakerPlayerBlocked(g_MapMakerPlayerX, newz, g_MapMakerPlayerY)) g_MapMakerPlayerZ = newz;

    if (mapmakerGetFloorHeight(g_MapMakerPlayerX, g_MapMakerPlayerZ,
            g_MapMakerPlayerY - MM_PLAYER_EYE_HEIGHT, &floor))
    {
        f32 targeteye = floor + MM_PLAYER_EYE_HEIGHT;
        if (g_MapMakerPlayerY <= targeteye + MM_PLAYER_STEP + 4.0f)
        {
            g_MapMakerPlayerY = targeteye;
            g_MapMakerPlayerVelY = 0.0f;
            return;
        }
    }

    g_MapMakerPlayerVelY -= MM_PLAYER_GRAVITY * g_ClockTimer;
    if (g_MapMakerPlayerVelY < -MM_PLAYER_TERMINAL) g_MapMakerPlayerVelY = -MM_PLAYER_TERMINAL;
    g_MapMakerPlayerY += g_MapMakerPlayerVelY * g_ClockTimer;

    if (mapmakerGetFloorHeight(g_MapMakerPlayerX, g_MapMakerPlayerZ,
            g_MapMakerPlayerY - MM_PLAYER_EYE_HEIGHT + 8.0f, &floor)
            && g_MapMakerPlayerY <= floor + MM_PLAYER_EYE_HEIGHT)
    {
        g_MapMakerPlayerY = floor + MM_PLAYER_EYE_HEIGHT;
        g_MapMakerPlayerVelY = 0.0f;
    }

    if (g_MapMakerPlayerY < MAPMAKER_MIN_LAYER * MM_LAYER_HEIGHT - 400.0f)
    {
        g_MapMakerPlaytest = 0;
        g_MapMakerFreeView = 1;
        strcpy(g_MapMakerStorageStatus, "First Person View ended: fell below map bounds");
    }
}

static s32 mapmakerAdvancedMeshIsDeleted(const MapMakerAdvancedMesh *mesh);

static void mapmakerSetVtx(Vtx *vtx, s16 x, s16 y, s16 z, u8 r, u8 g, u8 b, u8 a)
{
    vtx->v.ob[0] = x;
    vtx->v.ob[1] = y;
    vtx->v.ob[2] = z;
    vtx->v.flag = 0;
    vtx->v.tc[0] = 0;
    vtx->v.tc[1] = 0;
    vtx->v.cn[0] = r;
    vtx->v.cn[1] = g;
    vtx->v.cn[2] = b;
    vtx->v.cn[3] = a;
}


static void mapmakerResetPrivateTexturePool(void);

static void mapmakerSetTc(Vtx *vtx, s16 s, s16 t)
{
    vtx->v.tc[0] = s;
    vtx->v.tc[1] = t;
}

static s32 mapmakerTextureIdUsable(u16 textureid)
{
    u32 thisoffset;
    u32 nextoffset;
    u32 bytes;

    if (textureid == MAPMAKER_TEXTURE_NONE || textureid > MM_TEXTURE_LAST)
        return 0;

    /* texLoad assumes the next table entry exists and that compressed-image
     * offsets increase.  Some decomp table slots are sentinels/aliases and are
     * not safe to browse blindly.  Reject them before romCopy can see a bogus
     * unsigned length. */
    thisoffset = *((u32 *)&g_Textures[textureid]) & 0x00ffffffu;
    nextoffset = *((u32 *)&g_Textures[textureid + 1]) & 0x00ffffffu;
    if (nextoffset <= thisoffset)
        return 0;
    bytes = nextoffset - thisoffset;
    /* image.c texLoad() uses a 4000-byte stack compression buffer and rounds
     * the DMA length up.  Keep enough alignment headroom that a malformed
     * texture-table span can never overwrite that buffer. */
    if (bytes < 2 || bytes > 0x0f70u)
        return 0;
    return 1;
}

static s32 mapmakerPrepareTexture(u16 textureid, sImageTableEntry *image, s32 *width, s32 *height)
{
    struct tex *tex;

    if (!mapmakerTextureIdUsable(textureid))
        return 0;

    /* Most authored modules reuse one material across several consecutive
     * faces. Avoid repeating texFind/texLoad metadata work for the exact same
     * resident texture within a frame/pool lifetime. The cache is invalidated
     * whenever the private texture pool is recycled. */
    if (textureid == g_MapMakerLastTextureId)
    {
        *image = g_MapMakerLastTextureImage;
        *width = g_MapMakerLastTextureWidth;
        *height = g_MapMakerLastTextureHeight;
        return 1;
    }

    /* Do not recycle the texture backing store while a display list is being
     * assembled. Earlier RDP commands may still point into it. Request a
     * recycle for the start of the next frame instead. */
    if (texFindInPool(textureid, NULL) == NULL
            && texFreeBytesInBuffer((struct texpool *)&ptr_texture_alloc_start) < 0x1800)
    {
        g_MapMakerTexturePoolRecyclePending = 1;
        return 0;
    }

    texLoadFromTextureNum(textureid, NULL);
    tex = texFindInPool(textureid, NULL);
    if (tex == NULL)
        return 0;

    image->index = textureid;
    image->width = tex->width;
    image->height = tex->height;
    image->level = 0;
    image->format = tex->gbiformat;
    image->depth = tex->depth;
    image->flagsS = G_TX_NOMIRROR | G_TX_WRAP;
    image->flagsT = G_TX_NOMIRROR | G_TX_WRAP;
    image->pad = 0;
    *width = tex->width;
    *height = tex->height;
    g_MapMakerLastTextureId = textureid;
    g_MapMakerLastTextureImage = *image;
    g_MapMakerLastTextureWidth = *width;
    g_MapMakerLastTextureHeight = *height;
    return 1;
}

static Gfx *mapmakerDrawQuad(Gfx *gdl, const s16 local[4][3], s32 cx, s32 basey, s32 cz,
        s32 rotation, u16 textureid, u16 texflags, u8 r, u8 g, u8 b)
{
    Vtx *vtx = dynAllocateVertices(4);
    sImageTableEntry image;
    s32 width = 32;
    s32 height = 32;
    s32 textured = mapmakerPrepareTexture(textureid, &image, &width, &height);
    s32 i;

    for (i = 0; i < 4; i++)
    {
        s32 rx;
        s32 rz;
        mapmakerRotateOffset(local[i][0], local[i][2], rotation, &rx, &rz);
        mapmakerSetVtx(&vtx[i], (s16)(cx + rx), (s16)(basey + local[i][1]),
                (s16)(cz + rz), textured ? 255 : r, textured ? 255 : g, textured ? 255 : b, 255);
    }

    if (textured)
    {
        /* Cover the whole module face with one complete source texture.
         * texSelect mode 4 is GoldenEye's cloud/special-surface blend mode;
         * using it here lets alpha/intensity patterns punch holes through
         * ordinary level textures (the visible cross-shaped floor bug).
         * Map Maker surfaces are opaque world geometry, so use mode 1. */
        s16 s0 = (texflags & 1) ? (s16)((width << 5) - 1) : 0;
        s16 s1 = (texflags & 1) ? 0 : (s16)((width << 5) - 1);
        s16 t0 = (texflags & 2) ? (s16)((height << 5) - 1) : 0;
        s16 t1 = (texflags & 2) ? 0 : (s16)((height << 5) - 1);
        mapmakerSetTc(&vtx[0], s0, t0);
        mapmakerSetTc(&vtx[1], s1, t0);
        mapmakerSetTc(&vtx[2], s1, t1);
        mapmakerSetTc(&vtx[3], s0, t1);
        texSelect(&gdl, &image, 1, 1, 0);
    }
    else
    {
        gSPTexture(gdl++, 0xffff, 0xffff, 0, G_TX_RENDERTILE, G_OFF);
        gDPSetCombineMode(gdl++, G_CC_SHADE, G_CC_SHADE);
    }

    gSPVertex(gdl++, osVirtualToPhysical(vtx), 4, 0);
    gSP2Triangles(gdl++, 0, 1, 2, 0, 0, 2, 3, 0);
    return gdl;
}

static void mapmakerRotateOffset(s32 x, s32 z, s32 rotation, s32 *outx, s32 *outz)
{
    switch (rotation & 3)
    {
    case 1: *outx = z;  *outz = -x; break;
    case 2: *outx = -x; *outz = -z; break;
    case 3: *outx = -z; *outz = x;  break;
    default:*outx = x;  *outz = z;  break;
    }
}

static u16 mapmakerSurfaceTexture(const MapMakerModuleInstance *module, s32 surface)
{
    if (module == NULL || surface < 0 || surface >= MAPMAKER_SURFACE_COUNT)
        return MAPMAKER_TEXTURE_NONE;
    return module->surface_texture[surface];
}

static Gfx *mapmakerDrawBox(Gfx *gdl, s32 cx, s32 basey, s32 cz,
        s32 width, s32 height, s32 depth, s32 rotation,
        const MapMakerModuleInstance *module, u8 r, u8 g, u8 b)
{
    s32 hx = width >> 1;
    s32 hz = depth >> 1;
    s16 q[4][3];

    /* Top */
    q[0][0] = -hx; q[0][1] = height; q[0][2] = -hz;
    q[1][0] =  hx; q[1][1] = height; q[1][2] = -hz;
    q[2][0] =  hx; q[2][1] = height; q[2][2] =  hz;
    q[3][0] = -hx; q[3][1] = height; q[3][2] =  hz;
    gdl = mapmakerDrawQuad(gdl, q, cx, basey, cz, rotation,
            mapmakerSurfaceTexture(module, MAPSURFACE_TOP), mapmakerGetSurfaceFlags(module, MAPSURFACE_TOP), r, g, b);

    /* Bottom */
    q[0][0] = -hx; q[0][1] = 0; q[0][2] =  hz;
    q[1][0] =  hx; q[1][1] = 0; q[1][2] =  hz;
    q[2][0] =  hx; q[2][1] = 0; q[2][2] = -hz;
    q[3][0] = -hx; q[3][1] = 0; q[3][2] = -hz;
    gdl = mapmakerDrawQuad(gdl, q, cx, basey, cz, rotation,
            mapmakerSurfaceTexture(module, MAPSURFACE_BOTTOM), mapmakerGetSurfaceFlags(module, MAPSURFACE_BOTTOM), r, g, b);

    /* North (-Z) */
    q[0][0] = -hx; q[0][1] = 0;      q[0][2] = -hz;
    q[1][0] =  hx; q[1][1] = 0;      q[1][2] = -hz;
    q[2][0] =  hx; q[2][1] = height; q[2][2] = -hz;
    q[3][0] = -hx; q[3][1] = height; q[3][2] = -hz;
    gdl = mapmakerDrawQuad(gdl, q, cx, basey, cz, rotation,
            mapmakerSurfaceTexture(module, MAPSURFACE_NORTH), mapmakerGetSurfaceFlags(module, MAPSURFACE_NORTH), r, g, b);

    /* South (+Z) */
    q[0][0] =  hx; q[0][1] = 0;      q[0][2] = hz;
    q[1][0] = -hx; q[1][1] = 0;      q[1][2] = hz;
    q[2][0] = -hx; q[2][1] = height; q[2][2] = hz;
    q[3][0] =  hx; q[3][1] = height; q[3][2] = hz;
    gdl = mapmakerDrawQuad(gdl, q, cx, basey, cz, rotation,
            mapmakerSurfaceTexture(module, MAPSURFACE_SOUTH), mapmakerGetSurfaceFlags(module, MAPSURFACE_SOUTH), r, g, b);

    /* West (-X) */
    q[0][0] = -hx; q[0][1] = 0;      q[0][2] =  hz;
    q[1][0] = -hx; q[1][1] = 0;      q[1][2] = -hz;
    q[2][0] = -hx; q[2][1] = height; q[2][2] = -hz;
    q[3][0] = -hx; q[3][1] = height; q[3][2] =  hz;
    gdl = mapmakerDrawQuad(gdl, q, cx, basey, cz, rotation,
            mapmakerSurfaceTexture(module, MAPSURFACE_WEST), mapmakerGetSurfaceFlags(module, MAPSURFACE_WEST), r, g, b);

    /* East (+X) */
    q[0][0] = hx; q[0][1] = 0;      q[0][2] = -hz;
    q[1][0] = hx; q[1][1] = 0;      q[1][2] =  hz;
    q[2][0] = hx; q[2][1] = height; q[2][2] =  hz;
    q[3][0] = hx; q[3][1] = height; q[3][2] = -hz;
    gdl = mapmakerDrawQuad(gdl, q, cx, basey, cz, rotation,
            mapmakerSurfaceTexture(module, MAPSURFACE_EAST), mapmakerGetSurfaceFlags(module, MAPSURFACE_EAST), r, g, b);

    return gdl;
}

static Gfx *mapmakerDrawRamp(Gfx *gdl, s32 cx, s32 basey, s32 cz, s32 rotation,
        const MapMakerModuleInstance *module, u8 r, u8 g, u8 b)
{
    s16 q[4][3];

    /* A ramp is an editor surface, not a solid wedge. Keep it paper-thin just
     * like floors/walls while collision still follows the sloped plane. */
    q[0][0] = -80; q[0][1] = 0;  q[0][2] = -80;
    q[1][0] =  80; q[1][1] = 0;  q[1][2] = -80;
    q[2][0] =  80; q[2][1] = MM_LAYER_HEIGHT; q[2][2] =  80;
    q[3][0] = -80; q[3][1] = MM_LAYER_HEIGHT; q[3][2] =  80;
    return mapmakerDrawQuad(gdl, q, cx, basey, cz, rotation,
            mapmakerSurfaceTexture(module, MAPSURFACE_TOP), mapmakerGetSurfaceFlags(module, MAPSURFACE_TOP), r, g, b);
}

static Gfx *mapmakerDrawPaperFloor(Gfx *gdl, s32 cx, s32 basey, s32 cz, s32 rotation,
        const MapMakerModuleInstance *module, u8 r, u8 g, u8 b)
{
    s16 q[4][3] = { {-80,0,-80}, {80,0,-80}, {80,0,80}, {-80,0,80} };
    return mapmakerDrawQuad(gdl, q, cx, basey, cz, rotation,
            mapmakerSurfaceTexture(module, MAPSURFACE_TOP), mapmakerGetSurfaceFlags(module, MAPSURFACE_TOP), r, g, b);
}

static Gfx *mapmakerDrawPaperWall(Gfx *gdl, s32 cx, s32 basey, s32 cz, s32 rotation,
        const MapMakerModuleInstance *module, u8 r, u8 g, u8 b)
{
    s16 q[4][3] = { {-80,0,0}, {80,0,0}, {80,MM_WALL_HEIGHT,0}, {-80,MM_WALL_HEIGHT,0} };
    return mapmakerDrawQuad(gdl, q, cx, basey, cz, rotation,
            mapmakerSurfaceTexture(module, MAPSURFACE_NORTH), mapmakerGetSurfaceFlags(module, MAPSURFACE_NORTH), r, g, b);
}

static Gfx *mapmakerDrawPaperDoorway(Gfx *gdl, s32 cx, s32 basey, s32 cz, s32 rotation,
        const MapMakerModuleInstance *module, u8 r, u8 g, u8 b)
{
    s16 q[4][3];
    u16 tex = mapmakerSurfaceTexture(module, MAPSURFACE_NORTH);
    q[0][0]=-80; q[0][1]=0; q[0][2]=0; q[1][0]=-43; q[1][1]=0; q[1][2]=0; q[2][0]=-43; q[2][1]=92; q[2][2]=0; q[3][0]=-80; q[3][1]=92; q[3][2]=0;
    gdl=mapmakerDrawQuad(gdl,q,cx,basey,cz,rotation,tex,mapmakerGetSurfaceFlags(module, MAPSURFACE_NORTH),r,g,b);
    q[0][0]=43; q[0][1]=0; q[1][0]=80; q[1][1]=0; q[2][0]=80; q[2][1]=92; q[3][0]=43; q[3][1]=92;
    gdl=mapmakerDrawQuad(gdl,q,cx,basey,cz,rotation,tex,mapmakerGetSurfaceFlags(module, MAPSURFACE_NORTH),r,g,b);
    q[0][0]=-80; q[0][1]=92; q[1][0]=80; q[1][1]=92; q[2][0]=80; q[2][1]=120; q[3][0]=-80; q[3][1]=120;
    return mapmakerDrawQuad(gdl,q,cx,basey,cz,rotation,tex,mapmakerGetSurfaceFlags(module, MAPSURFACE_NORTH),r,g,b);
}

static Gfx *mapmakerDrawModuleOffset(Gfx *gdl, const MapMakerModuleInstance *module,
        f32 offsetx, f32 offsety, f32 offsetz)
{
    f32 fx;
    f32 fy;
    f32 fz;
    s32 x;
    s32 y;
    s32 z;
    s32 rotation = module->rotation;
    mapmakerGetModuleOrigin(module, &fx, &fy, &fz);
#ifdef GE_MODDED_CHEATS
    if (mapmakerNativeTestActive() && mirrorLevelsIsEnabled())
    {
        fx = -fx;
        rotation = mapmakerNativeMirrorRotation(rotation);
    }
#endif
    x = (s32)(fx + offsetx); y = (s32)(fy + offsety); z = (s32)(fz + offsetz);

    switch (module->type)
    {
    case MAPMODULE_WALL:
        return mapmakerDrawPaperWall(gdl, x, y, z, rotation, module, 138, 153, 164);
    case MAPMODULE_CORNER:
    {
        s16 q[4][3];
        u16 tex = mapmakerSurfaceTexture(module, MAPSURFACE_NORTH);
        u16 flg = mapmakerGetSurfaceFlags(module, MAPSURFACE_NORTH);
        /* An L-corner, not two full crossing walls.  Each arm runs only from
         * the corner origin to the neighbouring cell corner. */
        q[0][0]=0; q[0][1]=0; q[0][2]=0; q[1][0]=80; q[1][1]=0; q[1][2]=0;
        q[2][0]=80; q[2][1]=MM_WALL_HEIGHT; q[2][2]=0; q[3][0]=0; q[3][1]=MM_WALL_HEIGHT; q[3][2]=0;
        gdl = mapmakerDrawQuad(gdl,q,x,y,z,rotation,tex,flg,153,145,127);
        q[0][0]=0; q[0][1]=0; q[0][2]=0; q[1][0]=0; q[1][1]=0; q[1][2]=80;
        q[2][0]=0; q[2][1]=MM_WALL_HEIGHT; q[2][2]=80; q[3][0]=0; q[3][1]=MM_WALL_HEIGHT; q[3][2]=0;
        return mapmakerDrawQuad(gdl,q,x,y,z,rotation,tex,flg,153,145,127);
    }
    case MAPMODULE_DOORWAY:
        return mapmakerDrawPaperDoorway(gdl, x, y, z, rotation, module, 118, 137, 151);
    case MAPMODULE_RAMP:
        return mapmakerDrawRamp(gdl, x, y, z, rotation, module, 129, 150, 117);
    case MAPMODULE_FLOOR:
    default:
        return mapmakerDrawPaperFloor(gdl, x, y, z, rotation, module, 155, 148, 110);
    }
}

static Gfx *mapmakerDrawModule(Gfx *gdl, const MapMakerModuleInstance *module)
{
    return mapmakerDrawModuleOffset(gdl, module, 0.0f, 0.0f, 0.0f);
}

static Gfx *mapmakerDrawAdvancedMesh(Gfx *gdl, const MapMakerAdvancedMesh *mesh)
{
    s32 l;
    u8 r = 112;
    u8 g = 145;
    u8 b = 118;

    if (mesh == NULL || mapmakerAdvancedMeshIsDeleted(mesh) || mesh->loop_count < 1) return gdl;

    switch (mesh->type)
    {
    case MAPADV_MESH_CUBE:      r = 142; g = 142; b = 154; break;
    case MAPADV_MESH_CIRCLE:    r = 118; g = 150; b = 154; break;
    case MAPADV_MESH_UV_SPHERE: r = 124; g = 142; b = 170; break;
    case MAPADV_MESH_ICO_SPHERE:r = 132; g = 136; b = 174; break;
    case MAPADV_MESH_CYLINDER:  r = 150; g = 135; b = 118; break;
    case MAPADV_MESH_CONE:      r = 158; g = 132; b = 112; break;
    case MAPADV_MESH_TORUS:     r = 145; g = 120; b = 156; break;
    default: break;
    }

    gSPTexture(gdl++, 0xffff, 0xffff, 0, G_TX_RENDERTILE, G_OFF);
    gDPSetCombineMode(gdl++, G_CC_SHADE, G_CC_SHADE);

    for (l = 0; l < mesh->loop_count; l++)
    {
        MapMakerAdvancedLoop *loop = &g_MapMakerAdvancedLoops[mesh->first_loop + l];
        Vtx *vtx;
        s32 i;

        if (loop->vertex_count < 3 || loop->vertex_count > MAPMAKER_ADV_MAX_POINTS_PER_LOOP) continue;
        if (loop->first_vertex + loop->vertex_count > g_MapMakerAdvancedVertexCount) continue;
        if (dynGetFreeVtx() < (s32)(loop->vertex_count * sizeof(Vtx) + 2048)
                || dynGetFreeGfx(gdl) < (s32)(loop->vertex_count * sizeof(Gfx) + 128)) return gdl;

        vtx = dynAllocateVertices(loop->vertex_count);
        for (i = 0; i < loop->vertex_count; i++)
        {
            MapMakerAdvancedVertex *src = &g_MapMakerAdvancedVertices[loop->first_vertex + i];
            mapmakerSetVtx(&vtx[i], src->x, src->y, src->z, r, g, b, 255);
        }
        gSPVertex(gdl++, osVirtualToPhysical(vtx), loop->vertex_count, 0);
        for (i = 1; i + 1 < loop->vertex_count; i++)
            gSP1Triangle(gdl++, 0, i, i + 1, 0);
    }
    return gdl;
}

static Gfx *mapmakerDrawAdvancedVertexHandle(Gfx *gdl, s32 vertexindex, s32 selected,
        f32 dirx, f32 diry, f32 dirz)
{
    MapMakerAdvancedVertex *src = &g_MapMakerAdvancedVertices[vertexindex];
    Vtx *vtx = dynAllocateVertices(9);
    f32 rx = dirz;
    f32 ry = 0.0f;
    f32 rz = -dirx;
    f32 rlen = sqrtf(rx * rx + rz * rz);
    f32 ux;
    f32 uy;
    f32 uz;
    s32 i;
    u8 r = selected ? 255 : 190;
    u8 g = selected ? 224 : 190;
    u8 b = selected ? 80 : 210;
    static const f32 circlex[8] = {1.0f, 0.7071f, 0.0f, -0.7071f, -1.0f, -0.7071f, 0.0f, 0.7071f};
    static const f32 circley[8] = {0.0f, 0.7071f, 1.0f, 0.7071f, 0.0f, -0.7071f, -1.0f, -0.7071f};

    if (rlen < 0.01f) { rx = 1.0f; rz = 0.0f; rlen = 1.0f; }
    rx /= rlen; rz /= rlen;
    ux = -diry * rz;
    uy = dirz * rx - dirx * rz;
    uz = diry * rx;
    rlen = sqrtf(ux * ux + uy * uy + uz * uz);
    if (rlen < 0.01f) { ux = 0.0f; uy = 1.0f; uz = 0.0f; }
    else { ux /= rlen; uy /= rlen; uz /= rlen; }

    mapmakerSetVtx(&vtx[0], src->x, src->y, src->z, r, g, b, 255);
    for (i = 0; i < 8; i++)
    {
        f32 px = src->x + (rx * circlex[i] + ux * circley[i]) * 12.0f;
        f32 py = src->y + (ry * circlex[i] + uy * circley[i]) * 12.0f;
        f32 pz = src->z + (rz * circlex[i] + uz * circley[i]) * 12.0f;
        mapmakerSetVtx(&vtx[i + 1], (s16)px, (s16)py, (s16)pz, r, g, b, 255);
    }
    gSPTexture(gdl++, 0xffff, 0xffff, 0, G_TX_RENDERTILE, G_OFF);
    gDPSetCombineMode(gdl++, G_CC_SHADE, G_CC_SHADE);
    gSPVertex(gdl++, osVirtualToPhysical(vtx), 9, 0);
    gSP2Triangles(gdl++, 0,1,2,0, 0,2,3,0);
    gSP2Triangles(gdl++, 0,3,4,0, 0,4,5,0);
    gSP2Triangles(gdl++, 0,5,6,0, 0,6,7,0);
    gSP2Triangles(gdl++, 0,7,8,0, 0,8,1,0);
    return gdl;
}

static Gfx *mapmakerDrawAdvancedPortal(Gfx *gdl, const MapMakerAdvancedPortal *portal)
{
    Vtx *vtx;
    s32 x0;
    s32 x1;
    s32 z0;
    s32 z1;
    s32 y0;
    s32 y1;

    if (portal == NULL || (portal->flags & 0x80) != 0) return gdl;
    if (dynGetFreeVtx() < (s32)(sizeof(Vtx) * 4 + 1024) || dynGetFreeGfx(gdl) < 64) return gdl;
    vtx = dynAllocateVertices(4);
    y0 = portal->y - portal->half_height;
    y1 = portal->y + portal->half_height;
    if (portal->axis == 0)
    {
        x0 = x1 = portal->x;
        z0 = portal->z - portal->half_width;
        z1 = portal->z + portal->half_width;
        mapmakerSetVtx(&vtx[0], x0, y0, z0, 80, 210, 255, 190);
        mapmakerSetVtx(&vtx[1], x1, y0, z1, 80, 210, 255, 190);
        mapmakerSetVtx(&vtx[2], x1, y1, z1, 80, 210, 255, 190);
        mapmakerSetVtx(&vtx[3], x0, y1, z0, 80, 210, 255, 190);
    }
    else
    {
        z0 = z1 = portal->z;
        x0 = portal->x - portal->half_width;
        x1 = portal->x + portal->half_width;
        mapmakerSetVtx(&vtx[0], x0, y0, z0, 80, 210, 255, 190);
        mapmakerSetVtx(&vtx[1], x1, y0, z1, 80, 210, 255, 190);
        mapmakerSetVtx(&vtx[2], x1, y1, z1, 80, 210, 255, 190);
        mapmakerSetVtx(&vtx[3], x0, y1, z0, 80, 210, 255, 190);
    }
    gSPTexture(gdl++, 0xffff, 0xffff, 0, G_TX_RENDERTILE, G_OFF);
    gDPSetCombineMode(gdl++, G_CC_SHADE, G_CC_SHADE);
    gSPVertex(gdl++, osVirtualToPhysical(vtx), 4, 0);
    gSP2Triangles(gdl++, 0, 1, 2, 0, 0, 2, 3, 0);
    return gdl;
}

static Gfx *mapmakerDrawAdvancedHandles(Gfx *gdl, f32 dirx, f32 diry, f32 dirz)
{
    MapMakerAdvancedMesh *mesh;
    s32 l;
    s32 shown = 0;

    if (g_MapMakerAdvancedSelectedMesh < 0 || g_MapMakerAdvancedSelectedMesh >= g_MapMakerAdvancedMeshCount)
        return gdl;
    mesh = &g_MapMakerAdvancedMeshes[g_MapMakerAdvancedSelectedMesh];
    if (mapmakerAdvancedMeshIsDeleted(mesh)) return gdl;

    /* High-poly helper primitives deliberately do not draw a handle for every
     * duplicated face vertex.  Cap the editor overlay to twelve handles so a Torus cannot blow
     * the title-stage vertex buffer merely by being selected. */
    for (l = 0; l < mesh->loop_count && shown < 12; l++)
    {
        MapMakerAdvancedLoop *loop = &g_MapMakerAdvancedLoops[mesh->first_loop + l];
        s32 j;
        for (j = 0; j < loop->vertex_count && shown < 12; j++)
        {
            s32 vi = loop->first_vertex + j;
            if (dynGetFreeVtx() < (s32)(sizeof(Vtx) * 9 + 2048) || dynGetFreeGfx(gdl) < 96) return gdl;
            gdl = mapmakerDrawAdvancedVertexHandle(gdl, vi,
                    vi == g_MapMakerAdvancedSelectedVertex, dirx, diry, dirz);
            shown++;
        }
    }
    return gdl;
}

static Gfx *mapmakerDrawGridStrip(Gfx *gdl, s32 x0, s32 z0, s32 x1, s32 z1,
        s32 y, s32 width, u8 r, u8 g, u8 b)
{
    Vtx *vtx = dynAllocateVertices(4);
    if (x0 == x1)
    {
        mapmakerSetVtx(&vtx[0], (s16)(x0 - width), (s16)y, (s16)z0, r, g, b, 255);
        mapmakerSetVtx(&vtx[1], (s16)(x0 + width), (s16)y, (s16)z0, r, g, b, 255);
        mapmakerSetVtx(&vtx[2], (s16)(x1 + width), (s16)y, (s16)z1, r, g, b, 255);
        mapmakerSetVtx(&vtx[3], (s16)(x1 - width), (s16)y, (s16)z1, r, g, b, 255);
    }
    else
    {
        mapmakerSetVtx(&vtx[0], (s16)x0, (s16)y, (s16)(z0 - width), r, g, b, 255);
        mapmakerSetVtx(&vtx[1], (s16)x1, (s16)y, (s16)(z1 - width), r, g, b, 255);
        mapmakerSetVtx(&vtx[2], (s16)x1, (s16)y, (s16)(z1 + width), r, g, b, 255);
        mapmakerSetVtx(&vtx[3], (s16)x0, (s16)y, (s16)(z0 + width), r, g, b, 255);
    }
    gSPVertex(gdl++, osVirtualToPhysical(vtx), 4, 0);
    gSP2Triangles(gdl++, 0, 1, 2, 0, 0, 2, 3, 0);
    return gdl;
}

static Gfx *mapmakerDrawGrid(Gfx *gdl)
{
    s32 line;
    s32 coord;
    s32 y = g_MapMakerCursorY * MM_LAYER_HEIGHT + 2;
    f32 camx, camy, camz, dirx, diry, dirz;
    s32 cameraCellX;
    s32 cameraCellZ;
    s32 first;
    s32 last;
    s32 firstz;
    s32 lastz;
    s32 minx, maxx, minz, maxz;

    /* The logical grid is large, but the N64 should never pay for every line.
     * Draw full resolution nearby, 4-cell majors through the middle band, and
     * 8-cell majors in the far band.  This extends orientation range by more
     * than 5x while keeping submitted line count bounded and predictable. */
    mapmakerGetCamera(&camx, &camy, &camz, &dirx, &diry, &dirz);
    cameraCellX = mapmakerRoundCell(camx);
    cameraCellZ = mapmakerRoundCell(camz);
    first = cameraCellX - MM_GRID_FAR_RADIUS;
    last = cameraCellX + MM_GRID_FAR_RADIUS;
    firstz = cameraCellZ - MM_GRID_FAR_RADIUS;
    lastz = cameraCellZ + MM_GRID_FAR_RADIUS;

    if (first < MM_GRID_MIN) first = MM_GRID_MIN;
    if (last > MM_GRID_MAX) last = MM_GRID_MAX;
    if (firstz < MM_GRID_MIN) firstz = MM_GRID_MIN;
    if (lastz > MM_GRID_MAX) lastz = MM_GRID_MAX;

    minx = first * MM_CELL_SIZE;
    maxx = last * MM_CELL_SIZE;
    minz = firstz * MM_CELL_SIZE;
    maxz = lastz * MM_CELL_SIZE;

    for (line = first; line <= last; line++)
    {
        s32 d = line - cameraCellX;
        u8 shade;
        if (d < 0) d = -d;
        if (d > MM_GRID_MID_RADIUS && (line & 7) != 0) continue;
        if (d > MM_GRID_NEAR_RADIUS && d <= MM_GRID_MID_RADIUS && (line & 3) != 0) continue;
        shade = line == 0 ? 116 : ((line & 7) == 0 ? 82 : ((line & 3) == 0 ? 66 : 48));
        coord = line * MM_CELL_SIZE - MM_CELL_SIZE / 2;
        gdl = mapmakerDrawGridStrip(gdl, coord, minz - MM_CELL_SIZE / 2, coord, maxz + MM_CELL_SIZE / 2,
                y, MM_GRID_LINE_WIDTH, shade, shade + 8, shade + 12);
    }
    for (line = firstz; line <= lastz; line++)
    {
        s32 d = line - cameraCellZ;
        u8 shade;
        if (d < 0) d = -d;
        if (d > MM_GRID_MID_RADIUS && (line & 7) != 0) continue;
        if (d > MM_GRID_NEAR_RADIUS && d <= MM_GRID_MID_RADIUS && (line & 3) != 0) continue;
        shade = line == 0 ? 116 : ((line & 7) == 0 ? 82 : ((line & 3) == 0 ? 66 : 48));
        coord = line * MM_CELL_SIZE - MM_CELL_SIZE / 2;
        gdl = mapmakerDrawGridStrip(gdl, minx - MM_CELL_SIZE / 2, coord, maxx + MM_CELL_SIZE / 2, coord,
                y, MM_GRID_LINE_WIDTH, shade, shade + 8, shade + 12);
    }
    return gdl;
}

static Gfx *mapmakerDrawCursor(Gfx *gdl)
{
    Vtx *vtx = dynAllocateVertices(4);
    s32 x = g_MapMakerCursorX * MM_CELL_SIZE;
    s32 y = g_MapMakerCursorY * MM_LAYER_HEIGHT + 3;
    s32 z = g_MapMakerCursorZ * MM_CELL_SIZE;

    /* One clean square = one editable grid cell. Force an untextured shade
     * state so the cursor cannot inherit the previous module's texture/combine
     * state and turn into the malformed yellow polygon seen in the editor. */
    mapmakerSetVtx(&vtx[0], (s16)(x - 78), (s16)y, (s16)(z - 78), 255, 220, 64, 255);
    mapmakerSetVtx(&vtx[1], (s16)(x + 78), (s16)y, (s16)(z - 78), 255, 220, 64, 255);
    mapmakerSetVtx(&vtx[2], (s16)(x + 78), (s16)y, (s16)(z + 78), 255, 220, 64, 255);
    mapmakerSetVtx(&vtx[3], (s16)(x - 78), (s16)y, (s16)(z + 78), 255, 220, 64, 255);

    gDPPipeSync(gdl++);
    gSPTexture(gdl++, 0xffff, 0xffff, 0, G_TX_RENDERTILE, G_OFF);
    gDPSetCombineMode(gdl++, G_CC_SHADE, G_CC_SHADE);
    gSPVertex(gdl++, osVirtualToPhysical(vtx), 4, 0);
    gSPLine3D(gdl++, 0, 1, 0);
    gSPLine3D(gdl++, 1, 2, 0);
    gSPLine3D(gdl++, 2, 3, 0);
    gSPLine3D(gdl++, 3, 0, 0);
    return gdl;
}

static s32 mapmakerFindCursorModule(void)
{
    s32 i;
    for (i = 0; i < g_MapMakerModuleCount; i++)
    {
        MapMakerModuleInstance *m = &g_MapMakerModules[i];
        if (m->grid_x != g_MapMakerCursorX || m->grid_y != g_MapMakerCursorY || m->grid_z != g_MapMakerCursorZ) continue;

        /* Directional edge pieces may coexist in one cell. A floor/ramp still
         * occupies the cell once, while walls/doorways are keyed by direction. */
        if (g_MapMakerModuleType == MAPMODULE_WALL || g_MapMakerModuleType == MAPMODULE_CORNER || g_MapMakerModuleType == MAPMODULE_DOORWAY)
        {
            u16 wantedge = g_MapMakerPlacementEdge ? MAPMODULE_FLAG_EDGE_ANCHOR : 0;
            if (m->type == g_MapMakerModuleType && (m->rotation & 3) == (g_MapMakerRotation & 3)
                    && (m->flags & MAPMODULE_FLAG_EDGE_ANCHOR) == wantedge) return i;
        }
        else if (m->type == g_MapMakerModuleType)
        {
            return i;
        }
    }
    return -1;
}

static void mapmakerPlaceModule(void)
{
    s32 index = mapmakerFindCursorModule();
    MapMakerModuleInstance *module;

    if (index < 0)
    {
        s32 surface;
        if (g_MapMakerModuleCount >= MAPMAKER_MAX_MODULES) return;
        index = g_MapMakerModuleCount++;
        for (surface = 0; surface < MAPMAKER_SURFACE_COUNT; surface++)
            g_MapMakerModules[index].surface_texture[surface] = MAPMAKER_TEXTURE_NONE;
        g_MapMakerModules[index].flags = 0;
    }

    module = &g_MapMakerModules[index];
    module->grid_x = g_MapMakerCursorX;
    module->grid_y = g_MapMakerCursorY;
    module->grid_z = g_MapMakerCursorZ;
    module->type = g_MapMakerModuleType;
    module->rotation = g_MapMakerRotation;
    if (g_MapMakerModuleType == MAPMODULE_WALL || g_MapMakerModuleType == MAPMODULE_CORNER || g_MapMakerModuleType == MAPMODULE_DOORWAY)
    {
        if (g_MapMakerPlacementEdge) module->flags |= MAPMODULE_FLAG_EDGE_ANCHOR;
        else module->flags &= ~MAPMODULE_FLAG_EDGE_ANCHOR;
    }
    else
    {
        module->flags &= ~MAPMODULE_FLAG_EDGE_ANCHOR;
    }
    mapmakerMarkDirty();
}

static void mapmakerDeleteModule(void)
{
    s32 index = mapmakerFindCursorModule();
    if (index >= 0)
    {
        g_MapMakerModuleCount--;
        if (index != g_MapMakerModuleCount)
        {
            g_MapMakerModules[index] = g_MapMakerModules[g_MapMakerModuleCount];
        }
        mapmakerMarkDirty();
    }
}

static void mapmakerPlaceEntity(void)
{
    MapMakerEntityInstance *e;
    if (g_MapMakerEntityCount >= MAPMAKER_MAX_ENTITIES)
    {
        strcpy(g_MapMakerStorageStatus, "Placement pool full");
        return;
    }

    e = &g_MapMakerEntities[g_MapMakerEntityCount++];
    memset(e, 0, sizeof(*e));
    e->x = (s16)g_MapMakerEntityCursorX;
    e->y = (s16)g_MapMakerEntityCursorY;
    e->z = (s16)g_MapMakerEntityCursorZ;
    e->rotation = (u8)g_MapMakerRotation;

    if (g_MapMakerTool == MAPTOOL_PICKUP)
    {
        e->type = (u8)mapmakerPickupSetupType(g_MapMakerPickupType);
        e->subtype = (s16)g_MapMakerPickupSubtype;
        e->quantity = (u16)g_MapMakerPickupQuantity;
        strcpy(g_MapMakerStorageStatus, "Pickup placed");
    }
    else
    {
        e->type = (u8)g_MapMakerEntityType;
        e->object_id = (u16)g_MapMakerEntityObjectId;
        e->aux_id = (u16)g_MapMakerEntityAuxId;
        strcpy(g_MapMakerStorageStatus, "Entity placed");
    }

    mapmakerMarkDirty();
}

static void mapmakerDeleteNearestEntity(void)
{
    s32 i;
    s32 best = -1;
    f32 bestd = 80.0f * 80.0f;
    for (i = 0; i < g_MapMakerEntityCount; i++)
    {
        f32 dx = g_MapMakerEntities[i].x - g_MapMakerEntityCursorX;
        f32 dy = g_MapMakerEntities[i].y - g_MapMakerEntityCursorY;
        f32 dz = g_MapMakerEntities[i].z - g_MapMakerEntityCursorZ;
        f32 d = dx*dx + dy*dy + dz*dz;
        if (d < bestd) { bestd = d; best = i; }
    }
    if (best >= 0)
    {
        g_MapMakerEntityCount--;
        if (best != g_MapMakerEntityCount) g_MapMakerEntities[best] = g_MapMakerEntities[g_MapMakerEntityCount];
        mapmakerMarkDirty();
        strcpy(g_MapMakerStorageStatus, "Placement deleted");
    }
}

static void mapmakerPlacePlayerStart(void)
{
    MapMakerPlayerStart *start;
    s32 i;

    /* A numbered start is unique. Re-placing the selected slot moves it. */
    for (i = 0; i < g_MapMakerPlayerStartCount; i++)
    {
        if (g_MapMakerPlayerStarts[i].slot == g_MapMakerPlayerStartSlot)
        {
            start = &g_MapMakerPlayerStarts[i];
            start->x = (s16)g_MapMakerEntityCursorX;
            start->y = (s16)g_MapMakerEntityCursorY;
            start->z = (s16)g_MapMakerEntityCursorZ;
            start->rotation = (u8)g_MapMakerRotation;
            mapmakerMarkDirty();
            strcpy(g_MapMakerStorageStatus, "Player Start moved");
            return;
        }
    }

    if (g_MapMakerPlayerStartCount >= MAPMAKER_MAX_PLAYER_STARTS)
    {
        strcpy(g_MapMakerStorageStatus, "Player Start limit reached");
        return;
    }

    start = &g_MapMakerPlayerStarts[g_MapMakerPlayerStartCount++];
    start->x = (s16)g_MapMakerEntityCursorX;
    start->y = (s16)g_MapMakerEntityCursorY;
    start->z = (s16)g_MapMakerEntityCursorZ;
    start->rotation = (u8)g_MapMakerRotation;
    start->slot = (u8)g_MapMakerPlayerStartSlot;
    mapmakerMarkDirty();
    strcpy(g_MapMakerStorageStatus, "Player Start placed");
}

static void mapmakerDeleteNearestPlayerStart(void)
{
    s32 i;
    s32 best = -1;
    f32 bestd = 96.0f * 96.0f;
    for (i = 0; i < g_MapMakerPlayerStartCount; i++)
    {
        f32 dx = g_MapMakerPlayerStarts[i].x - g_MapMakerEntityCursorX;
        f32 dy = g_MapMakerPlayerStarts[i].y - g_MapMakerEntityCursorY;
        f32 dz = g_MapMakerPlayerStarts[i].z - g_MapMakerEntityCursorZ;
        f32 d = dx*dx + dy*dy + dz*dz;
        if (d < bestd) { bestd = d; best = i; }
    }
    if (best >= 0)
    {
        g_MapMakerPlayerStartCount--;
        if (best != g_MapMakerPlayerStartCount)
            g_MapMakerPlayerStarts[best] = g_MapMakerPlayerStarts[g_MapMakerPlayerStartCount];
        mapmakerMarkDirty();
        strcpy(g_MapMakerStorageStatus, "Player Start deleted");
    }
}

static Gfx *mapmakerDrawPlayerStartMarker(Gfx *gdl, const MapMakerPlayerStart *start, s32 selected)
{
    Vtx *v = dynAllocateVertices(5);
    s32 dx = 0;
    s32 dz = 0;
    u8 r = selected ? 255 : 96;
    u8 g = selected ? 224 : 255;
    u8 b = selected ? 64 : 96;
    switch (start->rotation & 3)
    {
    case 1: dx = 42; break;
    case 2: dz = -42; break;
    case 3: dx = -42; break;
    default: dz = 42; break;
    }
    mapmakerSetVtx(&v[0], start->x, start->y, start->z, r,g,b,255);
    mapmakerSetVtx(&v[1], start->x, start->y + 72, start->z, r,g,b,255);
    mapmakerSetVtx(&v[2], start->x + dx, start->y + 36, start->z + dz, r,g,b,255);
    mapmakerSetVtx(&v[3], start->x + (dx >> 1) - (dz >> 2), start->y + 36, start->z + (dz >> 1) + (dx >> 2), r,g,b,255);
    mapmakerSetVtx(&v[4], start->x + (dx >> 1) + (dz >> 2), start->y + 36, start->z + (dz >> 1) - (dx >> 2), r,g,b,255);
    gDPPipeSync(gdl++); gSPTexture(gdl++,0xffff,0xffff,0,G_TX_RENDERTILE,G_OFF); gDPSetCombineMode(gdl++,G_CC_SHADE,G_CC_SHADE);
    gSPVertex(gdl++,osVirtualToPhysical(v),5,0);
    gSPLine3D(gdl++,0,1,0); gSPLine3D(gdl++,0,2,0);
    gSP1Triangle(gdl++,2,3,4,0);
    return gdl;
}

static Gfx *mapmakerDrawEntityMarker(Gfx *gdl, const MapMakerEntityInstance *e, s32 selected)
{
    Vtx *v = dynAllocateVertices(6);
    s32 pickup = mapmakerSetupTypeIsPickup(e->type);
    u8 r = selected ? 255 : (pickup ? 120 : 100);
    u8 g = selected ? 220 : (pickup ? 255 : 220);
    u8 b = selected ? 64 : (pickup ? 120 : 255);
    s16 x=e->x, y=e->y, z=e->z;
    mapmakerSetVtx(&v[0],x-18,y,z,r,g,b,255); mapmakerSetVtx(&v[1],x+18,y,z,r,g,b,255);
    mapmakerSetVtx(&v[2],x,y,z-18,r,g,b,255); mapmakerSetVtx(&v[3],x,y,z+18,r,g,b,255);
    mapmakerSetVtx(&v[4],x,y-18,z,r,g,b,255); mapmakerSetVtx(&v[5],x,y+42,z,r,g,b,255);
    gDPPipeSync(gdl++); gSPTexture(gdl++,0xffff,0xffff,0,G_TX_RENDERTILE,G_OFF); gDPSetCombineMode(gdl++,G_CC_SHADE,G_CC_SHADE);
    gSPVertex(gdl++,osVirtualToPhysical(v),6,0);
    gSPLine3D(gdl++,0,1,0); gSPLine3D(gdl++,2,3,0); gSPLine3D(gdl++,4,5,0);
    return gdl;
}

static s32 mapmakerChooseRandomMusic(void)
{
    s32 count = 0;
    while (random_tracks[count] != M_NONE) count++;
    if (count <= 0) return M_INTRO;
    return random_tracks[randomGetNext() % count];
}

static void mapmakerPlaySelectedMusic(void)
{
    if (g_MapMakerMusicTrack == MM_MUSIC_RANDOM)
        musicTrack1Play(mapmakerChooseRandomMusic());
    else if (g_MapMakerMusicTrack >= MM_MUSIC_FIRST && g_MapMakerMusicTrack <= MM_MUSIC_LAST)
        musicTrack1Play(g_MapMakerMusicTrack);
}

static void mapmakerChangeMusic(s32 delta)
{
    const s32 count = MM_MUSIC_LAST - MM_MUSIC_FIRST + 2; /* + Random */
    s32 index;

    if (g_MapMakerMusicTrack == MM_MUSIC_RANDOM) index = 0;
    else index = g_MapMakerMusicTrack - MM_MUSIC_FIRST + 1;
    index += delta;
    while (index < 0) index += count;
    while (index >= count) index -= count;
    g_MapMakerMusicTrack = index == 0 ? MM_MUSIC_RANDOM : MM_MUSIC_FIRST + index - 1;
    mapmakerMarkDirty();

    /* GoldenEye's sequence load is synchronous. Delay preview until the user
     * has stopped scrolling, including for the Random entry. */
    g_MapMakerMusicPreviewPending = 1;
    g_MapMakerMusicPreviewDelay = 8;
}

static void mapmakerTickMusicPreview(void)
{
    if (!g_MapMakerMusicPreviewPending) return;
    if (g_MapMakerMusicPreviewDelay > 0)
    {
        g_MapMakerMusicPreviewDelay -= g_ClockTimer;
        return;
    }
    g_MapMakerMusicPreviewPending = 0;
    mapmakerPlaySelectedMusic();
}


static void mapmakerResetPrivateTexturePool(void)
{
    /* Texture browsing can touch hundreds of different retail textures.
     * texLoad allocates decompressed data monotonically inside the pool, so
     * merely wrapping the texture ID does not reclaim memory.  The module
     * graph stores texture IDs, not texture pointers, therefore the private
     * Map Maker pool is safe to recycle and all visible materials reload on
     * demand on the following render. */
    g_TexCacheCount = 0;
    g_MapMakerLastTextureId = MAPMAKER_TEXTURE_NONE;
    texInitPool((struct texpool *)&ptr_texture_alloc_start,
            (u8 *)g_MapMakerTextureBuffer, MM_TEXTURE_POOL_BYTES);
    /* Keep the Expansion-Pak spill cache in the same lifetime domain as the
     * private Map Maker texture pool. Otherwise a recycled material ID could
     * resolve to stale overflow-pool texels from the previous browse state. */
    texResetStageOverflowPool();
}

static void mapmakerChangeMaterialTexture(s32 delta)
{
    s32 value = (s32)g_MapMakerMaterialTexture[g_MapMakerMaterialSlot];
    s32 step = delta < 0 ? -1 : 1;
    s32 remaining = delta < 0 ? -delta : delta;
    s32 guard = 0;

    /* Browse only texture-table entries that texLoad can safely consume.
     * This avoids the remaining crashes caused by sentinel/alias slots while
     * preserving the retail texture IDs for painted surfaces. */
    while (remaining > 0 && guard < (MM_TEXTURE_LAST + 1) * 2)
    {
        value += step;
        if (value < 0) value = MM_TEXTURE_LAST;
        if (value > MM_TEXTURE_LAST) value = 0;
        if (mapmakerTextureIdUsable((u16)value)) remaining--;
        guard++;
    }

    if (mapmakerTextureIdUsable((u16)value))
    {
        g_MapMakerMaterialTexture[g_MapMakerMaterialSlot] = (u16)value;
        mapmakerMarkDirty();
    }
}

static void mapmakerChangeMaterialSlot(s32 delta)
{
    g_MapMakerMaterialSlot += delta;
    while (g_MapMakerMaterialSlot < 0) g_MapMakerMaterialSlot += MAPMAKER_MATERIAL_SLOTS;
    while (g_MapMakerMaterialSlot >= MAPMAKER_MATERIAL_SLOTS) g_MapMakerMaterialSlot -= MAPMAKER_MATERIAL_SLOTS;
}

static void mapmakerSetFreeView(s32 enable)
{
    if (enable && !g_MapMakerFreeView)
    {
        f32 camx;
        f32 camy;
        f32 camz;
        f32 dirx;
        f32 diry;
        f32 dirz;
        mapmakerGetCamera(&camx, &camy, &camz, &dirx, &diry, &dirz);
        g_MapMakerFreeCameraX = camx;
        g_MapMakerFreeCameraY = camy;
        g_MapMakerFreeCameraZ = camz;
        g_MapMakerCameraYaw += 3.14159265f;
        g_MapMakerCameraPitch = -g_MapMakerCameraPitch;
        g_MapMakerFreeView = 1;
        mapmakerUpdateFreeCursor();
    }
    else if (!enable && g_MapMakerFreeView)
    {
        g_MapMakerCameraYaw -= 3.14159265f;
        if (g_MapMakerCameraPitch < 0.0f) g_MapMakerCameraPitch = -g_MapMakerCameraPitch;
        if (g_MapMakerCameraPitch < -1.5707963f) g_MapMakerCameraPitch = -1.5707963f;
        if (g_MapMakerCameraPitch > 1.5707963f) g_MapMakerCameraPitch = 1.5707963f;
        g_MapMakerFreeView = 0;
    }
}

static void mapmakerTickFreeView(void)
{
    MapMakerControlAxes axes;
    u32 held = joyGetButtons(PLAYER_1, R_TRIG);
    f32 moveSpeed = MM_FREE_FLY_SPEED;
    f32 cp;
    f32 forwardx;
    f32 forwardy;
    f32 forwardz;
    f32 rightx;
    f32 rightz;

    mapmakerReadEditorAxes(&axes);
    /* Map Maker's view yaw increases toward screen-left, so convert the
     * control-space convention (positive = turn right) at the camera boundary. */
    g_MapMakerCameraYaw -= axes.yaw * MM_FREE_LOOK_SPEED * g_ClockTimer;
    g_MapMakerCameraPitch += axes.pitch * 0.025f * g_ClockTimer;
    if (g_MapMakerCameraPitch < -1.5707963f) g_MapMakerCameraPitch = -1.5707963f;
    if (g_MapMakerCameraPitch > 1.5707963f) g_MapMakerCameraPitch = 1.5707963f;

    /* Match the project's real Fly Mode behavior: forward/back follows the
     * full look vector, including pitch, so no separate Fly Up/Down buttons
     * are necessary. */
    cp = cosf(g_MapMakerCameraPitch);
    forwardx = sinf(g_MapMakerCameraYaw) * cp;
    forwardy = sinf(g_MapMakerCameraPitch);
    forwardz = cosf(g_MapMakerCameraYaw) * cp;
    /* Camera/world basis is mirrored versus GoldenEye control-space X.
     * Positive strafe must still mean move right on screen. */
    rightx = -cosf(g_MapMakerCameraYaw);
    rightz = sinf(g_MapMakerCameraYaw);
    if (held & R_TRIG)
        moveSpeed *= 2.0f;
    g_MapMakerFreeCameraX += (forwardx * axes.forward + rightx * axes.strafe) * moveSpeed * g_ClockTimer;
    g_MapMakerFreeCameraY += forwardy * axes.forward * moveSpeed * g_ClockTimer;
    g_MapMakerFreeCameraZ += (forwardz * axes.forward + rightz * axes.strafe) * moveSpeed * g_ClockTimer;

    mapmakerUpdateFreeCursor();
}

static s16 mapmakerAdvancedSnapCoord(f32 value)
{
    s32 snapped;
    if (value >= 0.0f)
        snapped = (s32)((value + 5.0f) / 10.0f) * 10;
    else
        snapped = (s32)((value - 5.0f) / 10.0f) * 10;
    if (snapped < -32760) snapped = -32760;
    if (snapped > 32760) snapped = 32760;
    return (s16)snapped;
}

static s32 mapmakerAdvancedMeshIsDeleted(const MapMakerAdvancedMesh *mesh)
{
    return (mesh->flags & 0x8000) != 0;
}

static s32 mapmakerAdvancedReserveMesh(s32 loopcount, s32 vertexcount, s32 *meshindex)
{
    s32 i;
    s32 index = -1;

    if (loopcount <= 0 || vertexcount <= 0
            || g_MapMakerAdvancedLoopCount + loopcount > MAPMAKER_ADV_MAX_LOOPS
            || g_MapMakerAdvancedVertexCount + vertexcount > MAPMAKER_ADV_MAX_VERTICES)
    {
        strcpy(g_MapMakerStorageStatus, "Advanced: primitive exceeds mesh pool");
        return 0;
    }

    for (i = 0; i < g_MapMakerAdvancedMeshCount; i++)
    {
        if (mapmakerAdvancedMeshIsDeleted(&g_MapMakerAdvancedMeshes[i]))
        {
            index = i;
            break;
        }
    }
    if (index < 0)
    {
        if (g_MapMakerAdvancedMeshCount >= MAPMAKER_ADV_MAX_MESHES)
        {
            strcpy(g_MapMakerStorageStatus, "Advanced: mesh limit reached");
            return 0;
        }
        index = g_MapMakerAdvancedMeshCount++;
    }

    memset(&g_MapMakerAdvancedMeshes[index], 0, sizeof(MapMakerAdvancedMesh));
    g_MapMakerAdvancedMeshes[index].first_loop = (u16)g_MapMakerAdvancedLoopCount;
    g_MapMakerAdvancedMeshes[index].type = (u8)g_MapMakerAdvancedMeshType;
    g_MapMakerAdvancedMeshes[index].material = (u8)g_MapMakerMaterialSlot;
    g_MapMakerAdvancedMeshes[index].room = (u8)g_MapMakerAdvancedRoom;
    *meshindex = index;
    return 1;
}

static s32 mapmakerAdvancedAddFace(MapMakerAdvancedMesh *mesh, const MapMakerAdvancedVertex *points, s32 count)
{
    MapMakerAdvancedLoop *loop;

    if (count < 3 || count > MAPMAKER_ADV_MAX_POINTS_PER_LOOP
            || g_MapMakerAdvancedLoopCount >= MAPMAKER_ADV_MAX_LOOPS
            || g_MapMakerAdvancedVertexCount + count > MAPMAKER_ADV_MAX_VERTICES)
        return 0;

    loop = &g_MapMakerAdvancedLoops[g_MapMakerAdvancedLoopCount++];
    loop->first_vertex = (u16)g_MapMakerAdvancedVertexCount;
    loop->vertex_count = (u8)count;
    loop->flags = 0;
    memcpy(&g_MapMakerAdvancedVertices[g_MapMakerAdvancedVertexCount], points,
            count * sizeof(MapMakerAdvancedVertex));
    g_MapMakerAdvancedVertexCount += count;
    mesh->loop_count++;
    return 1;
}

static void mapmakerAdvancedMakePoint(MapMakerAdvancedVertex *v, f32 x, f32 y, f32 z)
{
    v->x = mapmakerAdvancedSnapCoord(x);
    v->y = mapmakerAdvancedSnapCoord(y);
    v->z = mapmakerAdvancedSnapCoord(z);
}

static s32 mapmakerAdvancedCreateDefaultMesh(void)
{
    MapMakerAdvancedMesh *mesh;
    MapMakerAdvancedVertex face[MAPMAKER_ADV_MAX_POINTS_PER_LOOP];
    s32 meshindex;
    s32 x = g_MapMakerCursorX * MM_CELL_SIZE;
    s32 y = g_MapMakerCursorY * MM_LAYER_HEIGHT + MM_FLOOR_HEIGHT;
    s32 z = g_MapMakerCursorZ * MM_CELL_SIZE;
    s32 i;
    s32 j;
    s32 loops = 1;
    s32 verts = 4;
    s32 segments = 12;
    f32 a0;
    f32 a1;

    switch (g_MapMakerAdvancedMeshType)
    {
    case MAPADV_MESH_CUBE: loops = 6; verts = 24; break;
    case MAPADV_MESH_CIRCLE: loops = 1; verts = segments; break;
    case MAPADV_MESH_UV_SPHERE: loops = 32; verts = 112; break;
    case MAPADV_MESH_ICO_SPHERE: loops = 20; verts = 60; break;
    case MAPADV_MESH_CYLINDER: loops = segments + 2; verts = segments * 4 + segments * 2; break;
    case MAPADV_MESH_CONE: loops = segments + 1; verts = segments * 3 + segments; break;
    case MAPADV_MESH_TORUS: loops = 48; verts = 192; break;
    default: break;
    }

    if (!mapmakerAdvancedReserveMesh(loops, verts, &meshindex)) return 0;
    mesh = &g_MapMakerAdvancedMeshes[meshindex];

    if (g_MapMakerAdvancedMeshType == MAPADV_MESH_PLANE)
    {
        mapmakerAdvancedMakePoint(&face[0], x - 80, y, z - 80);
        mapmakerAdvancedMakePoint(&face[1], x + 80, y, z - 80);
        mapmakerAdvancedMakePoint(&face[2], x + 80, y, z + 80);
        mapmakerAdvancedMakePoint(&face[3], x - 80, y, z + 80);
        mapmakerAdvancedAddFace(mesh, face, 4);
    }
    else if (g_MapMakerAdvancedMeshType == MAPADV_MESH_CUBE)
    {
        static const s8 corners[8][3] = {
            {-1,-1,-1},{1,-1,-1},{1,1,-1},{-1,1,-1},
            {-1,-1,1},{1,-1,1},{1,1,1},{-1,1,1}
        };
        static const u8 faces[6][4] = {
            {0,1,2,3},{5,4,7,6},{4,0,3,7},{1,5,6,2},{3,2,6,7},{4,5,1,0}
        };
        for (i = 0; i < 6; i++)
        {
            for (j = 0; j < 4; j++)
            {
                s32 c = faces[i][j];
                mapmakerAdvancedMakePoint(&face[j], x + corners[c][0] * 80,
                        y + corners[c][1] * 80 + 80, z + corners[c][2] * 80);
            }
            mapmakerAdvancedAddFace(mesh, face, 4);
        }
    }
    else if (g_MapMakerAdvancedMeshType == MAPADV_MESH_CIRCLE)
    {
        for (i = 0; i < segments; i++)
        {
            a0 = (M_TAU_F * i) / segments;
            mapmakerAdvancedMakePoint(&face[i], x + cosf(a0) * 80.0f, y, z + sinf(a0) * 80.0f);
        }
        mapmakerAdvancedAddFace(mesh, face, segments);
    }
    else if (g_MapMakerAdvancedMeshType == MAPADV_MESH_CYLINDER
            || g_MapMakerAdvancedMeshType == MAPADV_MESH_CONE)
    {
        for (i = 0; i < segments; i++)
        {
            a0 = (M_TAU_F * i) / segments;
            mapmakerAdvancedMakePoint(&face[i], x + cosf(a0) * 70.0f, y - 80.0f, z + sinf(a0) * 70.0f);
        }
        mapmakerAdvancedAddFace(mesh, face, segments);

        if (g_MapMakerAdvancedMeshType == MAPADV_MESH_CYLINDER)
        {
            for (i = 0; i < segments; i++)
            {
                a0 = (M_TAU_F * (segments - 1 - i)) / segments;
                mapmakerAdvancedMakePoint(&face[i], x + cosf(a0) * 70.0f, y + 80.0f, z + sinf(a0) * 70.0f);
            }
            mapmakerAdvancedAddFace(mesh, face, segments);
        }

        for (i = 0; i < segments; i++)
        {
            a0 = (M_TAU_F * i) / segments;
            a1 = (M_TAU_F * ((i + 1) % segments)) / segments;
            if (g_MapMakerAdvancedMeshType == MAPADV_MESH_CONE)
            {
                mapmakerAdvancedMakePoint(&face[0], x + cosf(a0) * 70.0f, y - 80.0f, z + sinf(a0) * 70.0f);
                mapmakerAdvancedMakePoint(&face[1], x + cosf(a1) * 70.0f, y - 80.0f, z + sinf(a1) * 70.0f);
                mapmakerAdvancedMakePoint(&face[2], x, y + 80.0f, z);
                mapmakerAdvancedAddFace(mesh, face, 3);
            }
            else
            {
                mapmakerAdvancedMakePoint(&face[0], x + cosf(a0) * 70.0f, y - 80.0f, z + sinf(a0) * 70.0f);
                mapmakerAdvancedMakePoint(&face[1], x + cosf(a1) * 70.0f, y - 80.0f, z + sinf(a1) * 70.0f);
                mapmakerAdvancedMakePoint(&face[2], x + cosf(a1) * 70.0f, y + 80.0f, z + sinf(a1) * 70.0f);
                mapmakerAdvancedMakePoint(&face[3], x + cosf(a0) * 70.0f, y + 80.0f, z + sinf(a0) * 70.0f);
                mapmakerAdvancedAddFace(mesh, face, 4);
            }
        }
    }
    else if (g_MapMakerAdvancedMeshType == MAPADV_MESH_ICO_SPHERE)
    {
        /* Low-cost 20-triangle icosahedron: enough to author a round prop/room
         * shell without creating a pathological N64 vertex budget. */
        static const s16 base[12][3] = {
            {-42,68,0},{42,68,0},{-42,-68,0},{42,-68,0},
            {0,-42,68},{0,42,68},{0,-42,-68},{0,42,-68},
            {68,0,-42},{68,0,42},{-68,0,-42},{-68,0,42}
        };
        static const u8 tri[20][3] = {
            {0,11,5},{0,5,1},{0,1,7},{0,7,10},{0,10,11},
            {1,5,9},{5,11,4},{11,10,2},{10,7,6},{7,1,8},
            {3,9,4},{3,4,2},{3,2,6},{3,6,8},{3,8,9},
            {4,9,5},{2,4,11},{6,2,10},{8,6,7},{9,8,1}
        };
        for (i = 0; i < 20; i++)
        {
            for (j = 0; j < 3; j++)
            {
                s32 c = tri[i][j];
                mapmakerAdvancedMakePoint(&face[j], x + base[c][0], y + base[c][1], z + base[c][2]);
            }
            mapmakerAdvancedAddFace(mesh, face, 3);
        }
    }
    else if (g_MapMakerAdvancedMeshType == MAPADV_MESH_UV_SPHERE)
    {
        s32 lat;
        s32 segs = 8;
        s32 bands = 4;
        for (lat = 0; lat < bands; lat++)
        {
            f32 p0 = -M_PI_F * 0.5f + (M_PI_F * lat) / bands;
            f32 p1 = -M_PI_F * 0.5f + (M_PI_F * (lat + 1)) / bands;
            for (i = 0; i < segs; i++)
            {
                a0 = (M_TAU_F * i) / segs;
                a1 = (M_TAU_F * ((i + 1) % segs)) / segs;
                if (lat == 0)
                {
                    mapmakerAdvancedMakePoint(&face[0], x, y - 80.0f, z);
                    mapmakerAdvancedMakePoint(&face[1], x + cosf(p1) * cosf(a1) * 80.0f, y + sinf(p1) * 80.0f, z + cosf(p1) * sinf(a1) * 80.0f);
                    mapmakerAdvancedMakePoint(&face[2], x + cosf(p1) * cosf(a0) * 80.0f, y + sinf(p1) * 80.0f, z + cosf(p1) * sinf(a0) * 80.0f);
                    mapmakerAdvancedAddFace(mesh, face, 3);
                }
                else if (lat == bands - 1)
                {
                    mapmakerAdvancedMakePoint(&face[0], x, y + 80.0f, z);
                    mapmakerAdvancedMakePoint(&face[1], x + cosf(p0) * cosf(a0) * 80.0f, y + sinf(p0) * 80.0f, z + cosf(p0) * sinf(a0) * 80.0f);
                    mapmakerAdvancedMakePoint(&face[2], x + cosf(p0) * cosf(a1) * 80.0f, y + sinf(p0) * 80.0f, z + cosf(p0) * sinf(a1) * 80.0f);
                    mapmakerAdvancedAddFace(mesh, face, 3);
                }
                else
                {
                    mapmakerAdvancedMakePoint(&face[0], x + cosf(p0) * cosf(a0) * 80.0f, y + sinf(p0) * 80.0f, z + cosf(p0) * sinf(a0) * 80.0f);
                    mapmakerAdvancedMakePoint(&face[1], x + cosf(p0) * cosf(a1) * 80.0f, y + sinf(p0) * 80.0f, z + cosf(p0) * sinf(a1) * 80.0f);
                    mapmakerAdvancedMakePoint(&face[2], x + cosf(p1) * cosf(a1) * 80.0f, y + sinf(p1) * 80.0f, z + cosf(p1) * sinf(a1) * 80.0f);
                    mapmakerAdvancedMakePoint(&face[3], x + cosf(p1) * cosf(a0) * 80.0f, y + sinf(p1) * 80.0f, z + cosf(p1) * sinf(a0) * 80.0f);
                    mapmakerAdvancedAddFace(mesh, face, 4);
                }
            }
        }
    }
    else if (g_MapMakerAdvancedMeshType == MAPADV_MESH_TORUS)
    {
        s32 major;
        s32 minor;
        for (major = 0; major < 8; major++)
        {
            f32 ma0 = (M_TAU_F * major) / 8.0f;
            f32 ma1 = (M_TAU_F * ((major + 1) % 8)) / 8.0f;
            for (minor = 0; minor < 6; minor++)
            {
                f32 mi0 = (M_TAU_F * minor) / 6.0f;
                f32 mi1 = (M_TAU_F * ((minor + 1) % 6)) / 6.0f;
                f32 r0 = 58.0f + cosf(mi0) * 22.0f;
                f32 r1 = 58.0f + cosf(mi1) * 22.0f;
                mapmakerAdvancedMakePoint(&face[0], x + cosf(ma0) * r0, y + sinf(mi0) * 22.0f, z + sinf(ma0) * r0);
                mapmakerAdvancedMakePoint(&face[1], x + cosf(ma1) * r0, y + sinf(mi0) * 22.0f, z + sinf(ma1) * r0);
                mapmakerAdvancedMakePoint(&face[2], x + cosf(ma1) * r1, y + sinf(mi1) * 22.0f, z + sinf(ma1) * r1);
                mapmakerAdvancedMakePoint(&face[3], x + cosf(ma0) * r1, y + sinf(mi1) * 22.0f, z + sinf(ma0) * r1);
                mapmakerAdvancedAddFace(mesh, face, 4);
            }
        }
    }

    g_MapMakerAdvancedSelectedMesh = meshindex;
    g_MapMakerAdvancedSelectedVertex = mesh->loop_count > 0
        ? g_MapMakerAdvancedLoops[mesh->first_loop].first_vertex : -1;
    mapmakerMarkDirty();
    strcpy(g_MapMakerStorageStatus, "Advanced: primitive created");
    return 1;
}

static void mapmakerAdvancedUpdateSelection(void)
{
    f32 camx;
    f32 camy;
    f32 camz;
    f32 dirx;
    f32 diry;
    f32 dirz;
    f32 bestscore = 1000000.0f;
    s32 bestvertex = -1;
    s32 bestmesh = -1;
    s32 m;

    if (g_MapMakerAdvancedDraggingVertex) return;
    mapmakerGetCamera(&camx, &camy, &camz, &dirx, &diry, &dirz);

    for (m = 0; m < g_MapMakerAdvancedMeshCount; m++)
    {
        MapMakerAdvancedMesh *mesh = &g_MapMakerAdvancedMeshes[m];
        s32 l;
        if (mapmakerAdvancedMeshIsDeleted(mesh)) continue;
        for (l = 0; l < mesh->loop_count; l++)
        {
            MapMakerAdvancedLoop *loop = &g_MapMakerAdvancedLoops[mesh->first_loop + l];
            s32 j;
            for (j = 0; j < loop->vertex_count; j++)
            {
                s32 vi = loop->first_vertex + j;
                MapMakerAdvancedVertex *v = &g_MapMakerAdvancedVertices[vi];
                f32 vx = v->x - camx;
                f32 vy = v->y - camy;
                f32 vz = v->z - camz;
                f32 along = vx * dirx + vy * diry + vz * dirz;
                f32 px;
                f32 py;
                f32 pz;
                f32 dist2;
                f32 score;
                if (along < 10.0f || along > 6000.0f) continue;
                px = vx - dirx * along;
                py = vy - diry * along;
                pz = vz - dirz * along;
                dist2 = px * px + py * py + pz * pz;
                if (dist2 > 28.0f * 28.0f) continue;
                score = dist2 + along * 0.002f;
                if (score < bestscore)
                {
                    bestscore = score;
                    bestvertex = vi;
                    bestmesh = m;
                }
            }
        }
    }

    g_MapMakerAdvancedSelectedVertex = bestvertex;
    g_MapMakerAdvancedSelectedMesh = bestmesh;
}

static s32 mapmakerAdvancedBuildDragPlane(s32 meshindex, f32 *point, f32 *normal)
{
    MapMakerAdvancedMesh *mesh;
    MapMakerAdvancedLoop *loop;
    MapMakerAdvancedVertex *a;
    MapMakerAdvancedVertex *b;
    MapMakerAdvancedVertex *c;
    f32 ux;
    f32 uy;
    f32 uz;
    f32 vx;
    f32 vy;
    f32 vz;
    f32 len;

    if (meshindex < 0 || meshindex >= g_MapMakerAdvancedMeshCount) return 0;
    mesh = &g_MapMakerAdvancedMeshes[meshindex];
    if (mesh->loop_count < 1) return 0;
    loop = &g_MapMakerAdvancedLoops[mesh->first_loop];
    if (loop->vertex_count < 3) return 0;
    a = &g_MapMakerAdvancedVertices[loop->first_vertex];
    b = &g_MapMakerAdvancedVertices[loop->first_vertex + 1];
    c = &g_MapMakerAdvancedVertices[loop->first_vertex + 2];
    ux = b->x - a->x; uy = b->y - a->y; uz = b->z - a->z;
    vx = c->x - a->x; vy = c->y - a->y; vz = c->z - a->z;
    normal[0] = uy * vz - uz * vy;
    normal[1] = uz * vx - ux * vz;
    normal[2] = ux * vy - uy * vx;
    len = sqrtf(normal[0] * normal[0] + normal[1] * normal[1] + normal[2] * normal[2]);
    if (len < 0.001f) return 0;
    normal[0] /= len; normal[1] /= len; normal[2] /= len;
    point[0] = a->x; point[1] = a->y; point[2] = a->z;
    return 1;
}

static void mapmakerAdvancedBeginOrEndDrag(void)
{
    if (g_MapMakerAdvancedDraggingVertex)
    {
        g_MapMakerAdvancedDraggingVertex = 0;
        mapmakerMarkDirty();
        strcpy(g_MapMakerStorageStatus, "Advanced: vertex placed");
        return;
    }

    if (g_MapMakerAdvancedSelectedVertex < 0 || g_MapMakerAdvancedSelectedMesh < 0)
    {
        strcpy(g_MapMakerStorageStatus, "Advanced: aim at a circular corner handle first");
        return;
    }
    if (!mapmakerAdvancedBuildDragPlane(g_MapMakerAdvancedSelectedMesh,
            g_MapMakerAdvancedDragPlanePoint, g_MapMakerAdvancedDragPlaneNormal))
    {
        strcpy(g_MapMakerStorageStatus, "Advanced: selected mesh has an invalid edit plane");
        return;
    }
    g_MapMakerAdvancedDragOriginal = g_MapMakerAdvancedVertices[g_MapMakerAdvancedSelectedVertex];
    g_MapMakerAdvancedDraggingVertex = 1;
    strcpy(g_MapMakerStorageStatus, "Advanced: dragging vertex - A place, B cancel");
}

static void mapmakerAdvancedCancelDrag(void)
{
    if (!g_MapMakerAdvancedDraggingVertex) return;
    g_MapMakerAdvancedVertices[g_MapMakerAdvancedSelectedVertex] = g_MapMakerAdvancedDragOriginal;
    g_MapMakerAdvancedDraggingVertex = 0;
    strcpy(g_MapMakerStorageStatus, "Advanced: vertex drag cancelled");
}

static void mapmakerAdvancedUpdateDrag(void)
{
    f32 camx;
    f32 camy;
    f32 camz;
    f32 dirx;
    f32 diry;
    f32 dirz;
    f32 denom;
    f32 t;
    MapMakerAdvancedVertex *v;

    if (!g_MapMakerAdvancedDraggingVertex || g_MapMakerAdvancedSelectedVertex < 0) return;
    mapmakerGetCamera(&camx, &camy, &camz, &dirx, &diry, &dirz);
    denom = dirx * g_MapMakerAdvancedDragPlaneNormal[0]
        + diry * g_MapMakerAdvancedDragPlaneNormal[1]
        + dirz * g_MapMakerAdvancedDragPlaneNormal[2];
    if (denom > -0.001f && denom < 0.001f) return;
    t = ((g_MapMakerAdvancedDragPlanePoint[0] - camx) * g_MapMakerAdvancedDragPlaneNormal[0]
        + (g_MapMakerAdvancedDragPlanePoint[1] - camy) * g_MapMakerAdvancedDragPlaneNormal[1]
        + (g_MapMakerAdvancedDragPlanePoint[2] - camz) * g_MapMakerAdvancedDragPlaneNormal[2]) / denom;
    if (t < 20.0f || t > 6000.0f) return;
    v = &g_MapMakerAdvancedVertices[g_MapMakerAdvancedSelectedVertex];
    v->x = mapmakerAdvancedSnapCoord(camx + dirx * t);
    v->y = mapmakerAdvancedSnapCoord(camy + diry * t);
    v->z = mapmakerAdvancedSnapCoord(camz + dirz * t);
}

static s32 mapmakerAdvancedGetSelectedCenter(f32 *cx, f32 *cy, f32 *cz)
{
    MapMakerAdvancedMesh *mesh;
    s32 l;
    s32 count = 0;
    f32 x = 0.0f;
    f32 y = 0.0f;
    f32 z = 0.0f;

    if (g_MapMakerAdvancedSelectedMesh < 0 || g_MapMakerAdvancedSelectedMesh >= g_MapMakerAdvancedMeshCount)
        return 0;
    mesh = &g_MapMakerAdvancedMeshes[g_MapMakerAdvancedSelectedMesh];
    if (mapmakerAdvancedMeshIsDeleted(mesh)) return 0;

    for (l = 0; l < mesh->loop_count; l++)
    {
        MapMakerAdvancedLoop *loop = &g_MapMakerAdvancedLoops[mesh->first_loop + l];
        s32 j;
        for (j = 0; j < loop->vertex_count; j++)
        {
            MapMakerAdvancedVertex *v = &g_MapMakerAdvancedVertices[loop->first_vertex + j];
            x += v->x; y += v->y; z += v->z; count++;
        }
    }
    if (count <= 0) return 0;
    *cx = x / count; *cy = y / count; *cz = z / count;
    return 1;
}

static void mapmakerAdvancedTransformSelected(s32 direction)
{
    MapMakerAdvancedMesh *mesh;
    f32 cx;
    f32 cy;
    f32 cz;
    s32 l;

    if (!mapmakerAdvancedGetSelectedCenter(&cx, &cy, &cz))
    {
        strcpy(g_MapMakerStorageStatus, "Advanced: aim at a mesh first");
        return;
    }
    mesh = &g_MapMakerAdvancedMeshes[g_MapMakerAdvancedSelectedMesh];

    for (l = 0; l < mesh->loop_count; l++)
    {
        MapMakerAdvancedLoop *loop = &g_MapMakerAdvancedLoops[mesh->first_loop + l];
        s32 j;
        for (j = 0; j < loop->vertex_count; j++)
        {
            MapMakerAdvancedVertex *v = &g_MapMakerAdvancedVertices[loop->first_vertex + j];
            f32 x = v->x - cx;
            f32 y = v->y - cy;
            f32 z = v->z - cz;

            if (g_MapMakerAdvancedTool == MAPADV_TOOL_MOVE)
            {
                f32 step = 10.0f * direction;
                if (g_MapMakerAdvancedAxis == MAPADV_AXIS_X || g_MapMakerAdvancedAxis == MAPADV_AXIS_ALL) x += step;
                if (g_MapMakerAdvancedAxis == MAPADV_AXIS_Y || g_MapMakerAdvancedAxis == MAPADV_AXIS_ALL) y += step;
                if (g_MapMakerAdvancedAxis == MAPADV_AXIS_Z || g_MapMakerAdvancedAxis == MAPADV_AXIS_ALL) z += step;
            }
            else if (g_MapMakerAdvancedTool == MAPADV_TOOL_SCALE)
            {
                f32 scale = direction > 0 ? 1.10f : 0.9090909f;
                if (g_MapMakerAdvancedAxis == MAPADV_AXIS_X || g_MapMakerAdvancedAxis == MAPADV_AXIS_ALL) x *= scale;
                if (g_MapMakerAdvancedAxis == MAPADV_AXIS_Y || g_MapMakerAdvancedAxis == MAPADV_AXIS_ALL) y *= scale;
                if (g_MapMakerAdvancedAxis == MAPADV_AXIS_Z || g_MapMakerAdvancedAxis == MAPADV_AXIS_ALL) z *= scale;
            }
            else if (g_MapMakerAdvancedTool == MAPADV_TOOL_ROTATE)
            {
                f32 angle = direction * (g_MapMakerArbitraryRotation ? (M_PI_F / 36.0f) : (M_PI_F / 4.0f));
                f32 sn = sinf(angle);
                f32 cs = cosf(angle);
                f32 nx = x;
                f32 ny = y;
                f32 nz = z;
                s32 axis = g_MapMakerAdvancedAxis == MAPADV_AXIS_ALL ? MAPADV_AXIS_Y : g_MapMakerAdvancedAxis;
                if (axis == MAPADV_AXIS_X) { ny = y * cs - z * sn; nz = y * sn + z * cs; }
                else if (axis == MAPADV_AXIS_Y) { nx = x * cs + z * sn; nz = -x * sn + z * cs; }
                else { nx = x * cs - y * sn; ny = x * sn + y * cs; }
                x = nx; y = ny; z = nz;
            }

            v->x = mapmakerAdvancedSnapCoord(cx + x);
            v->y = mapmakerAdvancedSnapCoord(cy + y);
            v->z = mapmakerAdvancedSnapCoord(cz + z);
        }
    }
    mapmakerMarkDirty();
}

static s32 mapmakerAdvancedExtrudeSelected(void)
{
    MapMakerAdvancedMesh *oldmesh;
    MapMakerAdvancedLoop *oldloop;
    MapMakerAdvancedMesh *newmesh;
    MapMakerAdvancedVertex base[MAPMAKER_ADV_MAX_POINTS_PER_LOOP];
    MapMakerAdvancedVertex face[MAPMAKER_ADV_MAX_POINTS_PER_LOOP];
    f32 normal[3];
    f32 point[3];
    s32 n;
    s32 i;
    s32 newindex;

    if (g_MapMakerAdvancedSelectedMesh < 0 || g_MapMakerAdvancedSelectedMesh >= g_MapMakerAdvancedMeshCount)
    {
        strcpy(g_MapMakerStorageStatus, "Depth: select a plane/circle first");
        return 0;
    }
    oldmesh = &g_MapMakerAdvancedMeshes[g_MapMakerAdvancedSelectedMesh];
    if (mapmakerAdvancedMeshIsDeleted(oldmesh) || oldmesh->loop_count != 1)
    {
        strcpy(g_MapMakerStorageStatus, "Depth applies to a paper-thin single-face mesh");
        return 0;
    }
    oldloop = &g_MapMakerAdvancedLoops[oldmesh->first_loop];
    n = oldloop->vertex_count;
    if (n < 3 || n > MAPMAKER_ADV_MAX_POINTS_PER_LOOP
            || !mapmakerAdvancedBuildDragPlane(g_MapMakerAdvancedSelectedMesh, point, normal))
    {
        strcpy(g_MapMakerStorageStatus, "Depth: invalid source face");
        return 0;
    }
    memcpy(base, &g_MapMakerAdvancedVertices[oldloop->first_vertex], n * sizeof(MapMakerAdvancedVertex));
    if (!mapmakerAdvancedReserveMesh(n + 2, n * 6, &newindex)) return 0;
    newmesh = &g_MapMakerAdvancedMeshes[newindex];
    newmesh->type = oldmesh->type;
    newmesh->material = oldmesh->material;
    newmesh->room = oldmesh->room;

    for (i = 0; i < n; i++) face[i] = base[i];
    mapmakerAdvancedAddFace(newmesh, face, n);
    for (i = 0; i < n; i++)
    {
        s32 ri = n - 1 - i;
        mapmakerAdvancedMakePoint(&face[i], base[ri].x + normal[0] * 40.0f,
                base[ri].y + normal[1] * 40.0f, base[ri].z + normal[2] * 40.0f);
    }
    mapmakerAdvancedAddFace(newmesh, face, n);
    for (i = 0; i < n; i++)
    {
        s32 next = (i + 1) % n;
        face[0] = base[i];
        face[1] = base[next];
        mapmakerAdvancedMakePoint(&face[2], base[next].x + normal[0] * 40.0f,
                base[next].y + normal[1] * 40.0f, base[next].z + normal[2] * 40.0f);
        mapmakerAdvancedMakePoint(&face[3], base[i].x + normal[0] * 40.0f,
                base[i].y + normal[1] * 40.0f, base[i].z + normal[2] * 40.0f);
        mapmakerAdvancedAddFace(newmesh, face, 4);
    }

    oldmesh->flags |= 0x8000;
    g_MapMakerAdvancedSelectedMesh = newindex;
    g_MapMakerAdvancedSelectedVertex = g_MapMakerAdvancedLoops[newmesh->first_loop].first_vertex;
    mapmakerMarkDirty();
    strcpy(g_MapMakerStorageStatus, "Depth: created 40-unit solid prism");
    return 1;
}

static s32 mapmakerAdvancedCreatePortal(void)
{
    MapMakerAdvancedPortal *portal;
    if (g_MapMakerAdvancedPortalCount >= MAPMAKER_ADV_MAX_PORTALS)
    {
        strcpy(g_MapMakerStorageStatus, "Portal limit reached");
        return 0;
    }
    portal = &g_MapMakerAdvancedPortals[g_MapMakerAdvancedPortalCount++];
    memset(portal, 0, sizeof(*portal));
    portal->x = (s16)(g_MapMakerCursorX * MM_CELL_SIZE);
    portal->y = (s16)(g_MapMakerCursorY * MM_LAYER_HEIGHT + 60);
    portal->z = (s16)(g_MapMakerCursorZ * MM_CELL_SIZE);
    portal->half_width = 80;
    portal->half_height = 60;
    portal->axis = g_MapMakerAdvancedAxis == MAPADV_AXIS_X ? 0 : 1;
    portal->room_a = (u8)g_MapMakerAdvancedRoom;
    portal->room_b = (u8)((g_MapMakerAdvancedRoom + 1) & 0xff);
    g_MapMakerAdvancedSelectedPortal = g_MapMakerAdvancedPortalCount - 1;
    mapmakerMarkDirty();
    strcpy(g_MapMakerStorageStatus, "Portal created between current room and next room");
    return 1;
}

static void mapmakerAdvancedAssignRoom(void)
{
    if (g_MapMakerAdvancedSelectedMesh < 0 || g_MapMakerAdvancedSelectedMesh >= g_MapMakerAdvancedMeshCount)
    {
        strcpy(g_MapMakerStorageStatus, "Room: aim at a mesh first");
        return;
    }
    g_MapMakerAdvancedMeshes[g_MapMakerAdvancedSelectedMesh].room = (u8)g_MapMakerAdvancedRoom;
    mapmakerMarkDirty();
    strcpy(g_MapMakerStorageStatus, "Room assigned to selected mesh");
}

static void mapmakerAdvancedDeleteSelectedMesh(void)
{
    if (g_MapMakerAdvancedSelectedMesh < 0 || g_MapMakerAdvancedSelectedMesh >= g_MapMakerAdvancedMeshCount)
    {
        strcpy(g_MapMakerStorageStatus, "Advanced: aim at a mesh corner first");
        return;
    }
    g_MapMakerAdvancedMeshes[g_MapMakerAdvancedSelectedMesh].flags |= 0x8000;
    g_MapMakerAdvancedSelectedMesh = -1;
    g_MapMakerAdvancedSelectedVertex = -1;
    mapmakerMarkDirty();
    strcpy(g_MapMakerStorageStatus, "Advanced: mesh deleted");
}

static s32 mapmakerScratchIsValid(void)
{
    return g_MapMakerScratch->magic == MAPMAKER_SCRATCH_MAGIC;
}

static void mapmakerScratchReset(void)
{
    /* Only clear the editor-owned portion. The linker guarantees the full
     * structure fits in the borrowed Expansion-Pak window. */
    memset(g_MapMakerScratch, 0, sizeof(*g_MapMakerScratch));
    g_MapMakerScratch->magic = MAPMAKER_SCRATCH_MAGIC;
}

void mapmakerBasicInit(void)
{
    s32 i;
    f32 orbitYaw = 3.9f;
    f32 orbitPitch = 0.55f;
    f32 cp = cosf(orbitPitch);

    /* The resident map intentionally survives leaving the editor.  Only the
     * transient camera/tool state is reset on re-entry. */
    if (!g_MapMakerInitialized || !mapmakerScratchIsValid())
    {
        mapmakerScratchReset();
        g_MapMakerModuleCount = 0;
        g_MapMakerEntityCount = 0;
        g_MapMakerPlayerStartCount = 0;
        g_MapMakerAdvancedMeshCount = 0;
        g_MapMakerAdvancedLoopCount = 0;
        g_MapMakerAdvancedVertexCount = 0;
        g_MapMakerAdvancedPortalCount = 0;
        for (i = 0; i < MAPMAKER_MATERIAL_SLOTS; i++)
            g_MapMakerMaterialTexture[i] = (u16)i;
        strcpy(g_MapMakerCurrentName, "Untitled");
        g_MapMakerDirty = 1;
        g_MapMakerMusicTrack = g_musicXTrack1CurrentTrackNum;
        if (g_MapMakerMusicTrack < MM_MUSIC_FIRST || g_MapMakerMusicTrack > MM_MUSIC_LAST)
            g_MapMakerMusicTrack = M_INTRO;
        g_MapMakerInitialized = 1;

        /* Native Test Mode loads the dedicated Map Maker stage, which
         * legitimately reuses the editor's borrowed Expansion-Pak scratch.
         * Rehydrate the exact map snapshot when returning instead of treating
         * that overwrite as a new/blank map. */
        if (g_MapMakerRestoreSessionOnInit && g_MapMakerSessionSaveValid)
            mapmakerApplySaveBlob(g_MapMakerSessionSave.bytes, MM_SAVE_BYTES);
    }
    g_MapMakerRestoreSessionOnInit = 0;

    g_MapMakerCursorX = 0;
    g_MapMakerCursorY = 0;
    g_MapMakerCursorZ = 0;
    g_MapMakerModuleType = MAPMODULE_FLOOR;
    g_MapMakerRotation = 0;
    g_MapMakerMoveRepeat = 0;
    g_MapMakerCameraYaw = orbitYaw + 3.14159265f;
    g_MapMakerCameraPitch = -orbitPitch;
    g_MapMakerCameraDistance = 1050.0f;
    g_MapMakerFreeCameraX = sinf(orbitYaw) * cp * g_MapMakerCameraDistance;
    g_MapMakerFreeCameraY = 40.0f + sinf(orbitPitch) * g_MapMakerCameraDistance;
    g_MapMakerFreeCameraZ = cosf(orbitYaw) * cp * g_MapMakerCameraDistance;
    g_MapMakerMenuOpen = 0;
    g_MapMakerMenuChoice = MM_MENU_RESUME;
    g_MapMakerMusicPreviewPending = 0;
    g_MapMakerMusicPreviewDelay = 0;
    g_MapMakerPaintDragModule = -1;
    g_MapMakerPaintDragSurface = -1;
    g_MapMakerTexturePoolRecyclePending = 0;
    g_MapMakerBuildDragX = g_MapMakerBuildDragY = g_MapMakerBuildDragZ = 9999;
    g_MapMakerBuildDragType = g_MapMakerBuildDragRotation = -1;
    g_MapMakerTextureFlip = 0;
    g_MapMakerEntityType = PROPDEF_PROP;
    g_MapMakerEntityObjectId = 0;
    g_MapMakerEntityAuxId = 0;
    g_MapMakerPickupType = MAPPICKUP_WEAPON_ITEM;
    g_MapMakerPlayerStartSlot = 0;
    mapmakerResetPickupSelection();
    g_MapMakerSnapEnabled = 1;
    g_MapMakerPlacementEdge = 1;
    g_MapMakerGridSize = MM_CELL_SIZE;
    g_MapMakerPlaytest = 0;
    g_MapMakerTool = MAPTOOL_BUILD;
    g_MapMakerFreeView = 1;
    g_MapMakerMaterialSlot = 0;
    g_MapMakerSelectedModule = -1;
    g_MapMakerSelectedSurface = -1;
    g_MapMakerAdvancedMeshType = MAPADV_MESH_PLANE;
    g_MapMakerAdvancedTool = MAPADV_TOOL_CREATE;
    g_MapMakerAdvancedAxis = MAPADV_AXIS_ALL;
    g_MapMakerAdvancedRoom = 0;
    g_MapMakerAdvancedSelectedPortal = -1;
    g_MapMakerAdvancedSelectedMesh = -1;
    g_MapMakerAdvancedSelectedVertex = -1;
    g_MapMakerAdvancedDraggingVertex = 0;
    g_MapMakerPlayerVelY = 0.0f;

    /* Map Maker temporarily owns the global frontend texture-pool descriptor.
     * Snapshot it so leaving the editor can restore the folder/portrait pool;
     * otherwise the next mission/multiplayer portrait screen can reuse stale
     * Map Maker texture cache entries and display corrupted thumbnails. */
    memcpy(&g_MapMakerSavedDefaultTexturePool,
            (struct texpool *)&ptr_texture_alloc_start,
            sizeof(g_MapMakerSavedDefaultTexturePool));
    g_MapMakerFrontendPoolCaptured = 1;
    mapmakerResetPrivateTexturePool();

    if (g_MapMakerEditorMode == MAPMAKER_EDITOR_ADVANCED)
        strcpy(g_MapMakerStorageStatus, "Advanced: A select, L tool, Z action, hold B erase");
    else
        strcpy(g_MapMakerStorageStatus, "Edit Mode - resident map preserved in RAM");
    mapmakerUpdateFreeCursor();
    mapmakerUpdateSurfaceSelection();
}

s32 mapmakerBasicTick(void)
{
    u32 pressed = joyGetButtonsPressedThisFrame(PLAYER_1, 0xffff);
    u32 held = joyGetButtons(PLAYER_1, 0xffff);
    s32 stickx = joyGetStickX(PLAYER_1);
    s32 sticky = joyGetStickY(PLAYER_1);

    if (g_MapMakerPlaytest)
    {
        mapmakerTickPlaytest();
        return 0;
    }

    if (g_MapMakerMenuOpen)
    {
        if (pressed & (START_BUTTON | B_BUTTON))
        {
            g_MapMakerMusicPreviewPending = 0;
            g_MapMakerMusicPreviewDelay = 0;
            g_MapMakerMenuOpen = 0;
            g_MapMakerMoveRepeat = MM_MOVE_REPEAT;
            return 0;
        }

        if (pressed & (U_JPAD | U_CBUTTONS))
            g_MapMakerMenuChoice = (g_MapMakerMenuChoice + MM_MENU_COUNT - 1) % MM_MENU_COUNT;
        if (pressed & (D_JPAD | D_CBUTTONS))
            g_MapMakerMenuChoice = (g_MapMakerMenuChoice + 1) % MM_MENU_COUNT;

        if (g_MapMakerMenuChoice == MM_MENU_TOOL)
        {
            if (pressed & (L_JPAD | R_JPAD | A_BUTTON | Z_TRIG))
                g_MapMakerTool = (g_MapMakerTool + 1) % MAPTOOL_COUNT;
        }
        else if (g_MapMakerMenuChoice == MM_MENU_VIEW)
        {
            if (pressed & (L_JPAD | R_JPAD | A_BUTTON | Z_TRIG))
                mapmakerSetFreeView(!g_MapMakerFreeView);
        }
        else if (g_MapMakerMenuChoice == MM_MENU_MATERIAL)
        {
            if (pressed & L_JPAD) mapmakerChangeMaterialTexture(-1);
            if (pressed & R_JPAD) mapmakerChangeMaterialTexture(1);
            if (pressed & L_TRIG) mapmakerChangeMaterialSlot(-1);
            if (pressed & R_TRIG) mapmakerChangeMaterialSlot(1);
            if (pressed & L_CBUTTONS) mapmakerChangeMaterialTexture(-32);
            if (pressed & R_CBUTTONS) mapmakerChangeMaterialTexture(32);
            if (pressed & Z_TRIG) mapmakerChangeMaterialTexture(1);
        }
        else if (g_MapMakerMenuChoice == MM_MENU_SNAP)
        {
            if (pressed & (L_JPAD | R_JPAD | A_BUTTON | Z_TRIG)) g_MapMakerSnapEnabled = !g_MapMakerSnapEnabled;
        }
        else if (g_MapMakerMenuChoice == MM_MENU_GRID)
        {
            if (pressed & L_JPAD) g_MapMakerGridSize -= 20;
            if (pressed & R_JPAD) g_MapMakerGridSize += 20;
            if (g_MapMakerGridSize < 20) g_MapMakerGridSize = 20;
            if (g_MapMakerGridSize > 640) g_MapMakerGridSize = 640;
        }
        else if (g_MapMakerMenuChoice == MM_MENU_PLACE)
        {
            if (pressed & (L_JPAD | R_JPAD | A_BUTTON | Z_TRIG)) g_MapMakerPlacementEdge = !g_MapMakerPlacementEdge;
        }
        else if (g_MapMakerMenuChoice == MM_MENU_FLIP)
        {
            if (pressed & (L_JPAD | R_JPAD | A_BUTTON | Z_TRIG)) g_MapMakerTextureFlip = (g_MapMakerTextureFlip + 1) & 3;
        }
        else if (g_MapMakerMenuChoice == MM_MENU_FOG)
        {
            if (pressed & (L_JPAD | R_JPAD | A_BUTTON | Z_TRIG)) g_MapMakerEditorFog = !g_MapMakerEditorFog;
        }
        else if (g_MapMakerMenuChoice == MM_MENU_ARBITRARY)
        {
            if (pressed & (L_JPAD | R_JPAD | A_BUTTON | Z_TRIG)) g_MapMakerArbitraryRotation = !g_MapMakerArbitraryRotation;
        }
        else if (g_MapMakerMenuChoice == MM_MENU_MUSIC)
        {
            if (pressed & L_JPAD) mapmakerChangeMusic(-1);
            if (pressed & R_JPAD) mapmakerChangeMusic(1);
            if (pressed & L_CBUTTONS) mapmakerChangeMusic(-5);
            if (pressed & R_CBUTTONS) mapmakerChangeMusic(5);
            if (pressed & (A_BUTTON | Z_TRIG))
            {
                g_MapMakerMusicPreviewPending = 0;
                g_MapMakerMusicPreviewDelay = 0;
                mapmakerPlaySelectedMusic();
            }
        }
        else if (pressed & (A_BUTTON | Z_TRIG))
        {
            if (g_MapMakerMenuChoice == MM_MENU_RESUME)
            {
                g_MapMakerMenuOpen = 0;
                g_MapMakerMoveRepeat = MM_MOVE_REPEAT;
            }
            else if (g_MapMakerMenuChoice == MM_MENU_FIRSTPERSON)
            {
                if (g_MapMakerEditorMode == MAPMAKER_EDITOR_ADVANCED)
                    strcpy(g_MapMakerStorageStatus, "Advanced First Person View waits for BG/STAN/room compiler");
                else
                    mapmakerBeginPlaytest();
            }
            else if (g_MapMakerMenuChoice == MM_MENU_NATIVETEST)
            {
                if (g_MapMakerEditorMode == MAPMAKER_EDITOR_ADVANCED)
                    strcpy(g_MapMakerStorageStatus, "Advanced Native Test waits for BG/STAN/room compiler");
                else if (mapmakerRequestNativeTest())
                    return 2;
            }
            else if (g_MapMakerMenuChoice == MM_MENU_SAVE)
            {
                /* GEMAP v8 already persists Advanced meshes/loops/vertices/
                 * portals, so Advanced no longer needs an artificial menu gate. */
                mapmakerSaveMap();
            }
            else if (g_MapMakerMenuChoice == MM_MENU_LOAD)
            {
                mapmakerLoadMap();
            }
            else if (g_MapMakerMenuChoice == MM_MENU_EXIT)
            {
                return 1;
            }
        }

        mapmakerTickMusicPreview();
        return 0;
    }

    if (pressed & START_BUTTON)
    {
        g_MapMakerMenuOpen = 1;
        g_MapMakerMenuChoice = MM_MENU_RESUME;
        return 0;
    }

    if (g_MapMakerEditorMode == MAPMAKER_EDITOR_ADVANCED)
    {
        if (!g_MapMakerFreeView) mapmakerSetFreeView(1);
        mapmakerTickFreeView();
        mapmakerAdvancedUpdateDrag();

        /* A is selection in Advanced.  Selection remains stable until the user
         * explicitly aims and presses A again; it no longer changes merely
         * because the camera crossed another vertex. */
        if ((pressed & A_BUTTON) && !g_MapMakerAdvancedDraggingVertex)
            mapmakerAdvancedUpdateSelection();

        /* L cycles tools.  R+L keeps the old axis cycling function without
         * spending another face button. */
        if (pressed & L_TRIG)
        {
            if (held & R_TRIG)
            {
                g_MapMakerAdvancedAxis = (g_MapMakerAdvancedAxis + 1) % MAPADV_AXIS_COUNT;
                sprintf(g_MapMakerStorageStatus, "Advanced axis: %s", mapmakerAdvancedGetAxisName());
            }
            else
            {
                if (g_MapMakerAdvancedDraggingVertex) mapmakerAdvancedCancelDrag();
                g_MapMakerAdvancedTool = (g_MapMakerAdvancedTool + 1) % MAPADV_TOOL_COUNT;
                sprintf(g_MapMakerStorageStatus, "Advanced tool: %s", mapmakerAdvancedGetToolName());
            }
        }

        /* Height layers are a first-class Advanced feature.  The tools which
         * use Up/Down for a value selector retain Basic-mode semantics below. */
        if (g_MapMakerAdvancedTool != MAPADV_TOOL_TEXTURE
                && !(g_MapMakerAdvancedTool == MAPADV_TOOL_PICKUP && (held & R_TRIG)))
        {
            if ((pressed & U_JPAD) && g_MapMakerCursorY < MAPMAKER_MAX_LAYER) g_MapMakerCursorY++;
            if ((pressed & D_JPAD) && g_MapMakerCursorY > MAPMAKER_MIN_LAYER) g_MapMakerCursorY--;
        }

        if (g_MapMakerAdvancedTool == MAPADV_TOOL_CREATE)
        {
            if (pressed & L_JPAD)
                g_MapMakerAdvancedMeshType = (g_MapMakerAdvancedMeshType + MAPADV_MESH_TYPE_COUNT - 1) % MAPADV_MESH_TYPE_COUNT;
            if (pressed & R_JPAD)
                g_MapMakerAdvancedMeshType = (g_MapMakerAdvancedMeshType + 1) % MAPADV_MESH_TYPE_COUNT;
            if (pressed & Z_TRIG) mapmakerAdvancedCreateDefaultMesh();
        }
        else if (g_MapMakerAdvancedTool == MAPADV_TOOL_VERTEX)
        {
            if (pressed & Z_TRIG) mapmakerAdvancedBeginOrEndDrag();
        }
        else if (g_MapMakerAdvancedTool == MAPADV_TOOL_MOVE
                || g_MapMakerAdvancedTool == MAPADV_TOOL_SCALE)
        {
            if (pressed & Z_TRIG) mapmakerAdvancedTransformSelected((held & R_TRIG) ? -1 : 1);
        }
        else if (g_MapMakerAdvancedTool == MAPADV_TOOL_ROTATE)
        {
            /* Rotation is deliberately an ordered chord: A must already be
             * held when R is pressed.  R-first then A only selects. */
            if ((held & A_BUTTON) && (pressed & R_TRIG)) mapmakerAdvancedTransformSelected(1);
            else if (pressed & Z_TRIG) mapmakerAdvancedTransformSelected((held & R_TRIG) ? -1 : 1);
        }
        else if (g_MapMakerAdvancedTool == MAPADV_TOOL_ROOM)
        {
            if (pressed & L_JPAD) g_MapMakerAdvancedRoom = (g_MapMakerAdvancedRoom + 255) & 255;
            if (pressed & R_JPAD) g_MapMakerAdvancedRoom = (g_MapMakerAdvancedRoom + 1) & 255;
            if (pressed & Z_TRIG) mapmakerAdvancedAssignRoom();
        }
        else if (g_MapMakerAdvancedTool == MAPADV_TOOL_PORTAL)
        {
            if (pressed & L_JPAD) g_MapMakerAdvancedRoom = (g_MapMakerAdvancedRoom + 255) & 255;
            if (pressed & R_JPAD) g_MapMakerAdvancedRoom = (g_MapMakerAdvancedRoom + 1) & 255;
            if (pressed & Z_TRIG) mapmakerAdvancedCreatePortal();
        }
        else if (g_MapMakerAdvancedTool == MAPADV_TOOL_DEPTH)
        {
            if (pressed & Z_TRIG) mapmakerAdvancedExtrudeSelected();
        }
        else if (g_MapMakerAdvancedTool == MAPADV_TOOL_BUILD)
        {
            if (pressed & L_JPAD) g_MapMakerModuleType = (g_MapMakerModuleType + MAPMODULE_COUNT - 1) % MAPMODULE_COUNT;
            if (pressed & R_JPAD) g_MapMakerModuleType = (g_MapMakerModuleType + 1) % MAPMODULE_COUNT;
            if (held & Z_TRIG)
            {
                if (g_MapMakerCursorX != g_MapMakerBuildDragX || g_MapMakerCursorY != g_MapMakerBuildDragY
                        || g_MapMakerCursorZ != g_MapMakerBuildDragZ || g_MapMakerModuleType != g_MapMakerBuildDragType
                        || g_MapMakerRotation != g_MapMakerBuildDragRotation)
                {
                    mapmakerPlaceModule();
                    g_MapMakerBuildDragX = g_MapMakerCursorX; g_MapMakerBuildDragY = g_MapMakerCursorY; g_MapMakerBuildDragZ = g_MapMakerCursorZ;
                    g_MapMakerBuildDragType = g_MapMakerModuleType; g_MapMakerBuildDragRotation = g_MapMakerRotation;
                }
            }
            else
            {
                g_MapMakerBuildDragX = g_MapMakerBuildDragY = g_MapMakerBuildDragZ = 9999;
                g_MapMakerBuildDragType = g_MapMakerBuildDragRotation = -1;
            }
            if (held & B_BUTTON) mapmakerDeleteModule();
        }
        else if (g_MapMakerAdvancedTool == MAPADV_TOOL_TEXTURE)
        {
            if (held & R_TRIG)
            {
                if (pressed & L_JPAD) mapmakerChangeMaterialSlot(-1);
                if (pressed & R_JPAD) mapmakerChangeMaterialSlot(1);
            }
            else
            {
                if (pressed & L_JPAD) mapmakerChangeMaterialTexture(-1);
                if (pressed & R_JPAD) mapmakerChangeMaterialTexture(1);
                if (pressed & U_JPAD) mapmakerChangeMaterialTexture(32);
                if (pressed & D_JPAD) mapmakerChangeMaterialTexture(-32);
            }
            mapmakerUpdateSurfaceSelection();
            if (held & Z_TRIG) mapmakerPaintSelectedSurface(0);
            if (pressed & B_BUTTON) mapmakerPaintSelectedSurface(1);
        }
        else if (g_MapMakerAdvancedTool == MAPADV_TOOL_ENTITY)
        {
            if (pressed & L_JPAD) { if (held & R_TRIG) mapmakerChangeEntityObjectId(-1); else mapmakerChangeEntityType(-1); }
            if (pressed & R_JPAD) { if (held & R_TRIG) mapmakerChangeEntityObjectId(1); else mapmakerChangeEntityType(1); }
            if (pressed & Z_TRIG) mapmakerPlaceEntity();
            if (held & B_BUTTON) mapmakerDeleteNearestEntity();
        }
        else if (g_MapMakerAdvancedTool == MAPADV_TOOL_PICKUP)
        {
            if (pressed & L_JPAD) { if (held & R_TRIG) mapmakerChangePickupSubtype(-1); else mapmakerChangePickupType(-1); }
            if (pressed & R_JPAD) { if (held & R_TRIG) mapmakerChangePickupSubtype(1); else mapmakerChangePickupType(1); }
            if (held & R_TRIG)
            {
                if (pressed & U_JPAD) mapmakerChangePickupQuantity(5);
                if (pressed & D_JPAD) mapmakerChangePickupQuantity(-5);
            }
            if (pressed & Z_TRIG) mapmakerPlaceEntity();
            if (held & B_BUTTON) mapmakerDeleteNearestEntity();
        }
        else if (g_MapMakerAdvancedTool == MAPADV_TOOL_PLAYER_START)
        {
            if (pressed & L_JPAD) { g_MapMakerPlayerStartSlot--; if (g_MapMakerPlayerStartSlot < 0) g_MapMakerPlayerStartSlot = MAPMAKER_MAX_PLAYER_STARTS - 1; }
            if (pressed & R_JPAD) { g_MapMakerPlayerStartSlot++; if (g_MapMakerPlayerStartSlot >= MAPMAKER_MAX_PLAYER_STARTS) g_MapMakerPlayerStartSlot = 0; }
            if (pressed & Z_TRIG) mapmakerPlacePlayerStart();
            if (held & B_BUTTON) mapmakerDeleteNearestPlayerStart();
        }

        /* B is continuous erase for Advanced geometry too.  Re-acquire the
         * item under the reticle after each delete so dragging the view across
         * several objects does not require repeated taps. */
        if ((held & B_BUTTON) && g_MapMakerAdvancedTool <= MAPADV_TOOL_DEPTH)
        {
            if (g_MapMakerAdvancedDraggingVertex) mapmakerAdvancedCancelDrag();
            else
            {
                mapmakerAdvancedUpdateSelection();
                if (g_MapMakerAdvancedSelectedMesh >= 0) mapmakerAdvancedDeleteSelectedMesh();
            }
        }
        return 0;
    }

    /* Fast authoring switch: A always advances the Basic tool.  Tool-specific
     * value selection stays on D-pad/trigger combinations so changing tools
     * never requires opening START. */
    if (pressed & A_BUTTON)
    {
        g_MapMakerTool = (g_MapMakerTool + 1) % MAPTOOL_COUNT;
        sprintf(g_MapMakerStorageStatus, "Tool: %s", mapmakerBasicGetToolName());
        pressed &= ~A_BUTTON;
    }

    /* D-pad remains editor tooling even when Free Fly is enabled.  Each tool
     * owns the controls which are most useful while authoring it.  In
     * particular Texture mode now browses the retail texture table directly,
     * so the top-right preview is a real visual picker rather than a passive
     * display of a value changed only through START. */
    if (g_MapMakerTool == MAPTOOL_TEXTURE)
    {
        if (held & R_TRIG)
        {
            if (pressed & L_JPAD) mapmakerChangeMaterialSlot(-1);
            if (pressed & R_JPAD) mapmakerChangeMaterialSlot(1);
        }
        else
        {
            if (pressed & L_JPAD) mapmakerChangeMaterialTexture(-1);
            if (pressed & R_JPAD) mapmakerChangeMaterialTexture(1);
            if (pressed & U_JPAD) mapmakerChangeMaterialTexture(32);
            if (pressed & D_JPAD) mapmakerChangeMaterialTexture(-32);
        }
    }
    else if (g_MapMakerTool == MAPTOOL_ENTITY)
    {
        if (pressed & L_JPAD) { if (held & R_TRIG) mapmakerChangeEntityObjectId(-1); else mapmakerChangeEntityType(-1); }
        if (pressed & R_JPAD) { if (held & R_TRIG) mapmakerChangeEntityObjectId(1); else mapmakerChangeEntityType(1); }
        if ((pressed & U_JPAD) && g_MapMakerCursorY < MAPMAKER_MAX_LAYER) g_MapMakerCursorY++;
        if ((pressed & D_JPAD) && g_MapMakerCursorY > MAPMAKER_MIN_LAYER) g_MapMakerCursorY--;
    }
    else if (g_MapMakerTool == MAPTOOL_PICKUP)
    {
        if (pressed & L_JPAD) { if (held & R_TRIG) mapmakerChangePickupSubtype(-1); else mapmakerChangePickupType(-1); }
        if (pressed & R_JPAD) { if (held & R_TRIG) mapmakerChangePickupSubtype(1); else mapmakerChangePickupType(1); }

        if (held & R_TRIG)
        {
            if (pressed & U_JPAD) mapmakerChangePickupQuantity(5);
            if (pressed & D_JPAD) mapmakerChangePickupQuantity(-5);
        }
        else
        {
            if ((pressed & U_JPAD) && g_MapMakerCursorY < MAPMAKER_MAX_LAYER) g_MapMakerCursorY++;
            if ((pressed & D_JPAD) && g_MapMakerCursorY > MAPMAKER_MIN_LAYER) g_MapMakerCursorY--;
        }
    }
    else if (g_MapMakerTool == MAPTOOL_PLAYER_START)
    {
        if (pressed & L_JPAD)
        {
            g_MapMakerPlayerStartSlot--;
            if (g_MapMakerPlayerStartSlot < 0) g_MapMakerPlayerStartSlot = MAPMAKER_MAX_PLAYER_STARTS - 1;
        }
        if (pressed & R_JPAD)
        {
            g_MapMakerPlayerStartSlot++;
            if (g_MapMakerPlayerStartSlot >= MAPMAKER_MAX_PLAYER_STARTS) g_MapMakerPlayerStartSlot = 0;
        }
        if ((pressed & U_JPAD) && g_MapMakerCursorY < MAPMAKER_MAX_LAYER) g_MapMakerCursorY++;
        if ((pressed & D_JPAD) && g_MapMakerCursorY > MAPMAKER_MIN_LAYER) g_MapMakerCursorY--;
    }
    else
    {
        if (pressed & L_JPAD)
            g_MapMakerModuleType = (g_MapMakerModuleType + MAPMODULE_COUNT - 1) % MAPMODULE_COUNT;
        if (pressed & R_JPAD)
            g_MapMakerModuleType = (g_MapMakerModuleType + 1) % MAPMODULE_COUNT;
        if ((pressed & U_JPAD) && g_MapMakerCursorY < MAPMAKER_MAX_LAYER) g_MapMakerCursorY++;
        if ((pressed & D_JPAD) && g_MapMakerCursorY > MAPMAKER_MIN_LAYER) g_MapMakerCursorY--;
    }

    if (g_MapMakerFreeView)
    {
        if (pressed & L_TRIG)
        {
            if (g_MapMakerTool == MAPTOOL_TEXTURE) mapmakerRotateSelectedTexture();
            else g_MapMakerRotation = (g_MapMakerRotation + 1) & 3;
        }
        mapmakerTickFreeView();
    }
    else
    {
        if (pressed & L_TRIG)
        {
            if (g_MapMakerTool == MAPTOOL_TEXTURE) mapmakerRotateSelectedTexture();
            else g_MapMakerRotation = (g_MapMakerRotation + 1) & 3;
        }

        if (held & L_CBUTTONS) g_MapMakerCameraYaw += 0.035f * g_ClockTimer;
        if (held & R_CBUTTONS) g_MapMakerCameraYaw -= 0.035f * g_ClockTimer;
        if (held & U_CBUTTONS) g_MapMakerCameraPitch += 0.025f * g_ClockTimer;
        if (held & D_CBUTTONS) g_MapMakerCameraPitch -= 0.025f * g_ClockTimer;
        if (g_MapMakerCameraPitch < 0.18f) g_MapMakerCameraPitch = 0.18f;
        if (g_MapMakerCameraPitch > 1.15f) g_MapMakerCameraPitch = 1.15f;

        if (g_MapMakerMoveRepeat > 0)
            g_MapMakerMoveRepeat -= g_ClockTimer;

        if (g_MapMakerMoveRepeat <= 0
                && (stickx > MM_STICK_THRESHOLD || stickx < -MM_STICK_THRESHOLD
                    || sticky > MM_STICK_THRESHOLD || sticky < -MM_STICK_THRESHOLD))
        {
            f32 sy = sinf(g_MapMakerCameraYaw);
            f32 cy = cosf(g_MapMakerCameraYaw);
            s32 movex = 0;
            s32 movez = 0;

            if (stickx > MM_STICK_THRESHOLD || stickx < -MM_STICK_THRESHOLD)
            {
                s32 sign = stickx > 0 ? 1 : -1;
                if (cy > 0.707f) movex = sign;
                else if (cy < -0.707f) movex = -sign;
                else if (sy > 0.0f) movez = -sign;
                else movez = sign;
            }
            else
            {
                s32 sign = sticky > 0 ? 1 : -1;
                if (sy > 0.707f) movex = -sign;
                else if (sy < -0.707f) movex = sign;
                else if (cy > 0.0f) movez = -sign;
                else movez = sign;
            }

            g_MapMakerCursorX += movex;
            g_MapMakerCursorZ += movez;
            if (g_MapMakerCursorX < MM_GRID_MIN) g_MapMakerCursorX = MM_GRID_MIN;
            if (g_MapMakerCursorX > MM_GRID_MAX) g_MapMakerCursorX = MM_GRID_MAX;
            if (g_MapMakerCursorZ < MM_GRID_MIN) g_MapMakerCursorZ = MM_GRID_MIN;
            if (g_MapMakerCursorZ > MM_GRID_MAX) g_MapMakerCursorZ = MM_GRID_MAX;
            g_MapMakerMoveRepeat = MM_MOVE_REPEAT;
        }
    }

    mapmakerUpdateSurfaceSelection();

    if (g_MapMakerTool == MAPTOOL_TEXTURE)
    {
        /* Z is paint-and-drag: keep applying the current material while the
         * cursor moves across neighbouring surfaces, but do not rewrite the
         * same face every frame. */
        if (held & Z_TRIG)
        {
            if (g_MapMakerSelectedModule >= 0 && g_MapMakerSelectedSurface >= 0
                    && (g_MapMakerSelectedModule != g_MapMakerPaintDragModule
                        || g_MapMakerSelectedSurface != g_MapMakerPaintDragSurface))
            {
                mapmakerPaintSelectedSurface(0);
                g_MapMakerPaintDragModule = g_MapMakerSelectedModule;
                g_MapMakerPaintDragSurface = g_MapMakerSelectedSurface;
            }
        }
        else
        {
            g_MapMakerPaintDragModule = -1;
            g_MapMakerPaintDragSurface = -1;
        }
        if (pressed & B_BUTTON) mapmakerPaintSelectedSurface(1);
    }
    else if (g_MapMakerTool == MAPTOOL_BUILD)
    {
        if (held & Z_TRIG)
        {
            if (g_MapMakerCursorX != g_MapMakerBuildDragX || g_MapMakerCursorY != g_MapMakerBuildDragY
                    || g_MapMakerCursorZ != g_MapMakerBuildDragZ || g_MapMakerModuleType != g_MapMakerBuildDragType
                    || g_MapMakerRotation != g_MapMakerBuildDragRotation)
            {
                mapmakerPlaceModule();
                g_MapMakerBuildDragX = g_MapMakerCursorX; g_MapMakerBuildDragY = g_MapMakerCursorY; g_MapMakerBuildDragZ = g_MapMakerCursorZ;
                g_MapMakerBuildDragType = g_MapMakerModuleType; g_MapMakerBuildDragRotation = g_MapMakerRotation;
            }
        }
        else
        {
            g_MapMakerBuildDragX = g_MapMakerBuildDragY = g_MapMakerBuildDragZ = 9999;
            g_MapMakerBuildDragType = g_MapMakerBuildDragRotation = -1;
        }
        if (pressed & B_BUTTON) mapmakerDeleteModule();
    }
    else if (g_MapMakerTool == MAPTOOL_ENTITY || g_MapMakerTool == MAPTOOL_PICKUP)
    {
        if (pressed & Z_TRIG) mapmakerPlaceEntity();
        if (pressed & B_BUTTON) mapmakerDeleteNearestEntity();
    }
    else if (g_MapMakerTool == MAPTOOL_PLAYER_START)
    {
        if (pressed & Z_TRIG) mapmakerPlacePlayerStart();
        if (pressed & B_BUTTON) mapmakerDeleteNearestPlayerStart();
    }

    return 0;
}

Gfx *mapmakerBasicRender(Gfx *gdl)
{
    Mtx *projection;
    Mtx *view;
    u16 perspNorm;
    f32 camx;
    f32 camy;
    f32 camz;
    f32 dirx;
    f32 diry;
    f32 dirz;
    f32 targetx;
    f32 targety;
    f32 targetz;
    s32 i;

    if (g_MapMakerTexturePoolRecyclePending && !g_MapMakerMenuOpen)
    {
        g_MapMakerTexturePoolRecyclePending = 0;
        mapmakerResetPrivateTexturePool();
    }

    projection = dynAllocateMatrix();
    view = dynAllocateMatrix();

    mapmakerGetCamera(&camx, &camy, &camz, &dirx, &diry, &dirz);
    targetx = camx + dirx * 200.0f;
    targety = camy + diry * 200.0f;
    targetz = camz + dirz * 200.0f;

    guPerspective(projection, &perspNorm, g_MapMakerPlaytest ? 60.0f : 55.0f,
            440.0f / 330.0f, 10.0f, 32000.0f, 1.0f);
    {
        f32 upx = 0.0f;
        f32 upy = 1.0f;
        f32 upz = 0.0f;

        /* World-up becomes parallel to the view vector at exactly +/-90 deg.
         * Give guLookAt a stable horizontal up vector at the poles so the
         * editor can genuinely look straight up/down instead of clamping a
         * few degrees short. */
        if (diry > 0.9999f || diry < -0.9999f)
        {
            f32 sign = diry >= 0.0f ? 1.0f : -1.0f;
            upx = -sinf(g_MapMakerCameraYaw) * sign;
            upy = 0.0f;
            upz = -cosf(g_MapMakerCameraYaw) * sign;
        }
        guLookAt(view, camx, camy, camz, targetx, targety, targetz, upx, upy, upz);
    }

    gDPPipeSync(gdl++);
    gDPSetCycleType(gdl++, G_CYC_1CYCLE);
    gDPSetCombineMode(gdl++, G_CC_SHADE, G_CC_SHADE);
    gSPClearGeometryMode(gdl++, G_LIGHTING | G_TEXTURE_GEN | G_TEXTURE_GEN_LINEAR | G_CULL_BOTH | G_FOG);
    if (g_MapMakerEditorFog)
    {
        /* Optional editor-only black distance fog masks the finite world/grid
         * submission horizon without changing authored map/environment data. */
        gDPSetFogColor(gdl++, 0, 0, 0, 255);
        gSPFogPosition(gdl++, 850, 1000);
        gDPSetRenderMode(gdl++, G_RM_FOG_SHADE_A, G_RM_AA_ZB_OPA_SURF2);
        gSPSetGeometryMode(gdl++, G_ZBUFFER | G_SHADE | G_SHADING_SMOOTH | G_FOG);
    }
    else
    {
        gDPSetRenderMode(gdl++, G_RM_AA_ZB_OPA_SURF, G_RM_AA_ZB_OPA_SURF2);
        gSPSetGeometryMode(gdl++, G_ZBUFFER | G_SHADE | G_SHADING_SMOOTH);
    }
    gSPPerspNormalize(gdl++, perspNorm);
    gSPMatrix(gdl++, osVirtualToPhysical(projection), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_PROJECTION);
    gSPMatrix(gdl++, osVirtualToPhysical(view), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);

    if (!g_MapMakerPlaytest)
        gdl = mapmakerDrawGrid(gdl);

    /* Draw by visibility priority rather than source-array order.  The old
     * single pass made high-index modules disappear first when the dynamic
     * VTX/GFX budget became tight.  Three cheap bucket passes keep nearby and
     * forward-facing geometry stable before spending budget on distant work. */
    {
        s32 pass;
        s32 outofbudget = 0;
        for (pass = 0; pass < 3 && !outofbudget; pass++)
        {
            for (i = 0; i < g_MapMakerModuleCount; i++)
            {
                f32 mx = g_MapMakerModules[i].grid_x * MM_CELL_SIZE - camx;
                f32 mz = g_MapMakerModules[i].grid_z * MM_CELL_SIZE - camz;
                f32 dist2 = mx * mx + mz * mz;
                f32 forward = mx * dirx + mz * dirz;
                s32 bucket;
                if (dist2 > MM_EDITOR_DRAW_RADIUS * MM_EDITOR_DRAW_RADIUS) continue;
                if (dist2 > 1200.0f * 1200.0f && forward < -480.0f) continue;
                if (dist2 <= 1800.0f * 1800.0f) bucket = 0;
                else if (dist2 <= 4800.0f * 4800.0f && forward > -160.0f) bucket = 1;
                else bucket = 2;
                if (bucket != pass) continue;
                if (dynGetFreeVtx() < MM_RENDER_VTX_RESERVE || dynGetFreeGfx(gdl) < MM_RENDER_GFX_RESERVE)
                {
                    outofbudget = 1;
                    break;
                }
                gdl = mapmakerDrawModule(gdl, &g_MapMakerModules[i]);
            }
        }
    }

    if (g_MapMakerEditorMode == MAPMAKER_EDITOR_ADVANCED)
    {
        for (i = 0; i < g_MapMakerAdvancedMeshCount; i++)
        {
            if (dynGetFreeVtx() < MM_RENDER_VTX_RESERVE || dynGetFreeGfx(gdl) < MM_RENDER_GFX_RESERVE) break;
            gdl = mapmakerDrawAdvancedMesh(gdl, &g_MapMakerAdvancedMeshes[i]);
        }
        for (i = 0; i < g_MapMakerAdvancedPortalCount; i++)
        {
            if (dynGetFreeVtx() < 2048 || dynGetFreeGfx(gdl) < 128) break;
            gdl = mapmakerDrawAdvancedPortal(gdl, &g_MapMakerAdvancedPortals[i]);
        }
    }

    if (!g_MapMakerPlaytest && g_MapMakerEditorMode == MAPMAKER_EDITOR_ADVANCED
            && g_MapMakerAdvancedTool == MAPADV_TOOL_VERTEX)
        gdl = mapmakerDrawAdvancedHandles(gdl, dirx, diry, dirz);

    if (!g_MapMakerPlaytest)
    {
        for (i = 0; i < g_MapMakerEntityCount; i++)
        {
            f32 dx = g_MapMakerEntities[i].x - camx;
            f32 dz = g_MapMakerEntities[i].z - camz;
            if (dx * dx + dz * dz > MM_MARKER_DRAW_RADIUS * MM_MARKER_DRAW_RADIUS) continue;
            if (dynGetFreeVtx() < 4096 || dynGetFreeGfx(gdl) < 256) break;
            gdl = mapmakerDrawEntityMarker(gdl, &g_MapMakerEntities[i], 0);
        }
        for (i = 0; i < g_MapMakerPlayerStartCount; i++)
        {
            f32 dx = g_MapMakerPlayerStarts[i].x - camx;
            f32 dz = g_MapMakerPlayerStarts[i].z - camz;
            if (dx * dx + dz * dz > MM_MARKER_DRAW_RADIUS * MM_MARKER_DRAW_RADIUS) continue;
            if (dynGetFreeVtx() < 4096 || dynGetFreeGfx(gdl) < 256) break;
            gdl = mapmakerDrawPlayerStartMarker(gdl, &g_MapMakerPlayerStarts[i], 0);
        }
        if (g_MapMakerTool == MAPTOOL_ENTITY || g_MapMakerTool == MAPTOOL_PICKUP)
        {
            MapMakerEntityInstance cursorEntity;
            memset(&cursorEntity, 0, sizeof(cursorEntity));
            cursorEntity.x = (s16)g_MapMakerEntityCursorX;
            cursorEntity.y = (s16)g_MapMakerEntityCursorY;
            cursorEntity.z = (s16)g_MapMakerEntityCursorZ;
            cursorEntity.rotation = (u8)g_MapMakerRotation;
            if (g_MapMakerTool == MAPTOOL_PICKUP)
            {
                cursorEntity.type = (u8)mapmakerPickupSetupType(g_MapMakerPickupType);
                cursorEntity.subtype = (s16)g_MapMakerPickupSubtype;
                cursorEntity.quantity = (u16)g_MapMakerPickupQuantity;
            }
            else
            {
                cursorEntity.type = (u8)g_MapMakerEntityType;
                cursorEntity.object_id = (u16)g_MapMakerEntityObjectId;
                cursorEntity.aux_id = (u16)g_MapMakerEntityAuxId;
            }
            gdl = mapmakerDrawEntityMarker(gdl, &cursorEntity, 1);
        }
        else if (g_MapMakerTool == MAPTOOL_BUILD) gdl = mapmakerDrawCursor(gdl);
        else if (g_MapMakerTool == MAPTOOL_PLAYER_START)
        {
            MapMakerPlayerStart cursorStart;
            cursorStart.x = (s16)g_MapMakerEntityCursorX;
            cursorStart.y = (s16)g_MapMakerEntityCursorY;
            cursorStart.z = (s16)g_MapMakerEntityCursorZ;
            cursorStart.rotation = (u8)g_MapMakerRotation;
            cursorStart.slot = (u8)g_MapMakerPlayerStartSlot;
            gdl = mapmakerDrawPlayerStartMarker(gdl, &cursorStart, 1);
        }
    }

    gDPPipeSync(gdl++);
    return gdl;
}

#ifdef GE_MODDED_CHEATS
/* Keep the editor/save data canonical.  Native Test maps canonical authored
 * X into the reflected gameplay world only at the runtime boundary.  This
 * means live On/Off toggles need no destructive rewrite of Map Maker data. */
static f32 mapmakerNativeEffectiveOriginX(void)
{
    return mirrorLevelsIsEnabled() ? -g_MapMakerNativeOriginX : g_MapMakerNativeOriginX;
}

static f32 mapmakerNativeLocalXToWorld(f32 x)
{
    if (mirrorLevelsIsEnabled())
        x = -x;
    return x + mapmakerNativeEffectiveOriginX();
}

static f32 mapmakerNativeWorldXToLocal(f32 x)
{
    x -= mapmakerNativeEffectiveOriginX();
    if (mirrorLevelsIsEnabled())
        x = -x;
    return x;
}

static s32 mapmakerNativeMirrorRotation(s32 rotation)
{
    if (mirrorLevelsIsEnabled())
        return (4 - (rotation & 3)) & 3;
    return rotation & 3;
}
#else
#define mapmakerNativeEffectiveOriginX() (g_MapMakerNativeOriginX)
#define mapmakerNativeLocalXToWorld(x) ((x) + g_MapMakerNativeOriginX)
#define mapmakerNativeWorldXToLocal(x) ((x) - g_MapMakerNativeOriginX)
#define mapmakerNativeMirrorRotation(rotation) ((rotation) & 3)
#endif

/* -------------------------------------------------------------------------
 * Native Basic-map test mode
 * ------------------------------------------------------------------------- */

s32 mapmakerNativeTestRequested(void)
{
    return g_MapMakerNativeTestPending;
}

s32 mapmakerNativeTestActive(void)
{
    /* Do not take over rendering/collision until the serialized editor map has
     * survived the dedicated stage load AND has been anchored onto the private
     * bootstrap STAN.  A half-entered state must leave the bootstrap shell
     * visible rather than suppressing all world rendering. */
    return g_MapMakerNativeTestPending && g_MapMakerNativeTestEntered
        && g_MapMakerNativeTestRuntimeReady && g_MapMakerNativeOriginValid;
}

void mapmakerNativeTestMarkStageEntered(void)
{
    if (g_MapMakerNativeTestPending)
        g_MapMakerNativeTestEntered = 1;
}

void mapmakerNativeTestBeginReturn(void)
{
    if (!g_MapMakerNativeTestPending) return;
    /* Stop all authored-world hooks before the title/front-end stage starts.
     * The separate returning latch is what MENU_RUN_STAGE consumes. */
    g_MapMakerNativeReturning = 1;
    g_MapMakerNativeTestEntered = 0;
    g_MapMakerNativeTestRuntimeReady = 0;
    g_MapMakerNativeOriginValid = 0;
}

s32 mapmakerNativeTestShouldReturnToEditor(void)
{
    return g_MapMakerNativeTestPending && g_MapMakerNativeReturning;
}

void mapmakerNativeTestFinishReturn(void)
{
    g_MapMakerNativeTestPending = 0;
    g_MapMakerNativeTestEntered = 0;
    g_MapMakerNativeReturning = 0;
    g_MapMakerNativeTestRuntimeReady = 0;
    g_MapMakerNativeOriginValid = 0;
    g_MapMakerNativeOriginX = 0.0f;
    g_MapMakerNativeOriginY = 0.0f;
    g_MapMakerNativeOriginZ = 0.0f;
    g_MapMakerNativeModules = NULL;
    g_MapMakerNativeModuleCount = 0;
    g_MapMakerNativeStarts = NULL;
    g_MapMakerNativeStartCount = 0;
    /* The editor scratch survived the child stage; no Controller Pak/session
     * reconstruction is required. */
    g_MapMakerRestoreSessionOnInit = 0;
}

void mapmakerNativeTestStagePrepare(void)
{
    g_MapMakerNativeTestRuntimeReady = 0;
    g_MapMakerNativeOriginValid = 0;
    if (!g_MapMakerNativeTestPending || !g_MapMakerNativeTestEntered
            || g_MapMakerNativeReturning || !mapmakerScratchIsValid())
        return;
    if (g_MapMakerModuleCount < 0 || g_MapMakerModuleCount > MAPMAKER_MAX_MODULES
            || g_MapMakerPlayerStartCount <= 0 || g_MapMakerPlayerStartCount > MAPMAKER_MAX_PLAYER_STARTS)
        return;

    g_MapMakerNativeModuleCount = g_MapMakerModuleCount;
    g_MapMakerNativeStartCount = g_MapMakerPlayerStartCount;
    g_MapMakerNativeModules = g_MapMakerModules;
    g_MapMakerNativeStarts = g_MapMakerPlayerStarts;
    memset(g_MapMakerNativeVelY, 0, sizeof(g_MapMakerNativeVelY));
    g_MapMakerNativeTestRuntimeReady = 1;
    mapmakerPlaySelectedMusic();
}

void mapmakerNativeTestPlaceCurrentPlayer(s32 playernum)
{
    MapMakerPlayerStart *start = NULL;
    StandTile *anchor;
    f32 localfloor;
    f32 hostfloor;
    f32 angle;
    s32 i;

    if (!g_MapMakerNativeTestRuntimeReady || g_MapMakerNativeStartCount <= 0 || g_CurrentPlayer == NULL)
        return;

    for (i = 0; i < g_MapMakerNativeStartCount; i++)
    {
        if (g_MapMakerNativeStarts[i].slot == playernum)
        {
            start = &g_MapMakerNativeStarts[i];
            break;
        }
    }
    if (start == NULL) start = &g_MapMakerNativeStarts[playernum % g_MapMakerNativeStartCount];

    /* Keep GoldenEye's fully initialized dedicated bootstrap spawn as the
     * engine anchor.  The private setup owns Pad 0 -> p1a -> room 1, so the
     * normal intro/pad/STAN contract is valid before authored-world takeover.
     * Translate the authored map so its selected Player Start lands exactly on
     * that private bootstrap spawn; no retail map coordinates participate. */
    anchor = g_CurrentPlayer->field_488.current_tile_ptr;
    if (anchor == NULL)
        return;

    localfloor = start->y;
    mapmakerGetFloorHeightFrom(g_MapMakerNativeModules, g_MapMakerNativeModuleCount,
            start->x, start->z, start->y + MM_PLAYER_STEP, &localfloor);

    hostfloor = g_CurrentPlayer->field_70;
#ifdef GE_MODDED_CHEATS
    /* Store a canonical origin even when the bootstrap player is already in
     * mirrored world space.  Runtime helpers derive the effective reflected
     * origin from this value, so live cheat toggles stay reversible. */
    g_MapMakerNativeOriginX = (mirrorLevelsIsEnabled()
            ? -g_CurrentPlayer->field_488.collision_position.x
            :  g_CurrentPlayer->field_488.collision_position.x) - start->x;
#else
    g_MapMakerNativeOriginX = g_CurrentPlayer->field_488.collision_position.x - start->x;
#endif
    g_MapMakerNativeOriginY = hostfloor - localfloor;
    g_MapMakerNativeOriginZ = g_CurrentPlayer->field_488.collision_position.z - start->z;
    g_MapMakerNativeOriginValid = 1;

    /* Preserve the private bootstrap position, room and STAN.  Only facing
     * and the Map Maker-owned floor state are substituted. */
    g_CurrentPlayer->field_70 = localfloor + g_MapMakerNativeOriginY;
    g_CurrentPlayer->stanHeight = g_CurrentPlayer->field_70;
    g_CurrentPlayer->field_7C = 0.0f;
    angle = (start->rotation & 3) * (M_TAU_F * 0.25f);
#ifdef GE_MODDED_CHEATS
    if (mirrorLevelsIsEnabled())
        angle = -angle;
#endif
    g_CurrentPlayer->vv_theta = (angle * 360.0f) / M_TAU_F;
    g_CurrentPlayer->field_488.theta_transform.x = -sinf(angle);
    g_CurrentPlayer->field_488.theta_transform.y = 0.0f;
    g_CurrentPlayer->field_488.theta_transform.z = cosf(angle);
    g_CurrentPlayer->bondprevpos = g_CurrentPlayer->field_488.collision_position;
    g_CurrentPlayer->cameratile = anchor;

    /* The dedicated setup has no authored cinematic. Switch to gameplay only
     * after normal setup/intro/player initialization has completed. Loading is
     * left untouched; this remains a post-load presentation handoff. */
    camera_fade_active = 0;
    currentPlayerSetFadeColour(0, 0, 0, 0.0f);
    currentPlayerSetFadeFrac(0.0f, 0.0f);
    bondviewSetCameraMode(CAMERAMODE_FP);
    /* Native Test skips the ordinary intro presentation, so explicitly load
     * the dedicated stage's neutral gameplay environment. */
    fogLoadLevelEnvironment(lvlGetCurrentStageToLoad(), 0);
    currentPlayerSetFadeColour(0, 0, 0, 0.0f);
    currentPlayerSetFadeFrac(0.0f, 0.0f);

    if (playernum >= 0 && playernum < 4) g_MapMakerNativeVelY[playernum] = 0.0f;
}

s32 mapmakerNativeTryMove(f32 oldx, f32 oldz, f32 *newx, f32 *newz, f32 feety, f32 eyey)
{
    f32 localoldx;
    f32 localoldz;
    f32 localx;
    f32 localz;
    f32 localeyey;

    if (!mapmakerNativeTestActive()) return 0;

    localoldx = mapmakerNativeWorldXToLocal(oldx);
    localoldz = oldz - g_MapMakerNativeOriginZ;
    localx = mapmakerNativeWorldXToLocal(*newx);
    localz = *newz - g_MapMakerNativeOriginZ;
    localeyey = eyey - g_MapMakerNativeOriginY;

    if (mapmakerPlayerBlockedBy(g_MapMakerNativeModules, g_MapMakerNativeModuleCount,
            localx, localoldz, localeyey))
        localx = localoldx;
    if (mapmakerPlayerBlockedBy(g_MapMakerNativeModules, g_MapMakerNativeModuleCount,
            localx, localz, localeyey))
        localz = localoldz;

    (void)feety;
    *newx = mapmakerNativeLocalXToWorld(localx);
    *newz = localz + g_MapMakerNativeOriginZ;
    return 1;
}

void mapmakerNativeUpdatePlayerY(void)
{
    f32 localfloor;
    f32 localfeet;
    f32 localx;
    f32 localz;
    f32 newy;
    f32 newvel;
    f32 gravity;

    if (!mapmakerNativeTestActive() || g_CurrentPlayer == NULL) return;

    localx = mapmakerNativeWorldXToLocal(g_CurrentPlayer->field_488.collision_position.x);
    localz = g_CurrentPlayer->field_488.collision_position.z - g_MapMakerNativeOriginZ;
    localfeet = g_CurrentPlayer->field_70 - g_MapMakerNativeOriginY;
    localfloor = localfeet;

    /* Feed the authored Map Maker floor into GoldenEye's normal vertical
     * integration constants.  Native Test used to run a separate 2.2/tick
     * gravity model, which is why falling visibly differed from retail levels. */
    if (mapmakerGetFloorHeightFrom(g_MapMakerNativeModules, g_MapMakerNativeModuleCount,
            localx, localz, localfeet, &localfloor))
        g_CurrentPlayer->stanHeight = localfloor + g_MapMakerNativeOriginY;
    else
        g_CurrentPlayer->stanHeight = g_CurrentPlayer->field_70 - 32768.0f;

    if (g_CurrentPlayer->stanHeight >= g_CurrentPlayer->field_70)
    {
        g_CurrentPlayer->field_70 = g_CurrentPlayer->stanHeight;
        if (g_CurrentPlayer->field_7C < 0.0f) g_CurrentPlayer->field_7C = 0.0f;
        return;
    }

    newvel = g_CurrentPlayer->field_7C;
    newy = g_CurrentPlayer->field_70;
    gravity = 0.27777779f;
    {
        f32 nextvel = newvel - (g_GlobalTimerDelta * gravity);
        newy += g_GlobalTimerDelta * (newvel + nextvel) * 0.5f;
        newvel = nextvel;
    }

    if (newy < g_CurrentPlayer->stanHeight)
    {
        newy = g_CurrentPlayer->stanHeight;
        /* Keep the retail fall acceleration/landing point.  The presentation
         * bounce flags are owned by bondview2.c's version-specific constants
         * and are deliberately left to that subsystem. */
        newvel = 0.0f;
    }

    g_CurrentPlayer->field_70 = newy;
    g_CurrentPlayer->field_7C = newvel;
}

Gfx *mapmakerNativeRenderWorld(Gfx *gdl)
{
    s32 i;
    f32 camx;
    f32 camz;
    f32 dirx;
    f32 dirz;
    if (!mapmakerNativeTestActive()) return gdl;

    camx = g_CurrentPlayer->field_488.collision_position.x;
    camz = g_CurrentPlayer->field_488.collision_position.z;
    dirx = g_CurrentPlayer->field_488.theta_transform.x;
    dirz = g_CurrentPlayer->field_488.theta_transform.z;

    /* bgLevelRender normally restores the gameplay world matrices after the
     * sky pass. Native Test replaces bgLevelRender with this routine, so it
     * must perform that state handoff itself. Without it the authored world
     * inherits whichever modelview/projection the sky, watch or weapon path
     * happened to leave active. A melee animation happened to reload a usable
     * matrix, which is why the map previously appeared only while punching. */
    gdl = bgScissorCurrentPlayerViewDefault(fogRenderClearFogMode(gdl));
    gSPMatrix(gdl++, g_viProjectionMatrix, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_PROJECTION);
    gdl = bondviewGfxPlayerField5cMatrix(gdl);

    gDPPipeSync(gdl++);
    gDPSetCycleType(gdl++, G_CYC_1CYCLE);
    gDPSetRenderMode(gdl++, G_RM_AA_ZB_OPA_SURF, G_RM_AA_ZB_OPA_SURF2);
    gSPClearGeometryMode(gdl++, G_LIGHTING | G_TEXTURE_GEN | G_TEXTURE_GEN_LINEAR | G_CULL_BOTH);
    gSPSetGeometryMode(gdl++, G_ZBUFFER | G_SHADE | G_SHADING_SMOOTH);

    for (i = 0; i < g_MapMakerNativeModuleCount; i++)
    {
        f32 mx = mapmakerNativeLocalXToWorld(g_MapMakerNativeModules[i].grid_x * MM_CELL_SIZE) - camx;
        f32 mz = g_MapMakerNativeModules[i].grid_z * MM_CELL_SIZE + g_MapMakerNativeOriginZ - camz;
        f32 dist2 = mx * mx + mz * mz;
        f32 forward = mx * dirx + mz * dirz;

        if (dist2 > MM_NATIVE_DRAW_RADIUS * MM_NATIVE_DRAW_RADIUS) continue;
        if (dist2 > 1000.0f * 1000.0f && forward < -400.0f) continue;
        if (dynGetFreeVtx() < MM_RENDER_VTX_RESERVE || dynGetFreeGfx(gdl) < MM_RENDER_GFX_RESERVE) break;
        gdl = mapmakerDrawModuleOffset(gdl, &g_MapMakerNativeModules[i],
                mapmakerNativeEffectiveOriginX(), g_MapMakerNativeOriginY, g_MapMakerNativeOriginZ);
    }

    /* Leave a predictable world-space state for props/weapon effects which
     * render after the background pass. */
    gDPPipeSync(gdl++);
    return gdl;
}

const char *mapmakerBasicGetModuleName(void)
{
    switch (g_MapMakerModuleType)
    {
    case MAPMODULE_WALL: return "Wall";
    case MAPMODULE_CORNER: return "Corner";
    case MAPMODULE_DOORWAY: return "Doorway";
    case MAPMODULE_RAMP: return "Ramp";
    default: return "Floor";
    }
}

s32 mapmakerBasicGetModuleCount(void) { return g_MapMakerModuleCount; }
s32 mapmakerBasicGetCursorX(void) { return g_MapMakerCursorX; }
s32 mapmakerBasicGetCursorY(void) { return g_MapMakerCursorY; }
s32 mapmakerBasicGetCursorZ(void) { return g_MapMakerCursorZ; }
s32 mapmakerBasicGetRotationDegrees(void) { return (g_MapMakerRotation & 3) * 90; }
s32 mapmakerBasicIsMenuOpen(void) { return g_MapMakerMenuOpen; }
s32 mapmakerBasicGetMenuChoice(void) { return g_MapMakerMenuChoice; }
s32 mapmakerBasicGetMusicTrack(void) { return g_MapMakerMusicTrack; }
const char *mapmakerBasicGetMusicName(void)
{
    if (g_MapMakerMusicTrack == MM_MUSIC_RANDOM) return "Random";
    if (g_MapMakerMusicTrack < MM_MUSIC_FIRST || g_MapMakerMusicTrack > MM_MUSIC_LAST) return "Unknown";
    return g_MapMakerMusicNames[g_MapMakerMusicTrack];
}
s32 mapmakerBasicIsPlaytesting(void) { return g_MapMakerPlaytest; }
const char *mapmakerBasicGetStorageStatus(void) { return g_MapMakerStorageStatus; }
s32 mapmakerBasicGetTool(void) { return g_MapMakerTool; }
const char *mapmakerBasicGetToolName(void)
{
    if (g_MapMakerTool == MAPTOOL_TEXTURE) return "Texture";
    if (g_MapMakerTool == MAPTOOL_ENTITY) return "Entity";
    if (g_MapMakerTool == MAPTOOL_PICKUP) return "Pickup";
    if (g_MapMakerTool == MAPTOOL_PLAYER_START) return "Player Start";
    return "Build";
}
const char *mapmakerBasicGetPlacementModeName(void) { return g_MapMakerPlacementEdge ? "Edge" : "Center"; }
const char *mapmakerBasicGetTextureFlipName(void)
{
    static const char *names[4] = {"Normal", "Flip H", "Flip V", "Flip H+V"};
    return names[g_MapMakerTextureFlip & 3];
}
const char *mapmakerBasicGetEntityTypeName(void) { return mapmakerSetupTypeName(g_MapMakerEntityType); }
const char *mapmakerBasicGetEntityClassName(void) { return mapmakerSetupTypeClassName(g_MapMakerEntityType); }
s32 mapmakerBasicGetEntitySetupType(void) { return g_MapMakerEntityType; }
s32 mapmakerBasicGetEntityObjectId(void) { return g_MapMakerEntityObjectId; }
const char *mapmakerBasicGetEntityIdLabel(void)
{
    if (g_MapMakerEntityType == PROPDEF_GUARD) return "Body ID";
    if (mapmakerEntityTypeUsesPropId(g_MapMakerEntityType)) return "Prop ID";
    return "Primary ID";
}
const char *mapmakerBasicGetPickupTypeName(void) { return mapmakerPickupTypeName(g_MapMakerPickupType); }
const char *mapmakerBasicGetPickupSubtypeName(void)
{
    if (g_MapMakerPickupType == MAPPICKUP_WEAPON_ITEM)
    {
        if (g_MapMakerPickupSubtype >= ITEM_UNARMED && g_MapMakerPickupSubtype < ITEM_IDS_MAX)
        {
            const char *name = (const char *)get_ptr_short_watch_text_for_item((ITEM_IDS)g_MapMakerPickupSubtype);
            if (name != NULL && name[0]) return name;
        }
        return "Unknown Item";
    }
    if (g_MapMakerPickupType == MAPPICKUP_AMMO_MAGAZINE || g_MapMakerPickupType == MAPPICKUP_AMMO_CRATE)
    {
        if (g_MapMakerPickupSubtype >= 0 && g_MapMakerPickupSubtype < AMMOTYPE_MAX)
            return g_MapMakerAmmoTypeNames[g_MapMakerPickupSubtype];
        return "Unknown Ammo";
    }
    if (g_MapMakerPickupType == MAPPICKUP_KEY) return "Key ID";
    return "Armour";
}
s32 mapmakerBasicGetPickupSetupType(void) { return mapmakerPickupSetupType(g_MapMakerPickupType); }
s32 mapmakerBasicGetPickupSubtype(void) { return g_MapMakerPickupSubtype; }
s32 mapmakerBasicGetPickupQuantity(void) { return g_MapMakerPickupQuantity; }
s32 mapmakerBasicPickupHasQuantity(void)
{
    return g_MapMakerPickupType == MAPPICKUP_AMMO_CRATE || g_MapMakerPickupType == MAPPICKUP_ARMOUR;
}
s32 mapmakerBasicGetPlayerStartCount(void) { return g_MapMakerPlayerStartCount; }
s32 mapmakerBasicGetPlayerStartSlot(void) { return g_MapMakerPlayerStartSlot; }
s32 mapmakerBasicGetEntityCount(void) { return g_MapMakerEntityCount; }
s32 mapmakerBasicGetSnapEnabled(void) { return g_MapMakerSnapEnabled; }
s32 mapmakerBasicGetGridSize(void) { return g_MapMakerGridSize; }
s32 mapmakerBasicGetEditorFogEnabled(void) { return g_MapMakerEditorFog; }
s32 mapmakerAdvancedGetArbitraryRotationEnabled(void) { return g_MapMakerArbitraryRotation; }
s32 mapmakerBasicIsFreeView(void) { return g_MapMakerFreeView; }
const char *mapmakerBasicGetViewName(void) { return g_MapMakerFreeView ? "Free Fly" : "Orbit"; }
s32 mapmakerBasicGetMaterialSlot(void) { return g_MapMakerMaterialSlot; }
s32 mapmakerBasicGetMaterialTexture(void) { return g_MapMakerMaterialTexture[g_MapMakerMaterialSlot]; }
s32 mapmakerBasicGetMaterialPreview(struct sImageTableEntry *image)
{
    s32 width;
    s32 height;
    if (image == NULL) return 0;
    return mapmakerPrepareTexture(g_MapMakerMaterialTexture[g_MapMakerMaterialSlot], image, &width, &height);
}
s32 mapmakerBasicGetSelectedModule(void) { return g_MapMakerSelectedModule; }
const char *mapmakerBasicGetSelectedSurfaceName(void)
{
    switch (g_MapMakerSelectedSurface)
    {
    case MAPSURFACE_TOP: return "Top";
    case MAPSURFACE_BOTTOM: return "Bottom";
    case MAPSURFACE_NORTH: return "North";
    case MAPSURFACE_SOUTH: return "South";
    case MAPSURFACE_WEST: return "West";
    case MAPSURFACE_EAST: return "East";
    default: return "None";
    }
}
const char *mapmakerBasicGetControlStyleName(void)
{
    switch (mapmakerGetConfiguredControlType())
    {
    case CONTROLLER_CONFIG_SOLITARE: return "1.2 Solitaire";
    case CONTROLLER_CONFIG_KISSY: return "1.3 Kissy";
    case CONTROLLER_CONFIG_GOODNIGHT: return "1.4 Goodnight";
    case CONTROLLER_CONFIG_PLENTY: return "2.1 Plenty";
    case CONTROLLER_CONFIG_GALORE: return "2.2 Galore";
    case CONTROLLER_CONFIG_DOMINO: return "2.3 Domino";
    case CONTROLLER_CONFIG_GOODHEAD: return "2.4 Goodhead";
    default: return "1.1 Honey";
    }
}

const char *mapmakerBasicGetMapName(void)
{
    return (g_MapMakerInitialized && g_MapMakerCurrentName[0]) ? g_MapMakerCurrentName : "Untitled";
}

s32 mapmakerBasicHasResidentMap(void)
{
    return g_MapMakerInitialized;
}

s32 mapmakerBasicLoadFromControllerPak(void)
{
    return mapmakerLoadMapInternal(0);
}

void mapmakerBasicRestoreFrontendResources(void)
{
    if (g_MapMakerFrontendPoolCaptured)
    {
        memcpy((struct texpool *)&ptr_texture_alloc_start,
                &g_MapMakerSavedDefaultTexturePool,
                sizeof(g_MapMakerSavedDefaultTexturePool));
        g_MapMakerFrontendPoolCaptured = 0;
    }
    g_TexCacheCount = 0;
}

#endif
