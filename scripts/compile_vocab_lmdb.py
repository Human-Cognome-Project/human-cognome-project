#!/usr/bin/env python3
"""
Compile pre-ordered LMDB vocab tables for GPU loading.

Reads hcp_english.tokens from Postgres, strips regular morphological inflections,
orders base forms per the tier scheme, and writes binary LMDB sub-databases
that the engine can memcpy to VRAM.

Morphological stripping: regular inflected forms (plurals, past tense, progressives,
possessives, contractions) are excluded if their base form exists in the vocab.
The engine strips suffixes at resolution time and sets morph bit flags.

Output LMDB: data/vocab.lmdb/

Sub-databases:
  vbed_02 .. vbed_16  — packed entry buffers, one per word length
  vbed_02_meta .. vbed_16_meta — 16-byte VBedMeta structs

Entry format (fixed-width per sub-db):
  word:     [wordLength] bytes (the word, padded with \\0 if shorter)
  tokenId:  [14] bytes (5-pair decomposed token_id, always 14 chars)

Entry order within each length:
  1. Labels (subcategory='label'), ordered by freq_rank ASC (ranked first, then alphabetical)
  2. Non-labels with freq_rank, ordered by freq_rank ASC
  3. Non-labels without freq_rank, ordered alphabetically

VBedMeta struct (little-endian):
  uint32 total_entries
  uint32 label_count     (tier 0 boundary)
  uint32 tier1_end       (tier 1 boundary — end of freq-ranked non-labels)
  uint32 tier2_end       (tier 2 boundary — for now same as total_entries)

License: frequency ordering derived from:
  - Wikipedia word frequency 2023 (MIT License)
  - OpenSubtitles FrequencyWords (CC BY-SA-4.0)
"""

import os
import struct
import argparse
import lmdb
import psycopg2

DB_PARAMS = {
    "dbname": "hcp_english",
    "user": "hcp",
    "password": "hcp_dev",
    "host": "localhost",
}

LMDB_PATH = os.path.join(os.path.dirname(os.path.dirname(__file__)), "data", "vocab.lmdb")
TOKEN_ID_WIDTH = 14
MIN_LEN = 2
MAX_LEN = 16
MAX_DBS = 40


# ---------------------------------------------------------------------------
# Morphological stripping
# ---------------------------------------------------------------------------

def build_vocab_set(cur):
    """Load all word names from hcp_english into a set for base-existence checks."""
    cur.execute("SELECT DISTINCT name FROM tokens WHERE ns LIKE 'AB%';")
    return {row[0] for row in cur.fetchall()}


def try_strip(word, vocab_set):
    """
    Attempt morphological stripping. Returns (base, morph_bits) if strippable,
    or None if the word should be kept as-is.

    Rules are applied in priority order (longest/most specific suffix first).
    A strip is only valid if the resulting base form exists in vocab_set.

    Morph bits (16-bit field):
      0: PLURAL    1: POSS      2: POSS_PL   3: PAST
      4: PROG      5: 3RD       6: NEG       7: COND
      8: WILL      9: HAVE     10: BE       11: AM
    """
    n = len(word)

    # --- Contractions (apostrophe forms) ---
    # These are checked first because they contain special chars

    if n > 4 and word.endswith("n't"):
        base = word[:-3]
        # Special: "won't" -> "will", "can't" -> "can", "shan't" -> "shall"
        # These don't strip cleanly — skip them (they'll stay as entries)
        if base in vocab_set:
            return (base, 1 << 6)  # NEG

    if n > 3 and word.endswith("'re"):
        base = word[:-3]
        if base in vocab_set:
            return (base, 1 << 10)  # BE

    if n > 3 and word.endswith("'ve"):
        base = word[:-3]
        if base in vocab_set:
            return (base, 1 << 9)  # HAVE

    if n > 3 and word.endswith("'ll"):
        base = word[:-3]
        if base in vocab_set:
            return (base, 1 << 8)  # WILL

    if n > 2 and word.endswith("'s"):
        base = word[:-2]
        if base in vocab_set:
            return (base, 1 << 1)  # POSS

    if n > 2 and word.endswith("'m"):
        base = word[:-2]
        if base in vocab_set:
            return (base, 1 << 11)  # AM

    if n > 2 and word.endswith("'d"):
        base = word[:-2]
        if base in vocab_set:
            return (base, 1 << 7)  # COND

    # --- Inflectional suffixes ---

    # -ies -> -y (plural/verb: families->family, carries->carry)
    if n > 4 and word.endswith("ies"):
        base = word[:-3] + "y"
        if base in vocab_set:
            return (base, (1 << 0) | (1 << 5))  # PLURAL | 3RD

    # -ied -> -y (past: carried->carry)
    if n > 4 and word.endswith("ied"):
        base = word[:-3] + "y"
        if base in vocab_set:
            return (base, 1 << 3)  # PAST

    # -ves -> -f / -fe (plural: knives->knife, wolves->wolf)
    if n > 4 and word.endswith("ves"):
        base_f = word[:-3] + "f"
        base_fe = word[:-3] + "fe"
        if base_fe in vocab_set:
            return (base_fe, 1 << 0)  # PLURAL
        if base_f in vocab_set:
            return (base_f, 1 << 0)  # PLURAL

    # Doubled consonant + -ing (running->run, stopping->stop)
    if n > 5 and word.endswith("ing") and word[-4] == word[-5]:
        base = word[:-4]
        if len(base) >= 2 and base in vocab_set:
            return (base, 1 << 4)  # PROG

    # Doubled consonant + -ed (stopped->stop, planned->plan)
    if n > 4 and word.endswith("ed") and word[-3] == word[-4]:
        base = word[:-3]
        if len(base) >= 2 and base in vocab_set:
            return (base, 1 << 3)  # PAST

    # Doubled consonant + -er (runner->run, winner->win)
    if n > 4 and word.endswith("er") and word[-3] == word[-4]:
        base = word[:-3]
        if len(base) >= 2 and base in vocab_set:
            return (base, 0)  # Agent noun — no morph bit yet, but strip from vocab

    # -ing with silent-e restoration (baking->bake, coming->come)
    if n > 4 and word.endswith("ing"):
        base_e = word[:-3] + "e"
        base = word[:-3]
        if base_e in vocab_set:
            return (base_e, 1 << 4)  # PROG
        if base in vocab_set:
            return (base, 1 << 4)  # PROG

    # -ed with silent-e (baked->bake)
    if n > 3 and word.endswith("ed"):
        base_e = word[:-1]  # baked -> bake (just remove d)
        base = word[:-2]    # walked -> walk
        if base_e in vocab_set and base_e != word:
            return (base_e, 1 << 3)  # PAST
        if base in vocab_set:
            return (base, 1 << 3)  # PAST

    # -er (comparative/agent: taller->tall, worker->work)
    # Check -e restoration too (nicer->nice)
    if n > 3 and word.endswith("er"):
        base_e = word[:-1]  # nicer -> nice (just remove r)
        base = word[:-2]    # taller -> tall
        if base_e in vocab_set and base_e != word:
            return (base_e, 0)  # Comparative — no morph bit yet
        if base in vocab_set:
            return (base, 0)

    # -est (superlative: tallest->tall, nicest->nice)
    if n > 4 and word.endswith("est"):
        base_e = word[:-2]  # nicest -> nice (remove st)
        base = word[:-3]    # tallest -> tall
        if base_e in vocab_set and base_e != word:
            return (base_e, 0)  # Superlative — no morph bit yet
        if base in vocab_set:
            return (base, 0)

    # -ses, -xes, -zes, -ches, -shes (sibilant plurals: boxes->box, churches->church)
    if n > 3 and word.endswith("es"):
        base = word[:-2]
        if base.endswith(("s", "x", "z", "ch", "sh")) and base in vocab_set:
            return (base, (1 << 0) | (1 << 5))  # PLURAL | 3RD

    # General -es (tomatoes->tomato)
    if n > 3 and word.endswith("es"):
        base = word[:-2]
        if base in vocab_set:
            return (base, (1 << 0) | (1 << 5))  # PLURAL | 3RD
        # Also try -e (dances -> dance, not danc)
        base_e = word[:-1]
        if base_e in vocab_set:
            return (base_e, (1 << 0) | (1 << 5))  # PLURAL | 3RD

    # General -s (dogs->dog, walks->walk)
    if n > 3 and word.endswith("s") and not word.endswith("ss") and not word.endswith("us"):
        base = word[:-1]
        if base in vocab_set:
            return (base, (1 << 0) | (1 << 5))  # PLURAL | 3RD

    # -ly (adverb: quickly->quick)
    if n > 4 and word.endswith("ly"):
        base = word[:-2]
        if base in vocab_set:
            return (base, 0)  # Adverb — no morph bit yet

    # -ness (noun: darkness->dark)
    if n > 5 and word.endswith("ness"):
        base = word[:-4]
        if base in vocab_set:
            return (base, 0)
        # -iness -> -y (happiness -> happy)
        if word.endswith("iness"):
            base_y = word[:-5] + "y"
            if base_y in vocab_set:
                return (base_y, 0)

    return None


def build_strip_set(cur, vocab_set):
    """
    Build the set of words that should be stripped (excluded from LMDB).
    Returns dict: {stripped_word: (base, morph_bits)}
    Only strips non-label entries.
    """
    cur.execute("""
        SELECT DISTINCT name FROM tokens
        WHERE ns LIKE 'AB%' AND subcategory IS DISTINCT FROM 'label';
    """)

    stripped = {}
    for (name,) in cur.fetchall():
        result = try_strip(name, vocab_set)
        if result:
            stripped[name] = result

    return stripped


# ---------------------------------------------------------------------------
# LMDB compilation
# ---------------------------------------------------------------------------

def fetch_entries(cur, word_length):
    """Fetch all entries for a word length, pre-ordered by tier scheme."""
    cur.execute("""
        SELECT name, token_id, subcategory, freq_rank
        FROM tokens
        WHERE ns LIKE 'AB%%'
          AND length(name) = %s
        ORDER BY
            CASE
                WHEN subcategory = 'label' THEN 0
                WHEN freq_rank IS NOT NULL THEN 1
                ELSE 2
            END,
            CASE WHEN freq_rank IS NOT NULL THEN freq_rank ELSE 999999999 END,
            name
    """, (word_length,))
    return cur.fetchall()


def pack_entry(word, token_id, word_length):
    """Pack a single entry as fixed-width bytes."""
    word_bytes = word.encode("utf-8")[:word_length]
    word_bytes = word_bytes.ljust(word_length, b"\x00")

    tid_bytes = token_id.encode("ascii")[:TOKEN_ID_WIDTH]
    tid_bytes = tid_bytes.ljust(TOKEN_ID_WIDTH, b"\x00")

    return word_bytes + tid_bytes


def pack_meta(total_entries, label_count, tier1_end, tier2_end):
    """Pack VBedMeta as 16 bytes, little-endian uint32s."""
    return struct.pack("<IIII", total_entries, label_count, tier1_end, tier2_end)


def compile_lmdb(dry_run=False, no_strip=False):
    conn = psycopg2.connect(**DB_PARAMS)
    cur = conn.cursor()

    # Build morph stripping data
    if no_strip:
        strip_set = {}
        print("Morphological stripping: DISABLED\n")
    else:
        print("Building vocab set for base-existence checks...")
        vocab_set = build_vocab_set(cur)
        print(f"  {len(vocab_set):,} unique word forms")

        print("Computing morphological strip set...")
        strip_set = build_strip_set(cur, vocab_set)
        print(f"  {len(strip_set):,} inflected forms will be excluded")
        print()

    if not dry_run:
        os.makedirs(LMDB_PATH, exist_ok=True)
        env = lmdb.open(LMDB_PATH, map_size=512 * 1024 * 1024, max_dbs=MAX_DBS)

    total_entries_all = 0
    total_stripped_all = 0
    total_bytes_all = 0

    print(f"{'Len':>3} | {'Labels':>7} | {'Ranked':>7} | {'Unranked':>8} | {'Total':>7} | {'Stripped':>8} | {'Bytes':>10}")
    print("-" * 80)

    for wlen in range(MIN_LEN, MAX_LEN + 1):
        rows = fetch_entries(cur, wlen)
        if not rows:
            print(f"{wlen:>3} | {'(empty)':>7}")
            continue

        entry_size = wlen + TOKEN_ID_WIDTH

        label_count = 0
        ranked_nonlabel_count = 0
        unranked_count = 0
        stripped_count = 0

        packed_entries = bytearray()
        for name, token_id, subcategory, freq_rank in rows:
            # Skip stripped inflected forms (never strip labels)
            if subcategory != "label" and name in strip_set:
                stripped_count += 1
                continue

            packed_entries.extend(pack_entry(name, token_id, wlen))
            if subcategory == "label":
                label_count += 1
            elif freq_rank is not None:
                ranked_nonlabel_count += 1
            else:
                unranked_count += 1

        total = label_count + ranked_nonlabel_count + unranked_count
        tier1_end = label_count + ranked_nonlabel_count
        tier2_end = total

        total_entries_all += total
        total_stripped_all += stripped_count
        buf_size = len(packed_entries)
        total_bytes_all += buf_size

        print(f"{wlen:>3} | {label_count:>7,} | {ranked_nonlabel_count:>7,} | {unranked_count:>8,} | {total:>7,} | {stripped_count:>8,} | {buf_size:>10,}")

        if not dry_run:
            db_data = env.open_db(f"vbed_{wlen:02d}".encode())
            db_meta = env.open_db(f"vbed_{wlen:02d}_meta".encode())

            meta_bytes = pack_meta(total, label_count, tier1_end, tier2_end)

            with env.begin(write=True) as txn:
                txn.put(b"data", bytes(packed_entries), db=db_data)
                txn.put(b"meta", meta_bytes, db=db_meta)

    print("-" * 80)
    print(f"{'':>3} | {'':>7} | {'':>7} | {'':>8} | {total_entries_all:>7,} | {total_stripped_all:>8,} | {total_bytes_all:>10,}")

    original = total_entries_all + total_stripped_all
    pct = 100 * total_stripped_all / original if original else 0
    print(f"\nTotal: {total_entries_all:,} entries ({total_bytes_all / 1024 / 1024:.1f} MB)")
    print(f"Stripped: {total_stripped_all:,} inflected forms ({pct:.1f}% reduction from {original:,})")

    if not dry_run:
        env.close()
        db_file = os.path.join(LMDB_PATH, "data.mdb")
        if os.path.exists(db_file):
            fsize = os.path.getsize(db_file)
            print(f"LMDB file: {db_file} ({fsize / 1024 / 1024:.1f} MB on disk)")
        print(f"\nWritten to {LMDB_PATH}")
    else:
        print("\n[DRY RUN] No files written.")

    # Verification
    if not dry_run:
        env = lmdb.open(LMDB_PATH, max_dbs=MAX_DBS, readonly=True)
        print("\nVerification (first 5 entries per length):")
        for wlen in range(MIN_LEN, MAX_LEN + 1):
            db_data = env.open_db(f"vbed_{wlen:02d}".encode())
            db_meta = env.open_db(f"vbed_{wlen:02d}_meta".encode())
            with env.begin() as txn:
                meta_raw = txn.get(b"meta", db=db_meta)
                if not meta_raw:
                    continue
                total, labels, t1end, t2end = struct.unpack("<IIII", meta_raw)
                data_raw = txn.get(b"data", db=db_data)
                if not data_raw:
                    continue
                entry_size = wlen + TOKEN_ID_WIDTH
                print(f"\n  vbed_{wlen:02d}: {total} entries (labels={labels}, t1_end={t1end}, t2_end={t2end})")
                for i in range(min(5, total)):
                    offset = i * entry_size
                    word = data_raw[offset:offset + wlen].rstrip(b"\x00").decode("utf-8", errors="replace")
                    tid = data_raw[offset + wlen:offset + entry_size].rstrip(b"\x00").decode("ascii", errors="replace")
                    tag = "LABEL" if i < labels else ("RANKED" if i < t1end else "UNRNK")
                    print(f"    [{i:5d}] {tag:6s} {word:<{wlen}} → {tid}")
        env.close()

    cur.close()
    conn.close()


def main():
    parser = argparse.ArgumentParser(description="Compile vocab LMDB for GPU loading")
    parser.add_argument("--dry-run", action="store_true", help="Show stats without writing")
    parser.add_argument("--no-strip", action="store_true", help="Disable morphological stripping")
    args = parser.parse_args()

    compile_lmdb(dry_run=args.dry_run, no_strip=args.no_strip)


if __name__ == "__main__":
    main()
