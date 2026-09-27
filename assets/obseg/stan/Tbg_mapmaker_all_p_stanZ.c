/*
 * GoldenEye Plus Map Maker native-test bootstrap STAN.
 *
 * This is a dedicated Map Maker resource.  It is intentionally tiny: one
 * room-1 support tile at the native-stage origin, used only to let GoldenEye's
 * normal player/setup bootstrap complete before Map Maker takes over movement
 * and authored-world collision.
 */
#include <ultra64.h>
#include <game/stan.h>

StandTile tile_0;

StandFileHeader Tbg_mapmaker_all_p_stanZ = {
    NULL,
    &tile_0,
    { 0x00, 0x00, 0x00, 0x00 }
};

/* id 0x000100 decodes as p1a.  Counter-clockwise in X/Z gives an upward floor. */
StandTile tile_0 = {
    0x000100, 0x01,
    0x0,
    0x0, 0x0, 0x0,
    3,
    0x0, 0x1, 0x2,
    {
        { -1000, 0, -1000, 0x0000 },
        {     0, 0,  1000, 0x0000 },
        {  1000, 0, -1000, 0x0000 }
    }
};

StandTile tile_1 = {
    0x000000, 0x00,
    0x0,
    0x0, 0x0, 0x0,
    0,
    0x0, 0x0, 0x0
};

StandFileFooter footer = {
    "unstric",
    NULL,
    NULL,
    NULL,
    NULL
};
