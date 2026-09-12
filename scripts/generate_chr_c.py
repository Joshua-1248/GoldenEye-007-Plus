#!/usr/bin/env python3
"""
Character Model Parser for GoldenEye 007

Parses N64 binary character files (CnameZ.bin) and generates C source code (Model.c).

Binary Format (same as props):
    Offset 0x00: Switches array (if numSwitches > 0)
    Offset 0x??: Texture table (ModelFileTextures[numtextures])  
    Offset 0x??: RootNode and complete ModelNode tree with embedded data

All pointers in the binary are stored as offsets from base address 0x05000000.

Usage:
    python3 scripts/generate_chr_c.py [--dry-run] [--force] [--cleanup] [chr ...]
"""

import struct
import sys
import re
from pathlib import Path
from typing import Dict, List, Tuple, Optional, Any
from dataclasses import dataclass, field

# Add parent directory to path and import prop parser
import importlib.util
spec = importlib.util.spec_from_file_location("generate_prop_model_c", Path(__file__).parent / "generate_prop_model_c.py")
prop_module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(prop_module)

# Import needed functions and classes from prop parser
BinaryModelParser = prop_module.BinaryModelParser
load_image_map = prop_module.load_image_map
generate_model_c = prop_module.generate_model_c

def generate_chr_c(chr_name: str, parsed_model: Dict, metadata: Dict, image_map: Dict, binary_data: bytes) -> str:
    """Generate C source code for a character model - uses prop generator with chr-specific header"""
    
    # Use the prop model generator but replace the header include
    c_code = generate_model_c(chr_name, parsed_model, metadata, image_map, binary_data)
    
    # Replace prop-specific header include with chr-specific one
    c_code = c_code.replace(
        f'#include "{chr_name}/ModelFileHeader.inc.c"',
        f'#include "{chr_name}/modelFileHeader.inc.c"'
    )
    
    # Update the source file reference
    c_code = c_code.replace(
        f"// Source: P{chr_name}Z.bin",
        f"// Source: C{chr_name}Z.bin"
    )
    
    return c_code


def main():
    import argparse
    
    parser = argparse.ArgumentParser(description="Generate Model.c from binary chr files")
    parser.add_argument('chrs', nargs='*', help="Character names to process (default: all)")
    parser.add_argument('--dry-run', action='store_true', help="Show what would be done")
    parser.add_argument('--force', action='store_true', help="Overwrite existing Model.c files")
    parser.add_argument('--cleanup', action='store_true', help="Delete source CnameZ.bin files after successful conversion")
    args = parser.parse_args()
    
    # Load image mapping
    image_map = load_image_map()
    print(f"Loaded {len(image_map)} image definitions")
    
    # Find all chrs
    chr_dir = Path("assets/obseg/chr")
    all_chrs = []
    chr_name_map = {}  # lowercase -> actual case
    
    for bin_file in chr_dir.glob("C*Z.bin"):
        # Extract name between C and Z, preserve original case
        actual_name = bin_file.stem[1:-1]
        lower_name = actual_name.lower()
        all_chrs.append(lower_name)
        chr_name_map[lower_name] = actual_name
    
    chrs_to_process = args.chrs if args.chrs else all_chrs
    
    stats = {'processed': 0, 'skipped': 0, 'errors': 0}
    
    for chr_name_input in sorted(chrs_to_process):
        chr_name_lower = chr_name_input.lower()
        
        # Get the actual case from the binary file
        if chr_name_lower not in chr_name_map:
            print(f"  ✗ {chr_name_input}: Binary file not found")
            stats['errors'] += 1
            continue
        
        actual_chr_name = chr_name_map[chr_name_lower]
        bin_file = chr_dir / f"C{actual_chr_name}Z.bin"
        
        # Find the actual directory name (case-insensitive search)
        chr_subdirs = list(chr_dir.glob(f"{actual_chr_name}"))
        if not chr_subdirs:
            # Try case-insensitive
            chr_subdirs = [d for d in chr_dir.iterdir() 
                           if d.is_dir() and d.name.lower() == chr_name_lower]
        
        if not chr_subdirs:
            print(f"  ⊘ {actual_chr_name}: Missing metadata directory")
            stats['skipped'] += 1
            continue
        
        actual_dir_name = chr_subdirs[0].name
        
        # Parse metadata using the actual directory name  
        # Note: chr uses different metadata file names
        metadata_dir = chr_dir / actual_dir_name
        metadata = {}
        
        # Check for chr-specific metadata files
        header_file = metadata_dir / "modelFileHeader.inc.c"
        if header_file.exists():
            with open(header_file, 'r') as f:
                header_content = f.read()
                metadata['header'] = header_content
                
                # Parse MODELFILEHEADER macro to extract num_switches and num_textures
                # Format: MODELFILEHEADER(name, rootnode, skeleton, switches, numswitches, nummatrices, boundingradius, numrecords, numtextures)
                match = re.search(r'MODELFILEHEADER\([^,]+,\s*[^,]+,\s*[^,]+,\s*[^,]+,\s*(\d+|0x[0-9A-Fa-f]+),\s*[^,]+,\s*[^,]+,\s*[^,]+,\s*(\d+|0x[0-9A-Fa-f]+)\)', header_content)
                if match:
                    num_switches = int(match.group(1), 0)  # 0 base auto-detects hex/decimal
                    num_textures = int(match.group(2), 0)
                    metadata['num_switches'] = num_switches
                    metadata['num_textures'] = num_textures
        
        if not metadata or 'num_switches' not in metadata:
            print(f"  ⊘ {actual_chr_name}: Missing or invalid metadata files")
            stats['skipped'] += 1
            continue
        
        output_file = chr_dir / actual_dir_name / "Model.c"
        if output_file.exists() and not args.force:
            print(f"  ⊘ {actual_chr_name}: Model.c already exists (use --force)")
            stats['skipped'] += 1
            continue
        
        try:
            # Parse binary (same format as props)
            with open(bin_file, 'rb') as f:
                binary_data = f.read()
            
            parser = BinaryModelParser(binary_data, metadata, image_map)
            parsed_model = parser.parse()
            
            # Generate C code
            c_code = generate_chr_c(actual_dir_name, parsed_model, metadata, image_map, binary_data)
            
            if args.dry_run:
                if len(parsed_model['switches']) > 0:
                    print(f"  ✓ {actual_chr_name}: Would generate Model.c ({len(parsed_model['nodes'])} nodes, {len(parsed_model['textures'])} textures, {len(parsed_model['switches'])} switches)")
                else:
                    print(f"  ✓ {actual_chr_name}: Would generate Model.c ({len(parsed_model['nodes'])} nodes, {len(parsed_model['textures'])} textures)")
            else:
                output_file.parent.mkdir(parents=True, exist_ok=True)
                with open(output_file, 'w') as f:
                    f.write(c_code)
                    
                # Report generation
                if len(parsed_model['switches']) > 0:
                    print(f"  ✓ {actual_chr_name}: Generated Model.c ({len(parsed_model['nodes'])} nodes, {len(parsed_model['textures'])} textures, {len(parsed_model['switches'])} switches)")
                else:
                    print(f"  ✓ {actual_chr_name}: Generated Model.c ({len(parsed_model['nodes'])} nodes, {len(parsed_model['textures'])} textures)")
                
                # Clean up source binary file after successful conversion
                if args.cleanup and bin_file.exists():
                    bin_file.unlink()
                    print(f"    Cleaned up {bin_file.name}")
            
            stats['processed'] += 1
            
        except Exception as e:
            print(f"  ✗ {actual_chr_name}: Error - {e}")
            stats['errors'] += 1
            import traceback
            traceback.print_exc()
    
    print(f"\n=== Summary ===")
    print(f"Processed: {stats['processed']}")
    print(f"Skipped: {stats['skipped']}")
    print(f"Errors: {stats['errors']}")


if __name__ == "__main__":
    main()
