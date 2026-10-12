#!/usr/bin/env python3
"""Josh_7774 GoldenEye custom-head registration and native asset smoke audit."""
from pathlib import Path
import re,sys
R=Path(__file__).resolve().parents[1]
checks=[]
def read(path):return (R/path).read_text(errors='replace')
def ck(name,ok):
    ok=bool(ok);checks.append((name,ok));print(('[PASS] ' if ok else '[FAIL] ')+name)
C=read('src/bondconstants.h');F=read('src/game/front.c')
H=read('assets/obseg/chr/chrModelFileHeaders.inc.c')
Q=read('assets/obseg/chr/chrModelFileRecords.inc.c')
I=read('assets/obseg/file_resource_id_enums.h')
T=read('assets/obseg/file_resource_table.inc.c')
S=read('assets/obseg/ob_seg.s')
A=read('assets/obseg/obseg.h')
B=read('assets/obseg/Makefile.filelist')
D=read('assets/images.def')
M=read('assets/obseg/chr/headjosh/Model.c')
ck('append-only head ID', 'HEAD_Male_Josh = HEAD_END' in C and C.index('HEAD_END,')<C.index('HEAD_Male_Josh = HEAD_END'))
ck('both Josh_7774 character rows assigned without changing another character',
   F.count('BODY_Civilian_4, MP_JOSH_7774_HEAD, 1.0}')==2 and
   '#define MP_JOSH_7774_HEAD HEAD_Male_Josh' in F and
   '#define MP_JOSH_7774_HEAD HEAD_Male_Dwayne' in F)
ck('custom head record is appended before existing model sentinel',
   'headjosh/modelFileHeader.inc.c' in H and
   'headjosh/chrModelFileRecord.inc.c' in Q and
   Q.index('headjosh/chrModelFileRecord.inc.c')<Q.rfind('{0, 0, 1.0, 0.0'))
ck('matching append-only resource index/table/ROM layout',
   I.index('MP_SETUPCOURTYARD,')<I.index('HEADJOSH,')<I.index('OBENDSEG') and
   '{HEADJOSH, "CheadjoshZ", &CheadjoshZ}' in T and
   S.index('Ump_setupcourtyardZ:')<S.index('obseg_file_Z chr, CheadjoshZ')<S.index('.global ob__ob_end_seg'))
ck('head resource build gated to MODDED_CHEATS',
   'ifeq ($(MODDED_CHEATS),YES)\nCHRNAMELIST += headjosh\nendif' in B)
ck('new head ROM resource has a compiler-visible declaration',
   '#ifdef GE_MODDED_CHEATS\nextern u8 bg_courtyard_all_p_seg[];' in A and
   'extern u8 CheadjoshZ[];' in A and
   A.index('extern u8 Ump_setupcourtyardZ[];') < A.index('extern u8 CheadjoshZ[];') < A.index('#endif', A.index('extern u8 Ump_setupcourtyardZ[];')))
ck('three new texture IDs appended after existing Plus textures',
   D.index('IMAGE(MP_COURTYARD,')<D.index('IMAGE(JOSH_7774_FRONT,')<D.index('IMAGE(JOSH_7774_LEFT,')<D.index('IMAGE(JOSH_7774_RIGHT,'))
ck('independent three-texture native model with retail eyes/auxiliary assets unchanged',
   all('IMAGE_JOSH_7774_'+n in M for n in ('FRONT','LEFT','RIGHT')) and
   all('IMAGE_'+n in M for n in ('1995','1996')) and
   all('IMAGE_'+n not in M for n in ('1833','1834','1835')))
ck('GoldenEye 16-vertex/4-bit-index cache respected',
   max(int(v) for v in re.findall(r'gsSPVertex\(0x0400[0-9A-Fa-f]{4},\s*(\d+)',M))<=16)
ck('native draw model retains exactly 53 source triangles',
   sum(int(k) for k in re.findall(r'gsSP([124])Triangles?\(',M.split('Gfx GFX_PRIMARY_0xc30[] = ')[0]))==53 and
   re.findall(r'#define VERTEXGROUPCOUNT[012] (\d+)',M)==['110','11','20'])
ck('matching model header and file record',
   'MODELFILEHEADER(headjosh, 0, NULL, 0, 2, 1, 218.35564, 0, 5)' in read('assets/obseg/chr/headjosh/modelFileHeader.inc.c') and
   '"CheadjoshZ"' in read('assets/obseg/chr/headjosh/chrModelFileRecord.inc.c'))
for n in ('FRONT','LEFT','RIGHT'):
    b=(R/f'assets/images/split/JOSH_7774_{n}.bin').read_bytes()
    # Decoder reads exactly 2048 x 15-bit samples, as texReadUncompressed(RGB15) does.
    ok=len(b)==3844 and b[:4]==bytes([1,0x32,0x04,0x00])
    if ok:
        bits=nbits=0;off=4
        for i in range(2048):
            while nbits<15:
                bits=(bits<<8)|b[off];off+=1;nbits+=8
            nbits-=15
            sample=(bits>>nbits)&0x7fff
            bits &= (1<<nbits)-1
        ok=off==len(b) and nbits==0
    ck(f'{n} RGB15 GoldenEye texture validates',ok)
failed=[name for name,ok in checks if not ok]
print(f"\nJOSH_7774 R20 HEAD AUDIT: {'PASS' if not failed else 'FAIL'} ({len(checks)-len(failed)}/{len(checks)})")
sys.exit(bool(failed))
