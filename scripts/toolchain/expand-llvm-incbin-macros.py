#!/usr/bin/env python3
import re, shlex, sys
from pathlib import Path

if len(sys.argv) != 2:
    raise SystemExit('usage: expand-llvm-incbin-macros.py <asm>')

lines = Path(sys.argv[1]).read_text().splitlines()
macros = {}
out = []
i = 0
while i < len(lines):
    line = lines[i]
    m = re.match(r'^\s*\.macro\s+(\w+)(?:\s+(.*?))?\s*$', line)
    if not m:
        out.append(line); i += 1; continue
    name = m.group(1)
    params = []
    if m.group(2):
        params = [p.strip().split('=')[0] for p in re.split(r'[\s,]+', m.group(2).strip()) if p.strip()]
    body=[]; i += 1
    depth=1
    while i < len(lines):
        if re.match(r'^\s*\.macro\b', lines[i]): depth += 1
        if re.match(r'^\s*\.endm\b', lines[i]):
            depth -= 1
            if depth == 0: break
        body.append(lines[i]); i += 1
    macros[name]=(params,body)
    i += 1

def split_args(s):
    # These source macros use simple identifiers/paths, not quoted comma expressions.
    return [x for x in re.split(r'[\s,]+', s.strip()) if x]

def expand_line(line, depth=0):
    if depth > 20: raise RuntimeError('macro recursion too deep')
    stripped=line.strip()
    if not stripped or stripped.startswith('#'):
        return [line]
    tok = stripped.split(None,1)[0]
    if tok not in macros:
        return [line]
    rest = stripped[len(tok):].strip()
    args=split_args(rest)
    params,body=macros[tok]
    vals={p:(args[n] if n < len(args) else '') for n,p in enumerate(params)}
    result=[]
    for b in body:
        x=b
        for p,v in vals.items():
            x=x.replace('\\'+p, v)
        # LLVM/GAS doesn't need C-style escaping in quoted paths.
        x=x.replace('\\/', '/').replace('\\.', '.')
        for y in expand_line(x, depth+1): result.append(y)
    return result

for line in out:
    for x in expand_line(line):
        print(x)
