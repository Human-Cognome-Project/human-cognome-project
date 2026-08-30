"""Full bootstrap of 6.25M numerals into hcp_core under AA.AH.AA.*.*

Extends the test script to insert values 0-6,249,999. Existing 0-100 from
the test pass will be skipped via ON CONFLICT DO NOTHING.

Richer explication ranges:
- 1-1000: prime_factorization + factor_pairs + divisors
- 1-100: + partition_count + partitions_2el
- 1001-6,249,999: basic (value + predecessor_id + successor_id only)

Run in background; reports progress every 100k.
"""

import json
import os
import sys
import time
from functools import lru_cache
import psycopg2
from psycopg2.extras import execute_values

sys.path.insert(0, "/opt/project/repo/src")
from hcp.core.token_id import encode_pair, BASE


# ---- Configuration ----
DB = dict(
    host="192.168.68.60",
    port=5435,
    user="hcp",
    password=os.environ.get("PGPASSWORD", "hcp_dev"),
    dbname="hcp_core",
)

NS_NUMERAL = "AH"
NS_NUMERAL_BASIC = "AA"

FULL_MAX = 6_249_999  # full first sub-block: 2500 * 2500 - 1
FACTORING_MAX = 1000
PARTITION_MAX = 100

BATCH_SIZE = 10_000
PROGRESS_INTERVAL = 100_000


# ---- Math utilities ----
def factorint(n: int) -> dict:
    """Prime factorization via trial division. Returns {prime: exponent}."""
    if n <= 1:
        return {}
    factors = {}
    d = 2
    while d * d <= n:
        while n % d == 0:
            factors[d] = factors.get(d, 0) + 1
            n //= d
        d += 1
    if n > 1:
        factors[n] = factors.get(n, 0) + 1
    return factors


@lru_cache(maxsize=None)
def _partition_helper(n: int, max_part: int) -> int:
    if n == 0:
        return 1
    if n < 0 or max_part == 0:
        return 0
    return _partition_helper(n - max_part, max_part) + _partition_helper(n, max_part - 1)


def partition(n: int) -> int:
    if n < 0:
        return 0
    return _partition_helper(n, n)


def canonical_2_partitions(n: int) -> list[list[int]]:
    return [[a, n - a] for a in range(1, n // 2 + 1)]


# ---- Encoding ----
def encode_numeral_position(value: int) -> tuple[str, str]:
    if not 0 <= value < BASE * BASE * BASE * BASE:
        raise ValueError(f"Value out of range: {value}")
    p4_val = value // (BASE * BASE)
    p5_val = value % (BASE * BASE)
    return encode_pair(p4_val), encode_pair(p5_val)


def numeral_token_id(value: int) -> str:
    p4, p5 = encode_numeral_position(value)
    return f"AA.{NS_NUMERAL}.{NS_NUMERAL_BASIC}.{p4}.{p5}"


# ---- Metadata construction ----
def build_metadata(value: int) -> dict:
    meta = {"value": value}

    if value > 0:
        meta["predecessor_id"] = numeral_token_id(value - 1)
    if value < FULL_MAX:
        meta["successor_id"] = numeral_token_id(value + 1)

    if 1 <= value <= FACTORING_MAX:
        pf = factorint(value) if value > 1 else {}
        meta["prime_factorization"] = {str(k): v for k, v in pf.items()}

        pairs = []
        i = 1
        while i * i <= value:
            if value % i == 0:
                pairs.append([i, value // i])
            i += 1
        meta["factor_pairs"] = pairs

        divs = sorted(set(d for pair in pairs for d in pair))
        meta["divisors"] = divs

    if 1 <= value <= PARTITION_MAX:
        meta["partition_count"] = int(partition(value))
        meta["partitions_2el"] = canonical_2_partitions(value)

    return meta


def build_row(value: int) -> tuple:
    p4, p5 = encode_numeral_position(value)
    meta = build_metadata(value)
    return (
        "AA",
        NS_NUMERAL,
        NS_NUMERAL_BASIC,
        p4,
        p5,
        str(value),
        "numeral",
        "basic",
        json.dumps(meta),
    )


def insert_range(conn, start: int, end_inclusive: int):
    total = end_inclusive - start + 1
    start_time = time.time()
    last_progress = start

    cur = conn.cursor()
    rows = []
    inserted_estimate = 0

    for value in range(start, end_inclusive + 1):
        rows.append(build_row(value))
        if len(rows) >= BATCH_SIZE:
            execute_values(
                cur,
                """
                INSERT INTO tokens (ns, p2, p3, p4, p5, name, category, subcategory, metadata)
                VALUES %s
                ON CONFLICT (token_id) DO NOTHING
                """,
                rows,
                template="(%s, %s, %s, %s, %s, %s, %s, %s, %s::jsonb)",
            )
            conn.commit()
            inserted_estimate += len(rows)
            rows = []

            if value - last_progress >= PROGRESS_INTERVAL:
                elapsed = time.time() - start_time
                rate = (value - start + 1) / elapsed if elapsed > 0 else 0
                remaining = (end_inclusive - value) / rate if rate > 0 else 0
                pct = (value - start + 1) / total * 100
                print(
                    f"  progress: value={value} ({pct:.1f}%) "
                    f"elapsed={elapsed:.0f}s rate={rate:.0f}/s ETA={remaining:.0f}s",
                    flush=True,
                )
                last_progress = value

    # Final batch
    if rows:
        execute_values(
            cur,
            """
            INSERT INTO tokens (ns, p2, p3, p4, p5, name, category, subcategory, metadata)
            VALUES %s
            ON CONFLICT (token_id) DO NOTHING
            """,
            rows,
            template="(%s, %s, %s, %s, %s, %s, %s, %s, %s::jsonb)",
        )
        conn.commit()
        inserted_estimate += len(rows)

    cur.close()
    total_elapsed = time.time() - start_time
    print(
        f"Done. Processed {inserted_estimate} rows in {total_elapsed:.0f}s "
        f"({inserted_estimate / total_elapsed:.0f}/s avg)",
        flush=True,
    )


def main():
    print(f"[{time.strftime('%Y-%m-%d %H:%M:%S')}] Connecting to hcp_core...", flush=True)
    conn = psycopg2.connect(**DB)
    try:
        # Allocations already registered by test; this is idempotent insert+conflict-skip
        # so it doesn't matter if we re-run the existing 0-100 range.
        print(f"[{time.strftime('%Y-%m-%d %H:%M:%S')}] Starting insert 0..{FULL_MAX}", flush=True)
        insert_range(conn, 0, FULL_MAX)

        # Final verification
        cur = conn.cursor()
        cur.execute(
            "SELECT COUNT(*) FROM tokens WHERE ns='AA' AND p2='AH' AND p3='AA'"
        )
        final_count = cur.fetchone()[0]
        cur.close()
        print(
            f"[{time.strftime('%Y-%m-%d %H:%M:%S')}] Final count under AA.AH.AA: {final_count}",
            flush=True,
        )
    finally:
        conn.close()


if __name__ == "__main__":
    main()
