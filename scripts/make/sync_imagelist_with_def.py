#!/usr/bin/env python3
"""
Sync imagelist.u.csv with images.def:
1. Read names from images.def (in order)
2. Read offsets/sizes from imagelist.u.csv 
3. Generate new imagelist.csv with proper names and ROM offsets
4. Update images.def with actual sizes from CSV
"""

import sys
from pathlib import Path

def parse_images_def(images_def_path):
    """Parse images.def and extract IMAGE names in order"""
    image_names = []
    
    with open(images_def_path, 'r') as f:
        for line in f:
            line = line.strip()
            if line.startswith('IMAGE('):
                # Extract: IMAGE(NAME, SIZE, ...)
                content = line[6:-1]  # Remove "IMAGE(" and ")"
                parts = [p.strip() for p in content.split(',', 2)]
                
                if len(parts) >= 1:
                    name = parts[0]
                    image_names.append(name)
    
    return image_names

def read_old_imagelist(csv_path):
    """Read old imagelist.u.csv and extract offset/size pairs"""
    entries = []
    
    with open(csv_path, 'r') as f:
        for line in f:
            line = line.strip()
            if line:
                parts = line.split(',')
                if len(parts) >= 2:
                    offset = int(parts[0])
                    size = int(parts[1])
                    entries.append({'offset': offset, 'size': size})
    
    return entries

def generate_named_imagelist(image_names, old_entries, output_csv, split_dir):
    """Generate new imagelist.csv with proper names and actual sizes
    
    Uses CSV offsets for ROM images, but recalculates ALL sizes from actual files.
    CSV offsets preserved (for ROM layout), but sizes updated from disk.
    images.def is the complete database - CSV is only for initial extraction offsets.
    """
    import os
    
    rom_image_count = len(old_entries)
    total_images = len(image_names)
    
    if total_images < rom_image_count:
        print(f"WARNING: images.def has {total_images} images but CSV has {rom_image_count} entries")
        print(f"Using first {total_images} entries from CSV")
        rom_image_count = total_images
    elif total_images > rom_image_count:
        print(f"INFO: images.def has {total_images} images, CSV has {rom_image_count} (ROM) entries")
        print(f"Will calculate offsets for {total_images - rom_image_count} new images")
    
    # Start offset calculation from beginning
    current_offset = old_entries[0]['offset'] if rom_image_count > 0 else 0
    
    with open(output_csv, 'w') as f:
        # Write ROM images with CSV offsets but recalculated sizes
        for i in range(rom_image_count):
            name = image_names[i]
            path = f"{split_dir}/{name}.bin"
            
            # Use ROM offset from CSV
            offset = old_entries[i]['offset']
            
            # Get actual file size (recalculate, don't trust CSV)
            if os.path.exists(path):
                size = os.path.getsize(path)
            else:
                # Fall back to CSV size if file missing
                size = old_entries[i]['size']
                print(f"WARNING: {path} not found, using CSV size 0x{size:X}")
            
            f.write(f"{offset},{size},{path},0,1\n")
            
            # Track offset for new images
            if i == rom_image_count - 1:
                current_offset = offset + size
        
        # Write new images with calculated offsets and actual sizes
        for i in range(rom_image_count, total_images):
            name = image_names[i]
            path = f"{split_dir}/{name}.bin"
            
            # Get actual file size
            if os.path.exists(path):
                size = os.path.getsize(path)
            else:
                # Default size if file doesn't exist yet
                size = 0x1000
                print(f"WARNING: {path} not found, using default size 0x{size:X}")
            
            f.write(f"{current_offset},{size},{path},0,1\n")
            current_offset += size
    
    print(f"Generated {output_csv} with {total_images} entries ({rom_image_count} from ROM, {total_images - rom_image_count} new)")
    print(f"All sizes recalculated from actual .bin files (CSV offsets preserved for ROM images)")
    return total_images

def update_images_def_sizes(images_def_path, sizes):
    """Update images.def with actual sizes from CSV"""
    
    lines = []
    image_index = 0
    
    with open(images_def_path, 'r') as f:
        for line in f:
            if line.strip().startswith('IMAGE('):
                # Extract and rebuild with new size
                content = line[line.index('(')+1:line.rindex(')')]
                parts = [p.strip() for p in content.split(',')]
                
                if len(parts) >= 2 and image_index < len(sizes):
                    # Update size (keep as hex)
                    new_size = sizes[image_index]
                    parts[1] = f"0x{new_size:X}"
                    
                    # Rebuild line
                    new_content = ', '.join(parts)
                    line = f"IMAGE({new_content})\n"
                    image_index += 1
            
            lines.append(line)
    
    # Write back
    with open(images_def_path, 'w') as f:
        f.writelines(lines)
    
    print(f"Updated {image_index} image sizes in {images_def_path}")

def main():
    if len(sys.argv) < 2:
        print("Usage: sync_imagelist_with_def.py <output_csv> [--update-def]")
        print("Example: sync_imagelist_with_def.py build/u/imagelist.csv --update-def")
        sys.exit(1)
    
    output_csv = sys.argv[1]
    update_def = '--update-def' in sys.argv
    
    # Paths
    images_def = 'assets/images.def'
    old_csv = 'imagelist.u.csv'
    split_dir = 'assets/images/split'
    
    # Read data
    print(f"Reading image names from {images_def}...")
    image_names = parse_images_def(images_def)
    print(f"Found {len(image_names)} image names")
    
    print(f"\nReading ROM offsets/sizes from {old_csv}...")
    old_entries = read_old_imagelist(old_csv)
    print(f"Found {len(old_entries)} ROM entries (for initial extraction)")
    
    # Generate new CSV with names
    print(f"\nGenerating {output_csv}...")
    count = generate_named_imagelist(image_names, old_entries, output_csv, split_dir)
    
    # Optionally update images.def with sizes
    if update_def:
        print(f"\nUpdating {images_def} with sizes from CSV...")
        sizes = [entry['size'] for entry in old_entries[:len(image_names)]]
        update_images_def_sizes(images_def, sizes)
        print("✓ images.def updated")
    else:
        print("\nSkipping images.def update (use --update-def to enable)")
    
    print(f"\n✓ Done! Generated {count} entries")

if __name__ == '__main__':
    main()
