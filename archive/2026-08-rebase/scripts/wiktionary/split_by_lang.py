#!/usr/bin/env python3
"""Stream-split a gzipped Wiktextract JSONL into per-language files.

Reads stdin (decompressed JSONL) line by line, groups by lang_code,
buffers in memory, flushes per-language buffers to append-mode files
periodically so we never hold thousands of file descriptors open.

Usage: gzip -dc raw.jsonl.gz | split_by_lang.py OUTDIR
"""
import json
import os
import sys
from collections import defaultdict

OUTDIR = sys.argv[1]
FLUSH_LINES = 200_000  # buffer ~200k lines then flush all langs
PROGRESS_EVERY = 500_000

os.makedirs(OUTDIR, exist_ok=True)

buffers = defaultdict(list)
counts = defaultdict(int)
no_lang = 0
total = 0
parse_err = 0

def flush():
    for lang, lines in buffers.items():
        if not lines:
            continue
        path = os.path.join(OUTDIR, f"{lang}.jsonl")
        with open(path, "a", encoding="utf-8") as f:
            f.writelines(lines)
        lines.clear()

buffered = 0
for line in sys.stdin:
    total += 1
    try:
        obj = json.loads(line)
    except Exception:
        parse_err += 1
        continue
    lang = obj.get("lang_code")
    if not lang:
        no_lang += 1
        # write to special bucket
        lang = "__no_lang__"
    # sanitize - lang codes should be ascii alnum + underscore/hyphen
    safe = "".join(c if (c.isalnum() or c in "-_") else "_" for c in lang)[:32]
    buffers[safe].append(line)
    counts[safe] += 1
    buffered += 1
    if buffered >= FLUSH_LINES:
        flush()
        buffered = 0
    if total % PROGRESS_EVERY == 0:
        sys.stderr.write(f"[{total:>10,}] langs_seen={len(counts)} parse_err={parse_err} no_lang={no_lang}\n")
        sys.stderr.flush()

flush()

# write summary
summary_path = os.path.join(OUTDIR, "_counts.tsv")
with open(summary_path, "w", encoding="utf-8") as f:
    f.write("lang_code\tcount\n")
    for lang, n in sorted(counts.items(), key=lambda kv: -kv[1]):
        f.write(f"{lang}\t{n}\n")

sys.stderr.write(f"DONE total={total:,} langs={len(counts)} parse_err={parse_err} no_lang={no_lang}\n")
sys.stderr.write(f"Summary: {summary_path}\n")
