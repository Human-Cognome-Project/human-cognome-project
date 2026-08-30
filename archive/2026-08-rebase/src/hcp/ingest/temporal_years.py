"""Ingest CE year tokens into core shard at AA.AD.CE.{year}.

This creates temporal reference tokens for Common Era years. While not all
cultures use the CE system, it serves as a standard computational reference
for temporal indexing and PBM metadata.

Years also exist in other namespaces:
- y* (name components): atomic elements in proper nouns like "Class of 2024"
- x* (things): years as distinct cultural entities like "1984" or "2020"
"""

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent.parent.parent))

from hcp.db.postgres import connect as connect_core, insert_token
from hcp.core.token_id import encode_pair


def ingest_years(conn, start_year: int = 0, end_year: int = 3000):
    """Ingest CE year tokens into core shard.

    Args:
        conn: connection to hcp_core
        start_year: first year to ingest (default 0 CE)
        end_year: last year to ingest (default 3000 CE)
    """
    print(f"Ingesting CE years {start_year}-{end_year}...")

    inserted = 0

    with conn.cursor() as cur:
        for year in range(start_year, end_year + 1):
            # Encode year as two base-50 pairs
            # For years 0-2499, first pair is AA, second varies
            # For years 2500+, both pairs vary
            first_pair = encode_pair(year // 2500)
            second_pair = encode_pair(year % 2500)

            token_id = f"AA.AD.CE.{first_pair}.{second_pair}"
            name = f"{year} CE"

            metadata = {
                "year": year,
                "era": "CE",
                "temporal_type": "year"
            }

            insert_token(cur, token_id, name,
                        category="temporal",
                        subcategory="year_ce",
                        metadata=metadata)

            inserted += 1

            # Progress indicator
            if year % 100 == 0:
                print(f"  {token_id}  {name}")

    conn.commit()
    print(f"\nInserted {inserted} year tokens")


def verify_ingestion(conn):
    """Print stats on ingested years."""
    with conn.cursor() as cur:
        cur.execute("""
            SELECT
                COUNT(*) as total,
                MIN((metadata->>'year')::int) as min_year,
                MAX((metadata->>'year')::int) as max_year
            FROM tokens
            WHERE category = 'temporal'
              AND subcategory = 'year_ce'
        """)

        total, min_year, max_year = cur.fetchone()

        print(f"\n{'='*60}")
        print(f"CE Year Tokens Summary:")
        print(f"{'='*60}")
        print(f"Total years:  {total}")
        print(f"Range:        {min_year} CE to {max_year} CE")

        # Show samples
        print(f"\nSample tokens:")
        cur.execute("""
            SELECT token_id, name
            FROM tokens
            WHERE category = 'temporal'
              AND subcategory = 'year_ce'
              AND (metadata->>'year')::int IN (0, 1, 1000, 1776, 1984, 2000, 2024, 2500, 3000)
            ORDER BY (metadata->>'year')::int
        """)

        for token_id, name in cur.fetchall():
            print(f"  {token_id}  {name}")


def main():
    import argparse

    parser = argparse.ArgumentParser(description='Ingest CE year tokens into core shard')
    parser.add_argument('--start', type=int, default=0, help='First year to ingest')
    parser.add_argument('--end', type=int, default=3000, help='Last year to ingest')
    args = parser.parse_args()

    print("Connecting to core database...")
    conn = connect_core()

    try:
        ingest_years(conn, args.start, args.end)
        verify_ingestion(conn)
    finally:
        conn.close()

    print("\nDone! CE year tokens ingested into core shard at AA.AD.CE.*")
    return 0


if __name__ == '__main__':
    sys.exit(main())
