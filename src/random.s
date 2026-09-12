# assembler directives
.set noat      # allow manual use of $at
.set noreorder # don't insert nops after branches
.set gp=64

.include "macros.inc"

.section .data
.balign 8
glabel g_randomSeed
.word 0xAB8D9F77
.word 0x81280783

.section .text, "ax"
glabel randomGetNext
/* 00B050 7000A450 3C048002 */  lui   $a0, %hi(g_randomSeed)
/* 00B054 7000A454 DC844460 */  ld    $a0, %lo(g_randomSeed)($a0)
/* 00B058 7000A458 3C018002 */  lui   $at, %hi(g_randomSeed)
/* 00B05C 7000A45C 000437FC */  dsll32 $a2, $a0, 0x1f
/* 00B060 7000A460 00042FF8 */  dsll  $a1, $a0, 0x1f
/* 00B064 7000A464 000637FA */  dsrl  $a2, $a2, 0x1f
/* 00B068 7000A468 0005283E */  dsrl32 $a1, $a1, 0
/* 00B06C 7000A46C 0004233C */  dsll32 $a0, $a0, 0xc
/* 00B070 7000A470 00C53025 */  or    $a2, $a2, $a1
/* 00B074 7000A474 0004203E */  dsrl32 $a0, $a0, 0
/* 00B078 7000A478 00C43026 */  xor   $a2, $a2, $a0
/* 00B07C 7000A47C 0006253A */  dsrl  $a0, $a2, 0x14
/* 00B080 7000A480 30840FFF */  andi  $a0, $a0, 0xfff
/* 00B084 7000A484 00862026 */  xor   $a0, $a0, $a2
/* 00B088 7000A488 0004103C */  dsll32 $v0, $a0, 0
/* 00B08C 7000A48C FC244460 */  sd    $a0, %lo(g_randomSeed)($at)
/* 00B090 7000A490 03E00008 */  jr    $ra
/* 00B094 7000A494 0002103F */   dsra32 $v0, $v0, 0

glabel randomSetSeed
/* 00B098 7000A498 64840001 */  daddiu $a0, $a0, 1
/* 00B09C 7000A49C 3C018002 */  lui   $at, %hi(g_randomSeed)
/* 00B0A0 7000A4A0 FC244460 */  sd    $a0, %lo(g_randomSeed)($at)
/* 00B0A4 7000A4A4 03E00008 */  jr    $ra
/* 00B0A8 7000A4A8 24040000 */   li    $a0, 0

glabel randomGetNextFrom
/* 00B0AC 7000A4AC DC870000 */  ld    $a3, ($a0)
/* 00B0B0 7000A4B0 000737FC */  dsll32 $a2, $a3, 0x1f
/* 00B0B4 7000A4B4 00072FF8 */  dsll  $a1, $a3, 0x1f
/* 00B0B8 7000A4B8 000637FA */  dsrl  $a2, $a2, 0x1f
/* 00B0BC 7000A4BC 0005283E */  dsrl32 $a1, $a1, 0
/* 00B0C0 7000A4C0 00073B3C */  dsll32 $a3, $a3, 0xc
/* 00B0C4 7000A4C4 00C53025 */  or    $a2, $a2, $a1
/* 00B0C8 7000A4C8 0007383E */  dsrl32 $a3, $a3, 0
/* 00B0CC 7000A4CC 00C73026 */  xor   $a2, $a2, $a3
/* 00B0D0 7000A4D0 00063D3A */  dsrl  $a3, $a2, 0x14
/* 00B0D4 7000A4D4 30E70FFF */  andi  $a3, $a3, 0xfff
/* 00B0D8 7000A4D8 00E63826 */  xor   $a3, $a3, $a2
/* 00B0DC 7000A4DC 0007103C */  dsll32 $v0, $a3, 0
/* 00B0E0 7000A4E0 FC870000 */  sd    $a3, ($a0)
/* 00B0E4 7000A4E4 03E00008 */  jr    $ra
/* 00B0E8 7000A4E8 0002103F */   dsra32 $v0, $v0, 0