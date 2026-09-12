/* Linker script for prop model binary generation */
/* Base address matches game's prop loading address */

SECTIONS
{
    . = 0x05000000;
    
    .data : {
        *(.data)
        *(.data*)
    }
    
    /DISCARD/ : {
        *(.reginfo)
        *(.MIPS.abiflags)
        *(.pdr)
        *(.comment)
        *(.gnu.attributes)
    }
}
