#ifndef _RLE_H_
#define _RLE_H_

#include <ultra64.h>

void rle_expand_8bit(u8 *src, u8 *dst);
void rle_expand_rgb_to_u16_5551(u8 *src, u16 *dst);
void rle_expand_rgb_to_rgba32(u8 *src, u8 *dst);
#endif
