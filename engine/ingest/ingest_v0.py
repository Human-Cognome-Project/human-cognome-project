#!/usr/bin/env python3
"""ingest_v0 — THE full-file ingestion routine (single door, db-native).

Plan line: data-plan §4 (ONE ingestion routine; the engine receives the FULL
FILE — all the data, structure and everything, without exception — P 1377) +
§6 (event ledger: ingestion = a dated initiation, a horizon event keyed to
OBSERVED time, never sim-tick) + §2 (ledger lives IN the db, not side files).

What this routine does, completely:
  1. reads the file's complete bytes — no cleaning, no extraction, no format
     sniffing, no encoding detection, no stripping. A PDF goes in whole
     (binary streams, xref, everything); formats condense as emergent frames
     at corpus mass, they are never told apart up front.
  2. records the ingestion as a dated ledger event in the db
     (hcp_english engine.event_ledger_v0, status 'evidenced', observed-time
     horizon stamp; optional --emission-date = the document's own date, the
     attestation hook that rides ingestion free). The ledger is APPEND-ONLY:
     adjustments are new dated events referencing `supersedes`, never edits.
  3. hands the engine the complete record: a handover manifest naming the
     source path + sha256 + ledger event id. The kernel side (pour_raw.py)
     consumes the FILE ITSELF and must verify the sha before pouring — the
     routine transforms nothing, so handover-byte-identity is checkable
     end-to-end. The manifest is EMIT-ONLY (regenerable from the ledger row);
     the db row is the store.

STOP condition honored (data-plan §4): this code contains no branch that
decides what matters before the engine sees it. One code path for every file.

Run: ~/engine/venv/bin/python ingest_v0.py <file> [--emission-date YYYY-MM-DD]
                                                  [--provenance TEXT]
"""
import argparse
import datetime
import hashlib
import json
import os
import sys

import psycopg2

LEDGER_DDL = """CREATE TABLE IF NOT EXISTS engine.event_ledger_v0(
  event_id bigint GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
  event_class text NOT NULL,
  status text NOT NULL DEFAULT 'evidenced',
  observed_time timestamptz NOT NULL DEFAULT now(),
  emission_stamp date,
  content_sha256 text,
  n_bytes bigint,
  source text,
  provenance text NOT NULL,
  supersedes bigint REFERENCES engine.event_ledger_v0(event_id),
  detail text)"""


def connect():
    return psycopg2.connect(host="192.168.68.60", port=5435, user="hcp",
                            password=os.environ.get("HCP_PW", "hcp_dev"),
                            dbname="hcp_english")


def ingest(path, emission_date=None, provenance="engine ingest_v0"):
    """The single door. Returns the handover manifest (dict)."""
    with open(path, "rb") as f:
        blob = f.read()                       # the complete record, whole
    sha = hashlib.sha256(blob).hexdigest()

    conn = connect()
    conn.autocommit = True
    cur = conn.cursor()
    cur.execute("CREATE SCHEMA IF NOT EXISTS engine")
    cur.execute(LEDGER_DDL)
    cur.execute("""INSERT INTO engine.event_ledger_v0
                   (event_class, status, emission_stamp, content_sha256,
                    n_bytes, source, provenance, detail)
                   VALUES ('ingestion-initiation', 'evidenced', %s, %s, %s,
                           %s, %s, %s)
                   RETURNING event_id, observed_time""",
                (emission_date, sha, len(blob), os.path.abspath(path),
                 provenance,
                 "full-file, zero selection; engine receives everything whole"))
    event_id, observed = cur.fetchone()
    conn.close()

    # handover-byte-identity: what the engine will read == what was ledgered
    with open(path, "rb") as f:
        assert hashlib.sha256(f.read()).hexdigest() == sha, \
            "source changed between ledger and handover"

    return {
        "artifact": "ingest-v0 handover manifest (EMIT-ONLY; the ledger row is the store)",
        "plan_line": "data-plan §4 one full-file routine + §6 ledger + §2 ledger-in-db",
        "source": os.path.abspath(path),
        "content_sha256": sha,
        "n_bytes": len(blob),
        "ledger_event_id": event_id,
        "observed_time": observed.isoformat(),
        "emission_stamp": emission_date,
        "kernel_contract": "pour_raw.py consumes the FILE at `source`; verify "
                           "content_sha256 before pouring (routine transforms "
                           "nothing, so identity is checkable end-to-end)",
    }


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("file")
    ap.add_argument("--emission-date", default=None,
                    help="the document's own date (attestation hook), if known")
    ap.add_argument("--provenance", default="engine ingest_v0")
    args = ap.parse_args()
    if args.emission_date:
        datetime.date.fromisoformat(args.emission_date)   # validate only
    man = ingest(args.file, args.emission_date, args.provenance)
    print(json.dumps(man, indent=1))
    return man


if __name__ == "__main__":
    sys.exit(0 if main() else 1)
