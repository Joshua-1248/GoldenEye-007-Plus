
.section .data
.byte  0x80, 0x37, 0x12, 0x40 # PI BSD Domain 1 register
.word  0x0000000F # clock rate setting
.word  0x80000400 # entry point
.word  0x00001447 # release
.word  0xDCBC50D1 # checksum1
.word  0x09FD1AA3 # checksum2
.word  0x00000000 # unknown
.word  0x00000000 # unknown
.ifdef GE_MODDED_CHEATS
.ifdef GE_UNLOCK_SAVE1
.ascii "GoldenEye 007 Plus  " # GoldenEye 007 Plus
.else
.ascii "GoldenEye 007 Plus  " # GoldenEye 007 Plus
.endif
.else
.ifdef GE_UNLOCK_SAVE1
.ascii "GOLDENEYE OPT ALL   " # Physical optimized ROM, everything unlocked
.else
.ifdef GE_PHYSICAL_FASTPATHS
.ascii "GoldenEye 007 Plus  " # GoldenEye 007 Plus
.else
.ifdef GE_PHYSICAL_CODE
.ascii "GoldenEye 007 Plus  " # GoldenEye 007 Plus
.else
.ascii "GOLDENEYE           " # Retail ROM name: 20 bytes
.endif
.endif
.endif
.endif
.word  0x00000000 # unknown
.word  0x0000004E # cartridge
.ifdef GE_MODDED_CHEATS
/*
 * V84: use the de-facto N64 homebrew/flashcart configuration header for
 * GoldenEye 007 Plus.  EverDrive OS and modern universal flashcart menus
 * recognize ROM ID "ED" and interpret byte 0x3f as save/config metadata.
 * Retail/non-modded builds deliberately retain GoldenEye's original "GE" ID.
 */
.ascii "ED"       # homebrew flashcart configuration ROM ID
.else
.ascii "GE"       # retail GoldenEye cartridge ID
.endif
.ifdef LANG_US
.ascii "E"        # country
.endif
.ifdef LANG_JP
.ascii "J"        # country
.endif
.ifdef LANG_EU
.ascii "P"        # country
.endif
.ifdef GE_MODDED_CHEATS
/* Homebrew header config byte (offset 0x3f): high nibble is save type. */
.ifdef GE_SAVE_EEPROM16K
.byte  0x20       # EEPROM 16 Kbit (2 KiB), no RTC, fixed region
.else
.ifdef GE_SAVE_SRAM
.byte  0x30       # SRAM 256 Kbit (32 KiB), no RTC, fixed region
.else
.byte  0x00       # no cartridge save backend requested
.endif
.endif
.else
.ifdef GE_UNLOCK_SAVE1
.byte  0x10       # physical optimized revision 16, everything unlocked
.else
.ifdef GE_PHYSICAL_FASTPATHS
.byte  0x10       # physical optimized gameplay revision 16
.else
.ifdef GE_PHYSICAL_CODE
.byte  0x01       # physical baseline revision
.else
.byte  0x00       # retail version
.endif
.endif
.endif
.endif
