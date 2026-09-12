#include <ultra64.h>
#include <PR/os.h>
#include <PR/gbi.h>
#include <bondconstants.h>
#include <fr.h>
#include <memp.h>
#include "bg.h"
#include "bondview.h"
#include "debugmenu_handler.h"
#include "decompress.h"
#include "bgfog.h"
#include "lv.h"
#include "math_ceil.h"
#include "matrixmath.h"
#include "player.h"
#include "explosion.h"
#include "bgroomtrans.h"

// new file, per EU

/**
 * Scan the Gfx commands in the half-open range [start, end) and replace any
 * commands that match the first entry of a pair in the primary fog LUT
 * (DL_LUT_PRIMARY_ADDFOG). For each matching pair the command is replaced by
 * the LUT's second entry.
 *
 * This is used to patch display lists to add the "primary" fog variants.
 *
 * Parameters:
 *   start - pointer to first Gfx in the range to scan
 *   end   - pointer one-past-last Gfx in the range
 */
void bgApplyPrimaryFogLUTEntries(Gfx *start, Gfx *end)
{
    Gfx *curGfx;
    Gfx *lutPair;

    curGfx = start;

    while (curGfx < end)
    {
         /* DL_LUT_PRIMARY_ADDFOG is an array of (match,replacement) Gfx pairs,
        terminated by an entry with words.w0 == 0. Each pair occupies two
        Gfx entries, so we step by 2. */
        for (lutPair = DL_LUT_PRIMARY_ADDFOG; lutPair->words.w0 != 0; lutPair += 2)
        {
            if ((lutPair->words.w0 == curGfx->words.w0) && (lutPair->words.w1 == curGfx->words.w1))
            {
                /* Replace current command with the LUT's replacement (the second entry). */
                *curGfx = *(lutPair+1);
            }
        }

        curGfx++;
    }
}


/**
 * Scan the Gfx commands in the range starting at 'start'. If 'end' is non-NULL
 * treat it as a one-past-last pointer and scan [start, end). If 'end' is NULL
 * treat 'start' as a null-terminated display list and scan until the sentinel
 * command G_ENDDL is encountered.
 *
 * For each Gfx in the scanned range, look up replacement entries in the
 * runtime-selected LUT: ptrDynamic_CC_RM_LUT[lutIndex]. The LUT is organized as
 * (match,replacement) pairs (two Gfx entries per pair), terminated by an
 * entry whose words.w0 == 0. When a match is found replace the command with
 * the LUT's replacement. Replacements are counted in a static counter for
 * telemetry/debugging.
 *
 * Parameters:
 *   start   - pointer to first Gfx to scan
 *   end     - pointer one-past-last Gfx to scan, or NULL to use G_ENDDL sentinel
 *   lutIndex- index into ptrDynamic_CC_RM_LUT selecting the active LUT
 */
void bgApplyDynamicCCRMLUT(Gfx *start, Gfx *end, enum CCRMLUT lutIndex)
{
    Gfx *curGfx;
    Gfx *lutPair;

    static s32 s_bg_lut_replacement_count = 0;

    curGfx = start;

    /* Loop until end pointer or sentinel G_ENDDL (when end==NULL) */
    while (((end != NULL) && (curGfx < end)) || ((end == NULL) && (((s8*)curGfx)[0] != (s8)G_ENDDL)))
    {
        for (lutPair = ptrDynamic_CC_RM_LUT[(s32)lutIndex]; lutPair->words.w0 != 0; lutPair += 2)
        {
            if ((lutPair->words.w0 == curGfx->words.w0) && (lutPair->words.w1 == curGfx->words.w1))
            {
                s_bg_lut_replacement_count += 1;
                *curGfx = *(lutPair + 1);
            }
        }

        curGfx++;
    }
}
