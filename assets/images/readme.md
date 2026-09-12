# Image Assets

## Overview

This directory contains the texture image system for GoldenEye 007.

Original image files would have been SGI .rgb files. These would be passed through a converter (rgb2c) to get CI byte arrays.

## Directory Structure

- **`split/`** - Individual texture files (2698 images)
  - Named using IMAGE enum from images.def (e.g., COPYICON.bin, X.bin)
  - Extracted from ROM using ROM offsets from imagelist.u.csv
  - Raw binary texture data

- **`combined/`** - Combined texture binary
  - `combined.bin` - All textures concatenated in ROM order
  - Padded to 16-byte (0x10) boundary
  - Size: 3075872 bytes (3075864 data + 8 padding)
  - SHA1 (unpadded): 044fca472bf6ef6691fa02ff1b65ff474d86a9fa

## Build System

The image build system synchronizes two sources:
- **`imagelist.u.csv`** (root) - ROM offsets and sizes (ground truth for positioning)
- **`assets/images.def`** - Image names and properties (ground truth for naming)

See `scripts/IMAGE_BUILD_SYSTEM.md` for detailed documentation.

## Quick Commands

```bash
# Extract images from ROM
./scripts/extract_baserom.u.sh

# Rebuild combined.bin
make assets/images/combined/combined.bin

# Full ROM build
make
```
