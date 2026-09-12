# assembler directives
.set noat      # allow manual use of $at
.set noreorder # don't insert nops after branches
.set gp=64

.include "macros.inc"

#  
#  Address: 7F01D1C0
#  
#  Unreferenced.
#  
#  The j instruction in the assembly indicates this is likely hand-written assembly
#  since the IDO compiler wouldn't typically emit that instruction in this situation.
#  

.section .text, "ax"
.balign 16
glabel sub_GAME_7F01D1C0
/* 051CF0 7F01D1C0 27BDFFF0 */  addiu $sp, $sp, -0x10
/* 051CF4 7F01D1C4 240C00FF */  li    $t4, 255
/* 051CF8 7F01D1C8 90880000 */  lbu   $t0, ($a0)
/* 051CFC 7F01D1CC 24840001 */  addiu $a0, $a0, 1
.L7F01D1D0:
/* 051D00 7F01D1D0 90890000 */  lbu   $t1, ($a0)
/* 051D04 7F01D1D4 24840001 */  addiu $a0, $a0, 1
/* 051D08 7F01D1D8 152C001A */  bne   $t1, $t4, .L7F01D244
/* 051D0C 7F01D1DC 240B00FF */   li    $t3, 255
/* 051D10 7F01D1E0 90890000 */  lbu   $t1, ($a0)
/* 051D14 7F01D1E4 240A0000 */  li    $t2, 0
/* 051D18 7F01D1E8 112C000C */  beq   $t1, $t4, .L7F01D21C
/* 051D1C 7F01D1EC 24840001 */   addiu $a0, $a0, 1
.L7F01D1F0:
/* 051D20 7F01D1F0 01495021 */  addu  $t2, $t2, $t1
/* 051D24 7F01D1F4 19200005 */  blez  $t1, .L7F01D20C
/* 051D28 7F01D1F8 2529FFFF */   addiu $t1, $t1, -1
.L7F01D1FC:
/* 051D2C 7F01D1FC A0EB0000 */  sb    $t3, ($a3)
/* 051D30 7F01D200 24E70001 */  addiu $a3, $a3, 1
/* 051D34 7F01D204 1D20FFFD */  bgtz  $t1, .L7F01D1FC
/* 051D38 7F01D208 2529FFFF */   addiu $t1, $t1, -1
.L7F01D20C:
/* 051D3C 7F01D20C 90890000 */  lbu   $t1, ($a0)
/* 051D40 7F01D210 396B00FF */  xori  $t3, $t3, 0xff
/* 051D44 7F01D214 152CFFF6 */  bne   $t1, $t4, .L7F01D1F0
/* 051D48 7F01D218 24840001 */   addiu $a0, $a0, 1
.L7F01D21C:
/* 051D4C 7F01D21C 0145082A */  slt   $at, $t2, $a1
/* 051D50 7F01D220 10200006 */  beqz  $at, .L7F01D23C
/* 051D54 7F01D224 254A0001 */   addiu $t2, $t2, 1
.L7F01D228:
/* 051D58 7F01D228 A0EB0000 */  sb    $t3, ($a3)
/* 051D5C 7F01D22C 24E70001 */  addiu $a3, $a3, 1
/* 051D60 7F01D230 0145082A */  slt   $at, $t2, $a1
/* 051D64 7F01D234 1420FFFC */  bnez  $at, .L7F01D228
/* 051D68 7F01D238 254A0001 */   addiu $t2, $t2, 1
.L7F01D23C:
/* 051D6C 7F01D23C 0BC074A7 */  j     .L7F01D29C
/* 051D70 7F01D240 24C6FFFF */   addiu $a2, $a2, -1

.L7F01D244:
/* 051D74 7F01D244 312A001F */  andi  $t2, $t1, 0x1f
/* 051D78 7F01D248 01485021 */  addu  $t2, $t2, $t0
/* 051D7C 7F01D24C 00094942 */  srl   $t1, $t1, 5
/* 051D80 7F01D250 25290001 */  addiu $t1, $t1, 1
/* 051D84 7F01D254 00C93023 */  subu  $a2, $a2, $t1
.L7F01D258:
/* 051D88 7F01D258 01405825 */  move  $t3, $t2
/* 051D8C 7F01D25C 19600005 */  blez  $t3, .L7F01D274
/* 051D90 7F01D260 256BFFFF */   addiu $t3, $t3, -1
.L7F01D264:
/* 051D94 7F01D264 A0EC0000 */  sb    $t4, ($a3)
/* 051D98 7F01D268 24E70001 */  addiu $a3, $a3, 1
/* 051D9C 7F01D26C 1D60FFFD */  bgtz  $t3, .L7F01D264
/* 051DA0 7F01D270 256BFFFF */   addiu $t3, $t3, -1
.L7F01D274:
/* 051DA4 7F01D274 00AA5823 */  subu  $t3, $a1, $t2
/* 051DA8 7F01D278 19600005 */  blez  $t3, .L7F01D290
/* 051DAC 7F01D27C 256BFFFF */   addiu $t3, $t3, -1
.L7F01D280:
/* 051DB0 7F01D280 A0E00000 */  sb    $zero, ($a3)
/* 051DB4 7F01D284 24E70001 */  addiu $a3, $a3, 1
/* 051DB8 7F01D288 1D60FFFD */  bgtz  $t3, .L7F01D280
/* 051DBC 7F01D28C 256BFFFF */   addiu $t3, $t3, -1
.L7F01D290:
/* 051DC0 7F01D290 2529FFFF */  addiu $t1, $t1, -1
/* 051DC4 7F01D294 1D20FFF0 */  bgtz  $t1, .L7F01D258
/* 051DC8 7F01D298 00000000 */   nop
.L7F01D29C:
/* 051DCC 7F01D29C 1CC0FFCC */  bgtz  $a2, .L7F01D1D0
/* 051DD0 7F01D2A0 00801025 */   move  $v0, $a0
/* 051DD4 7F01D2A4 03E00008 */  jr    $ra
/* 051DD8 7F01D2A8 27BD0010 */   addiu $sp, $sp, 0x10
