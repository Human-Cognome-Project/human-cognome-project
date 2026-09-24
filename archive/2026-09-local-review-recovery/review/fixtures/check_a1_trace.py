"""Offline checks of observed evidence; not a production model acceptance test."""
import json
from pathlib import Path

trace = json.loads(Path(__file__).with_name('a1-live-trace.json').read_text())
assert trace['read_only'] and 'error_type' not in trace
sections = trace['sections']
byte = ['AA', 'AA', 'AA', 'AA', 'DL']
latin1 = ['AA', 'AD', 'AB', 'AA', 'AA']
latin2 = ['AA', 'AD', 'AB', 'AB', 'AA']
by_codepoint = {r[2]['codepoint']: r[0] for r in sections['characters']['rows']}
rows = [r for k, v in sections.items() if k.startswith('character_components_') for r in v['rows']]
assert any(r[:4] == [by_codepoint[161], latin1, 0, byte] for r in rows)
assert not any(r[0] == by_codepoint[260] and r[1] == latin2 for r in rows)
assert sections['byte_components']['rows'] == []
for codec, codepoint in [('iso8859_1',161), ('iso8859_2',260)]:
    value = bytes([0xA1]).decode(codec)
    assert ord(value) == codepoint and value.encode(codec) == bytes([0xA1])
# Each context's ordinal sequence is independently preserved, including repeats.
groups = {}
for parent, resolution, ordinal, child, legacy_weight in rows:
    groups.setdefault((tuple(parent), tuple(resolution)), []).append((ordinal, child))
for composition in groups.values():
    assert [ordinal for ordinal, _ in composition] == list(range(len(composition)))
print('A1 evidence checks passed; missing foundation links remain explicit.')
