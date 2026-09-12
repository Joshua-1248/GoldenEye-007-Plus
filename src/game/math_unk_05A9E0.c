#include <ultra64.h>

// some kind of matrix helper / convenience function.
/**
 * Address: 7F05A9E0
 */
f32 modelGetBendStretchScale(f32 halfangle) 
{
    return sqrtf(((sinf(halfangle) / cosf(halfangle)) + 1.0f));
}

  

