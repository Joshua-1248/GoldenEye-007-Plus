#include <PR/os.h>
#include <io/controller.h>
#include <ultra64.h>

s32 __osPackEepReadData(u8);
OSPifRam __osEepPifRam;

s32 osEepromRead(OSMesgQueue *mq, u8 address, u8 *buffer) {
    s32 ret;
    s32 i;
    u8 *ptr;
    OSContStatus data;
    __OSContEepromFormat format;
#ifdef GE_SAVE_EEPROM16K
    u16 type;
#endif
    ret = 0;
    i = 0;
    ptr = (u8 *) &__osEepPifRam;
#ifdef GE_SAVE_EEPROM16K
    /*
     * V84: use the later libultra-style EEPROM transaction sequence for the
     * Plus 16-Kbit backend.  In particular, do not rewrite the RDRAM PIF
     * command buffer between the command DMA and response DMA.  This is the
     * sequence used by later 16-Kbit EEPROM titles and is friendlier to both
     * real EEPROM and flashcart Joybus emulation.
     */
    __osSiGetAccess();
    ret = __osEepStatus(mq, &data);

    if (ret == 0) {
        type = data.type & (CONT_EEPROM | CONT_EEP16K);

        switch (type) {
        case CONT_EEPROM:
            if (address >= EEPROM_MAXBLOCKS) {
                ret = -1;
            }
            break;
        case CONT_EEPROM | CONT_EEP16K:
            if ((u32)address >= EEP16K_MAXBLOCKS) {
                ret = -1;
            }
            break;
        default:
            ret = CONT_NO_RESPONSE_ERROR;
            break;
        }
    }

    if (ret != 0) {
        __osSiRelAccess();
        return ret;
    }
#else
    if (address > 0x40) {
        return -1;
    }
    __osSiGetAccess();
    ret = __osEepStatus(mq, &data);
    if (ret != 0 || data.type != CONT_EEPROM) {
        return CONT_NO_RESPONSE_ERROR;
    }
#endif
    while (data.status & CONT_EEPROM_BUSY) {
        __osEepStatus(mq, &data);
    }
    __osPackEepReadData(address);
    ret = __osSiRawStartDma(OS_WRITE, &__osEepPifRam);
    osRecvMesg(mq, NULL, OS_MESG_BLOCK);
#ifdef GE_SAVE_EEPROM16K
    ret = __osSiRawStartDma(OS_READ, &__osEepPifRam);
#else
    for (i = 0; i < 0x10; i++) {
        __osEepPifRam.ramarray[i] = 255;
    }
    __osEepPifRam.pifstatus = 0;
    ret = __osSiRawStartDma(OS_READ, &__osEepPifRam);
#endif
    __osContLastCmd = CONT_CMD_READ_EEPROM;
    osRecvMesg(mq, NULL, OS_MESG_BLOCK);
    for (i = 0; i < 4; i++) {
        ptr++;
    }
    format = *(__OSContEepromFormat *) ptr;
    ret = (format.rxsize & 0xc0) >> 4;
    if (ret == 0) {
        for (i = 0; i < 8; i++) {
            *buffer++ = ((u8 *) &format.data)[i];
        }
    }
    __osSiRelAccess();
    return ret;
}

s32 __osPackEepReadData(u8 address) {
    u8 *ptr;
    __OSContEepromFormat format;
    s32 i;
    ptr = (u8 *) &__osEepPifRam;
#ifdef GE_SAVE_EEPROM16K
    /* Later libultra builds only populate the active Joybus command. */
    __osEepPifRam.pifstatus = CONT_CMD_EXE;
    format.txsize = CONT_CMD_READ_EEPROM_TX;
    format.rxsize = CONT_CMD_READ_EEPROM_RX;
    format.cmd = CONT_CMD_READ_EEPROM;
    format.address = address;
    for (i = 0; i < 4; i++) {
        *ptr++ = 0;
    }
    *(__OSContEepromFormat *) ptr = format;
    ptr += sizeof(__OSContEepromFormat);
    *ptr = CONT_CMD_END;
#else
    for (i = 0; i < 0x10; i++) {
        __osEepPifRam.ramarray[i] = 255;
    }
    __osEepPifRam.pifstatus = 1;
    format.txsize = 2;
    format.rxsize = 8;
    format.cmd = 4;
    format.address = address;
    for (i = 0; i < 8; i++) {
        ((u8 *) &format.data)[i] = 0;
    }
    for (i = 0; i < 4; i++) {
        *ptr++ = 0;
    }
    *(__OSContEepromFormat *) ptr = format;
    ptr += 0xc;
    *ptr = 254;
#endif
#ifdef AVOID_UB
    return 0;
#endif
}
