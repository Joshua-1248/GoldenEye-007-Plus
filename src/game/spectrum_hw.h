#include <ultra64.h>

/**
* comments copied from the MIT licensed project
* https://github.com/Dotneteer/spectnetide/blob/master/Core/Spect.Net.SpectrumEmu/Cpu/Z80Operations.cs
* Copyright (c) 2017 Istvan Novak
*/

#define SPEC_HEX_OP_NOP 0x00
#define SPEC_HEX_OP_LD_BC_HHLL 0x01
#define SPEC_HEX_OP_LD_pBCp_A 0x02
#define SPEC_HEX_OP_INC_BC 0x03
#define SPEC_HEX_OP_INC_B 0x04
#define SPEC_HEX_OP_DEC_B 0x05
#define SPEC_HEX_OP_LD_B_NN 0x06
#define SPEC_HEX_OP_RLCA 0x07
#define SPEC_HEX_OP_EX_AF_AF 0x08
#define SPEC_HEX_OP_ADD_HL_BC 0x09
#define SPEC_HEX_OP_LD_A_pBCp 0x0A
#define SPEC_HEX_OP_DEC_BC 0x0B
#define SPEC_HEX_OP_INC_C 0x0C
#define SPEC_HEX_OP_DEC_C 0x0D
#define SPEC_HEX_OP_LD_C_NN 0x0E
#define SPEC_HEX_OP_CA 0x0F
#define SPEC_HEX_OP_DJNZ_NN 0x10
#define SPEC_HEX_OP_LD_DE_HHLL 0x11
#define SPEC_HEX_OP_LD_pDEp_A 0x12
#define SPEC_HEX_OP_INC_DE 0x13
#define SPEC_HEX_OP_INC_D 0x14
#define SPEC_HEX_OP_DEC_D 0x15
#define SPEC_HEX_OP_LD_D_NN 0x16
#define SPEC_HEX_OP_RLA 0x17
#define SPEC_HEX_OP_JR_NN 0x18
#define SPEC_HEX_OP_ADD_HL_DE 0x19
#define SPEC_HEX_OP_LD_A_pDEp 0x1A
#define SPEC_HEX_OP_DEC_DE 0x1B
#define SPEC_HEX_OP_INC_E 0x1C
#define SPEC_HEX_OP_DEC_E 0x1D
#define SPEC_HEX_OP_LD_E_NN 0x1E
#define SPEC_HEX_OP_RRA 0x1F
#define SPEC_HEX_OP_JR_NZ_NN 0x20
#define SPEC_HEX_OP_LD_HL_HHLL 0x21
#define SPEC_HEX_OP_LD_pHHLLp_HL 0x22
#define SPEC_HEX_OP_INC_HL 0x23
#define SPEC_HEX_OP_INC_H 0x24
#define SPEC_HEX_OP_DEC_H 0x25
#define SPEC_HEX_OP_LD_H_NN 0x26
#define SPEC_HEX_OP_DAA 0x27
#define SPEC_HEX_OP_JR_Z_NN 0x28
#define SPEC_HEX_OP_ADD_HL_HL 0x29
#define SPEC_HEX_OP_LD_HL_pHHLLp 0x2A
#define SPEC_HEX_OP_DEC_HL 0x2B
#define SPEC_HEX_OP_INC_L 0x2C
#define SPEC_HEX_OP_DEC_L 0x2D
#define SPEC_HEX_OP_LD_L_NN 0x2E
#define SPEC_HEX_OP_CPL 0x2F
#define SPEC_HEX_OP_JR_NC_NN 0x30
#define SPEC_HEX_OP_LD_SP_HHLL 0x31
#define SPEC_HEX_OP_LD_pHHLLp_A 0x32
#define SPEC_HEX_OP_INC_SP 0x33
#define SPEC_HEX_OP_INC_pHLp 0x34
#define SPEC_HEX_OP_DEC_pHLp 0x35
#define SPEC_HEX_OP_LD_pHLp_NN 0x36
#define SPEC_HEX_OP_SCF 0x37
#define SPEC_HEX_OP_JR_C_NN 0x38
#define SPEC_HEX_OP_ADD_HL_SP 0x39
#define SPEC_HEX_OP_LD_A_pHHLLp 0x3A
#define SPEC_HEX_OP_DEC_SP 0x3B
#define SPEC_HEX_OP_INC_A 0x3C
#define SPEC_HEX_OP_DEC_A 0x3D
#define SPEC_HEX_OP_LD_A_NN 0x3E
#define SPEC_HEX_OP_CCF 0x3F
#define SPEC_HEX_OP_LD_B_B 0x40
#define SPEC_HEX_OP_LD_B_C 0x41
#define SPEC_HEX_OP_LD_B_D 0x42
#define SPEC_HEX_OP_LD_B_E 0x43
#define SPEC_HEX_OP_LD_B_H 0x44
#define SPEC_HEX_OP_LD_B_L 0x45
#define SPEC_HEX_OP_LD_B_pHLp 0x46
#define SPEC_HEX_OP_LD_B_A 0x47
#define SPEC_HEX_OP_LD_C_B 0x48
#define SPEC_HEX_OP_LD_C_C 0x49
#define SPEC_HEX_OP_LD_C_D 0x4A
#define SPEC_HEX_OP_LD_C_E 0x4B
#define SPEC_HEX_OP_LD_C_H 0x4C
#define SPEC_HEX_OP_LD_C_L 0x4D
#define SPEC_HEX_OP_LD_C_pHLp 0x4E
#define SPEC_HEX_OP_LD_C_A 0x4F
#define SPEC_HEX_OP_LD_D_B 0x50
#define SPEC_HEX_OP_LD_D_C 0x51
#define SPEC_HEX_OP_LD_D_D 0x52
#define SPEC_HEX_OP_LD_D_E 0x53
#define SPEC_HEX_OP_LD_D_H 0x54
#define SPEC_HEX_OP_LD_D_L 0x55
#define SPEC_HEX_OP_LD_D_pHLp 0x56
#define SPEC_HEX_OP_LD_D_A 0x57
#define SPEC_HEX_OP_LD_E_B 0x58
#define SPEC_HEX_OP_LD_E_C 0x59
#define SPEC_HEX_OP_LD_E_D 0x5A
#define SPEC_HEX_OP_LD_E_E 0x5B
#define SPEC_HEX_OP_LD_E_H 0x5C
#define SPEC_HEX_OP_LD_E_L 0x5D
#define SPEC_HEX_OP_LD_E_pHLp 0x5E
#define SPEC_HEX_OP_LD_E_A 0x5F
#define SPEC_HEX_OP_LD_H_B 0x60
#define SPEC_HEX_OP_LD_H_C 0x61
#define SPEC_HEX_OP_LD_H_D 0x62
#define SPEC_HEX_OP_LD_H_E 0x63
#define SPEC_HEX_OP_LD_H_H 0x64
#define SPEC_HEX_OP_LD_H_L 0x65
#define SPEC_HEX_OP_LD_H_pHLp 0x66
#define SPEC_HEX_OP_LD_H_A 0x67
#define SPEC_HEX_OP_LD_L_B 0x68
#define SPEC_HEX_OP_LD_L_C 0x69
#define SPEC_HEX_OP_LD_L_D 0x6A
#define SPEC_HEX_OP_LD_L_E 0x6B
#define SPEC_HEX_OP_LD_L_H 0x6C
#define SPEC_HEX_OP_LD_L_L 0x6D
#define SPEC_HEX_OP_LD_L_pHLp 0x6E
#define SPEC_HEX_OP_LD_L_A 0x6F
#define SPEC_HEX_OP_LD_pHLp_B 0x70
#define SPEC_HEX_OP_LD_pHLp_C 0x71
#define SPEC_HEX_OP_LD_pHLp_D 0x72
#define SPEC_HEX_OP_LD_pHLp_E 0x73
#define SPEC_HEX_OP_LD_pHLp_H 0x74
#define SPEC_HEX_OP_LD_pHLp_L 0x75
#define SPEC_HEX_OP_HALT 0x76
#define SPEC_HEX_OP_LD_pHLp_A 0x77
#define SPEC_HEX_OP_LD_A_B 0x78
#define SPEC_HEX_OP_LD_A_C 0x79
#define SPEC_HEX_OP_LD_A_D 0x7A
#define SPEC_HEX_OP_LD_A_E 0x7B
#define SPEC_HEX_OP_LD_A_H 0x7C
#define SPEC_HEX_OP_LD_A_L 0x7D
#define SPEC_HEX_OP_LD_A_pHLp 0x7E
#define SPEC_HEX_OP_LD_A_A 0x7F
#define SPEC_HEX_OP_ADD_A_B 0x80
#define SPEC_HEX_OP_ADD_A_C 0x81
#define SPEC_HEX_OP_ADD_A_D 0x82
#define SPEC_HEX_OP_ADD_A_E 0x83
#define SPEC_HEX_OP_ADD_A_H 0x84
#define SPEC_HEX_OP_ADD_A_L 0x85
#define SPEC_HEX_OP_ADD_A_pHLp 0x86
#define SPEC_HEX_OP_ADD_A_A 0x87
#define SPEC_HEX_OP_ADC_A_B 0x88
#define SPEC_HEX_OP_ADC_A_C 0x89
#define SPEC_HEX_OP_ADC_A_D 0x8A
#define SPEC_HEX_OP_ADC_A_E 0x8B
#define SPEC_HEX_OP_ADC_A_H 0x8C
#define SPEC_HEX_OP_ADC_A_L 0x8D
#define SPEC_HEX_OP_ADC_A_pHLp 0x8E
#define SPEC_HEX_OP_ADC_A_A 0x8F
#define SPEC_HEX_OP_SUB_A_B 0x90
#define SPEC_HEX_OP_SUB_A_C 0x91
#define SPEC_HEX_OP_SUB_A_D 0x92
#define SPEC_HEX_OP_SUB_A_E 0x93
#define SPEC_HEX_OP_SUB_A_H 0x94
#define SPEC_HEX_OP_SUB_A_L 0x95
#define SPEC_HEX_OP_SUB_A_pHLp 0x96
#define SPEC_HEX_OP_SUB_A_A 0x97
#define SPEC_HEX_OP_SBC_A_B 0x98
#define SPEC_HEX_OP_SBC_A_C 0x99
#define SPEC_HEX_OP_SBC_A_D 0x9A
#define SPEC_HEX_OP_SBC_A_E 0x9B
#define SPEC_HEX_OP_SBC_A_H 0x9C
#define SPEC_HEX_OP_SBC_A_L 0x9D
#define SPEC_HEX_OP_SBC_A_pHLp 0x9E
#define SPEC_HEX_OP_SBC_A_A 0x9F
#define SPEC_HEX_OP_AND_B 0xA0
#define SPEC_HEX_OP_AND_C 0xA1
#define SPEC_HEX_OP_AND_D 0xA2
#define SPEC_HEX_OP_AND_E 0xA3
#define SPEC_HEX_OP_AND_H 0xA4
#define SPEC_HEX_OP_AND_L 0xA5
#define SPEC_HEX_OP_AND_pHLp 0xA6
#define SPEC_HEX_OP_AND_A 0xA7
#define SPEC_HEX_OP_XOR_B 0xA8
#define SPEC_HEX_OP_XOR_C 0xA9
#define SPEC_HEX_OP_XOR_D 0xAA
#define SPEC_HEX_OP_XOR_E 0xAB
#define SPEC_HEX_OP_XOR_H 0xAC
#define SPEC_HEX_OP_XOR_L 0xAD
#define SPEC_HEX_OP_XOR_pHLp 0xAE
#define SPEC_HEX_OP_XOR_A 0xAF
#define SPEC_HEX_OP_OR_B 0xB0
#define SPEC_HEX_OP_OR_C 0xB1
#define SPEC_HEX_OP_OR_D 0xB2
#define SPEC_HEX_OP_OR_E 0xB3
#define SPEC_HEX_OP_OR_H 0xB4
#define SPEC_HEX_OP_OR_L 0xB5
#define SPEC_HEX_OP_OR_pHLp 0xB6
#define SPEC_HEX_OP_OR_A 0xB7
#define SPEC_HEX_OP_CP_B 0xB8
#define SPEC_HEX_OP_CP_C 0xB9
#define SPEC_HEX_OP_CP_D 0xBA
#define SPEC_HEX_OP_CP_E 0xBB
#define SPEC_HEX_OP_CP_H 0xBC
#define SPEC_HEX_OP_CP_L 0xBD
#define SPEC_HEX_OP_CP_pHLp 0xBE
#define SPEC_HEX_OP_CP_A 0xBF
#define SPEC_HEX_OP_RET_NZ 0xC0
#define SPEC_HEX_OP_POP_BC 0xC1
#define SPEC_HEX_OP_JP_NZ_HHLL 0xC2
#define SPEC_HEX_OP_JP_HHLL 0xC3
#define SPEC_HEX_OP_CALL_NZ_HHLL 0xC4
#define SPEC_HEX_OP_PUSH_BC 0xC5
#define SPEC_HEX_OP_ADD_A_NN 0xC6
#define SPEC_HEX_OP_RST_00 0xC7
#define SPEC_HEX_OP_RET_Z 0xC8
#define SPEC_HEX_OP_RET 0xC9
#define SPEC_HEX_OP_JP_Z_HHLL 0xCA
#define SPEC_HEX_OP_CB_Opcodes 0xCB
#define SPEC_HEX_OP_CALL_Z_HHLL 0xCC
#define SPEC_HEX_OP_CALL_HHLL 0xCD
#define SPEC_HEX_OP_ADC_A_NN 0xCE
#define SPEC_HEX_OP_RST_08 0xCF
#define SPEC_HEX_OP_RET_NC 0xD0
#define SPEC_HEX_OP_POP_DE 0xD1
#define SPEC_HEX_OP_JP_NC_HHLL 0xD2
#define SPEC_HEX_OP_OUT_pNNp_A 0xD3
#define SPEC_HEX_OP_CALL_NC_HHLL 0xD4
#define SPEC_HEX_OP_PUSH_DE 0xD5
#define SPEC_HEX_OP_SUB_A_NN 0xD6
#define SPEC_HEX_OP_RST_10 0xD7
#define SPEC_HEX_OP_RET_C 0xD8
#define SPEC_HEX_OP_EXX 0xD9
#define SPEC_HEX_OP_JP_C_HHLL 0xDA
#define SPEC_HEX_OP_IN_A_pNNp 0xDB
#define SPEC_HEX_OP_CALL_C_HHLL 0xDC
#define SPEC_HEX_OP_DD_Opcodes 0xDD
#define SPEC_HEX_OP_SBC_A_NN 0xDE
#define SPEC_HEX_OP_RST_18 0xDF
#define SPEC_HEX_OP_RET_PO 0xE0
#define SPEC_HEX_OP_POP_HL 0xE1
#define SPEC_HEX_OP_JP_PO_HHLL 0xE2
#define SPEC_HEX_OP_EX_pSPp_HL 0xE3
#define SPEC_HEX_OP_CALL_PO_HHLL 0xE4
#define SPEC_HEX_OP_PUSH_HL 0xE5
#define SPEC_HEX_OP_AND_NN 0xE6
#define SPEC_HEX_OP_RST_20 0xE7
#define SPEC_HEX_OP_RET_PE 0xE8
#define SPEC_HEX_OP_JP_pHLp 0xE9
#define SPEC_HEX_OP_JP_PE_HHLL 0xEA
#define SPEC_HEX_OP_EX_DE_HL 0xEB
#define SPEC_HEX_OP_CALL_Pe_HHLL 0xEC
#define SPEC_HEX_OP_ED_Opcodes 0xED
#define SPEC_HEX_OP_XOR_NN 0xEE
#define SPEC_HEX_OP_RST_28 0xEF
#define SPEC_HEX_OP_RET_P 0xF0
#define SPEC_HEX_OP_POP_AF 0xF1
#define SPEC_HEX_OP_JP_P_HHLL 0xF2
#define SPEC_HEX_OP_DI 0xF3
#define SPEC_HEX_OP_CALL_P_HHLL 0xF4
#define SPEC_HEX_OP_PUSH_AF 0xF5
#define SPEC_HEX_OP_OR_NN 0xF6
#define SPEC_HEX_OP_RST_30 0xF7
#define SPEC_HEX_OP_RET_M 0xF8
#define SPEC_HEX_OP_LD_SP_HL 0xF9
#define SPEC_HEX_OP_JP_M_HHLL 0xFA
#define SPEC_HEX_OP_EI 0xFB
#define SPEC_HEX_OP_CALL_M_HHLL 0xFC
#define SPEC_HEX_OP_FD_Opcodes 0xFD
#define SPEC_HEX_OP_CP_NN 0xFE
#define SPEC_HEX_OP_RST_38 0xFF


// flag comments from https://github.com/Dotneteer/spectnetide/blob/master/Core/Spect.Net.SpectrumEmu/Cpu/FlagsSetMask.cs
// mit license

/**
* The Sign Flag (S) stores the state of the most-significant bit of
* the Accumulator (bit 7). When the Z80 CPU performs arithmetic 
* operations on signed numbers, the binary twos complement notation 
* is used to represent and process numeric information.
*/
#define SPEC_FLAG_SIGN 0x80
#define SPEC_FLAG_SIGN_INDEX 7

/**
* The Zero Flag is set (1) or cleared (0) if the result generated by 
* the execution of certain instructions is 0. For 8-bit arithmetic and 
* logical operations, the Z flag is set to a 1 if the resulting byte in 
* the Accumulator is 0. If the byte is not 0, the Z flag is reset to 0.
*/
#define SPEC_FLAG_ZERO 0x40
#define SPEC_FLAG_ZERO_INDEX 6

#define SPEC_FLAG_UNUSED_5 0x20
#define SPEC_FLAG_UNUSED_5_INDEX 5

/**
* The Half Carry Flag (H) is set (1) or cleared (0) depending on the 
* Carry and Borrow status between bits 3 and 4 of an 8-bit arithmetic 
* operation. This flag is used by the Decimal Adjust Accumulator (DAA) 
* instruction to correct the result of a packed BCD add or subtract operation.
*/
#define SPEC_FLAG_HALF_CARRY 0x10
#define SPEC_FLAG_HALF_CARRY_INDEX 4

#define SPEC_FLAG_UNUSED_3 0x08
#define SPEC_FLAG_UNUSED_3_INDEX 3

/**
* The Parity/Overflow (P/V) Flag is set to a specific state depending on 
* the operation being performed. For arithmetic operations, this flag 
* indicates an overflow condition when the result in the Accumulator is 
* greater than the maximum possible number (+127) or is less than the 
* minimum possible number (–128). This overflow condition is determined by 
* examining the sign bits of the operands.
*/
#define SPEC_FLAG_PARITY_OVERFLOW 0x04
#define SPEC_FLAG_PARITY_OVERFLOW_INDEX 2

/**
* The Add/Subtract Flag (N) is used by the Decimal Adjust Accumulator 
* instruction (DAA) to distinguish between the ADD and SUB instructions.
* For ADD instructions, N is cleared to 0. For SUB instructions, N is set to 1.
*/
#define SPEC_FLAG_ADD_SUB 0x02
#define SPEC_FLAG_ADD_INDEX 1

/**
 * The Carry Flag (C) is set or cleared depending on the operation being performed.
*/
#define SPEC_FLAG_CARRY 0x01
#define SPEC_FLAG_CARRY_INDEX 0

#define SPEC_CPU_CYCLES(x) (x)

#define SPEC_MK_16(upper, lower) (((upper) << 8) | lower)
#define SPEC_MK_16b(upper, lower) ((lower & 0xff) | ((upper) << 8))
#define SPEC_MK_16c(upper, lower) (((upper) & 0xff00) | (lower))

#define SPEC_M_MEM_WRITE(addr, val) { \
if(1); \
if ((addr) >= 0x5B00) \
{ \
    ptr_spectrum_roms[(addr)] = val; \
} \
else if ((addr) >= 0x5800) \
{ \
    ptr_spectrum_roms[(addr)] = val; \
    ptr_300alloc[(addr) - 0x5800] = 1; \
} \
else if ((addr) >= 0x4000) \
{ \
    ptr_spectrum_roms[(addr)] = val; \
    ptr_300alloc[(((addr) & 0x1800) >> 3) | ((addr) & 0xFF)] = 1; \
} \
}


/*****
 * 
 * Some of these macros depend on local variables that are only defined in the main function:
 * SPEC_XM_GET_INDEX_R()
 * SPEC_XM_SET_INDEX_R(x)
 * 
*/

#define SPEC_M_INC(r1, flag_dest) \
{ \
r1++;  \
flag_dest = \
    (flag_dest & 1) \
    | (r1 & (SPEC_FLAG_SIGN | SPEC_FLAG_UNUSED_5 | SPEC_FLAG_UNUSED_3)) \
    | (!((r1 & 0xF)) << SPEC_FLAG_HALF_CARRY_INDEX) \
    | ((!r1) << SPEC_FLAG_ZERO_INDEX) \
    | ((r1 == 0x80) << SPEC_FLAG_PARITY_OVERFLOW_INDEX) \
; \
}

#define SPEC_M_DEC(r1, flag_dest) \
{ \
flag_dest =  \
    (flag_dest & 1) \
    | ((!(r1 & 0xf)) << SPEC_FLAG_HALF_CARRY_INDEX) \
    | SPEC_FLAG_ADD_SUB \
    , \
r1--, \
flag_dest |= \
     (r1 & (SPEC_FLAG_SIGN | SPEC_FLAG_UNUSED_5 | SPEC_FLAG_UNUSED_3)) \
    | ((r1 == 0x7f) << SPEC_FLAG_PARITY_OVERFLOW_INDEX) \
    | ((!r1) << SPEC_FLAG_ZERO_INDEX) \
; \
}

#define SPEC_M_ADD(src, val, flag_dest) \
{ \
    u16 res; \
    u8 other; \
    other = val; \
    res = src + other; \
    flag_dest = \
        (res & (SPEC_FLAG_SIGN | SPEC_FLAG_UNUSED_5 | SPEC_FLAG_UNUSED_3)) \
        | (res >> 8) \
        | ((((src & 0xF) + (other & 0xF)) > 0xF) << SPEC_FLAG_HALF_CARRY_INDEX) \
        | ((((~src) ^ other) & 0x80 & (res ^ src)) >> 5) \
        ; \
    src = (u8)res; \
    flag_dest |= ((!src) << SPEC_FLAG_ZERO_INDEX); \
}

#define SPEC_M_ED_ADD_16(sp1_reg16, r1, r2, r3, r4, flag_dest) \
{ \
sp1_reg16 = r2 + r4; \
r2 = (u8)sp1_reg16; \
sp1_reg16 = (u32)(sp1_reg16) >> 8; \
flag_dest = \
    ((((sp1_reg16) + (r1 & 0xF) + (r3 & 0xF)) > 0xF) << SPEC_FLAG_HALF_CARRY_INDEX) \
    | (flag_dest & (SPEC_FLAG_SIGN | SPEC_FLAG_ZERO | SPEC_FLAG_PARITY_OVERFLOW)) \
    ; \
sp1_reg16 += (r1 + r3); \
r1 = (u8)sp1_reg16; \
flag_dest |= \
    (r1 & (SPEC_FLAG_UNUSED_5 | SPEC_FLAG_UNUSED_3)) \
    | (sp1_reg16 >> 8) \
    ; \
}

#define SPEC_M_ED_ADD_16_INDEXED(r1, r2, flag_dest) \
{ \
u32 temp; \
temp = SPEC_XM_GET_INDEX_R(); \
flag_dest = \
    (flag_dest & (SPEC_FLAG_SIGN | SPEC_FLAG_ZERO | SPEC_FLAG_PARITY_OVERFLOW)) \
    | (((u32) ((temp & 0xFFF) + SPEC_MK_16(r1, r2)) > 0xFFFU) << SPEC_FLAG_HALF_CARRY_INDEX) \
    ; \
temp += SPEC_MK_16(r1, r2); \
SPEC_XM_SET_INDEX_R(temp); \
flag_dest |= \
    ((temp >> 8) & (SPEC_FLAG_UNUSED_5 | SPEC_FLAG_UNUSED_3)) \
    | (temp >> 0x10); \
; \
}

#define SPEC_M_ED_ADD_16_b(r1, r2, r3, r4, flag_dest) \
{ \
u16 temp; \
temp = r2 + (r4); \
r2 = (u8)temp; \
temp = (u32)(temp) >> 8; \
flag_dest = \
    ((((temp) + (r1 & 0xF) + (((r3) >> 8) & 0xF)) > 0xF) << SPEC_FLAG_HALF_CARRY_INDEX) \
    | (flag_dest & (SPEC_FLAG_SIGN | SPEC_FLAG_ZERO | SPEC_FLAG_PARITY_OVERFLOW)) \
    ; \
temp += (r1 + (r3)); \
r1 = (u8)temp; \
flag_dest |= \
    (r1 & (SPEC_FLAG_UNUSED_5 | SPEC_FLAG_UNUSED_3)) \
    | (temp >> 8) \
    ; \
}


#define SPEC_M_ED_ADD_16_b_ALIGN32(r1, r2, r3, r4, flag_dest) \
{ \
struct reg16_2 temp; \
temp.upperhalf.half = r2 + (r4); \
r2 = (u8)temp.upperhalf.half; \
temp.upperhalf.half >>= 8; \
flag_dest = \
    ((((temp.upperhalf.half) + (r1 & 0xF) + (((r3)) & 0xF) ) > 0xF) << SPEC_FLAG_HALF_CARRY_INDEX) \
    | (flag_dest & (SPEC_FLAG_SIGN | SPEC_FLAG_ZERO | SPEC_FLAG_PARITY_OVERFLOW)) \
    ; \
temp.upperhalf.half += (r1 + (r3)); \
r1 = (u8)temp.upperhalf.half; \
flag_dest |= \
    (r1 & (SPEC_FLAG_UNUSED_5 | SPEC_FLAG_UNUSED_3)) \
    | (temp.upperhalf.half >> 8) \
    ; \
}

#define SPEC_M_ED_ADD_16_c_ALIGN32(sp1_reg16_2, r1, r2, r3, r4, flag_dest) \
{ \
sp1_reg16_2.upperhalf.half = r2 + r4; \
r2 = sp1_reg16_2.upperhalf.upper; \
sp1_reg16_2.upperhalf.half = (u32)(sp1_reg16_2.upperhalf.half) >> 8; \
flag_dest = \
    ((((sp1_reg16_2.upperhalf.half) + (r1 & 0xF) + (r3 & 0xF)) > 0xF) << SPEC_FLAG_HALF_CARRY_INDEX) \
    | (flag_dest & (SPEC_FLAG_SIGN | SPEC_FLAG_ZERO | SPEC_FLAG_PARITY_OVERFLOW)) \
    ; \
sp1_reg16_2.upperhalf.half += (r1 + r3); \
r1 = sp1_reg16_2.upperhalf.upper; \
flag_dest |= \
    (r1 & (SPEC_FLAG_UNUSED_5 | SPEC_FLAG_UNUSED_3)) \
    | (sp1_reg16_2.upperhalf.half >> 8) \
    ; \
}

#define SPEC_M_SUB(src, val, flag_dest) \
{ \
    u16 res; \
    u8 other; \
    other = val; \
    res = (src - other) & 0x1ff; \
    flag_dest = \
        (res & (SPEC_FLAG_SIGN | SPEC_FLAG_UNUSED_5 | SPEC_FLAG_UNUSED_3)) \
        | (res >> 8) \
        | (((src & 0xF) < (other & 0xF)) << SPEC_FLAG_HALF_CARRY_INDEX) \
        | (((src ^ other) & 0x80 & (res ^ src)) >> 5) \
        | SPEC_FLAG_ADD_SUB \
        ; \
    src = (u8)res; \
    flag_dest |= ((!src) << SPEC_FLAG_ZERO_INDEX); \
}

#define SPEC_M_ADC(src, val, flag_dest) \
{ \
    u16 res; \
    u8 other; \
    other = val; \
    res = src + other + (flag_dest & 1); \
    flag_dest = \
        (res & (SPEC_FLAG_SIGN | SPEC_FLAG_UNUSED_5 | SPEC_FLAG_UNUSED_3)) \
        | (res >> 8) \
        | ((((src & 0xF) + (other & 0xF) + (flag_dest & 1)) > 0xF) << SPEC_FLAG_HALF_CARRY_INDEX) \
        | ((((~src) ^ other) & 0x80 & (res ^ src)) >> 5) \
        ; \
    src = (u8)res; \
    flag_dest |= ((!src) << SPEC_FLAG_ZERO_INDEX); \
}

#define SPEC_M_XOR(src, val, p, flag_dest) \
{ \
    src ^= val; \
    flag_dest = p[src] \
        | ( (src & (SPEC_FLAG_SIGN | SPEC_FLAG_UNUSED_5 | SPEC_FLAG_UNUSED_3)) \
        | ((!src) << SPEC_FLAG_ZERO_INDEX) ) \
    ; \
}

#define SPEC_M_OR(src, val, p, flag_dest) \
{ \
    src |= val; \
    flag_dest = p[src] \
        | ( (src & (SPEC_FLAG_SIGN | SPEC_FLAG_UNUSED_5 | SPEC_FLAG_UNUSED_3)) \
        | ((!src) << SPEC_FLAG_ZERO_INDEX) ) \
    ; \
}

#define SPEC_M_DEC_FROM_MEM(var, val, flag_dest) \
{ \
var = val; \
flag_dest =  \
    (flag_dest & 1) \
    | ((!(var & 0xf)) << SPEC_FLAG_HALF_CARRY_INDEX) \
    | SPEC_FLAG_ADD_SUB \
    , \
var--, \
flag_dest |= \
     (var & (SPEC_FLAG_SIGN | SPEC_FLAG_UNUSED_5 | SPEC_FLAG_UNUSED_3)) \
    | ((var == 0x7f) << SPEC_FLAG_PARITY_OVERFLOW_INDEX) \
    | ((!var) << SPEC_FLAG_ZERO_INDEX) \
; \
}

#define SPEC_M_SCB(u8_sp1, reg16_sp2, src, val, flag_dest) \
{ \
    u8_sp1 = val; \
    reg16_sp2 = (src - u8_sp1 - (flag_dest & 1)) & 0x1ff; \
    flag_dest = \
        (reg16_sp2 & (SPEC_FLAG_SIGN | SPEC_FLAG_UNUSED_5 | SPEC_FLAG_UNUSED_3)) \
        | (reg16_sp2 >> 8) \
        | (((src & 0xF) < ((u8_sp1 & 0xF) + (flag_dest & 1))) << SPEC_FLAG_HALF_CARRY_INDEX) \
        | (((src ^ u8_sp1) & 0x80 & (reg16_sp2 ^ src)) >> 5) \
        | SPEC_FLAG_ADD_SUB \
        ; \
    src = (u8)reg16_sp2; \
    flag_dest |= ((!src) << SPEC_FLAG_ZERO_INDEX); \
}

#define SPEC_M_SCB_ALIGN32(src, val, flag_dest) \
{ \
    struct reg16_2 res; \
    res.lowerhalf.upper = val; \
    res.upperhalf.half = (src - res.lowerhalf.upper - (flag_dest & 1)) & 0x1ff; \
    flag_dest = \
        (res.upperhalf.half & (SPEC_FLAG_SIGN | SPEC_FLAG_UNUSED_5 | SPEC_FLAG_UNUSED_3)) \
        | (res.upperhalf.half >> 8) \
        | (((src & 0xF) < ((res.lowerhalf.upper & 0xF) + (flag_dest & 1))) << SPEC_FLAG_HALF_CARRY_INDEX) \
        | (((src ^ res.lowerhalf.upper) & 0x80 & (res.upperhalf.half ^ src)) >> 5) \
        | SPEC_FLAG_ADD_SUB \
        ; \
    src = res.upperhalf.upper; \
    flag_dest |= ((!src) << SPEC_FLAG_ZERO_INDEX); \
}

#define SPEC_M_NEG(src, flag_dest) \
{ \
    src  = -(s32) src; \
    flag_dest = (src & (SPEC_FLAG_SIGN | SPEC_FLAG_UNUSED_5 | SPEC_FLAG_UNUSED_3)) \
        | ((!src) << SPEC_FLAG_ZERO_INDEX) \
        | (((src & 0xF) > 0) << SPEC_FLAG_HALF_CARRY_INDEX) \
        | ((src == 0x80) << SPEC_FLAG_PARITY_OVERFLOW_INDEX) \
        | SPEC_FLAG_ADD_SUB \
        | ((s32) src > 0); \
}

#define SPEC_M_COMPARE(src, val, flag_dest) \
{ \
    u16 res; \
    u8 other; \
    other = val; \
    res = (src - other) & 0x1ff; \
    flag_dest = \
        (res & (SPEC_FLAG_SIGN | SPEC_FLAG_UNUSED_5 | SPEC_FLAG_UNUSED_3)) \
        | (res >> 8) \
        | (((src & 0xF) < (other & 0xF)) << SPEC_FLAG_HALF_CARRY_INDEX) \
        | ( ((src ^ other) & 0x80 & (res ^ src) ) >> 5) \
        | SPEC_FLAG_ADD_SUB \
        | ((!(res)) << SPEC_FLAG_ZERO_INDEX); \
}

#define SPEC_M_CB_RLC(src, val, flag_dest) \
{ \
    src = ((src << 1) | (src >> 7)); \
    flag_dest = (val \
        | ((src & 1) \
        | (src & (SPEC_FLAG_SIGN | SPEC_FLAG_UNUSED_5 | SPEC_FLAG_UNUSED_3)) \
        | ((!src) << SPEC_FLAG_ZERO_INDEX)) \
    ); \
}

#define SPEC_M_CB_RRC(src, val, flag_dest) \
{ \
    u8 temp = (src & 1); \
    src = ((src >> 1) | (temp << 7)); \
    flag_dest = (val \
        | ((temp) \
        | (src & (SPEC_FLAG_SIGN | SPEC_FLAG_UNUSED_5 | SPEC_FLAG_UNUSED_3)) \
        | ((!src) << SPEC_FLAG_ZERO_INDEX)) \
    ); \
}

#define SPEC_M_CB_RL(src, val, flag_dest) \
{ \
    u8 temp = (src >> 7); \
    src = ((src << 1) | (flag_dest & 1)); \
    flag_dest = (val \
        | (( (temp) \
        | src & (SPEC_FLAG_SIGN | SPEC_FLAG_UNUSED_5 | SPEC_FLAG_UNUSED_3)) \
        | ((!src) << SPEC_FLAG_ZERO_INDEX) ) \
    ); \
}

#define SPEC_M_CB_RL2(src, val, flag_dest) \
{ \
    u8 temp = (src >> 7); \
    src = ((src << 1) | (flag_dest & 1)); \
    flag_dest = (val \
        | ((temp) \
        | (src & (SPEC_FLAG_SIGN | SPEC_FLAG_UNUSED_5 | SPEC_FLAG_UNUSED_3)) \
        | ((!src) << SPEC_FLAG_ZERO_INDEX)) \
    ); \
}

#define SPEC_M_CB_RR(src, val, flag_dest) \
{ \
    u8 temp = (src & 1); \
    src = ((src >> 1) | (flag_dest << 7)); \
    flag_dest = (val \
        | ( (temp) \
        | (src & (SPEC_FLAG_SIGN | SPEC_FLAG_UNUSED_5 | SPEC_FLAG_UNUSED_3)) \
        | ((!src) << SPEC_FLAG_ZERO_INDEX) ) \
    ); \
}

#define SPEC_M_CB_RR2(src, val, flag_dest) \
{ \
    u8 temp = (src & 1); \
    src = ((src >> 1) | (flag_dest << 7)); \
    flag_dest = (val \
        | ((temp) \
        | (src & (SPEC_FLAG_SIGN | SPEC_FLAG_UNUSED_5 | SPEC_FLAG_UNUSED_3)) \
        | ((!src) << SPEC_FLAG_ZERO_INDEX)) \
    ); \
}

#define SPEC_M_CB_SLA(src, val, flag_dest) \
{ \
    u8 temp = (src >> 7); \
    src = (src << 1); \
    flag_dest = (val \
        | ( (temp) \
        | (src & (SPEC_FLAG_SIGN | SPEC_FLAG_UNUSED_5 | SPEC_FLAG_UNUSED_3)) \
        | ((!src) << SPEC_FLAG_ZERO_INDEX) ) \
    ); \
}

#define SPEC_M_CB_SRA(src, val, flag_dest) \
{ \
    u8 temp = (src & 1); \
    src = ((s8)src >> 1); \
    flag_dest = (val \
        | ( (temp) \
        | (src & (SPEC_FLAG_SIGN | SPEC_FLAG_UNUSED_5 | SPEC_FLAG_UNUSED_3)) \
        | ((!src) << SPEC_FLAG_ZERO_INDEX) ) \
    ); \
}

#define SPEC_M_CB_SLS(src, val, flag_dest) \
{ \
    u8 temp = (src >> 7); \
    src = ((src << 1) | 1); \
    flag_dest = (val \
        | ( (temp) \
        | (src & (SPEC_FLAG_SIGN | SPEC_FLAG_UNUSED_5 | SPEC_FLAG_UNUSED_3)) \
        | ((!src) << SPEC_FLAG_ZERO_INDEX) ) \
    ); \
}

#define SPEC_M_CB_SRL(src, val, flag_dest) \
{ \
    u8 temp = (src & 1); \
    src = ((u32)src >> 1); \
    flag_dest = (val \
        | ( (temp) \
        | (src & (SPEC_FLAG_SIGN | SPEC_FLAG_UNUSED_5 | SPEC_FLAG_UNUSED_3)) \
        | ((!src) << SPEC_FLAG_ZERO_INDEX) ) \
    ); \
}

#define SPEC_M_ED_IN(r1, r2, r3, clock, flag_dest) \
{ \
u16 res; \
res = spectrum_input_handling(clock, r1, r2); \
r3 = res; \
clock += res >> 8; \
flag_dest = \
    ptr_pc_keyboard_table_alloc[r3] \
    | ( (flag_dest & 1) \
    | (r3 & (SPEC_FLAG_SIGN | SPEC_FLAG_UNUSED_5 | SPEC_FLAG_UNUSED_3)) \
    | (!r3 << SPEC_FLAG_ZERO_INDEX) ) \
    ; \
}

#define SPEC_M_ED_SBC(sp1_u16, sp2_reg16_2, r1, r2, r3, r4, flag_dest) \
{ \
sp1_u16 = SPEC_MK_16(r3, r4); \
sp2_reg16_2.word = ((SPEC_MK_16(r1, r2) - sp1_u16) - (flag_dest & 1)) & 0x1ffff; \
flag_dest = \
    ((sp2_reg16_2.word >> 8) & (SPEC_FLAG_SIGN | SPEC_FLAG_UNUSED_5 | SPEC_FLAG_UNUSED_3)) \
    | (sp2_reg16_2.word >> 0x10) \
    | SPEC_FLAG_ADD_SUB \
    | (((SPEC_MK_16(r1, r2) & 0xfff) < ((sp1_u16 & 0xfff) + (flag_dest & 1))) << SPEC_FLAG_HALF_CARRY_INDEX) \
    | (((SPEC_MK_16(r1, r2) ^ sp1_u16) & (SPEC_MK_16(r1, r2) ^ sp2_reg16_2.word) & 0x8000 ) >> 0xd) \
    | ((!(sp2_reg16_2.word & 0xffff)) << SPEC_FLAG_ZERO_INDEX) \
    | SPEC_FLAG_ADD_SUB \
    ; \
r1 = (u32)sp2_reg16_2.word >> 8; \
r2 = sp2_reg16_2.upperhalf.upper; \
}

#define SPEC_M_ED_ADC(r1, r2, r3, r4, flag_dest) \
{ \
u16 other; \
struct reg16_2 res; \
other = SPEC_MK_16(r3, r4); \
res.word = ((SPEC_MK_16(r1, r2) + other) + (flag_dest & 1)); \
flag_dest = \
    ((res.word >> 8) & (SPEC_FLAG_SIGN | SPEC_FLAG_UNUSED_5 | SPEC_FLAG_UNUSED_3)) \
    | (res.word >> 0x10) \
    | ((((SPEC_MK_16(r1, r2) & 0xfff) + (other & 0xfff) + (flag_dest & 1)) > 0xfff) << SPEC_FLAG_HALF_CARRY_INDEX) \
    | ((((~SPEC_MK_16(r1, r2)) ^ other) & (SPEC_MK_16(r1, r2) ^ res.word) & 0x8000 ) >> 0xd) \
    | ((!(res.word & 0xffff)) << SPEC_FLAG_ZERO_INDEX) \
    | SPEC_FLAG_ADD_SUB \
    ; \
r1 = (u32)res.word >> 8; \
r2 = res.upperhalf.upper; \
}






struct reg16 {
    union {
        u16 half;
        struct {
            u8 lower;
            u8 upper;
        };
    };
};

struct reg16_2 {
    union {
        u32 word;
        struct {
            struct reg16 lowerhalf;
            struct reg16 upperhalf;
        };
    };
};

