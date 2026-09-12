#include <ultra64.h>
#include "zlib.h"

//this definately isn't proper way this data was represented, but works for now
// rodata
//D:8005BF80
const u8 rz_header_bytes[40] = {
    0x11, 0x72, 0x00, 0x00, 0x11, 0x72, 0x00, 0x00, 0x11, 0x72, 0x00, 0x00, 0x11, 0x72, 0x00, 0x00, 0x11, 0x72, 0x00, 0x00,
    0x11, 0x72, 0x00, 0x00, 0x11, 0x72, 0x00, 0x00, 0x11, 0x72, 0x00, 0x00, 0x11, 0x72, 0x00, 0x00, 0x11, 0x72, 0x00, 0x00
};

struct aaa {
    u8 unk00;
    u8 unk01;
    u8 unk02;
    u8 unk03;
    u8 unk04;
    u8 unk05;
    u8 unk06;
    u8 unk07;
    u8 unk08;
    u8 unk09;
    u8 unk0A;
    u8 unk0B;
    s32 unk0C;
    s32 unk10;
    s32 unk14;
    u8 unk18;
};


u32 decompressdata(u8 *src, u8 *dst, struct huft *huffman_table)
{
  s32 header_check;
  struct aaa *p = &rz_header_bytes;
  struct aaa *header_alias;
    
  header_check = 0;
  rz_inbuf = src;
  header_alias = &(*p);
  rz_outbuf = dst;
  rz_hlist = huffman_table;
    
  if (((header_check = src[0]) != (*header_alias).unk00) || (src[1] != (*p).unk05))
  {
    header_check = src[-1];
    if ((header_check == p->unk08) != 0)
    {
      header_check = 2;
    }
    if (src[-2] == ((*header_alias).unk18 & 0xFF))
    {
      header_check = 1;

      // Weird but needed for matching.
      if (((*header_alias).unk18) && ((*header_alias).unk18));
    }
  }
    
  if (header_check);
    
  rz_inbuf = rz_inbuf + 2;
  rz_wp = 0;
  rz_inptr = 0;
    
  zlib_inflate();

  return rz_wp;
}


s32 rzipGetSomething(void) {
    return (rz_inbuf + rz_inptr);
}



