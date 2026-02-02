"""Ingest byte codes into the core database.

Creates tokens for all 256 byte codes and builds a PBM for the
ASCII encoding table (the source file itself).

Token addressing:
    AA.AA.AA.XX  — individual byte code tokens (XX = byte value)

Source PBM addressing:
    zA.AA.AA.AA.XX — PBMs for encoding table sources
"""

import json
from pathlib import Path

from ..core.token_id import encode_token_id, encode_pair, MODE_UNIVERSAL
from ..core.byte_codes import BYTE_TABLE, ByteCategory
from ..db.postgres import (
    connect, init_schema, insert_token, insert_scope, insert_pbm_entry, dump_sql
)

# Encoding tables scope sits at AA.AA.AA
SCOPE_ENCODING_TABLES = encode_token_id(0, 0, 0)

# ASCII table scope — first encoding table: AA.AA.AA.AA
SCOPE_ASCII = encode_token_id(0, 0, 0, 0)

# Source PBM for ASCII table: zA.AA.AA.AA.AA
SOURCE_PBM_ASCII = encode_token_id(2450, 0, 0, 0, 0)


def byte_token_id(value: int) -> str:
    """Get the Token ID for a byte code value (0-255)."""
    return encode_token_id(0, 0, 0, value)


def ingest_byte_codes(conn):
    """Insert all 256 byte code tokens into the database."""
    with conn.cursor() as cur:
        # Register the encoding tables scope
        insert_scope(cur, SCOPE_ENCODING_TABLES,
                     "Encoding Tables", "category")

        # Register ASCII as a sub-scope
        insert_scope(cur, SCOPE_ASCII,
                     "ASCII (US-ASCII, 7-bit)", "encoding_table",
                     parent_id=SCOPE_ENCODING_TABLES,
                     metadata={"standard": "ANSI X3.4-1986",
                               "size": 128,
                               "bits": 7})

        # Insert all 256 byte code tokens
        for bc in BYTE_TABLE:
            tid = byte_token_id(bc.value)
            metadata = {
                "hex": bc.hex,
                "bond_class": bc.bond_class.value,
                "display": bc.display,
            }
            if bc.ascii_char is not None:
                metadata["ascii_char"] = bc.ascii_char

            insert_token(cur, tid, bc.name,
                         category=bc.category.value,
                         subcategory=bc.bond_class.value,
                         metadata=metadata)

    conn.commit()


def ingest_ascii_source_pbm(conn, ascii_file: Path):
    """Build a PBM from the ASCII encoding table source file.

    This creates forward pair-bonds from the sequential byte values
    in the source file itself, recording the structure of the file
    as a PBM. Words in headers become TBD tokens at the byte level
    until the dictionary is ingested.
    """
    with conn.cursor() as cur:
        # Register the source PBM scope
        insert_scope(cur, SOURCE_PBM_ASCII,
                     "ASCII Table Source PBM", "source_pbm",
                     metadata={"source_file": str(ascii_file)})

        # Read the file as bytes
        data = ascii_file.read_bytes()

        # Build FPBs from sequential byte pairs
        for i in range(len(data) - 1):
            t0 = byte_token_id(data[i])
            t1 = byte_token_id(data[i + 1])
            insert_pbm_entry(cur, SOURCE_PBM_ASCII, t0, t1,
                             fbr=1, position=i)

    conn.commit()


def run(dump_path: str | None = None):
    """Run full byte code ingestion."""
    conn = connect()
    init_schema(conn)

    print("Ingesting 256 byte code tokens...")
    ingest_byte_codes(conn)
    print(f"  Tokens inserted under {SCOPE_ASCII}")

    # Ingest ASCII table source PBM if the file exists
    ascii_file = Path(__file__).parent.parent.parent.parent / "sources" / "data" / "encodings"
    # ASCII is the base — use the 8859-1 file which is a superset
    # but we could also just create the PBM from our own byte definitions
    # For now, if any encoding file exists, build a PBM from it
    latin1 = ascii_file / "8859-1.TXT"
    if latin1.exists():
        print(f"Building source PBM from {latin1}...")
        ingest_ascii_source_pbm(conn, latin1)
        print(f"  PBM stored under {SOURCE_PBM_ASCII}")

    if dump_path:
        print(f"Dumping database to {dump_path}...")
        dump_sql(conn, dump_path)
        print("  Done.")

    conn.close()


if __name__ == "__main__":
    run(dump_path="db/core.sql")
