#!/usr/bin/env python3
"""Eight-worker whole-stream CDATA fallback for GoldenEye 007 Plus.

Each worker compresses the *entire* current CDATA with different Zopfli settings.
This preserves cross-boundary LZ77 matches lost by eight independent chunks.
Only the final CDATA compression changes: ordinary asset compression is untouched.
"""
import argparse
from concurrent.futures import ProcessPoolExecutor, as_completed
from pathlib import Path
import zlib

# Each tuple is (Zopfli iterations, maximum number of splitting blocks).
# Includes independently tested, sub-72704-byte configurations for the
# R17 CDATA (SHA-256 3aef6427...). Zero means unlimited split candidates.
TRIALS = (
    (160, 0),
    (180, 0),
    (200, 64),
    (220, 0),
    (260, 0),
    (300, 0),
    (360, 0),
    (360, 64),
)
SLOT_LIMIT = 72704


def validate(raw, original):
    if not raw.startswith(b'\x11\x72'):
        raise RuntimeError('candidate lacks 0x1172 header')
    inflater = zlib.decompressobj(-15)
    uncompressed = inflater.decompress(raw[2:]) + inflater.flush()
    if not inflater.eof or inflater.unconsumed_tail or inflater.unused_data or uncompressed != original:
        raise RuntimeError('candidate failed strict raw-DEFLATE round-trip validation')


def worker(args):
    path, iterations, maxblocks = args
    import zopfli.gzip
    original = Path(path).read_bytes()
    gzip = zopfli.gzip.compress(original, numiterations=iterations,
                               blocksplitting=1,
                               blocksplittingmax=maxblocks,
                               blocksplittinglast=0)
    raw = b'\x11\x72' + gzip[10:-8]
    validate(raw, original)
    return iterations, maxblocks, raw


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('input', type=Path)
    parser.add_argument('output', type=Path)
    parser.add_argument('--workers', type=int, default=8)
    args = parser.parse_args()
    if not 1 <= args.workers <= 8:
        parser.error('--workers must be between 1 and 8')
    original = args.input.read_bytes()
    best = None
    best_details = None
    tasks = [(str(args.input), *spec) for spec in TRIALS[:args.workers]]
    print(f'CDATA fallback: {len(tasks)} independent full-stream Zopfli workers', flush=True)
    with ProcessPoolExecutor(max_workers=args.workers) as executor:
        futures = [executor.submit(worker, item) for item in tasks]
        for future in as_completed(futures):
            iterations, maxblocks, candidate = future.result()
            print(f'  Zopfli {iterations} iterations, maxblocks={maxblocks}: '
                  f'{len(candidate)} bytes', flush=True)
            if best is None or len(candidate) < len(best):
                best, best_details = candidate, (iterations, maxblocks)
    if best is None:
        raise RuntimeError('no valid CDATA compressed candidates')
    validate(best, original)
    if len(best) > SLOT_LIMIT:
        raise RuntimeError(f'eight-worker whole-stream CDATA best {len(best)} '
                           f'exceeds fixed ROM slot {SLOT_LIMIT}')
    args.output.write_bytes(best)
    print(f'CDATA fallback success: {len(best)} / {SLOT_LIMIT} bytes; '
          f'{SLOT_LIMIT - len(best)} bytes free; '
          f'winning configuration {best_details}', flush=True)


if __name__ == '__main__':
    main()
