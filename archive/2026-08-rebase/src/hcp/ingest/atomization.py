"""Add atomization data to Unicode character tokens.

Atomization = the byte-level breakdown of each character per encoding table.
Each Unicode character gets metadata showing how it's represented in:
- UTF-8 (computed from codepoint)
- Legacy encodings (parsed from source tables)

The atomization stores byte Token IDs (AA.AA.AA.AA.{byte}) plus the
encoding table Token ID as prefix.
"""

import re
from pathlib import Path

from ..core.token_id import encode_token_id
from ..db.postgres import connect


# Encoding tables scope: AA.AB.AA (Universal.TextEncodings.Universal)
SCOPE_ENCODING_TABLES = encode_token_id(0, 1, 0)

# Known encoding table Token IDs (3rd pair values under AA.AA.AA.{table})
# We'll assign these sequentially
ENCODING_TABLES = {
    'UTF-8': 0,      # AA.AA.AA.AA - ASCII/UTF-8 (universal)
    '8859-1': 1,     # AA.AA.AA.AB - Latin-1
    '8859-2': 2,
    '8859-3': 3,
    '8859-4': 4,
    '8859-5': 5,
    '8859-6': 6,
    '8859-7': 7,
    '8859-8': 8,
    '8859-9': 9,
    '8859-10': 10,
    '8859-13': 11,
    '8859-14': 12,
    '8859-15': 13,
    '8859-16': 14,
    'CP037': 15,
    'CP500': 16,
    'CP875': 17,
    'CP1026': 18,
    'CP1250': 19,
    'CP1251': 20,
    'CP1252': 21,
    'CP1253': 22,
    'CP1254': 23,
    'CP1255': 24,
    'CP1256': 25,
    'CP1257': 26,
    'CP1258': 27,
    'KOI8-R': 28,
}


def encoding_table_token_id(table_name: str) -> str:
    """Get the Token ID for an encoding table (AA.AB.AA.{table})."""
    table_idx = ENCODING_TABLES.get(table_name, 0)
    return encode_token_id(0, 1, 0, table_idx)


def byte_token_id(byte_value: int) -> str:
    """Get the Token ID for a single byte (0-255)."""
    return encode_token_id(0, 0, 0, 0, byte_value)


def codepoint_to_utf8_bytes(codepoint: int) -> list[int]:
    """Convert a Unicode codepoint to its UTF-8 byte sequence."""
    char = chr(codepoint)
    return list(char.encode('utf-8'))


def parse_encoding_table(filepath: Path) -> dict[int, int]:
    """Parse an encoding table file.

    Returns dict mapping Unicode codepoint -> byte value in this encoding.
    """
    mapping = {}

    with open(filepath, 'r', encoding='utf-8') as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith('#'):
                continue

            # Format: 0xXX\t0xXXXX\t# name
            parts = line.split('\t')
            if len(parts) >= 2:
                try:
                    byte_val = int(parts[0], 16)
                    codepoint = int(parts[1], 16)
                    mapping[codepoint] = byte_val
                except ValueError:
                    continue

    return mapping


def load_all_encoding_tables(source_dir: Path) -> dict[str, dict[int, int]]:
    """Load all encoding tables from source directory.

    Returns dict: table_name -> (codepoint -> byte_value)
    """
    tables = {}

    for filepath in source_dir.glob('*.TXT'):
        # Extract table name from filename
        name = filepath.stem
        # Normalize names: 8859-1.TXT -> 8859-1
        if name.startswith('8859'):
            name = '8859-' + name.split('-')[1] if '-' in name else name

        mapping = parse_encoding_table(filepath)
        if mapping:
            tables[name] = mapping
            print(f"  Loaded {name}: {len(mapping)} mappings")

    return tables


def build_atomization(codepoint: int, encoding_tables: dict) -> dict:
    """Build atomization data for a Unicode codepoint.

    Returns dict with encoding table names as keys, each containing:
    - table_token: the encoding table Token ID
    - bytes: list of byte Token IDs
    """
    atomization = {}

    # UTF-8 (always present for valid Unicode)
    try:
        utf8_bytes = codepoint_to_utf8_bytes(codepoint)
        atomization['UTF-8'] = {
            'table_token': encoding_table_token_id('UTF-8'),
            'bytes': [byte_token_id(b) for b in utf8_bytes],
            'raw': utf8_bytes,
        }
    except (ValueError, OverflowError):
        pass

    # Legacy encodings
    for table_name, mapping in encoding_tables.items():
        if codepoint in mapping:
            byte_val = mapping[codepoint]
            atomization[table_name] = {
                'table_token': encoding_table_token_id(table_name),
                'bytes': [byte_token_id(byte_val)],
                'raw': [byte_val],
            }

    return atomization


def add_atomization_to_tokens(conn, encoding_tables: dict):
    """Update Unicode tokens in database with atomization data."""
    import json

    updated = 0

    with conn.cursor() as cur:
        # Get all Unicode tokens (those with codepoint in metadata)
        cur.execute("""
            SELECT token_id, metadata
            FROM tokens
            WHERE metadata ? 'codepoint'
        """)

        rows = cur.fetchall()
        print(f"  Found {len(rows)} Unicode tokens")

        for token_id, metadata in rows:
            codepoint = metadata.get('codepoint')
            if codepoint is None:
                continue

            atomization = build_atomization(codepoint, encoding_tables)

            if atomization:
                # Update metadata with atomization
                metadata['atomization'] = atomization

                cur.execute("""
                    UPDATE tokens
                    SET metadata = %s::jsonb
                    WHERE token_id = %s
                """, (json.dumps(metadata), token_id))

                updated += 1

                if updated % 500 == 0:
                    print(f"    Updated {updated} tokens...")

    conn.commit()
    return updated


def run():
    """Add atomization data to all Unicode tokens."""
    source_dir = Path(__file__).parent.parent.parent.parent / "sources" / "data" / "encodings"

    print("Loading encoding tables...")
    encoding_tables = load_all_encoding_tables(source_dir)
    print(f"  Loaded {len(encoding_tables)} encoding tables")

    print("\nConnecting to database...")
    conn = connect()

    print("Adding atomization to Unicode tokens...")
    updated = add_atomization_to_tokens(conn, encoding_tables)

    print(f"\nDone! Updated {updated} tokens with atomization data.")

    conn.close()


if __name__ == "__main__":
    run()
