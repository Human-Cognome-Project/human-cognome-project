#!/usr/bin/env python3
"""
Compile entity name sequences into LMDB for engine-side entity annotation.

Reads entity_names from hcp_fic_entities and hcp_nf_entities,
assembles multi-token name sequences, and packs into fixed-width
LMDB sub-databases sorted by starter token_id (longest match first).

Output: data/vocab.lmdb/ (same LMDB env as vocab beds)

Sub-databases:
  entities_fic       — fiction entity name sequences
  entities_nf        — non-fiction entity name sequences
  entities_fic_meta  — 16-byte fiction metadata
  entities_nf_meta   — 16-byte non-fiction metadata

Entry format (fixed-width, 88 bytes per entry):
  starter_token_id:  [14] bytes  — first token of the name (lookup key)
  entity_id:         [14] bytes  — entity identifier
  name_group:        [1]  byte   — 0=primary, 1+=alias variant
  name_type:         [1]  byte   — enum (0=primary, 1=alias, 2=title, ...)
  token_count:       [1]  byte   — number of tokens (1-4)
  reserved:          [1]  byte   — padding
  token_1..4:        [14] bytes each — full token sequence (unused = null-padded)

Sort order: starter_token_id ASC, token_count DESC (longest match first).

Metadata (16 bytes, little-endian uint32s):
  total_entries      — total name sequences
  max_token_count    — longest name (tokens)
  distinct_entities  — unique entity_id count
  distinct_starters  — unique starter token_id count
"""

import os
import struct
import argparse
import lmdb
import psycopg
from collections import defaultdict

DB_FIC = {
    "dbname": "hcp_fic_entities",
    "user": "hcp",
    "password": "hcp_dev",
    "host": "localhost",
}

DB_NF = {
    "dbname": "hcp_nf_entities",
    "user": "hcp",
    "password": "hcp_dev",
    "host": "localhost",
}

LMDB_PATH = os.path.join(os.path.dirname(os.path.dirname(__file__)), "data", "vocab.lmdb")
TOKEN_ID_WIDTH = 14
MAX_TOKENS = 4
ENTRY_SIZE = 14 + 14 + 1 + 1 + 1 + 1 + (14 * MAX_TOKENS)  # = 88
MAX_DBS = 40

NAME_TYPE_MAP = {
    "primary": 0,
    "alias": 1,
    "title": 2,
    "epithet": 3,
    "birth_name": 4,
    "nickname": 5,
    "pen_name": 6,
    "regnal_name": 7,
    "married_name": 8,
}


def pad_tid(tid, width=TOKEN_ID_WIDTH):
    """Pad/truncate a token_id to fixed width."""
    b = tid.encode("ascii")[:width]
    return b.ljust(width, b"\x00")


def pack_entry(starter_tid, entity_id, name_group, name_type_str, token_ids):
    """Pack one entity name sequence as 88 fixed-width bytes."""
    buf = bytearray(ENTRY_SIZE)

    # starter_token_id (14 bytes)
    buf[0:14] = pad_tid(starter_tid)

    # entity_id (14 bytes)
    buf[14:28] = pad_tid(entity_id)

    # name_group (1 byte)
    buf[28] = min(name_group, 255)

    # name_type (1 byte)
    buf[29] = NAME_TYPE_MAP.get(name_type_str, 0)

    # token_count (1 byte)
    count = min(len(token_ids), MAX_TOKENS)
    buf[30] = count

    # reserved (1 byte)
    buf[31] = 0

    # token_1..4 (14 bytes each, starting at offset 32)
    for i in range(count):
        offset = 32 + i * TOKEN_ID_WIDTH
        buf[offset:offset + TOKEN_ID_WIDTH] = pad_tid(token_ids[i])

    return bytes(buf)


def pack_meta(total_entries, max_token_count, distinct_entities, distinct_starters):
    """Pack metadata as 16 bytes, little-endian uint32s."""
    return struct.pack("<IIII", total_entries, max_token_count,
                       distinct_entities, distinct_starters)


def fetch_entity_names(db_params):
    """
    Fetch and assemble entity name sequences from a database.

    Returns list of tuples:
        (starter_token_id, entity_id, name_group, name_type, [token_ids])
    """
    conn = psycopg.connect(**db_params)
    cur = conn.cursor()

    cur.execute("""
        SELECT entity_id, name_group, name_type, position, token_id
        FROM entity_names
        ORDER BY entity_id, name_group, position
    """)

    # Group by (entity_id, name_group)
    groups = defaultdict(list)
    for entity_id, name_group, name_type, position, token_id in cur.fetchall():
        groups[(entity_id, name_group)].append((position, name_type, token_id))

    sequences = []
    for (entity_id, name_group), rows in groups.items():
        # Sort by position within group
        rows.sort(key=lambda r: r[0])
        name_type = rows[0][1]  # name_type from first position
        token_ids = [r[2] for r in rows]

        if len(token_ids) > MAX_TOKENS:
            print(f"  WARNING: entity {entity_id} name_group {name_group} "
                  f"has {len(token_ids)} tokens (max {MAX_TOKENS}), truncating")
            token_ids = token_ids[:MAX_TOKENS]

        starter = token_ids[0]
        sequences.append((starter, entity_id, name_group, name_type, token_ids))

    cur.close()
    conn.close()
    return sequences


def compile_sequences(sequences):
    """Sort sequences by starter ASC, token_count DESC. Return packed buffer + stats."""
    # Sort: starter ascending, then token count descending (longest match first)
    sequences.sort(key=lambda s: (s[0], -len(s[4])))

    packed = bytearray()
    max_tc = 0
    entity_ids = set()
    starters = set()

    for starter, entity_id, name_group, name_type, token_ids in sequences:
        packed.extend(pack_entry(starter, entity_id, name_group, name_type, token_ids))
        max_tc = max(max_tc, len(token_ids))
        entity_ids.add(entity_id)
        starters.add(starter)

    return bytes(packed), len(sequences), max_tc, len(entity_ids), len(starters)


def compile_entity_lmdb(dry_run=False):
    print("Fetching fiction entities from hcp_fic_entities...")
    fic_seqs = fetch_entity_names(DB_FIC)
    print(f"  {len(fic_seqs)} name sequences")

    print("Fetching non-fiction entities from hcp_nf_entities...")
    try:
        nf_seqs = fetch_entity_names(DB_NF)
        print(f"  {len(nf_seqs)} name sequences")
    except Exception as e:
        print(f"  WARNING: could not read hcp_nf_entities: {e}")
        nf_seqs = []

    # Compile
    fic_buf, fic_total, fic_max_tc, fic_entities, fic_starters = compile_sequences(fic_seqs)
    nf_buf, nf_total, nf_max_tc, nf_entities, nf_starters = compile_sequences(nf_seqs)

    # Stats
    print(f"\n{'Source':>12} | {'Sequences':>10} | {'Entities':>9} | {'Starters':>9} | {'MaxLen':>6} | {'Bytes':>10}")
    print("-" * 70)
    print(f"{'Fiction':>12} | {fic_total:>10,} | {fic_entities:>9,} | {fic_starters:>9,} | {fic_max_tc:>6} | {len(fic_buf):>10,}")
    print(f"{'Non-fiction':>12} | {nf_total:>10,} | {nf_entities:>9,} | {nf_starters:>9,} | {nf_max_tc:>6} | {len(nf_buf):>10,}")
    print("-" * 70)
    print(f"{'Total':>12} | {fic_total + nf_total:>10,} | {fic_entities + nf_entities:>9,} | "
          f"{fic_starters + nf_starters:>9,} | {max(fic_max_tc, nf_max_tc):>6} | "
          f"{len(fic_buf) + len(nf_buf):>10,}")

    # Word count distribution
    print("\nToken count distribution:")
    for label, seqs in [("Fiction", fic_seqs), ("Non-fiction", nf_seqs)]:
        if not seqs:
            continue
        dist = defaultdict(int)
        for _, _, _, _, tids in seqs:
            dist[len(tids)] += 1
        parts = [f"{k}-token: {v}" for k, v in sorted(dist.items())]
        print(f"  {label}: {', '.join(parts)}")

    if dry_run:
        print("\n[DRY RUN] No files written.")
        return

    # Write to LMDB
    os.makedirs(LMDB_PATH, exist_ok=True)
    env = lmdb.open(LMDB_PATH, map_size=512 * 1024 * 1024, max_dbs=MAX_DBS)

    for db_name, buf, meta_args in [
        ("entities_fic", fic_buf, (fic_total, fic_max_tc, fic_entities, fic_starters)),
        ("entities_nf", nf_buf, (nf_total, nf_max_tc, nf_entities, nf_starters)),
    ]:
        if not buf:
            print(f"\n  Skipping {db_name} (no data)")
            continue
        db_data = env.open_db(db_name.encode())
        db_meta = env.open_db(f"{db_name}_meta".encode())
        meta_bytes = pack_meta(*meta_args)
        with env.begin(write=True) as txn:
            txn.put(b"data", buf, db=db_data)
            txn.put(b"meta", meta_bytes, db=db_meta)
        print(f"\n  Written {db_name}: {len(buf):,} bytes")

    env.close()
    print(f"\nEntity LMDB written to {LMDB_PATH}")

    # Verification
    print("\nVerification (first 10 entries per source):")
    env = lmdb.open(LMDB_PATH, max_dbs=MAX_DBS, readonly=True)
    for db_name in ["entities_fic", "entities_nf"]:
        db_data = env.open_db(db_name.encode())
        db_meta = env.open_db(f"{db_name}_meta".encode())
        with env.begin() as txn:
            meta_raw = txn.get(b"meta", db=db_meta)
            if not meta_raw:
                print(f"\n  {db_name}: no metadata")
                continue
            total, max_tc, n_entities, n_starters = struct.unpack("<IIII", meta_raw)
            data_raw = txn.get(b"data", db=db_data)
            if not data_raw:
                continue
            print(f"\n  {db_name}: {total} sequences, {n_entities} entities, "
                  f"{n_starters} starters, max {max_tc} tokens")
            for i in range(min(10, total)):
                offset = i * ENTRY_SIZE
                entry = data_raw[offset:offset + ENTRY_SIZE]
                starter = entry[0:14].rstrip(b"\x00").decode("ascii", errors="replace")
                eid = entry[14:28].rstrip(b"\x00").decode("ascii", errors="replace")
                ng = entry[28]
                nt = entry[29]
                tc = entry[30]
                tids = []
                for j in range(tc):
                    t_off = 32 + j * 14
                    tid = entry[t_off:t_off + 14].rstrip(b"\x00").decode("ascii", errors="replace")
                    tids.append(tid)
                nt_name = {v: k for k, v in NAME_TYPE_MAP.items()}.get(nt, f"?{nt}")
                print(f"    [{i:4d}] starter={starter}  entity={eid}  "
                      f"grp={ng} type={nt_name:8s} tokens={tids}")
    env.close()


def main():
    parser = argparse.ArgumentParser(description="Compile entity LMDB for engine annotation")
    parser.add_argument("--dry-run", action="store_true", help="Show stats without writing")
    args = parser.parse_args()

    compile_entity_lmdb(dry_run=args.dry_run)


if __name__ == "__main__":
    main()
