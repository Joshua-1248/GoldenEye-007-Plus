#ifndef __CHR_B_H__
#define __CHR_B_H__

#include <ultra64.h>
#include <bondtypes.h>

s32 load_body_head_if_not_loaded(s32 model);
Model *makeonebody(s32 body, s32 head, ModelFileHeader *bodyheader, ModelFileHeader *headheader, s32 sunglasses, Model *model);
Model *setup_chr_instance(s32 body, s32 head, ModelFileHeader *body_header, ModelFileHeader *head_header, s32 sunglasses);

#endif
