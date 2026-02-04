"""Ingest system abbreviations into the core database.

Creates tokens under AA.AD (Abbreviation Classes) for notation
conventions and document titles that appear in source data.

Token addressing:
    AA.AD.AA.{count}  — notation conventions (0x, #, etc.)
    AA.AD.AB.{count}  — encoding table names
"""

from ..core.token_id import encode_token_id
from ..db.postgres import connect, init_schema, insert_token, insert_scope

# Abbreviation classes scope: AA.AD
SCOPE_ABBREVIATIONS = encode_token_id(0, 3)

# Sub-scopes
SCOPE_NOTATION = encode_token_id(0, 3, 0)      # AA.AD.AA — notation conventions
SCOPE_TABLE_NAMES = encode_token_id(0, 3, 1)   # AA.AD.AB — encoding table names


def notation_token_id(index: int) -> str:
    """Get Token ID for a notation convention."""
    return encode_token_id(0, 3, 0, index)


def table_name_token_id(index: int) -> str:
    """Get Token ID for an encoding table name."""
    return encode_token_id(0, 3, 1, index)


# Notation conventions used in encoding tables
NOTATION_CONVENTIONS = [
    ("0x", "Hexadecimal prefix"),
    ("#", "Comment marker"),
    ("0xXX", "Single-byte hex placeholder"),
    ("0xXXXX", "Double-byte hex placeholder"),
]

# Official encoding table names (from Unicode.org source files)
ENCODING_TABLE_NAMES = [
    ("ISO/IEC 8859-1:1998", "Latin-1 Western European"),
    ("ISO/IEC 8859-2:1999", "Latin-2 Central European"),
    ("ISO/IEC 8859-3:1999", "Latin-3 South European"),
    ("ISO/IEC 8859-4:1998", "Latin-4 North European"),
    ("ISO/IEC 8859-5:1999", "Latin/Cyrillic"),
    ("ISO/IEC 8859-6:1999", "Latin/Arabic"),
    ("ISO/IEC 8859-7:2003", "Latin/Greek"),
    ("ISO/IEC 8859-8:1999", "Latin/Hebrew"),
    ("ISO/IEC 8859-9:1999", "Latin-5 Turkish"),
    ("ISO/IEC 8859-10:1998", "Latin-6 Nordic"),
    ("ISO/IEC 8859-13:1998", "Latin-7 Baltic Rim"),
    ("ISO/IEC 8859-14:1998", "Latin-8 Celtic"),
    ("ISO/IEC 8859-15:1999", "Latin-9 Western European with Euro"),
    ("ISO/IEC 8859-16:2001", "Latin-10 South-Eastern European"),
    ("IBM Code Page 037", "EBCDIC US/Canada"),
    ("IBM Code Page 500", "EBCDIC International"),
    ("IBM Code Page 875", "EBCDIC Greek"),
    ("IBM Code Page 1026", "EBCDIC Turkish"),
    ("Windows Code Page 1250", "Windows Central European"),
    ("Windows Code Page 1251", "Windows Cyrillic"),
    ("Windows Code Page 1252", "Windows Western European"),
    ("Windows Code Page 1253", "Windows Greek"),
    ("Windows Code Page 1254", "Windows Turkish"),
    ("Windows Code Page 1255", "Windows Hebrew"),
    ("Windows Code Page 1256", "Windows Arabic"),
    ("Windows Code Page 1257", "Windows Baltic"),
    ("Windows Code Page 1258", "Windows Vietnamese"),
    ("KOI8-R", "Russian Cyrillic"),
]


def ingest_abbreviations(conn):
    """Insert abbreviation tokens into the database."""
    with conn.cursor() as cur:
        # Register the abbreviation classes scope
        insert_scope(cur, SCOPE_ABBREVIATIONS,
                     "Abbreviation Classes", "category",
                     metadata={"mode": "universal"})

        # Register sub-scopes
        insert_scope(cur, SCOPE_NOTATION,
                     "Notation Conventions", "abbreviation_group",
                     parent_id=SCOPE_ABBREVIATIONS)
        insert_scope(cur, SCOPE_TABLE_NAMES,
                     "Encoding Table Names", "abbreviation_group",
                     parent_id=SCOPE_ABBREVIATIONS)

        # Insert notation conventions
        for i, (abbrev, desc) in enumerate(NOTATION_CONVENTIONS):
            tid = notation_token_id(i)
            insert_token(cur, tid, abbrev,
                         category="notation",
                         metadata={"display": abbrev, "description": desc})

        # Insert encoding table names
        for i, (name, desc) in enumerate(ENCODING_TABLE_NAMES):
            tid = table_name_token_id(i)
            insert_token(cur, tid, name,
                         category="table_name",
                         metadata={"display": name, "description": desc})

    conn.commit()
    return {
        "notation": len(NOTATION_CONVENTIONS),
        "table_names": len(ENCODING_TABLE_NAMES),
    }


def run():
    """Run abbreviation ingestion standalone."""
    conn = connect()
    init_schema(conn)

    print("Ingesting abbreviation tokens...")
    counts = ingest_abbreviations(conn)
    print(f"  Notation conventions: {counts['notation']}")
    print(f"  Table names: {counts['table_names']}")

    conn.close()


if __name__ == "__main__":
    run()
