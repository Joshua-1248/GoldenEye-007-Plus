#!/usr/bin/env python3
from pathlib import Path
import sys
c = Path('src/game/chraction.c').read_text()
setup = Path('assets/obseg/setup/u/UsetupjunZ.c').read_text()
start = c.find('bool sub_GAME_7F033B38(ChrRecord *self, f32 distance)')
end = c.find('\nvoid chrSetChrPreset(', start)
block = c[start:end]
checks = [
    ('guard-nearby helper found', start >= 0 and end > start),
    ('Jungle Natalya uses nearby-guard command', 'guard_try_setting_chr_preset_to_guard_within_distance' in setup),
    ('candidate must have prop', 'chr->prop != NULL' in block),
    ('candidate must be actual guard prop', 'chr->prop->type == PROP_TYPE_CHR' in block),
    ('viewer not accepted by guard gate', 'chr->prop->type == PROP_TYPE_VIEWER)' not in block.split('if ((chr != self)',1)[1].split('{',1)[0] if 'if ((chr != self)' in block else False),
    ('TP root cause documented', 'ChrRecord in g_ChrSlots for rendering/animation' in block),
    ('no SP escort damage helper added', 'chrlvFriendlyEscortIgnoresPlayerFire' not in c),
    ('no SP TARGET_BOND suppression added', 'Never let a friendly escort intentionally execute TARGET_BOND fire' not in c),
]
failed=[name for name,ok in checks if not ok]
for name,ok in checks:
    print(f"{'PASS' if ok else 'FAIL'}: {name}")
if failed:
    print(f"JUNGLE NATALYA TP V30H AUDIT: FAIL ({len(checks)-len(failed)}/{len(checks)})")
    sys.exit(1)
print(f"JUNGLE NATALYA TP V30H AUDIT: PASS ({len(checks)}/{len(checks)})")
