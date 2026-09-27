#include <ultra64.h>

s32 osEepromProbe(OSMesgQueue *mq) {
    s32 status = 0;
    OSContStatus sdata;

    __osSiGetAccess();
    status = __osEepStatus(mq, &sdata);
#ifdef GE_SAVE_EEPROM16K
    if (status == 0) {
        u16 type = sdata.type & (CONT_EEPROM | CONT_EEP16K);

        if (type == CONT_EEPROM)
            status = EEPROM_TYPE_4K;
        else if (type == (CONT_EEPROM | CONT_EEP16K))
            status = EEPROM_TYPE_16K;
        else
            status = 0;
    } else {
        status = 0;
    }
#else
    if (status == 0 && (sdata.type & CONT_EEPROM) != 0) {
        status = EEPROM_TYPE_4K;
    } else {
        status = 0;
    }
#endif
    __osSiRelAccess();
    return status;
}
