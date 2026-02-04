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
    connect, init_schema, insert_token, insert_scope, insert_namespace, dump_sql
)

# Encoding tables scope sits at AA.AA.AA
SCOPE_ENCODING_TABLES = encode_token_id(0, 0, 0)

# ASCII table scope — first encoding table: AA.AA.AA.AA
SCOPE_ASCII = encode_token_id(0, 0, 0, 0)


def byte_token_id(value: int) -> str:
    """Get the Token ID for a byte code value (0-255).

    Byte codes live at AA.AA.AA.AA.{value} (5-pair).
    """
    return encode_token_id(0, 0, 0, 0, value)


def ingest_namespace_allocations(conn):
    """Register all current namespace allocations in the database."""
    with conn.cursor() as cur:
        # Top-level modes
        insert_namespace(cur, "AA", "Universal", "mode",
                         description="Universal / computational — byte codes, NSM primitives, structural tokens, abbreviation classes")
        insert_namespace(cur, "AB", "Text Mode", "mode",
                         description="Text mode — ASCII/Unicode characters as text tokens, text-specific structures")
        insert_namespace(cur, "y*", "Proper Nouns & Abbreviations", "mode",
                         description="Proper nouns and abbreviations — names, places, organizations, acronyms. Will become separate DB.")
        insert_namespace(cur, "z*", "Source PBMs", "mode",
                         description="Replicable source PBMs — created works, documents, stored expressions")

        # Universal: encoding tables & definitions
        insert_namespace(cur, "AA.AA", "Encoding Tables & Definitions", "category",
                         description="Fixed, small token sets for foundational encodings and primitives",
                         parent="AA")
        insert_namespace(cur, "AA.AA.AA.AA.{count}", "Byte Codes", "token_range",
                         description="All 256 byte values (universal layer)",
                         parent="AA.AA", metadata={"size": 256})
        insert_namespace(cur, "AA.AA.AA.AB.{count}", "NSM Primitives", "token_range",
                         description="~65 Natural Semantic Metalanguage atoms",
                         parent="AA.AA", metadata={"size_approx": 65})

        # Variable token namespace
        insert_namespace(cur, "AA.AA.AB.*.*", "Variable Tokens", "variable_range",
                         description="Working variable tokens for operational use (up to 2500 x 2500 slots)",
                         parent="AA.AA")

        # Structural tokens
        insert_namespace(cur, "AA.AC", "Structural Tokens", "category",
                         description="System-internal tokens: delimiters, scope markers, TBD placeholders",
                         parent="AA")

        # Abbreviation classes
        insert_namespace(cur, "AA.AD", "Abbreviation Classes", "category",
                         description="Source-specific shortcodes, notation conventions, and abbreviations",
                         parent="AA")

        # Text mode: ASCII text characters
        insert_namespace(cur, "AB.AA", "ASCII Text Characters", "category",
                         description="128 ASCII values as text-mode tokens grouped by type",
                         parent="AB")
        insert_namespace(cur, "AB.AA.AA.AA.{count}", "Control Characters", "token_range",
                         description="Non-printing control characters",
                         parent="AB.AA", metadata={"size": 30})
        insert_namespace(cur, "AB.AA.AA.AB.{count}", "Whitespace", "token_range",
                         description="Tab, newline, CR, space",
                         parent="AB.AA", metadata={"size": 4})
        insert_namespace(cur, "AB.AA.AA.AC.{count}", "Digits", "token_range",
                         description="Digits 0-9",
                         parent="AB.AA", metadata={"size": 10})
        insert_namespace(cur, "AB.AA.AA.AD.{count}", "Uppercase Letters", "token_range",
                         description="Uppercase A-Z",
                         parent="AB.AA", metadata={"size": 26})
        insert_namespace(cur, "AB.AA.AA.AE.{count}", "Lowercase Letters", "token_range",
                         description="Lowercase a-z",
                         parent="AB.AA", metadata={"size": 26})
        insert_namespace(cur, "AB.AA.AA.AF.{count}", "Punctuation & Symbols", "token_range",
                         description="Punctuation and symbol characters",
                         parent="AB.AA", metadata={"size": 32})

        # Text mode: Unicode (future)
        insert_namespace(cur, "AB.AB", "Unicode Text Characters", "category",
                         description="Future allocation for Unicode characters beyond ASCII",
                         parent="AB")

        # Source PBMs
        insert_namespace(cur, "zA", "Source PBMs (Universal)", "mode",
                         description="Source PBMs for universal/computational content",
                         parent="z*")
        insert_namespace(cur, "zA.AA", "Source PBMs (Universal Direct)", "category",
                         description="Source PBMs for universal-mode content (byte-level)",
                         parent="zA")
        insert_namespace(cur, "zA.AB", "Source PBMs (Text Mode)", "category",
                         description="Source PBMs for text-mode content",
                         parent="zA")
        insert_namespace(cur, "zA.AA.AA.AA.{count}", "Encoding Table Source PBMs", "token_range",
                         description="Source PBMs for encoding tables (universal/computational sources)",
                         parent="zA.AA")

        # Source PBM 3rd pair convention: double-duty character pair
        # 1st char = document type, 2nd char = format variant
        # e.g. zA.AB.{XY} where X=type, Y=variant
        insert_namespace(cur, "zA.AB.A*", "Tables", "form_type",
                         description="3rd pair 1st char A = table-form documents. 2nd char: A=CSV, B=TSV, C=SSV, etc.",
                         parent="zA.AB",
                         metadata={"pair_convention": "double_duty",
                                   "char1": "document_type",
                                   "char2": "format_variant",
                                   "variants": {"A": "CSV", "B": "TSV", "C": "SSV"}})
        insert_namespace(cur, "zA.AB.B*", "Dictionaries", "form_type",
                         description="3rd pair 1st char B = dictionary-form documents. 2nd char: format variant.",
                         parent="zA.AB",
                         metadata={"pair_convention": "double_duty",
                                   "char1": "document_type",
                                   "char2": "format_variant"})
        insert_namespace(cur, "zA.AB.C*", "Books", "form_type",
                         description="3rd pair 1st char C = book-form documents. 2nd char: format variant.",
                         parent="zA.AB",
                         metadata={"pair_convention": "double_duty",
                                   "char1": "document_type",
                                   "char2": "format_variant"})

    conn.commit()


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


def run(dump_path: str | None = None):
    """Run full byte code ingestion."""
    from .ascii_text import ingest_ascii_text
    from .abbreviations import ingest_abbreviations

    conn = connect()
    init_schema(conn)

    print("Registering namespace allocations...")
    ingest_namespace_allocations(conn)

    print("Ingesting 256 byte code tokens...")
    ingest_byte_codes(conn)
    print(f"  Tokens inserted under {SCOPE_ASCII}")

    print("Ingesting 128 ASCII text-mode tokens...")
    counts = ingest_ascii_text(conn)
    print(f"  Text tokens inserted: {sum(counts.values())}")

    print("Ingesting abbreviation tokens...")
    abbrev_counts = ingest_abbreviations(conn)
    print(f"  Notation: {abbrev_counts['notation']}, Table names: {abbrev_counts['table_names']}")

    # Ingest encoding table source PBMs
    from .encoding_tables import ingest_all_encoding_tables
    source_dir = Path(__file__).parent.parent.parent.parent / "sources" / "data" / "encodings"
    if source_dir.exists():
        print("Building encoding table source PBMs...")
        results = ingest_all_encoding_tables(conn, source_dir)
        print(f"  {len(results)} PBMs created")

    if dump_path:
        print(f"Dumping database to {dump_path}...")
        dump_sql(conn, dump_path)
        print("  Done.")

    conn.close()


if __name__ == "__main__":
    run(dump_path="db/core.sql")
