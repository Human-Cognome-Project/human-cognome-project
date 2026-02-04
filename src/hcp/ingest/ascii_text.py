"""Ingest ASCII text-mode tokens into the core database.

Creates 128 text-mode tokens under AB.AA.AA.{group}.{count},
each referencing its byte code counterpart in AA.AA.AA.AA.{value}.

Groups (4th pair):
    AA — Control characters (non-printing): 33
    AB — Whitespace (tab, newline, CR, space): 4
    AC — Digits 0-9: 10
    AD — Uppercase A-Z: 26
    AE — Lowercase a-z: 26
    AF — Punctuation & symbols: 33
"""

from ..core.token_id import encode_token_id, MODE_TEXT
from ..core.byte_codes import BYTE_TABLE, ByteCategory
from ..db.postgres import connect, init_schema, insert_token, insert_scope

# Text mode scope for ASCII characters: AB.AA.AA
SCOPE_ASCII_TEXT = encode_token_id(1, 0, 0)

# Group codes (4th pair values)
GROUP_CONTROL = 0       # AA
GROUP_WHITESPACE = 1    # AB
GROUP_DIGITS = 2        # AC
GROUP_UPPERCASE = 3     # AD
GROUP_LOWERCASE = 4     # AE
GROUP_PUNCTUATION = 5   # AF

# Map byte categories to text-mode groups
_CATEGORY_TO_GROUP = {
    ByteCategory.CONTROL: GROUP_CONTROL,
    ByteCategory.WHITESPACE: GROUP_WHITESPACE,
    ByteCategory.DIGIT: GROUP_DIGITS,
    ByteCategory.LETTER_UPPER: GROUP_UPPERCASE,
    ByteCategory.LETTER_LOWER: GROUP_LOWERCASE,
    ByteCategory.PUNCTUATION: GROUP_PUNCTUATION,
}

_GROUP_NAMES = {
    GROUP_CONTROL: "Control Characters",
    GROUP_WHITESPACE: "Whitespace",
    GROUP_DIGITS: "Digits",
    GROUP_UPPERCASE: "Uppercase Letters",
    GROUP_LOWERCASE: "Lowercase Letters",
    GROUP_PUNCTUATION: "Punctuation & Symbols",
}


def ascii_text_token_id(group: int, index: int) -> str:
    """Get the Token ID for an ASCII text token."""
    return encode_token_id(1, 0, 0, group, index)


def byte_token_id(value: int) -> str:
    """Get the byte-code Token ID for a byte value.

    Byte codes live at AA.AA.AA.AA.{value} (5-pair).
    """
    return encode_token_id(0, 0, 0, 0, value)


def ingest_ascii_text(conn):
    """Insert all 128 ASCII text-mode tokens into the database."""
    # Track per-group counters
    group_counts = {g: 0 for g in _GROUP_NAMES}

    with conn.cursor() as cur:
        # Register the text-mode ASCII scope
        insert_scope(cur, SCOPE_ASCII_TEXT,
                     "ASCII Text Characters", "text_tokens",
                     metadata={"mode": "text", "standard": "US-ASCII",
                               "size": 128})

        # Register group sub-scopes
        for group_id, group_name in _GROUP_NAMES.items():
            scope_id = encode_token_id(1, 0, 0, group_id)
            insert_scope(cur, scope_id, group_name, "text_token_group",
                         parent_id=SCOPE_ASCII_TEXT)

        # Insert tokens for ASCII values 0-127
        for value in range(128):
            bc = BYTE_TABLE[value]
            cat = bc.category

            if cat not in _CATEGORY_TO_GROUP:
                continue  # skip non-ASCII categories (shouldn't happen for 0-127)

            group = _CATEGORY_TO_GROUP[cat]
            index = group_counts[group]
            group_counts[group] += 1

            tid = ascii_text_token_id(group, index)
            byte_ref = byte_token_id(value)

            metadata = {
                "byte_code_ref": byte_ref,
                "ascii_value": value,
                "hex": bc.hex,
                "display": bc.display,
            }
            if bc.ascii_char is not None:
                metadata["char"] = bc.ascii_char

            insert_token(cur, tid, bc.name,
                         category=cat.value,
                         subcategory=bc.bond_class.value,
                         metadata=metadata)

    conn.commit()
    return group_counts


def run():
    """Run ASCII text token ingestion standalone."""
    conn = connect()
    init_schema(conn)

    print("Ingesting 128 ASCII text-mode tokens...")
    counts = ingest_ascii_text(conn)
    for group_id, count in sorted(counts.items()):
        print(f"  {_GROUP_NAMES[group_id]}: {count}")
    print(f"  Total: {sum(counts.values())}")

    conn.close()


if __name__ == "__main__":
    run()
