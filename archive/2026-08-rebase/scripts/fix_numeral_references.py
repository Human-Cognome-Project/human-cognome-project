"""Fix numeral metadata to use token_id references instead of raw integers.

For rows 1-1000 (factoring) and 1-100 (partitions), replace integer values
in factor_pairs, divisors, partitions_2el, and prime_factorization keys with
the corresponding token_id strings (the newly-minted substrate references).

Keeps:
- value (raw int — the numeral's value itself, not a cross-reference)
- partition_count (raw int — value is > 6.25M and not in our substrate range)
- predecessor_id / successor_id (already token_ids; no change needed)
"""

import json
import os
import sys
import time
import psycopg2
from psycopg2.extras import execute_batch

sys.path.insert(0, "/opt/project/repo/src")
from hcp.core.token_id import encode_pair, BASE


DB = dict(
    host="192.168.68.60",
    port=5435,
    user="hcp",
    password=os.environ.get("PGPASSWORD", "hcp_dev"),
    dbname="hcp_core",
)


def numeral_token_id(value: int) -> str:
    p4_val = value // (BASE * BASE)
    p5_val = value % (BASE * BASE)
    return f"AA.AH.AA.{encode_pair(p4_val)}.{encode_pair(p5_val)}"


def remap_metadata(meta: dict) -> dict:
    """Return a new metadata dict with integer references replaced by token_id strings."""
    new = dict(meta)  # shallow copy
    # Keep: value, predecessor_id, successor_id, partition_count

    if "factor_pairs" in new:
        # [[a, b], ...] -> [[tid_a, tid_b], ...]
        new["factor_pairs"] = [[numeral_token_id(a), numeral_token_id(b)] for a, b in new["factor_pairs"]]

    if "divisors" in new:
        # [d, ...] -> [tid_d, ...]
        new["divisors"] = [numeral_token_id(d) for d in new["divisors"]]

    if "partitions_2el" in new:
        # [[a, b], ...] -> [[tid_a, tid_b], ...]
        new["partitions_2el"] = [[numeral_token_id(a), numeral_token_id(b)] for a, b in new["partitions_2el"]]

    if "prime_factorization" in new:
        # {"prime_str": exp, ...} -> {"tid_prime": exp, ...}
        # Note: only the KEYS are remapped to token_ids; exponents stay int
        new["prime_factorization"] = {numeral_token_id(int(p)): exp for p, exp in new["prime_factorization"].items()}

    return new


def main():
    print(f"[{time.strftime('%Y-%m-%d %H:%M:%S')}] Connecting to hcp_core...", flush=True)
    conn = psycopg2.connect(**DB)
    try:
        with conn.cursor() as cur:
            # Get the 1000 rows that need updating (1-1000 carry factoring metadata)
            cur.execute(
                """
                SELECT token_id, metadata
                FROM tokens
                WHERE ns='AA' AND p2='AH'
                  AND metadata ? 'factor_pairs'
                ORDER BY (metadata->>'value')::int
                """
            )
            rows = cur.fetchall()
            print(f"[{time.strftime('%Y-%m-%d %H:%M:%S')}] Loaded {len(rows)} rows to remap", flush=True)

            updates = []
            for token_id, meta in rows:
                new_meta = remap_metadata(meta)
                updates.append((json.dumps(new_meta), token_id))

            print(f"[{time.strftime('%Y-%m-%d %H:%M:%S')}] Applying {len(updates)} UPDATEs...", flush=True)
            execute_batch(
                cur,
                "UPDATE tokens SET metadata = %s::jsonb WHERE token_id = %s",
                updates,
                page_size=500,
            )
            conn.commit()

        # Verification
        with conn.cursor() as cur:
            cur.execute(
                """
                SELECT token_id, name, metadata
                FROM tokens
                WHERE ns='AA' AND p2='AH'
                  AND name IN ('1', '12', '100', '360', '997', '1000')
                ORDER BY (metadata->>'value')::int
                """
            )
            for tid, name, meta in cur.fetchall():
                print(f"\n  {tid}  name={name}")
                if "prime_factorization" in meta:
                    print(f"    prime_factorization: {meta['prime_factorization']}")
                if "factor_pairs" in meta:
                    print(f"    factor_pairs[0]: {meta['factor_pairs'][0] if meta['factor_pairs'] else '[]'}")
                if "divisors" in meta:
                    print(f"    divisors[:3]: {meta['divisors'][:3]}")
                if "partitions_2el" in meta:
                    print(f"    partitions_2el[0]: {meta['partitions_2el'][0] if meta['partitions_2el'] else '[]'}")
                if "partition_count" in meta:
                    print(f"    partition_count: {meta['partition_count']} (kept as raw int — out of substrate range)")

    finally:
        conn.close()
    print(f"\n[{time.strftime('%Y-%m-%d %H:%M:%S')}] Fix complete.", flush=True)


if __name__ == "__main__":
    main()
