
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
.ascii "GE"       # cartridge ID
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
.byte  0x15       # modded branch revision 21
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
