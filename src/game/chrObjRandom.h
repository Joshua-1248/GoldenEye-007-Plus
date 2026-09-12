#ifndef _CHROBJRANDOM_H_
#define _CHROBJRANDOM_H_

#include <ultra64.h>

extern u64 g_chrObjRandomSeed;

u32 chrObjRandomGetNext(void);
void chrObjRandomSetSeed(u32 param_1);

#endif