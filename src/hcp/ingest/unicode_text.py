"""Ingest Unicode text-mode tokens from the unicode_table.txt source.

Creates tokens under AB.AA.AA.{category}.{count} for Unicode characters,
where the 4th pair represents the major category and 5th pair is the
count within that category.

Source: https://gist.github.com/r-lyeh/f8273cbf22e8743eae2579eed806c1a4
"""

import re
from pathlib import Path

from ..core.token_id import encode_token_id
from ..db.postgres import connect, init_schema, insert_token, insert_scope, insert_namespace

# Text mode scope for Unicode characters: AB.AA.AA
SCOPE_UNICODE_TEXT = encode_token_id(1, 0, 0)

# Category starting offset (ASCII uses 0-5, we start Unicode at 6)
UNICODE_CATEGORY_OFFSET = 6

# Regex to parse character lines: CHAR  	U+XXXX (alt-NNN)	DESCRIPTION
CHAR_LINE_RE = re.compile(
    r'^(.+?)\s+U\+([0-9A-Fa-f]{4,6})\s+\(alt-(\d+)\)\s+(.+)$'
)


def normalize_category(raw_category: str) -> str:
    """Normalize category names to consolidate duplicates and variations."""
    # Strip any trailing qualifiers in parentheses for grouping
    cat = raw_category.strip()

    # Remove ALL parenthetical content and asterisks
    cat = re.sub(r'\s*\([^)]*\)', '', cat)
    cat = re.sub(r'\s*\*+', '', cat)
    cat = cat.strip()

    # Consolidation map for categories that appear multiple times
    consolidations = {
        "ASCII punctuation and symbols": "ASCII Punctuation",
        "Latin-1 punctuation and symbols": "Latin-1 Punctuation",
        "Latin-1 punctuation and symbols from here.)": "Latin-1 Punctuation",
        "General punctuation": "General Punctuation",
        "ASCII digits": "ASCII Digits",
        "Uppercase Latin alphabet": "Latin Uppercase",
        "Lowercase Latin alphabet": "Latin Lowercase",
        "Control character": "Control Characters",
        "Basic Russian alphabet": "Cyrillic",
        "Double punctuation for vertical text": "General Punctuation",
        "Quotation marks and apostrophe": "Quotation Marks",
        "Quotation marks": "Quotation Marks",
        "IPA extensions": "IPA Extensions",
        "Mathematical operator": "Mathematical Operators",
        "Miscellaneous mathematical symbols": "Miscellaneous Math Symbols",
        "Miscellaneous mathematical symbol": "Miscellaneous Math Symbols",
        "Light and heavy solid lines": "Box Drawing",
        "Light and heavy dashed lines": "Box Drawing",
        "Light and heavy line box components": "Box Drawing",
        "Light and double line box components": "Box Drawing",
        "Double lines": "Box Drawing",
        "Character cell arcs": "Box Drawing",
        "Character cell diagonals": "Box Drawing",
        "Light and heavy half lines": "Box Drawing",
        "Mixed light and heavy lines": "Box Drawing",
        "Block elements": "Block Elements",
        "Shade characters": "Block Elements",
        "Terminal graphic characters": "Block Elements",
        "Geometric shapes": "Geometric Shapes",
        "Control code graphics": "Geometric Shapes",
        "Miscellaneous symbols": "Miscellaneous Symbols",
        "Miscellaneous symbol": "Miscellaneous Symbols",
        "Miscellaneous technical": "Technical Symbols",
        "User interface symbols": "Technical Symbols",
        "Keyboard symbols": "Technical Symbols",
        "Keyboard symbol": "Technical Symbols",
        "Keyboard and UI symbols": "Technical Symbols",
        "Arrows with modifications": "Arrows",
        "Arrows with bent tips": "Arrows",
        "Keyboard symbols and circle arrows": "Arrows",
        "Paired arrows and harpoons": "Arrows",
        "Double arrows": "Arrows",
        "Miscellaneous arrows and keyboard symbols": "Arrows",
        "White arrows and keyboard symbols": "Arrows",
        "Miscellaneous arrows": "Arrows",
        "Simple arrows": "Arrows",
        "Long arrows": "Arrows",
        "Arrow tails": "Arrows",
        "Crossing arrows for knot theory": "Arrows",
        "Miscellaneous curved arrows": "Arrows",
        "Arrows combined with operators": "Arrows",
        "White and black arrows": "Arrows",
        "Dingbat arrow": "Arrows",
        "Dingbat arrows": "Arrows",
        "N-ary operators": "N-ary Operators",
        "Operators": "Operators",
        "Operator": "Operators",
        "Relations": "Relations",
        "Relation": "Relations",
        "Logical and set operators": "Logical Operators",
        "Logical operators": "Logical Operators",
        "Ceilings and floors": "Brackets and Delimiters",
        "Bracket pieces": "Brackets and Delimiters",
        "Frown and smile": "Brackets and Delimiters",
        "Half brackets": "Brackets and Delimiters",
        "Fullwidth brackets": "Brackets and Delimiters",
        "Set membership": "Set Theory",
        "Integrals": "Integrals",
        "Integral pieces": "Integrals",
        "Harpoons": "Harpoons",
        "Double-barbed harpoons": "Harpoons",
        "Weather and astrological symbols": "Astrological Symbols",
        "Astrological symbols": "Astrological Symbols",
        "Astrological signs": "Astrological Symbols",
        "Zodiacal symbols": "Astrological Symbols",
        "Astronomical symbol": "Astrological Symbols",
        "Religious and political symbols": "Religious Symbols",
        "Medical and healing symbols": "Medical Symbols",
        "Recycling symbols": "Recycling Symbols",
        "Warning signs": "Warning Signs",
        "Pointing hand symbols": "Hand Symbols",
        "Gender symbols": "Gender Symbols",
        "Gender symbol": "Gender Symbols",
        "Genealogical symbols": "Gender Symbols",
        "Chess symbols": "Game Symbols",
        "Playing card symbols": "Game Symbols",
        "Japanese chess symbols": "Game Symbols",
        "Symbols for draughts and checkers": "Game Symbols",
        "Dice": "Game Symbols",
        "Musical symbols": "Musical Symbols",
        "Dictionary and map symbols": "Map Symbols",
        "Map symbols from ARIB STD B24": "Map Symbols",
        "Symbols for closed captioning from ARIB STD B24": "Broadcast Symbols",
        "Yijing trigram symbols": "Chinese Symbols",
        "Pentagram symbols": "Occult Symbols",
        "Weather symbol": "Weather Symbols",
        "Stars, asterisks and snowflakes": "Stars and Asterisks",
        "Punctuation ornaments": "Ornaments",
        "Crosses": "Crosses",
        "Circles": "Circles",
        "Squares": "Squares",
        "Diamonds": "Diamonds",
        "European Latin": "European Latin",
        "Non-European and historic Latin": "Extended Latin",
        "Lowercase Claudian letter": "Extended Latin",
        "Orthographic Latin additions": "Extended Latin",
        "Latin general extensions": "Extended Latin",
        "Latin ligatures": "Ligatures",
        "Latin letters": "Latin Letters",
        "Greek letters": "Greek Letters",
        "Hebrew letterlike math symbols": "Math Letterlike",
        "Additional letterlike symbols": "Math Letterlike",
        "Double-struck italic math symbols": "Math Letterlike",
        "Roman numerals": "Numerals",
        "Fractions": "Fractions",
        "Subscripts": "Sub and Superscripts",
        "Superscripts": "Sub and Superscripts",
        "Currency symbols": "Currency Symbols",
        "Currency sign": "Currency Symbols",
        "CJK symbols and punctuation": "CJK Symbols",
        "Glyphs for vertical variants": "CJK Symbols",
        "Sidelining emphasis marks": "CJK Symbols",
        "Small form variants": "CJK Symbols",
        "Fullwidth ASCII variants": "Fullwidth Forms",
        "Fullwidth symbol variants": "Fullwidth Forms",
        "Overscores and underscores": "Combining Marks",
        "Electrotechnical symbols": "Technical Symbols",
        "Chemistry symbol": "Technical Symbols",
        "Specific symbols for space": "Technical Symbols",
        "Historic punctuation": "Historic Punctuation",
        "Miscellaneous additions": "Miscellaneous",
        "Miscellaneous": "Miscellaneous",
        "Special": "Special",
        "Replacement characters": "Special",
        "Addition for German typography": "Typography",
        "Dashes": "Dashes",
        "Punctuation": "Punctuation",
    }

    return consolidations.get(cat, cat)


def parse_description(desc: str) -> list[str]:
    """Parse a description field, splitting on '=' and ',' for equivalent terms."""
    desc = desc.strip()
    # Split by '=' or ',' to get all equivalent terms as an array
    parts = re.split(r'\s*[=,]\s*', desc)
    # Filter out empty strings and strip whitespace
    return [p.strip() for p in parts if p.strip()]


def parse_unicode_table(filepath: Path) -> list[dict]:
    """Parse the unicode_table.txt file into structured entries."""
    entries = []
    current_category = None
    preamble_done = False

    with open(filepath, 'r', encoding='utf-8') as f:
        for line_num, line in enumerate(f, 1):
            line = line.rstrip('\n\r')

            # Skip empty lines
            if not line.strip():
                continue

            # Check for character entry
            match = CHAR_LINE_RE.match(line)
            if match:
                preamble_done = True
                char, codepoint_hex, alt_code, description = match.groups()

                codepoint = int(codepoint_hex, 16)
                terms = parse_description(description)

                entries.append({
                    "char": char.strip(),
                    "codepoint": codepoint,
                    "codepoint_hex": codepoint_hex.upper(),
                    "alt_code": int(alt_code),
                    "terms": terms,
                    "raw_category": current_category,
                    "category": normalize_category(current_category) if current_category else "Uncategorized",
                    "line_num": line_num,
                })
            else:
                # Check if this looks like a section header (starts with letter, no U+)
                if line[0].isalpha() and 'U+' not in line and preamble_done:
                    # It's a category header
                    current_category = line.strip()
                elif not preamble_done:
                    # Check if this is a category header (even in preamble area)
                    if line.startswith("ASCII") or line.startswith("Uppercase") or line.startswith("Lowercase"):
                        current_category = line.strip()
                        preamble_done = True

    return entries


def build_category_map(entries: list[dict]) -> dict[str, int]:
    """Build a map from category name to category ID (4th pair value)."""
    # Get unique categories in order of first appearance
    seen = set()
    categories = []
    for e in entries:
        cat = e["category"]
        if cat not in seen:
            seen.add(cat)
            categories.append(cat)

    # Assign IDs starting from offset
    return {cat: UNICODE_CATEGORY_OFFSET + i for i, cat in enumerate(categories)}


def unicode_text_token_id(category_id: int, index: int) -> str:
    """Get the Token ID for a Unicode text token."""
    return encode_token_id(1, 0, 0, category_id, index)


# Homoglyph groups: characters that look visually identical
# Each tuple contains codepoints that are visually similar
HOMOGLYPH_GROUPS = [
    # Latin uppercase ↔ Cyrillic uppercase
    (0x0041, 0x0410, 0x0391),  # A, Cyrillic А, Greek Α
    (0x0042, 0x0412),          # B, Cyrillic В (VE)
    (0x0043, 0x0421, 0x03F9),  # C, Cyrillic С (ES), Greek lunate Sigma
    (0x0045, 0x0415),          # E, Cyrillic Е
    (0x0048, 0x041D, 0x0397),  # H, Cyrillic Н (EN), Greek Η (Eta)
    (0x0049, 0x0406, 0x0399),  # I, Cyrillic І, Greek Ι (Iota)
    (0x004A, 0x0408),          # J, Cyrillic Ј
    (0x004B, 0x041A, 0x039A),  # K, Cyrillic К, Greek Κ (Kappa)
    (0x004D, 0x041C, 0x039C),  # M, Cyrillic М, Greek Μ (Mu)
    (0x004E, 0x039D),          # N, Greek Ν (Nu)
    (0x004F, 0x041E, 0x039F),  # O, Cyrillic О, Greek Ο (Omicron)
    (0x0050, 0x0420, 0x03A1),  # P, Cyrillic Р (ER), Greek Ρ (Rho)
    (0x0053, 0x0405),          # S, Cyrillic Ѕ
    (0x0054, 0x0422, 0x03A4),  # T, Cyrillic Т, Greek Τ (Tau)
    (0x0058, 0x0425, 0x03A7),  # X, Cyrillic Х (HA), Greek Χ (Chi)
    (0x0059, 0x04AE, 0x03A5),  # Y, Cyrillic Ү, Greek Υ (Upsilon)
    (0x005A, 0x0396),          # Z, Greek Ζ (Zeta)
    # Latin lowercase ↔ Cyrillic lowercase
    (0x0061, 0x0430),          # a, Cyrillic а
    (0x0063, 0x0441),          # c, Cyrillic с
    (0x0065, 0x0435),          # e, Cyrillic е
    (0x006F, 0x043E, 0x03BF),  # o, Cyrillic о, Greek ο (omicron)
    (0x0070, 0x0440, 0x03C1),  # p, Cyrillic р, Greek ρ (rho)
    (0x0073, 0x0455),          # s, Cyrillic ѕ
    (0x0078, 0x0445, 0x03C7),  # x, Cyrillic х, Greek χ (chi)
    (0x0079, 0x0443),          # y, Cyrillic у
    # Digits ↔ letters
    (0x0030, 0x004F, 0x041E),  # 0, O, Cyrillic О
    (0x0031, 0x006C, 0x0049),  # 1, l, I (font dependent)
]

def build_homoglyph_map() -> dict[int, list[int]]:
    """Build a map from each codepoint to its visually similar codepoints."""
    result = {}
    for group in HOMOGLYPH_GROUPS:
        for cp in group:
            others = [c for c in group if c != cp]
            if cp in result:
                result[cp].extend(others)
            else:
                result[cp] = others
    return result


def build_surface_form_index(entries: list[dict], category_map: dict) -> dict[int, list[tuple[str, int]]]:
    """Build an index mapping codepoints to homoglyph token IDs.

    Returns a dict mapping codepoint to list of (token_id, other_codepoint) tuples
    for tokens whose characters are visual homoglyphs.
    """
    from collections import defaultdict

    homoglyph_map = build_homoglyph_map()

    # Build codepoint -> token_id lookup
    cp_to_tid = {}
    category_counts = {cat: 0 for cat in category_map}

    for entry in entries:
        cat_name = entry["category"]
        cat_id = category_map[cat_name]
        index = category_counts[cat_name]
        category_counts[cat_name] += 1

        tid = unicode_text_token_id(cat_id, index)
        cp = entry["codepoint"]
        cp_to_tid[cp] = tid

    # Build the index
    result = {}
    for cp, tid in cp_to_tid.items():
        if cp in homoglyph_map:
            matches = []
            for other_cp in homoglyph_map[cp]:
                if other_cp in cp_to_tid:
                    matches.append((cp_to_tid[other_cp], other_cp))
            if matches:
                result[cp] = matches

    return result


def ingest_unicode_text(conn, source_path: Path = None):
    """Insert Unicode text-mode tokens into the database."""
    if source_path is None:
        source_path = Path(__file__).parent.parent.parent.parent / "sources" / "unicode_table.txt"

    print(f"  Parsing {source_path}...")
    entries = parse_unicode_table(source_path)
    print(f"  Found {len(entries)} character entries")

    # Build category mapping
    category_map = build_category_map(entries)
    print(f"  Found {len(category_map)} major categories")

    # Build surface form cross-reference index
    surface_duplicates = build_surface_form_index(entries, category_map)
    print(f"  Found {len(surface_duplicates)} terms with identical surface forms")

    # Track per-category counters
    category_counts = {cat: 0 for cat in category_map}

    with conn.cursor() as cur:
        # Register each category as a sub-scope
        for cat_name, cat_id in category_map.items():
            scope_id = encode_token_id(1, 0, 0, cat_id)
            insert_scope(cur, scope_id, cat_name, "text_token_group",
                         parent_id=SCOPE_UNICODE_TEXT,
                         metadata={"category_id": cat_id})

        # Insert all tokens
        for entry in entries:
            cat_name = entry["category"]
            cat_id = category_map[cat_name]
            index = category_counts[cat_name]
            category_counts[cat_name] += 1

            tid = unicode_text_token_id(cat_id, index)

            metadata = {
                "codepoint": entry["codepoint"],
                "codepoint_hex": entry["codepoint_hex"],
                "char": entry["char"],
                "alt_code": entry["alt_code"],
                "terms": entry["terms"],
            }

            if entry["raw_category"] and entry["raw_category"] != entry["category"]:
                metadata["raw_category"] = entry["raw_category"]

            # Add cross-references for homoglyphs (visually identical chars)
            cp = entry["codepoint"]
            if cp in surface_duplicates:
                metadata["homoglyphs"] = [
                    {"token_id": t, "codepoint": f"U+{other_cp:04X}"}
                    for t, other_cp in surface_duplicates[cp]
                ]

            # Use first term as the canonical name
            insert_token(cur, tid, entry["terms"][0],
                         category=cat_name,
                         subcategory=entry.get("raw_category"),
                         metadata=metadata)

    conn.commit()

    return {
        "total": len(entries),
        "categories": len(category_map),
        "category_counts": category_counts,
        "category_map": category_map,
        "surface_duplicates": surface_duplicates,
    }


def run(source_path: str = None):
    """Run Unicode text token ingestion standalone."""
    conn = connect()
    init_schema(conn)

    print("Ingesting Unicode text-mode tokens...")
    path = Path(source_path) if source_path else None
    result = ingest_unicode_text(conn, path)

    print(f"\nCategories ({result['categories']} total):")
    for cat_name, count in sorted(result["category_counts"].items(),
                                   key=lambda x: result["category_map"][x[0]]):
        cat_id = result["category_map"][cat_name]
        print(f"  {cat_id:3d}: {cat_name}: {count}")

    print(f"\nTotal tokens: {result['total']}")

    conn.close()


if __name__ == "__main__":
    run()
