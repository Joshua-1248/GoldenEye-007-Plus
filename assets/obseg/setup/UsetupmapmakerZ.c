/*
 * GoldenEye Plus Map Maker native-test bootstrap setup.
 * Dedicated to LEVELID_MAP_MAKER; no retail stage setup is reused.
 */
#include "ultra64.h"
#include "bondtypes.h"
#include "bondaicommands.h"

PadRecord padlist[];
BoundPadRecord pad3dlist[];
s32 propDefs[];
s32 intro[];
waygroup pathsets[];
waypoint pathwaypoints[];
PathRecord patrolpaths[];
AIListRecord ailists[];

stagesetup UsetupmapmakerZ = {
    &pathwaypoints,
    &pathsets,
    &intro,
    &propDefs,
    &patrolpaths,
    &ailists,
    &padlist,
    &pad3dlist,
    NULL,
    NULL
};

/* Bootstrap Pad 0 lives on the dedicated p1a STAN tile. */
PadRecord padlist[] = {
    { {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, "p1a", 0 },
    { {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, NULL, 0 }
};

BoundPadRecord pad3dlist[] = {
    { {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, NULL, 0,
      {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f} }
};

s32 propDefs[] = {
    /* Type = EndProps */
    _mkword(0, _mkshort(0, 48))
};

s32 intro[] = {
    /* Type = Spawn; Pad 0; Player 0 */
    _mkword(0, _mkshort(0, 0)), 0, 0,
    /* Type = StartWeapon; Fist; right hand; Player 0 */
    _mkword(0, _mkshort(0, 1)), 1, -1, 0,
    /* Type = Cuff */
    _mkword(0, _mkshort(0, 5)), 3,
    /* Type = EndIntro */
    _mkword(0, _mkshort(0, 9))
};

waygroup pathsets[] = {
    { NULL, NULL, 0 }
};

waypoint pathwaypoints[] = {
    { 0xffffffff, NULL, 0x00000000, 0x00000000 }
};

PathRecord patrolpaths[] = {
    { NULL, 0x00, 0x00, 0x0000 }
};

AIListRecord ailists[] = {
    { NULL, 0x00000000 }
};
