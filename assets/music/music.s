# assembler directives
.set noat      # allow manual use of $at
.set noreorder # don't insert nops after branches


.section .music

#calculates and stores the total number of music samples
.global _musicsampletblSegmentRomStart
_musicsampletblSegmentRomStart:
.global number_music_samples
number_music_samples:
 .half (table_music_data_end - number_music_samples)/8
 .half 0x0000
number_music_samples_end:



.macro music_table_entry name sized
 .word \name - number_music_samples
 .half end_d_\name - d_\name
 .half end_\name - \name
.endm

.global table_music_data
table_music_data:
 music_table_entry Mno_music
 music_table_entry Msolo_death_abrev
 music_table_entry Mintro_eye
 music_table_entry Mtrain
 music_table_entry Mdepot
 music_table_entry Mjungle_unused
 music_table_entry Mcitadel
 music_table_entry Mfacility
 music_table_entry Mcontrol
 music_table_entry Mdam
 music_table_entry Mfrigate
 music_table_entry Marchives
 music_table_entry Msilo
 music_table_entry Mjungle_perimeter_unused
 music_table_entry Mstreets
 music_table_entry Mbunker1
 music_table_entry Mbunker2
 music_table_entry Mstatue
 music_table_entry Melevator_control
 music_table_entry Mcradle
 music_table_entry Mnull1
 music_table_entry Melevator_wc
 music_table_entry Megyptian
 music_table_entry Mfolders
 music_table_entry Mwatchmusic
 music_table_entry Maztec
 music_table_entry Mwatercaverns
 music_table_entry Mdeathsolo
 music_table_entry Msurface2
 music_table_entry Mtrainx
 music_table_entry Mnull2
 music_table_entry Mfacilityx
 music_table_entry Mdepotx
 music_table_entry Mcontrolx
 music_table_entry Mwatercavernsx
 music_table_entry Mdamx
 music_table_entry Mfrigatex
 music_table_entry Marchivesx
 music_table_entry Msilox
 music_table_entry Mnull3
 music_table_entry Mstreetsx
 music_table_entry Mbunker1x
 music_table_entry Mbunker2x
 music_table_entry Mjunglex
 music_table_entry Mnint_rare_logo
 music_table_entry Mstatuex
 music_table_entry Maztecx
 music_table_entry Megyptianx
 music_table_entry Mcradlex
 music_table_entry Mcuba
 music_table_entry Mrunway
 music_table_entry Mrunway_plane
 music_table_entry Msurface2x
 music_table_entry Mwindblowing
 music_table_entry Mmultideath_alt
 music_table_entry Mjungle
 music_table_entry Mrunwayx
 music_table_entry Msurface1
 music_table_entry Mmultiplayerdeath
 music_table_entry Msurface1x
 music_table_entry Msurface2_ending
 music_table_entry Mstatue_ending
 music_table_entry Mfrigate_outro
table_music_data_end:
.global _musicsampletblSegmentRomEnd
_musicsampletblSegmentRomEnd:

.macro music_file name
  .section .musiccompressed
  .global \name
  \name:
  .ifdef VERSION_US
    .incbin "build\/u\/assets\/music\/\name\.rz"
  .endif
  .ifdef VERSION_JP
    .incbin "build\/j\/assets\/music\/\name\.rz"
  .endif
  .ifdef VERSION_EU
    .incbin "build\/e\/assets\/music\/\name\.rz"
  .endif
  .ifdef VERSION_DEBUG
    .incbin "build\/d\/assets\/music\/\name\.rz"
  .endif
    /* Check if file size is odd, add 0xA to pad file if needed to make it even */
    .if (. - \name) % 2 != 0
      .byte 0xA
    .endif
  end_\name:

  .section .musicdecompressed
  d_\name:
    .incbin "assets\/music\/\name\.bin"
  end_d_\name:
.endm



music_file Mno_music
music_file Msolo_death_abrev
music_file Mintro_eye
music_file Mtrain
music_file Mdepot
music_file Mjungle_unused
music_file Mcitadel
music_file Mfacility
music_file Mcontrol
music_file Mdam
music_file Mfrigate
music_file Marchives
music_file Msilo
music_file Mjungle_perimeter_unused
music_file Mstreets
music_file Mbunker1
music_file Mbunker2
music_file Mstatue
music_file Melevator_control
music_file Mcradle
music_file Mnull1
music_file Melevator_wc
music_file Megyptian
music_file Mfolders
music_file Mwatchmusic
music_file Maztec
music_file Mwatercaverns
music_file Mdeathsolo
music_file Msurface2
music_file Mtrainx
music_file Mnull2
music_file Mfacilityx
music_file Mdepotx
music_file Mcontrolx
music_file Mwatercavernsx
music_file Mdamx
music_file Mfrigatex
music_file Marchivesx
music_file Msilox
music_file Mnull3
music_file Mstreetsx
music_file Mbunker1x
music_file Mbunker2x
music_file Mjunglex
music_file Mnint_rare_logo
music_file Mstatuex
music_file Maztecx
music_file Megyptianx
music_file Mcradlex
music_file Mcuba
music_file Mrunway
music_file Mrunway_plane
music_file Msurface2x
music_file Mwindblowing
music_file Mmultideath_alt
music_file Mjungle
music_file Mrunwayx
music_file Msurface1
music_file Mmultiplayerdeath
music_file Msurface1x
music_file Msurface2_ending
music_file Mstatue_ending
music_file Mfrigate_outro

.section .musiccompressed
.half 0
.word 0


