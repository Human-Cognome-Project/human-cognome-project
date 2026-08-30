#!/usr/bin/env python3
"""Stream-load a per-language .jsonl into source_wiktionary.wiktextract_raw.

Each line of the file is one JSON object (Wiktextract entry).
Sent to Postgres via COPY ... FROM STDIN; jsonb parsing happens server-side.
Generated columns (word, lang_code, pos, etym_number) auto-populate.

Usage: load_lang.py /path/to/en.jsonl
"""
import sys, time, os
import psycopg

INFILE = sys.argv[1]
DSN = os.environ.get(
    "WIKT_DSN",
    "host=127.0.0.1 port=5435 dbname=source_wiktionary user=hcp password=hcp_dev",
)
LOG_EVERY = 100_000

t0 = time.time()
sent = 0
skipped_empty = 0

with psycopg.connect(DSN) as conn:
    with conn.cursor() as cur:
        with cur.copy("COPY wiktextract_raw (raw) FROM STDIN") as copy:
            with open(INFILE, "r", encoding="utf-8") as f:
                for line in f:
                    line = line.rstrip("\n")
                    if not line:
                        skipped_empty += 1
                        continue
                    copy.write_row([line])
                    sent += 1
                    if sent % LOG_EVERY == 0:
                        elapsed = time.time() - t0
                        rate = sent / elapsed if elapsed > 0 else 0
                        sys.stderr.write(
                            f"[{sent:>10,}] {elapsed:>6.1f}s  {rate:>7.0f} rows/s\n"
                        )
                        sys.stderr.flush()
    conn.commit()

elapsed = time.time() - t0
sys.stderr.write(
    f"DONE sent={sent:,} empty={skipped_empty} time={elapsed:.1f}s "
    f"rate={sent/elapsed:.0f}/s\n"
)
