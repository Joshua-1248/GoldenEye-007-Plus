#ifndef _IN_GAME_MAPMAKER_H_
#define _IN_GAME_MAPMAKER_H_

#include <ultra64.h>

struct sImageTableEntry;

#ifdef GE_MAP_MAKER

#define GEMAP_MAGIC 0x47454d50u /* 'GEMP' */
#define GEMAP_VERSION 8
#define MAPMAKER_MAX_MODULES 2048
#define MAPMAKER_MIN_LAYER (-64)
#define MAPMAKER_MAX_LAYER 64
#define MAPMAKER_MATERIAL_SLOTS 16
#define MAPMAKER_SURFACE_COUNT 6
#define MAPMAKER_MATERIAL_NONE 0xff
#define MAPMAKER_TEXTURE_NONE 0xffff
#define MAPMAKER_ADV_MAX_MESHES 512
#define MAPMAKER_ADV_MAX_VERTICES 4096
#define MAPMAKER_ADV_MAX_LOOPS 1024
#define MAPMAKER_ADV_MAX_POINTS_PER_LOOP 16
#define MAPMAKER_ADV_MAX_PORTALS 512


typedef enum MapMakerEditorMode {
    MAPMAKER_EDITOR_BASIC = 0,
    MAPMAKER_EDITOR_ADVANCED
} MapMakerEditorMode;

typedef enum MapMakerAdvancedMeshType {
    /* Every new Advanced object begins as a paper-thin surface unless the
     * selected primitive is intrinsically closed.  MAPADV_MESH_PLANE keeps
     * the old Floor slot value so pre-v8 resident state is harmless. */
    MAPADV_MESH_PLANE = 0,
    MAPADV_MESH_CUBE,
    MAPADV_MESH_CIRCLE,
    MAPADV_MESH_UV_SPHERE,
    MAPADV_MESH_ICO_SPHERE,
    MAPADV_MESH_CYLINDER,
    MAPADV_MESH_CONE,
    MAPADV_MESH_TORUS,
    MAPADV_MESH_TYPE_COUNT
} MapMakerAdvancedMeshType;

typedef enum MapMakerAdvancedTool {
    MAPADV_TOOL_CREATE = 0,
    MAPADV_TOOL_MOVE,
    MAPADV_TOOL_ROTATE,
    MAPADV_TOOL_SCALE,
    MAPADV_TOOL_VERTEX,
    MAPADV_TOOL_ROOM,
    MAPADV_TOOL_PORTAL,
    MAPADV_TOOL_DEPTH,
    /* Advanced is a superset of Basic: these tools operate on the normal
     * module/entity representation without leaving Advanced mode. */
    MAPADV_TOOL_BUILD,
    MAPADV_TOOL_TEXTURE,
    MAPADV_TOOL_ENTITY,
    MAPADV_TOOL_PICKUP,
    MAPADV_TOOL_PLAYER_START,
    MAPADV_TOOL_COUNT
} MapMakerAdvancedTool;

typedef enum MapMakerAdvancedAxis {
    MAPADV_AXIS_X = 0,
    MAPADV_AXIS_Y,
    MAPADV_AXIS_Z,
    MAPADV_AXIS_ALL,
    MAPADV_AXIS_COUNT
} MapMakerAdvancedAxis;

typedef struct MapMakerAdvancedVertex {
    s16 x;
    s16 y;
    s16 z;
} MapMakerAdvancedVertex;

typedef struct MapMakerAdvancedLoop {
    u16 first_vertex;
    u8 vertex_count;
    u8 flags; /* bit 0 = hole; outer loop is always first */
} MapMakerAdvancedLoop;

typedef struct MapMakerAdvancedMesh {
    u16 first_loop;
    u8 loop_count;
    u8 type;
    u8 material;
    u8 room;
    u16 flags;
} MapMakerAdvancedMesh;

typedef struct MapMakerAdvancedPortal {
    s16 x;
    s16 y;
    s16 z;
    s16 half_width;
    s16 half_height;
    u8 axis;      /* 0 = plane normal X, 1 = plane normal Z */
    u8 room_a;
    u8 room_b;
    u8 flags;
} MapMakerAdvancedPortal;

typedef enum MapMakerModuleType {
    MAPMODULE_FLOOR = 0,
    MAPMODULE_WALL,
    MAPMODULE_CORNER,
    MAPMODULE_DOORWAY,
    MAPMODULE_RAMP,
    MAPMODULE_COUNT
} MapMakerModuleType;

typedef enum MapMakerSurface {
    MAPSURFACE_TOP = 0,
    MAPSURFACE_BOTTOM,
    MAPSURFACE_NORTH,
    MAPSURFACE_SOUTH,
    MAPSURFACE_WEST,
    MAPSURFACE_EAST
} MapMakerSurface;

typedef enum MapMakerTool {
    MAPTOOL_BUILD = 0,
    MAPTOOL_TEXTURE,
    MAPTOOL_ENTITY,
    MAPTOOL_PICKUP,
    MAPTOOL_PLAYER_START,
    MAPTOOL_COUNT
} MapMakerTool;

typedef enum MapMakerPickupType {
    MAPPICKUP_WEAPON_ITEM = 0,
    MAPPICKUP_AMMO_MAGAZINE,
    MAPPICKUP_AMMO_CRATE,
    MAPPICKUP_KEY,
    MAPPICKUP_ARMOUR,
    MAPPICKUP_TYPE_COUNT
} MapMakerPickupType;

#define MAPMAKER_MAX_ENTITIES 256
#define MAPMAKER_MAX_PLAYER_STARTS 16

typedef struct MapMakerPlayerStart {
    s16 x;
    s16 y;
    s16 z;
    u8 rotation;
    u8 slot;
} MapMakerPlayerStart;

typedef struct MapMakerEntityInstance {
    s16 x;
    s16 y;
    s16 z;
    /* GoldenEye setup PROPDEF_TYPE (00..2F).  Pickups use the same placement
     * record as ordinary entities; only their subtype/quantity interpretation
     * differs. */
    u8 type;
    u8 rotation;
    /* Primary setup/model/body/tag ID.  Meaning depends on type. */
    u16 object_id;
    /* ITEM_IDS, AMMOTYPE, key ID, or other type-specific subtype. */
    s16 subtype;
    /* Ammo amount / armour percentage / future type-specific amount. */
    u16 quantity;
    /* Raw setup/editor flags reserved for later Setup compiler passes. */
    u16 flags;
    /* Secondary ID for setup records which link two things. */
    u16 aux_id;
} MapMakerEntityInstance;

#define MAPMODULE_FLAG_EDGE_ANCHOR 0x1000u

typedef struct MapMakerModuleInstance {
    s16 grid_x;
    s16 grid_y;
    s16 grid_z;
    u8 type;
    u8 rotation;
    u16 flags;
    /* Basic-map surfaces keep the exact retail texture ID that was painted.
     * Browsing the material picker must never mutate already-painted faces. */
    u16 surface_texture[MAPMAKER_SURFACE_COUNT];
} MapMakerModuleInstance;

typedef struct GeMapHeader {
    u32 magic;
    u16 version;
    u16 header_size;
    u32 flags;
    u16 module_count;
    u16 object_count;
    u32 payload_size;
    u32 payload_crc;
    char name[24];
} GeMapHeader;

void mapmakerSetEditorMode(s32 mode);
s32 mapmakerGetEditorMode(void);
const char *mapmakerGetEditorModeName(void);
const char *mapmakerAdvancedGetMeshTypeName(void);
s32 mapmakerAdvancedGetMeshCount(void);
s32 mapmakerAdvancedGetSelectedVertex(void);
s32 mapmakerAdvancedIsDraggingVertex(void);
const char *mapmakerAdvancedGetToolName(void);
const char *mapmakerAdvancedGetAxisName(void);
s32 mapmakerAdvancedGetPortalCount(void);
s32 mapmakerAdvancedGetRoom(void);

void mapmakerBasicInit(void);
s32 mapmakerBasicTick(void);
Gfx *mapmakerBasicRender(Gfx *gdl);
const char *mapmakerBasicGetModuleName(void);
s32 mapmakerBasicGetModuleCount(void);
s32 mapmakerBasicGetCursorX(void);
s32 mapmakerBasicGetCursorY(void);
s32 mapmakerBasicGetCursorZ(void);
s32 mapmakerBasicGetRotationDegrees(void);
s32 mapmakerBasicIsMenuOpen(void);
s32 mapmakerBasicGetMenuChoice(void);
s32 mapmakerBasicGetMusicTrack(void);
const char *mapmakerBasicGetMusicName(void);
s32 mapmakerBasicIsPlaytesting(void);
const char *mapmakerBasicGetStorageStatus(void);

s32 mapmakerBasicGetTool(void);
const char *mapmakerBasicGetToolName(void);
const char *mapmakerBasicGetTextureFlipName(void);
const char *mapmakerBasicGetPlacementModeName(void);
const char *mapmakerBasicGetEntityTypeName(void);
const char *mapmakerBasicGetEntityClassName(void);
s32 mapmakerBasicGetEntitySetupType(void);
s32 mapmakerBasicGetEntityObjectId(void);
const char *mapmakerBasicGetEntityIdLabel(void);
const char *mapmakerBasicGetPickupTypeName(void);
const char *mapmakerBasicGetPickupSubtypeName(void);
s32 mapmakerBasicGetPickupSetupType(void);
s32 mapmakerBasicGetPickupSubtype(void);
s32 mapmakerBasicGetPickupQuantity(void);
s32 mapmakerBasicPickupHasQuantity(void);
s32 mapmakerBasicGetPlayerStartCount(void);
s32 mapmakerBasicGetPlayerStartSlot(void);

/* Native Basic-map test mode.  This uses GoldenEye's normal Bond/gameplay
 * loop; only authored Map Maker world collision/rendering is substituted. */
s32 mapmakerNativeTestRequested(void);
void mapmakerNativeTestMarkStageEntered(void);
void mapmakerNativeTestBeginReturn(void);
s32 mapmakerNativeTestShouldReturnToEditor(void);
void mapmakerNativeTestFinishReturn(void);
void mapmakerNativeTestStagePrepare(void);
void mapmakerNativeTestPlaceCurrentPlayer(s32 playernum);
s32 mapmakerNativeTestActive(void);
s32 mapmakerNativeTryMove(f32 oldx, f32 oldz, f32 *newx, f32 *newz, f32 feety, f32 eyey);
void mapmakerNativeUpdatePlayerY(void);
Gfx *mapmakerNativeRenderWorld(Gfx *gdl);
s32 mapmakerBasicGetEntityCount(void);
s32 mapmakerBasicGetSnapEnabled(void);
s32 mapmakerBasicGetGridSize(void);
s32 mapmakerBasicGetEditorFogEnabled(void);
s32 mapmakerAdvancedGetArbitraryRotationEnabled(void);
s32 mapmakerBasicIsFreeView(void);
const char *mapmakerBasicGetViewName(void);
s32 mapmakerBasicGetMaterialSlot(void);
s32 mapmakerBasicGetMaterialTexture(void);
s32 mapmakerBasicGetMaterialPreview(struct sImageTableEntry *image);
const char *mapmakerBasicGetSelectedSurfaceName(void);
s32 mapmakerBasicGetSelectedModule(void);
const char *mapmakerBasicGetControlStyleName(void);

/* Frontend/custom-map integration.  The resident editor map lives in RAM even
 * after leaving the editor.  Controller Pak loads are validated before the
 * resident map is changed. */
const char *mapmakerBasicGetMapName(void);
s32 mapmakerBasicHasResidentMap(void);
s32 mapmakerBasicLoadFromControllerPak(void);
void mapmakerBasicRestoreFrontendResources(void);

#endif

#endif
