#!/usr/bin/env python3
"""Scan all per-language jsonl files for English-related variants.

For each .jsonl file, read the first line, extract the `lang` human-readable
name, and report any whose name suggests an English-derived/related language.
"""
import json
import os
import sys

ROOT = sys.argv[1]
PATTERNS = [
    "English", "Anglo", "Pidgin", "Creole", "Patois", "Krio", "Tok Pisin",
    "Bislama", "Sranan", "Gullah", "Jamaican", "Singlish", "Scots",
    "Pitkern", "Ndyuka", "Saramaccan", "Aukan", "Kriol",
]

results = []
for fn in sorted(os.listdir(ROOT)):
    if not fn.endswith(".jsonl") or fn.startswith("_"):
        continue
    code = fn[:-len(".jsonl")]
    path = os.path.join(ROOT, fn)
    try:
        with open(path, encoding="utf-8") as f:
            first = f.readline()
        if not first.strip():
            continue
        obj = json.loads(first)
        lang = obj.get("lang", "")
    except Exception:
        continue
    if any(p.lower() in lang.lower() for p in PATTERNS):
        # count lines
        with open(path, encoding="utf-8") as f:
            n = sum(1 for _ in f)
        results.append((code, lang, n))

results.sort(key=lambda r: -r[2])
print(f"{'code':<10} {'name':<40} count")
for code, lang, n in results:
    print(f"{code:<10} {lang:<40} {n:>8}")
print(f"---\nTOTAL_VARIANTS = {len(results)}")
print(f"TOTAL_ENTRIES   = {sum(r[2] for r in results):,}")
