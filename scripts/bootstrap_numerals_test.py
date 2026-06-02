"""Test bootstrap of numerals 0-100 into hcp_core under AA.AH.AA.*.*

Verifies encoding, namespace_allocations registration, token inserts with
richer-explication metadata (factoring, partitions). Once verified, the full
6.25M run is straightforward extension.
"""

import json
import os
import sys
from functools import lru_cache
import psycopg2
from psycopg2.extras import execute_values

# Use the existing encoding utility
sys.path.insert(0, "/opt/project/repo/src")
from hcp.core.token_id import encode_pair, encode_token_id, BASE


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
    """Number of partitions of n using parts <= max_part."""
    if n == 0:
        return 1
    if n < 0 or max_part == 0:
        return 0
    return _partition_helper(n - max_part, max_part) + _partition_helper(n, max_part - 1)


def partition(n: int) -> int:
    """Number of unordered integer partitions of n (Hardy-Ramanujan p(n))."""
    if n < 0:
        return 0
    return _partition_helper(n, n)

# ---- Configuration ----
DB = dict(
    host="192.168.68.60",
    port=5435,
    user="hcp",
    password=os.environ.get("PGPASSWORD", "hcp_dev"),
    dbname="hcp_core",
)

# Numerical namespace allocation: AA.AH for Numerals; AA.AH.AA.*.* for basic
NS_NUMERAL = "AH"  # 2nd-level under AA
NS_NUMERAL_BASIC = "AA"  # 3rd-level for first 6.25M sub-block

# Encoding constants
PAIR_AA = encode_pair(0)  # "AA"
PAIR_AH = encode_pair(7)  # "AH"

TEST_MAX = 100  # this test pass: numerals 0..100
FACTORING_MAX = 1000  # range that gets factoring metadata in full run
PARTITION_MAX = 100  # range that gets partition metadata


def encode_numeral_position(value: int) -> tuple[str, str]:
    """Return (p4, p5) for numeral value N (0 to 6,249,999)."""
    if not 0 <= value < BASE * BASE * BASE * BASE:
        raise ValueError(f"Value out of range: {value}")
    p4_val = value // (BASE * BASE)
    p5_val = value % (BASE * BASE)
    return encode_pair(p4_val), encode_pair(p5_val)


def numeral_token_id(value: int) -> str:
    """Full token_id for numeral value N: AA.AH.AA.<p4>.<p5>"""
    p4, p5 = encode_numeral_position(value)
    return f"AA.{NS_NUMERAL}.{NS_NUMERAL_BASIC}.{p4}.{p5}"


def canonical_2_partitions(n: int) -> list[list[int]]:
    """Return canonical 2-element partitions of n: [[a, b], ...] where a <= b and a+b=n.

    Example: 6 -> [[1,5], [2,4], [3,3]] (excludes 0+6 and the trivial single-element)
    """
    return [[a, n - a] for a in range(1, n // 2 + 1)]


def build_metadata(value: int) -> dict:
    """Construct metadata for a numeral.

    Includes:
    - value (int)
    - predecessor_id and successor_id (full token_id strings, when in range)
    - prime_factorization (for 1..FACTORING_MAX)
    - factor_pairs (for 1..FACTORING_MAX)
    - divisors (for 1..FACTORING_MAX)
    - partition_count (for 1..PARTITION_MAX)
    - partitions_2el (canonical 2-element partitions, for 1..PARTITION_MAX)
    """
    meta = {"value": value}

    # Predecessor / successor
    if value > 0:
        meta["predecessor_id"] = numeral_token_id(value - 1)
    if value < BASE * BASE * BASE * BASE - 1:
        meta["successor_id"] = numeral_token_id(value + 1)

    # Factoring for 1..1000
    if 1 <= value <= FACTORING_MAX:
        # Prime factorization: {2: 2, 3: 1} for 12
        pf = factorint(value) if value > 1 else {}
        meta["prime_factorization"] = {str(k): v for k, v in pf.items()}

        # All factor pairs (a, b) with a <= b and a*b = value
        pairs = []
        i = 1
        while i * i <= value:
            if value % i == 0:
                pairs.append([i, value // i])
            i += 1
        meta["factor_pairs"] = pairs

        # All divisors
        divs = sorted(set(d for pair in pairs for d in pair))
        meta["divisors"] = divs

    # Partitions for 1..100
    if 1 <= value <= PARTITION_MAX:
        meta["partition_count"] = int(partition(value))
        meta["partitions_2el"] = canonical_2_partitions(value)

    return meta


def register_namespace_allocations(conn):
    """Insert namespace_allocations entries for AA.AH and AA.AH.AA.*.*"""
    with conn.cursor() as cur:
        # Check parent AA exists
        cur.execute("SELECT pattern FROM namespace_allocations WHERE pattern = 'AA'")
        if not cur.fetchone():
            raise RuntimeError("Parent namespace AA does not exist; cannot register AA.AH")

        # Insert AA.AH (Numerals category)
        cur.execute(
            """
            INSERT INTO namespace_allocations (pattern, name, description, alloc_type, parent, metadata)
            VALUES (%s, %s, %s, %s, %s, %s::jsonb)
            ON CONFLICT (pattern) DO NOTHING
            """,
            (
                "AA.AH",
                "Numerals",
                "Numerical value tokens. AA.AH.AA holds the basic 6.25M sub-block; "
                "additional sub-blocks (AB, AC) reserved for expansion up to 15.625B total.",
                "category",
                "AA",
                json.dumps({"capacity_per_subblock": 6250000, "max_capacity": 15625000000}),
            ),
        )

        # Insert AA.AH.AA.*.* (Basic Numerals token_range)
        cur.execute(
            """
            INSERT INTO namespace_allocations (pattern, name, description, alloc_type, parent, metadata)
            VALUES (%s, %s, %s, %s, %s, %s::jsonb)
            ON CONFLICT (pattern) DO NOTHING
            """,
            (
                "AA.AH.AA.*.*",
                "Basic Numerals",
                "Basic numerals 0 through 6,249,999. y = X+1 progression baseline; "
                "1-1000 carries factoring metadata; 1-100 carries partition metadata.",
                "token_range",
                "AA.AH",
                json.dumps({"size": 6250000, "value_range": [0, 6249999]}),
            ),
        )

    conn.commit()
    print(f"Registered namespace_allocations: AA.AH, AA.AH.AA.*.*")


def insert_numerals(conn, start: int, end_inclusive: int, batch_size: int = 5000):
    """Insert numeral tokens for values in [start, end_inclusive]."""
    rows = []
    for value in range(start, end_inclusive + 1):
        p4, p5 = encode_numeral_position(value)
        meta = build_metadata(value)
        rows.append(
            (
                "AA",  # ns
                NS_NUMERAL,  # p2 = "AH"
                NS_NUMERAL_BASIC,  # p3 = "AA"
                p4,
                p5,
                str(value),  # name
                "numeral",  # category
                "basic",  # subcategory
                json.dumps(meta),  # metadata
            )
        )

    inserted = 0
    with conn.cursor() as cur:
        for i in range(0, len(rows), batch_size):
            batch = rows[i : i + batch_size]
            execute_values(
                cur,
                """
                INSERT INTO tokens (ns, p2, p3, p4, p5, name, category, subcategory, metadata)
                VALUES %s
                ON CONFLICT (token_id) DO NOTHING
                """,
                batch,
                template="(%s, %s, %s, %s, %s, %s, %s, %s, %s::jsonb)",
            )
            inserted += cur.rowcount
    conn.commit()
    print(f"Inserted {inserted} numeral tokens (values {start}..{end_inclusive})")


def verify_sample(conn):
    """Spot-check a few inserted tokens for sanity."""
    with conn.cursor() as cur:
        cur.execute(
            """
            SELECT token_id, name, category, metadata
            FROM tokens
            WHERE ns='AA' AND p2='AH' AND p3='AA'
              AND name IN ('0', '1', '12', '60', '100')
            ORDER BY (metadata->>'value')::int
            """
        )
        rows = cur.fetchall()
        print(f"\nSample verification ({len(rows)} rows):")
        for tid, name, cat, meta in rows:
            meta_summary = {
                k: (
                    v
                    if k in ("value", "partition_count")
                    else (
                        f"<{len(v)} items>"
                        if isinstance(v, list)
                        else (f"<dict {len(v)} keys>" if isinstance(v, dict) else v)
                    )
                )
                for k, v in (meta or {}).items()
            }
            print(f"  {tid}  name={name}  category={cat}")
            print(f"    metadata: {meta_summary}")


def main():
    print(f"Connecting to hcp_core at {DB['host']}:{DB['port']}...")
    conn = psycopg2.connect(**DB)
    try:
        register_namespace_allocations(conn)
        insert_numerals(conn, 0, TEST_MAX)
        verify_sample(conn)
    finally:
        conn.close()
    print("\nTest pass complete.")


if __name__ == "__main__":
    main()
