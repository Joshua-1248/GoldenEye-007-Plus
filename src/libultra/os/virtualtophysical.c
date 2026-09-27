#include <os_internal.h>
#include <R4300.h>
#include "osint.h"

u32 osVirtualToPhysical(void *addr)
{
#ifdef GE_PHYSICAL_FASTPATHS
    u32 value = (u32)addr;

    /* KSEG0 and KSEG1 are contiguous and map identically to physical RDRAM.
     * Preserve the original TLB fallback exactly for every other address. */
    if ((value - 0x80000000U) < 0x40000000U)
    {
        return value & 0x1fffffffU;
    }

    return __osProbeTLB(addr);
#else
    if (IS_KSEG0(addr))
    {
        return K0_TO_PHYS(addr);
    }
    else if (IS_KSEG1(addr))
    {
        return K1_TO_PHYS(addr);
    }
    else
    {
        return __osProbeTLB(addr);
    }
#endif
}
