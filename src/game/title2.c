#include <ultra64.h>
#include "title2.h"


f32 D_8002BB00 = 0; // Set but never read
f32 D_8002BB04 = 0; // Set but never read
f32 D_8002BB08 = 0; // Set but never read
f32 D_8002BB0C = 255.0; // Set but never read
f32 D_8002BB10 = 255.0; // Set but never read
f32 D_8002BB14 = 255.0; // Set but never read


/**
 * Address: 7F01B240
 * 
 * Render the gun barrel background in the folder select menu. 
 * Like all front end screens, this screen renders at the high res 440x330 resolution.
 * The 440x299 8-bit background texture is far too large to fit in the N64's 4KB texture cache (TMEM),
 * so instead the texture is read one 440 x 1 pixel row at a time. That row of pixels is then applied to a rectangle and the game
 * renders 299 one pixel tall rectangles, moving down the screen, until the screen is filled.
 * 
 * The game also gives the rectangles a color, starting darker at the top and lighter towards the bottom, creating a smooth vertical gradient.
 */
Gfx* titleRenderFolderMenuBackgroundLines(Gfx* gdl, u8 *image, s32 xOffset, struct FolderSelectColour* topColour, struct FolderSelectColour* bottomColour) 
{
    f32 topRed;
    f32 topGreen;
    f32 topBlue;
    f32 bottomRed;
    f32 bottomGreen;
    f32 bottomBlue;
    f32 temp_f14_2; // unused
    s32 i;

    topRed = topColour->red;
    topGreen = topColour->green;
    topBlue = topColour->blue;
    bottomRed = bottomColour->red;
    bottomGreen = bottomColour->green;
    bottomBlue = bottomColour->blue;
    D_8002BB0C = bottomRed;
    D_8002BB10 = bottomGreen;
    D_8002BB14 = bottomBlue;

    i = 0;

    while ((i + 1) < 300) 
    {
        // Load a 440 wide by 1 pixel tall 8-bit intensity texture.
        gDPLoadTextureBlock(
            gdl++, image, G_IM_FMT_I, G_IM_SIZ_8b, 440, 1, 0, G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMIRROR | G_TX_CLAMP,
            G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD
        );

        gDPSetPrimColor(
            gdl++, 0, 0, topRed + (((bottomRed - topRed) * i) / 299.0f),
            topGreen + (((bottomGreen - topGreen) * i) / 299.0f), topBlue + (((bottomBlue - topBlue) * i) / 299.0f), 255
        );

        if (xOffset < 0) 
        {
            gSPTextureRectangle(
                gdl++, 0, (i + 0x10) << 2, (440 << 2) - 1, ((((i + 1) + 0x10)) << 2) - 1, G_TX_RENDERTILE, (-xOffset) << 5, 0,
                1 << 10, 1 << 10
            );
        } 
        else 
        {
            gSPTextureRectangle(
                gdl++, xOffset << 2, (i + 0x10) << 2, (440 << 2) - 1, ((((i + 1) + 0x10)) << 2) - 1, G_TX_RENDERTILE, 0, 0,
                1 << 10, 1 << 10
            );
        }

        i++;
        image += 440; // Advance the image data by 440 bytes, since each texel is 1 byte.
    }

    D_8002BB08 = topBlue;
    D_8002BB04 = topGreen;
    D_8002BB00 = topRed;

    return gdl;
}


/**
 * Unreferenced.
 */
Gfx *sub_GAME_7F01B6E0(Gfx *gdl, u8 *image, s32 arg2)
{
    static f32 D_8002BB2C = 0;
    static f32 D_8002BB28 = 0;
    static f32 D_8002BB24 = 0;
    static f32 D_8002BB20 = 0;
    static f32 D_8002BB1C = 0;
    static f32 D_8002BB18 = 0;
    s32 i;
    s32 xOffset;

    i = 0;
    xOffset = 0;
    D_8002BB2C = arg2;
    D_8002BB28 = arg2;
    D_8002BB24 = arg2;
    D_8002BB20 = arg2;
    D_8002BB1C = arg2;
    D_8002BB18 = arg2;

    while ((i + 1) < 218)
    {
        gDPLoadTextureBlock(gdl++, image, G_IM_FMT_RGBA, G_IM_SIZ_16b, 320, 1, 0, G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMASK,G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);

        gDPSetPrimColor(gdl++, 0, 0, D_8002BB2C + (((D_8002BB20 - arg2) * i) / 217.0f), D_8002BB28 + (((D_8002BB1C - arg2) * i) / 217.0f), D_8002BB24 + (((D_8002BB18 - arg2) * i) / 217.0f), 0xff);

        if (xOffset < 0)
        {
            gSPTextureRectangle(gdl++, 0, (i + 12) << 2, (320 << 2) - 1, ((i + 13) << 2) - 1, G_TX_RENDERTILE, (-xOffset) << 5, 0, 1 << 10, 1 << 10);
        }
        else
        {
            gSPTextureRectangle(gdl++, xOffset << 2, (i + 12) << 2, (320 << 2) - 1, ((i + 13) << 2) - 1, G_TX_RENDERTILE, 0, 0, 1 << 10, 1 << 10);
        }

        i++;
        image += 0x280;
    }

    return gdl;
}