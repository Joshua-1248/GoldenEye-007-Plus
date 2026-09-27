# Project Structure

```
goldeneye_src
|-- .github/workflows: GitHub use only
├── assets: game assets
│   ├── font: font data
│   ├── images: image data
│   │   └── split: split image data
│   ├── music: music data
│   ├── obseg: animation data
│   │   ├── bg: bg data
│   │   ├── brief: briefing data
│   │   ├── chr: c model data
│   │   ├── gun: g model data
│   │   ├── prop: p model data
│   │   ├── setup: setup data
│   │   ├── stan: stan data
│   │   └── text: text data
│   └── ramrom: demo data
├── bin: files that haven't been touched
├── build: output directory
├── include: header files
├── rsp: Custom GBI code (Assembly) (C0 and 4Tri)
├── src: C source code for game
│   ├── game: core ge specific code 0x7f000000 range
│   ├── inflate: statically linked initial decompression code
│   ├── libultra: currently used libultra.s
│   └── libultrarare: libultra modified by Rare
└── tools: build tools
```

Documentation has been moved to https://github.com/kholdfuzion/goldeneye_docs/tree/master/notes

Style Guide is https://github.com/kholdfuzion/goldeneye_src/blob/AIListLogic/notes/StyleGuide.md

This decompilation was only made possible thanks to many awesome 00 Agents who will be revealed only if they wish.

GE and PD documentation made by Zoinkity.
