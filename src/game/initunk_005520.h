#ifndef _INITUNK_005520_H_
#define _INITUNK_005520_H_
#include <ultra64.h>

void modelmgrSetLevelResetting(bool resetting);
void modelmgrResetSlotCounts(void);
void modelmgrAllocateModelSlots(s32 numobjs);
void modelmgrAllocateAnimModelSlots(s32 numanimated);

#endif
