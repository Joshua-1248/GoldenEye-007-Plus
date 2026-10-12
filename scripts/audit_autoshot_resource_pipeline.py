#!/usr/bin/env python3
"""Hard build guard for the historical Auto Shotgun-safe 0x1172 pipeline."""
from pathlib import Path
import re, subprocess, tempfile, sys

ROOT = Path(__file__).resolve().parents[1]
COMP = ROOT / 'tools' / '1172compress.sh'
CDATA = ROOT / 'tools' / 'data_compress.sh'
GZIP = ROOT / 'tools' / 'gzipsrc' / 'gzip'
ASSET_MAKES = [
    ROOT/'assets/Makefile.music', ROOT/'assets/obseg/Makefile.prop',
    ROOT/'assets/obseg/Makefile.brief', ROOT/'assets/obseg/Makefile.setup',
    ROOT/'assets/obseg/Makefile.stan', ROOT/'assets/obseg/Makefile.text',
    ROOT/'assets/obseg/Makefile.gun', ROOT/'assets/obseg/Makefile.chr',
]

def fail(msg):
    print('AUTOSHOT PIPELINE AUDIT: FAIL:', msg, file=sys.stderr)
    raise SystemExit(1)

def require(cond, msg):
    if not cond: fail(msg)

require(COMP.exists(), 'tools/1172compress.sh missing')
require(CDATA.exists(), 'tools/data_compress.sh missing')
require(GZIP.exists() and GZIP.stat().st_mode & 0o111, 'bundled historical gzip missing/not executable')
ct = COMP.read_text(errors='replace')
dt = CDATA.read_text(errors='replace')
require('--cdata-zopfli' in ct, 'compressor lacks explicit cdata-only Zopfli mode')
require('GE_ZOPFLI_1172' in ct and 'Intentionally ignore' in ct, 'environment override guard missing')
require('GZ="gzip"' not in ct and "GZ='gzip'" not in ct, 'ordinary resource compressor may fall back to system gzip')
# R9 R2 correctly checks compressor status and refuses stale CDATA. Recognize
# the guarded invocation instead of requiring the older unchecked literal line.
# The mode is still explicitly cdata-only: ordinary 0x1172 asset rules must not
# inherit Turtledeflate or Zopfli settings.
require(re.search(
    r'(?m)^\s*if\s+!\s+tools/1172compress\.sh\s+'
    r'"?build/\$2/data_seg"?\s+"?build/\$2/data_seg\.rz"?'
    r'\s+--cdata-zopfli;\s*then\s*$', dt) is not None,
    'final cdata must use checked, explicit cdata-only Zopfli mode')
require('rm -f "build/$2/data_seg.rz"' in dt and
        'ERROR: final CDATA compression failed' in dt and
        'ERROR: final CDATA compressor produced no output' in dt and
        'ERROR: final CDATA verification failed' in dt and
        'if ! python3 - "build/$2/data_seg" "build/$2/data_seg.rz"' in dt and
        'decoded != original' in dt and
        'decoder.eof' in dt,
        'final cdata must reject stale, missing or mismatched compressed segments')
for mf in ASSET_MAKES:
    require(mf.exists(), f'missing asset makefile {mf.relative_to(ROOT)}')
    txt = mf.read_text(errors='replace')
    require('--cdata-zopfli' not in txt, f'ordinary asset makefile requests cdata Zopfli: {mf.relative_to(ROOT)}')

# Behavioral guard: default compression must exactly equal bundled historical gzip
# framing, even if hostile legacy environment variables request Zopfli/system gzip.
with tempfile.TemporaryDirectory() as td:
    td = Path(td)
    src = td/'fixture.bin'; out = td/'out.rz'; gz = td/'fixture.gz'
    src.write_bytes((b'GoldenEye Auto Shotgun resource pipeline guard\0' * 97) + bytes(range(256)))
    env = dict(__import__('os').environ)
    env['GE_ZOPFLI_1172'] = 'YES'
    env['GZ'] = '/bin/gzip'
    env['GE_CDATA_TURTLE'] = '1'
    env['GE_CDATA_TURTLE_BIN'] = '/nonexistent-turtledeflate-must-not-be-used-for-assets'
    subprocess.run([str(COMP), str(src), str(out)], cwd=ROOT, env=env, check=True,
                   stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    subprocess.run([str(GZIP), '--no-name', '--best', '-c', str(src)], cwd=ROOT, check=True,
                   stdout=gz.open('wb'), stderr=subprocess.PIPE)
    raw = gz.read_bytes()
    expected = b'\x11\x72' + raw[10:-8]
    require(out.read_bytes() == expected,
            'default 0x1172 output differs from bundled historical gzip/raw-DEFLATE stream')

print('AUTOSHOT PIPELINE AUDIT: PASS')
print('  ordinary 0x1172 resources: locked to bundled historical gzip/raw-DEFLATE')
print('  environment overrides: ignored for ordinary resources')
print('  final cdata/data_seg: guarded --cdata-zopfli with optional Turtledeflate')
print('  stale/missing/mismatched final CDATA: fatal build error')
