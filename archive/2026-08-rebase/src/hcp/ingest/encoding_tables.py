"""Ingest encoding table source files as PBMs.

Creates source PBMs for all encoding table files under zA.AA.AA.AA.{count}.
Each PBM captures the exact byte sequence of the source file.

Source files: sources/data/encodings/*.TXT
"""

from pathlib import Path

from ..core.token_id import encode_token_id
from ..db.postgres import connect, init_schema, dump_sql
from ..db.pbm import build_pbm, store_pbm

# Base URL for Unicode.org encoding mappings
UNICODE_ORG_BASE = "https://unicode.org/Public/MAPPINGS"

# Encoding table source files with metadata
# (filename, title, source_url_path, has_headers)
ENCODING_TABLES = [
    # ISO 8859 series
    ("8859-1.TXT", "ISO/IEC 8859-1:1998", "ISO8859/8859-1.TXT", True),
    ("8859-2.TXT", "ISO/IEC 8859-2:1999", "ISO8859/8859-2.TXT", True),
    ("8859-3.TXT", "ISO/IEC 8859-3:1999", "ISO8859/8859-3.TXT", True),
    ("8859-4.TXT", "ISO/IEC 8859-4:1998", "ISO8859/8859-4.TXT", True),
    ("8859-5.TXT", "ISO/IEC 8859-5:1999", "ISO8859/8859-5.TXT", True),
    ("8859-6.TXT", "ISO/IEC 8859-6:1999", "ISO8859/8859-6.TXT", True),
    ("8859-7.TXT", "ISO/IEC 8859-7:2003", "ISO8859/8859-7.TXT", True),
    ("8859-8.TXT", "ISO/IEC 8859-8:1999", "ISO8859/8859-8.TXT", True),
    ("8859-9.TXT", "ISO/IEC 8859-9:1999", "ISO8859/8859-9.TXT", True),
    ("8859-10.TXT", "ISO/IEC 8859-10:1998", "ISO8859/8859-10.TXT", True),
    ("8859-13.TXT", "ISO/IEC 8859-13:1998", "ISO8859/8859-13.TXT", True),
    ("8859-14.TXT", "ISO/IEC 8859-14:1998", "ISO8859/8859-14.TXT", True),
    ("8859-15.TXT", "ISO/IEC 8859-15:1999", "ISO8859/8859-15.TXT", True),
    ("8859-16.TXT", "ISO/IEC 8859-16:2001", "ISO8859/8859-16.TXT", True),
    # IBM EBCDIC code pages
    ("CP037.TXT", "IBM Code Page 037", "VENDORS/MICSFT/EBCDIC/CP037.TXT", True),
    ("CP500.TXT", "IBM Code Page 500", "VENDORS/MICSFT/EBCDIC/CP500.TXT", True),
    ("CP875.TXT", "IBM Code Page 875", "VENDORS/MICSFT/EBCDIC/CP875.TXT", True),
    ("CP1026.TXT", "IBM Code Page 1026", "VENDORS/MICSFT/EBCDIC/CP1026.TXT", True),
    # Windows code pages
    ("CP1250.TXT", "Windows Code Page 1250", "VENDORS/MICSFT/WINDOWS/CP1250.TXT", True),
    ("CP1251.TXT", "Windows Code Page 1251", "VENDORS/MICSFT/WINDOWS/CP1251.TXT", True),
    ("CP1252.TXT", "Windows Code Page 1252", "VENDORS/MICSFT/WINDOWS/CP1252.TXT", True),
    ("CP1253.TXT", "Windows Code Page 1253", "VENDORS/MICSFT/WINDOWS/CP1253.TXT", True),
    ("CP1254.TXT", "Windows Code Page 1254", "VENDORS/MICSFT/WINDOWS/CP1254.TXT", True),
    ("CP1255.TXT", "Windows Code Page 1255", "VENDORS/MICSFT/WINDOWS/CP1255.TXT", True),
    ("CP1256.TXT", "Windows Code Page 1256", "VENDORS/MICSFT/WINDOWS/CP1256.TXT", True),
    ("CP1257.TXT", "Windows Code Page 1257", "VENDORS/MICSFT/WINDOWS/CP1257.TXT", True),
    ("CP1258.TXT", "Windows Code Page 1258", "VENDORS/MICSFT/WINDOWS/CP1258.TXT", True),
    # KOI8
    ("KOI8-R.TXT", "KOI8-R", "VENDORS/MISC/KOI8-R.TXT", True),
]


def encoding_table_pbm_id(index: int) -> str:
    """Get the PBM scope ID for an encoding table by index."""
    # zA.AA.AA.AA.{count}
    return encode_token_id(2450, 0, 0, 0, index)


def byte_token_id(value: int) -> str:
    """Get the Token ID for a byte code value (0-255).

    Byte codes live at AA.AA.AA.AA.{value} (5-pair).
    """
    return encode_token_id(0, 0, 0, 0, value)


def ingest_encoding_table_pbm(conn, source_dir: Path, index: int,
                               filename: str, title: str, url_path: str,
                               has_headers: bool):
    """Build and store a PBM for a single encoding table file."""
    filepath = source_dir / filename
    if not filepath.exists():
        return None

    data = filepath.read_bytes()
    token_sequence = [byte_token_id(b) for b in data]

    scope_id = encoding_table_pbm_id(index)
    pbm_data = build_pbm(scope_id, token_sequence)

    metadata = {
        "format": {
            "type": "table",
            "delimiter": "\t",
            "has_headers": has_headers,
            "token_prefix": "AA.AA.AA",
        },
        "title": title,
        "source_url": f"{UNICODE_ORG_BASE}/{url_path}",
    }

    store_pbm(conn, scope_id, pbm_data,
              scope_name=f"{title} Source PBM",
              scope_type="source_pbm",
              metadata=metadata)

    return scope_id


def ingest_all_encoding_tables(conn, source_dir: Path):
    """Ingest PBMs for all encoding table files."""
    results = []
    for i, (filename, title, url_path, has_headers) in enumerate(ENCODING_TABLES):
        scope_id = ingest_encoding_table_pbm(
            conn, source_dir, i, filename, title, url_path, has_headers
        )
        if scope_id:
            results.append((scope_id, title))
    return results


def run(dump_path: str | None = None):
    """Run encoding table PBM ingestion standalone."""
    conn = connect()
    init_schema(conn)

    source_dir = Path(__file__).parent.parent.parent.parent / "sources" / "data" / "encodings"

    print(f"Ingesting encoding table PBMs from {source_dir}...")
    results = ingest_all_encoding_tables(conn, source_dir)
    for scope_id, title in results:
        print(f"  {scope_id}: {title}")
    print(f"Total: {len(results)} PBMs")

    if dump_path:
        print(f"Dumping database to {dump_path}...")
        dump_sql(conn, dump_path)

    conn.close()


if __name__ == "__main__":
    run(dump_path="db/core.sql")
