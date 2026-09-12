#!/usr/bin/env python3
"""
Generate imagelist.csv from images.def (ground truth).
Reads images.def to get image names and order, gets sizes from actual files,
generates offsets automatically. Also updates images.def with calculated sizes.
"""

import sys
from pathlib import Path

def parse_images_def(images_def_path):
    """Parse images.def and extract IMAGE names and sizes in order"""
    image_entries = []
    
    with open(images_def_path, 'r') as f:
        for line in f:
            line = line.strip()
            if line.startswith('IMAGE('):
                # Extract: IMAGE(NAME, SIZE, ...)
                content = line[6:-1]  # Remove "IMAGE(" and ")"
                parts = [p.strip() for p in content.split(',', 2)]
                
                if len(parts) >= 2:
                    name = parts[0]
                    size_str = parts[1]
                    # Parse hex size (0xHEX format)
                    if size_str.startswith('0x'):
                        size = int(size_str, 16)
                    else:
                        size = int(size_str)
                    image_entries.append({'name': name, 'size': size})
    
    return image_entries

def get_image_size(image_path):
    """Get size of image file, return 0 if doesn't exist"""
    path = Path(image_path)
    if path.exists():
        return path.stat().st_size
    else:
        return 0

def generate_imagelist(images_def, split_dir, output_csv, update_def=True):
    """Generate imagelist.csv with offsets and sizes, optionally update images.def"""
    
    image_entries = parse_images_def(images_def)
    
    print(f"Found {len(image_entries)} images in images.def")
    
    # Calculate offsets and actual sizes
    current_offset = 0
    entries = []
    missing_files = []
    size_mismatches = []
    
    for entry in image_entries:
        name = entry['name']
        def_size = entry['size']
        image_path = f"{split_dir}/{name}.bin"
        actual_size = get_image_size(image_path)
        
        if actual_size == 0:
            missing_files.append(name)
            # Use size from .def if file missing
            size_to_use = def_size
        else:
            size_to_use = actual_size
            # Check for mismatch
            if actual_size != def_size:
                size_mismatches.append({
                    'name': name,
                    'def_size': def_size,
                    'actual_size': actual_size
                })
        
        entries.append({
            'offset': current_offset,
            'size': size_to_use,
            'path': image_path,
            'name': name
        })
        
        current_offset += size_to_use
    
    # Write CSV
    with open(output_csv, 'w') as f:
        for entry in entries:
            # Format: offset,size,path,0,1
            f.write(f"{entry['offset']},{entry['size']},{entry['path']},0,1\n")
    
    print(f"Generated {output_csv}")
    print(f"Total size: {current_offset} bytes ({current_offset // 1024}KB)")
    
    if missing_files:
        print(f"\nWarning: {len(missing_files)} images missing files:")
        for name in missing_files[:10]:  # Show first 10
            print(f"  - {name}.bin")
        if len(missing_files) > 10:
            print(f"  ... and {len(missing_files) - 10} more")
    
    if size_mismatches and update_def:
        print(f"\nUpdating {len(size_mismatches)} size mismatches in {images_def}")
        update_images_def_sizes(images_def, size_mismatches)

def update_images_def_sizes(images_def_path, mismatches):
    """Update images.def with corrected sizes"""
    # Create lookup dict
    mismatch_dict = {m['name']: m['actual_size'] for m in mismatches}
    
    # Read and update
    lines = []
    with open(images_def_path, 'r') as f:
        for line in f:
            stripped = line.strip()
            if stripped.startswith('IMAGE('):
                # Extract name
                content = stripped[6:-1]
                parts = [p.strip() for p in content.split(',', 2)]
                if len(parts) >= 2:
                    name = parts[0]
                    if name in mismatch_dict:
                        # Update size
                        new_size = f"0x{mismatch_dict[name]:X}"
                        rest = parts[2] if len(parts) > 2 else ""
                        lines.append(f"IMAGE({name}, {new_size}, {rest})\n")
                        continue
            lines.append(line)
    
    # Write back
    with open(images_def_path, 'w') as f:
        f.writelines(lines)

def main():
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <output_csv>")
        print(f"  Reads: assets/images.def")
        print(f"  Scans: assets/images/split/")
        print(f"  Writes: <output_csv>")
        sys.exit(1)
    
    output_csv = sys.argv[1]
    
    generate_imagelist(
        'assets/images.def',
        'assets/images/split',
        output_csv
    )

if __name__ == '__main__':
    main()
