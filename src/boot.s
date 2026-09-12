# assembler directives
.set noat      # allow manual use of $at
.set noreorder # don't insert nops after branches
.set gp=64

.include "macros.inc"

.section .text, "ax"

glabel boot
.ifdef GE_PHYSICAL_CODE
# Physical-code mode executes this resident block through KSEG0, so the
# retail 0x70000000->physical-0 wired TLB entry is unnecessary.  Preserve
# the exact boot-block byte budget so subsequent resident offsets remain
# stable while the architecture is brought up.
.rept 23
 nop
.endr
.else
/* 001050 70000450 24020001 */  li    $v0, 1
/* 001054 70000454 24030000 */  li    $v1, 0
/* 001058 70000458 24040000 */  li    $a0, 0
/* 00105C 7000045C 3C057000 */  lui   $a1, 0x7000
/* 001060 70000460 2406001F */  li    $a2, 31
/* 001064 70000464 24070001 */  li    $a3, 1
/* 001068 70000468 3C08007F */  lui   $t0, (0x007FE000 >> 16)
/* 00106C 7000046C 3508E000 */  ori   $t0, (0x007FE000 & 0xFFFF)
/* 001070 70000470 40820000 */  mtc0  $v0, $0
/* 001074 70000474 00031B02 */  srl   $v1, $v1, 0xc
/* 001078 70000478 00031980 */  sll   $v1, $v1, 6
/* 00107C 7000047C 00661821 */  addu  $v1, $v1, $a2
/* 001080 70000480 40831000 */  mtc0  $v1, $2
/* 001084 70000484 00042302 */  srl   $a0, $a0, 0xc
/* 001088 70000488 00042180 */  sll   $a0, $a0, 6
/* 00108C 7000048C 00872021 */  addu  $a0, $a0, $a3
/* 001090 70000490 40841800 */  mtc0  $a0, $3
/* 001094 70000494 00052342 */  srl   $a0, $a1, 0xd
/* 001098 70000498 00042340 */  sll   $a0, $a0, 0xd
/* 00109C 7000049C 40845000 */  mtc0  $a0, $10
/* 0010A0 700004A0 40882800 */  mtc0  $t0, $5
/* 0010A4 700004A4 00000000 */  nop
/* 0010A8 700004A8 42000002 */  tlbwi
.endif
/* 0010AC 700004AC 3C0A7000 */  lui   $t2, %hi(init)
/* 0010B0 700004B0 254A0510 */  addiu $t2, $t2, %lo(init)
/* 0010B4 700004B4 01400008 */  jr    $t2
/* 0010B8 700004B8 00000000 */   nop

glabel get_csegmentSegmentStart
/* 0010BC 700004BC 3C028002 */  lui   $v0, %hi(_csegmentSegmentStart)
/* 0010C0 700004C0 03E00008 */  jr    $ra
/* 0010C4 700004C4 24420D90 */   addiu $v0, $v0, %lo(_csegmentSegmentStart)

glabel get_cdataSegmentRomStart
/* 0010C8 700004C8 3C020000 */  lui   $v0, %hi(_cdataSegmentRomStart)
/* 0010CC 700004CC 03E00008 */  jr    $ra
/* 0010D0 700004D0 24421990 */   addiu $v0, $v0, %lo(_cdataSegmentRomStart)

glabel get_cdataSegmentRomEnd
/* 0010D4 700004D4 3C020000 */  lui   $v0, %hi(_cdataSegmentRomEnd)
/* 0010D8 700004D8 03E00008 */  jr    $ra
/* 0010DC 700004DC 24423590 */   addiu $v0, $v0, %lo(_cdataSegmentRomEnd)

glabel get_inflateSegmentRomStart
/* 0010E0 700004E0 3C020000 */  lui   $v0, %hi(_inflateSegmentRomStart)
/* 0010E4 700004E4 03E00008 */  jr    $ra
/* 0010E8 700004E8 24423590 */   addiu $v0, $v0, %lo(_inflateSegmentRomStart)

glabel get_inflateSegmentRomEnd
/* 0010EC 700004EC 3C020000 */  lui   $v0, %hi(_inflateSegmentRomEnd)
/* 0010F0 700004F0 03E00008 */  jr    $ra
/* 0010F4 700004F4 24424B30 */   addiu $v0, $v0, %lo(_inflateSegmentRomEnd)

glabel jump_decompressfile
/* 0010F8 700004F8 3C077020 */  lui   $a3, %hi(decompress_entry)
/* 0010FC 700004FC 24E7141C */  addiu $a3, $a3, %lo(decompress_entry)
/* 001100 70000500 00E00008 */  jr    $a3
/* 001104 70000504 00000000 */   nop