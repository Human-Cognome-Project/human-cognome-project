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

The SAME door also opens on a db-resident document (kernel seam ask, silas
2026-09-01): `--db-doc <pbm_documents.id | all>` hands the kernel BOTH halves —
(a) the full-file byte stream (repo Gutenberg file matched by the doc's own
name; linkage marked HYPOTHESIZED because document_provenance.source_path is
empty — it confirms when P's provenance lands or a reconstruction check runs)
and (b) the complete atomized record: EVERY hcp_fic_pbm table carrying the
doc's rows, enumerated with row counts + deterministic content hashes, as
POINTERS into the live db (the db is the store; nothing is extracted or
re-shaped). No table is skipped, no column selected — the kernel reads what
it needs from the whole.

Run: ~/engine/venv/bin/python ingest_v0.py <file> [--emission-date YYYY-MM-DD]
                                                  [--provenance TEXT]
     ~/engine/venv/bin/python ingest_v0.py --db-doc <id|all>
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


def connect(dbname="hcp_english"):
    return psycopg2.connect(host="192.168.68.60", port=5435, user="hcp",
                            password=os.environ.get("HCP_PW", "hcp_dev"),
                            dbname=dbname)


# every hcp_fic_pbm table and how a doc's rows are keyed in it — the COMPLETE
# per-doc record (live-enumerated schema 2026-09-01); no table skipped.
DOC_TABLES = {
    "pbm_documents":          "SELECT t FROM pbm_documents t WHERE id = %(d)s",
    "document_provenance":    "SELECT t FROM document_provenance t WHERE doc_id = %(d)s",
    "document_relationships": "SELECT t FROM document_relationships t "
                              "WHERE source_doc_id = %(d)s OR target_doc_id = %(tok)s",
    "docvar_groups":          "SELECT t FROM docvar_groups t WHERE doc_id = %(d)s",
    "pbm_docvars":            "SELECT t FROM pbm_docvars t WHERE doc_id = %(d)s",
    "pbm_starters":           "SELECT t FROM pbm_starters t WHERE doc_id = %(d)s",
    "pbm_char_bonds":         "SELECT t FROM pbm_char_bonds t WHERE starter_id IN "
                              "(SELECT id FROM pbm_starters WHERE doc_id = %(d)s)",
    "pbm_word_bonds":         "SELECT t FROM pbm_word_bonds t WHERE starter_id IN "
                              "(SELECT id FROM pbm_starters WHERE doc_id = %(d)s)",
    "pbm_marker_bonds":       "SELECT t FROM pbm_marker_bonds t WHERE starter_id IN "
                              "(SELECT id FROM pbm_starters WHERE doc_id = %(d)s)",
    "pbm_var_bonds":          "SELECT t FROM pbm_var_bonds t WHERE starter_id IN "
                              "(SELECT id FROM pbm_starters WHERE doc_id = %(d)s)",
}

TEXTS = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                     "..", "..", "data", "gutenberg", "texts")


def ingest_db_doc(doc_pk, provenance="engine ingest_v0 --db-doc"):
    """The same door, db-resident entrance. Returns the handover manifest."""
    fic = connect("hcp_fic_pbm")
    fic.autocommit = True
    cur = fic.cursor()
    cur.execute("SELECT id, ns||'.'||p2||'.'||p3||'.'||p4||'.'||p5, name, "
                "total_slots, unique_tokens FROM pbm_documents WHERE id = %s",
                (doc_pk,))
    row = cur.fetchone()
    if row is None:
        raise SystemExit(f"no pbm_documents row id={doc_pk}")
    _, token, name, total_slots, unique_tokens = row

    tables = {}
    for tname, q in DOC_TABLES.items():
        cur.execute(f"SELECT count(*), md5(string_agg(t::text, E'\\n' "
                    f"ORDER BY t::text)) FROM ({q}) s(t)",
                    {"d": doc_pk, "tok": token})
        n, h = cur.fetchone()
        tables[tname] = {"rows": int(n), "md5": h}
    fic.close()
    record_md5 = hashlib.md5("".join(
        f"{t}:{v['md5']}" for t, v in sorted(tables.items())
        if v["md5"]).encode()).hexdigest()

    # (a) the byte-stream half: repo file matched by the doc's own name
    path = os.path.abspath(os.path.join(TEXTS, name + ".txt"))
    stream = {"path": path, "exists": os.path.exists(path),
              "linkage": "hypothesized (matched by doc name; "
                         "document_provenance.source_path is empty)"}
    if stream["exists"]:
        blob = open(path, "rb").read()
        stream["sha256"] = hashlib.sha256(blob).hexdigest()
        stream["n_bytes"] = len(blob)

    eng = connect()
    eng.autocommit = True
    ec = eng.cursor()
    ec.execute("CREATE SCHEMA IF NOT EXISTS engine")
    ec.execute(LEDGER_DDL)
    ec.execute("""INSERT INTO engine.event_ledger_v0
                  (event_class, status, content_sha256, n_bytes, source,
                   provenance, detail)
                  VALUES ('ingestion-initiation', 'evidenced', %s, %s, %s, %s, %s)
                  RETURNING event_id, observed_time""",
               (stream.get("sha256"), stream.get("n_bytes"),
                f"db:hcp_fic_pbm:id={doc_pk} {token} {name}", provenance,
                f"complete db record, all {len(tables)} tables enumerated; "
                f"record_md5={record_md5}; total_slots={total_slots}; "
                f"byte-stream linkage HYPOTHESIZED (matched by name)"))
    event_id, observed = ec.fetchone()
    eng.close()

    return {
        "artifact": "ingest-v0 handover manifest, db-resident doc "
                    "(EMIT-ONLY; ledger row + live db are the store)",
        "plan_line": "data-plan §4 one full-file routine + §6 ledger + §2 ledger-in-db",
        "source": f"db:hcp_fic_pbm:id={doc_pk}",
        "doc": {"token": token, "name": name, "total_slots": total_slots,
                "unique_tokens": unique_tokens},
        "byte_stream": stream,
        "record": {"tables": tables, "record_md5": record_md5},
        "ledger_event_id": event_id,
        "observed_time": observed.isoformat(),
        "kernel_contract": "byte stream = the file at byte_stream.path (verify "
                           "sha256 before pouring); starters/positions = live "
                           "rows per `record.tables` (verify counts+md5); the "
                           "routine selected nothing — read what you need from "
                           "the whole",
    }


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
    ap.add_argument("file", nargs="?")
    ap.add_argument("--db-doc", default=None,
                    help="pbm_documents.id, or 'all' for every doc")
    ap.add_argument("--emission-date", default=None,
                    help="the document's own date (attestation hook), if known")
    ap.add_argument("--provenance", default="engine ingest_v0")
    args = ap.parse_args()
    if bool(args.file) == bool(args.db_doc):
        ap.error("exactly one of <file> or --db-doc")
    if args.db_doc:
        if args.db_doc == "all":
            fic = connect("hcp_fic_pbm")
            c = fic.cursor()
            c.execute("SELECT id FROM pbm_documents ORDER BY id")
            ids = [r[0] for r in c.fetchall()]
            fic.close()
        else:
            ids = [int(args.db_doc)]
        mans = [ingest_db_doc(i, args.provenance) for i in ids]
        print(json.dumps(mans if len(mans) > 1 else mans[0], indent=1))
        return mans
    if args.emission_date:
        datetime.date.fromisoformat(args.emission_date)   # validate only
    man = ingest(args.file, args.emission_date, args.provenance)
    print(json.dumps(man, indent=1))
    return man


if __name__ == "__main__":
    sys.exit(0 if main() else 1)
