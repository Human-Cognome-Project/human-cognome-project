"""Ingest English words from Kaikki dictionary into the English shard.

Phase 1: Establish Token IDs with atomization for all words.
Phase 2 (later): Import definitions/senses as Token ID sequences.

Token addressing (English language family shard):
    AB.AB.{layer}{sub}.{high}.{low}

Layers (by derivation, bottom to top):
    A = Affixes (prefix, suffix, infix, interfix, circumfix)
    B = Fragments (reserved)
    C = Words (noun, verb, adj, adv, prep, etc.)
    D = Derivatives (abbreviation, initialism, acronym, contraction, clipping)
    E = Multi-word (phrase, prep_phrase, proverb, adv_phrase)
"""

import json
from collections import defaultdict

from ..core.token_id import (
    encode_word_token_id, BASE,
    LAYER_AFFIX, LAYER_WORD, LAYER_DERIVATIVE, LAYER_MULTIWORD,
    SUB_PREFIX, SUB_SUFFIX, SUB_INFIX, SUB_INTERFIX, SUB_CIRCUMFIX, SUB_AFFIX,
    SUB_NOUN, SUB_VERB, SUB_ADJ, SUB_ADV, SUB_PREP, SUB_CONJ, SUB_DET,
    SUB_PRON, SUB_INTJ, SUB_NUM, SUB_SYMBOL, SUB_PARTICLE, SUB_PUNCT,
    SUB_ARTICLE, SUB_POSTP, SUB_CHARACTER,
    SUB_ABBREVIATION, SUB_INITIALISM, SUB_ACRONYM, SUB_CONTRACTION, SUB_CLIPPING,
    SUB_PHRASE, SUB_PREP_PHRASE, SUB_PROVERB, SUB_ADV_PHRASE,
)
from ..db.postgres import connect as connect_core
from ..db.english import connect as connect_english, init_schema, insert_token


# POS to layer/sub mapping
POS_TO_LAYER_SUB = {
    # Layer A: Affixes
    'prefix': (LAYER_AFFIX, SUB_PREFIX),
    'suffix': (LAYER_AFFIX, SUB_SUFFIX),
    'infix': (LAYER_AFFIX, SUB_INFIX),
    'interfix': (LAYER_AFFIX, SUB_INTERFIX),
    'circumfix': (LAYER_AFFIX, SUB_CIRCUMFIX),
    'affix': (LAYER_AFFIX, SUB_AFFIX),
    # Layer C: Words
    'noun': (LAYER_WORD, SUB_NOUN),
    'verb': (LAYER_WORD, SUB_VERB),
    'adj': (LAYER_WORD, SUB_ADJ),
    'adv': (LAYER_WORD, SUB_ADV),
    'prep': (LAYER_WORD, SUB_PREP),
    'conj': (LAYER_WORD, SUB_CONJ),
    'det': (LAYER_WORD, SUB_DET),
    'pron': (LAYER_WORD, SUB_PRON),
    'intj': (LAYER_WORD, SUB_INTJ),
    'num': (LAYER_WORD, SUB_NUM),
    'symbol': (LAYER_WORD, SUB_SYMBOL),
    'particle': (LAYER_WORD, SUB_PARTICLE),
    'punct': (LAYER_WORD, SUB_PUNCT),
    'article': (LAYER_WORD, SUB_ARTICLE),
    'postp': (LAYER_WORD, SUB_POSTP),
    'character': (LAYER_WORD, SUB_CHARACTER),
    # Layer E: Multi-word
    'phrase': (LAYER_MULTIWORD, SUB_PHRASE),
    'prep_phrase': (LAYER_MULTIWORD, SUB_PREP_PHRASE),
    'proverb': (LAYER_MULTIWORD, SUB_PROVERB),
    'adv_phrase': (LAYER_MULTIWORD, SUB_ADV_PHRASE),
}

# Layer names for display
LAYER_NAMES = {
    LAYER_AFFIX: 'affix',
    LAYER_WORD: 'word',
    LAYER_DERIVATIVE: 'derivative',
    LAYER_MULTIWORD: 'multiword',
}

# Sense tags that identify derivatives
DERIVATIVE_TAGS = {
    'abbreviation': SUB_ABBREVIATION,
    'initialism': SUB_INITIALISM,
    'acronym': SUB_ACRONYM,
    'contraction': SUB_CONTRACTION,
    'clipping': SUB_CLIPPING,
}


def build_char_lookup(core_conn) -> dict[str, str]:
    """Build lookup table: character -> Token ID.

    Reads from core database (character tokens at AA.AB.AA.*).
    """
    char_to_token = {}
    with core_conn.cursor() as cur:
        cur.execute("""
            SELECT token_id, metadata->>'char' as char
            FROM tokens
            WHERE token_id LIKE 'AA.AB.AA.%'
            AND metadata ? 'char'
        """)
        for token_id, char in cur.fetchall():
            if char:
                char_to_token[char] = token_id
    return char_to_token


def build_word_lookup(eng_conn) -> dict[str, str]:
    """Build lookup table: word -> Token ID.

    Used for phrase atomization (phrase → word tokens).
    """
    word_to_token = {}
    with eng_conn.cursor() as cur:
        cur.execute("SELECT token_id, name FROM tokens")
        for token_id, name in cur.fetchall():
            if name:
                word_to_token[name] = token_id
    return word_to_token


def atomize_to_chars(word: str, char_lookup: dict[str, str]) -> list[str]:
    """Convert word to list of character Token IDs."""
    result = []
    for char in word:
        token_id = char_lookup.get(char)
        if token_id:
            result.append(token_id)
    return result


def atomize_to_words(phrase: str, word_lookup: dict[str, str]) -> list[str] | None:
    """Convert phrase to list of word Token IDs.

    Returns None if any word is missing from lookup (phrase is neologism).
    """
    words = phrase.split()
    result = []
    for word in words:
        token_id = word_lookup.get(word)
        if not token_id:
            return None
        result.append(token_id)
    return result


def get_expanded_form(data: dict) -> str | None:
    """Extract expanded form from derivative entry's alt_of field."""
    senses = data.get('senses', [])
    for sense in senses:
        tags = sense.get('tags', [])
        if any(tag in DERIVATIVE_TAGS for tag in tags):
            alt_of = sense.get('alt_of', [])
            if alt_of and isinstance(alt_of, list) and len(alt_of) > 0:
                return alt_of[0].get('word')
    return None


def check_root_exists(core_conn, word: str) -> bool:
    """Check if a word/phrase exists in kaikki_entries."""
    with core_conn.cursor() as cur:
        cur.execute("SELECT 1 FROM kaikki_entries WHERE word = %s LIMIT 1", (word,))
        return cur.fetchone() is not None


class TokenCounter:
    """Track sequential token counts per layer/sub."""

    def __init__(self):
        self.counts = defaultdict(int)

    def next(self, layer: int, sub: int) -> tuple[int, int]:
        """Get next count pair for layer/sub, increment counter."""
        key = (layer, sub)
        count = self.counts[key]
        self.counts[key] += 1
        high = count // (BASE * BASE)
        low = count % (BASE * BASE)
        return high, low

    def get_counts(self) -> dict:
        return dict(self.counts)


def ingest_layer_a(core_conn, eng_conn, char_lookup: dict, counter: TokenCounter) -> int:
    """Ingest Layer A: Affixes."""
    inserted = 0
    affix_pos = ['prefix', 'suffix', 'infix', 'interfix', 'circumfix', 'affix']

    with core_conn.cursor() as core_cur, eng_conn.cursor() as eng_cur:
        for pos in affix_pos:
            if pos not in POS_TO_LAYER_SUB:
                continue
            layer, sub = POS_TO_LAYER_SUB[pos]

            core_cur.execute("SELECT DISTINCT word FROM kaikki_entries WHERE pos = %s", (pos,))

            for (word,) in core_cur.fetchall():
                high, low = counter.next(layer, sub)
                token_id = encode_word_token_id(layer, sub, high, low)
                atomization = atomize_to_chars(word, char_lookup)

                insert_token(eng_cur, token_id, word,
                             layer='affix', subcategory=pos,
                             atomization=atomization)
                inserted += 1

                if inserted % 500 == 0:
                    print(f"    Layer A: {inserted} affixes...")

    eng_conn.commit()
    return inserted


def ingest_layer_c(core_conn, eng_conn, char_lookup: dict, counter: TokenCounter,
                   seen_words: set) -> int:
    """Ingest Layer C: Words (excluding names and multi-word)."""
    inserted = 0
    word_pos = ['noun', 'verb', 'adj', 'adv', 'prep', 'conj', 'det', 'pron',
                'intj', 'num', 'symbol', 'particle', 'punct', 'article',
                'postp', 'character']

    with core_conn.cursor() as core_cur, eng_conn.cursor() as eng_cur:
        for pos in word_pos:
            if pos not in POS_TO_LAYER_SUB:
                continue
            layer, sub = POS_TO_LAYER_SUB[pos]

            core_cur.execute("""
                SELECT DISTINCT word FROM kaikki_entries
                WHERE pos = %s AND word NOT IN (
                    SELECT DISTINCT word FROM kaikki_entries WHERE pos = 'name'
                )
            """, (pos,))

            for (word,) in core_cur.fetchall():
                if word in seen_words:
                    continue

                high, low = counter.next(layer, sub)
                token_id = encode_word_token_id(layer, sub, high, low)
                atomization = atomize_to_chars(word, char_lookup)

                insert_token(eng_cur, token_id, word,
                             layer='word', subcategory=pos,
                             atomization=atomization)
                seen_words.add(word)
                inserted += 1

                if inserted % 10000 == 0:
                    print(f"    Layer C: {inserted} words...")

    eng_conn.commit()
    return inserted


def ingest_layer_e(core_conn, eng_conn, char_lookup: dict, word_lookup: dict,
                   counter: TokenCounter, seen_words: set) -> int:
    """Ingest Layer E: Multi-word phrases."""
    inserted = 0
    phrase_pos = ['phrase', 'prep_phrase', 'proverb', 'adv_phrase']

    with core_conn.cursor() as core_cur, eng_conn.cursor() as eng_cur:
        for pos in phrase_pos:
            if pos not in POS_TO_LAYER_SUB:
                continue
            layer, sub = POS_TO_LAYER_SUB[pos]

            core_cur.execute("SELECT DISTINCT word FROM kaikki_entries WHERE pos = %s", (pos,))

            for (word,) in core_cur.fetchall():
                if word in seen_words:
                    continue

                high, low = counter.next(layer, sub)
                token_id = encode_word_token_id(layer, sub, high, low)

                # Try atomizing to words first
                word_atomization = atomize_to_words(word, word_lookup)
                if word_atomization:
                    atomization = word_atomization
                else:
                    atomization = atomize_to_chars(word, char_lookup)

                insert_token(eng_cur, token_id, word,
                             layer='multiword', subcategory=pos,
                             atomization=atomization)
                seen_words.add(word)
                inserted += 1

                if inserted % 1000 == 0:
                    print(f"    Layer E: {inserted} phrases...")

    eng_conn.commit()
    return inserted


def ingest_layer_d(core_conn, eng_conn, char_lookup: dict, counter: TokenCounter,
                   seen_words: set) -> int:
    """Ingest Layer D: Derivatives (abbreviations, initialisms, etc.)."""
    inserted = 0

    with core_conn.cursor() as core_cur, eng_conn.cursor() as eng_cur:
        for tag, sub in DERIVATIVE_TAGS.items():
            layer = LAYER_DERIVATIVE

            core_cur.execute("""
                SELECT DISTINCT e.word, e.data
                FROM kaikki_entries e,
                LATERAL jsonb_array_elements(e.data->'senses') AS sense,
                LATERAL jsonb_array_elements_text(sense->'tags') AS t
                WHERE t = %s
            """, (tag,))

            for word, data in core_cur.fetchall():
                if word in seen_words:
                    continue

                # Check if expanded form exists
                expanded = get_expanded_form(data)
                if expanded and not check_root_exists(core_conn, expanded):
                    continue

                high, low = counter.next(layer, sub)
                token_id = encode_word_token_id(layer, sub, high, low)
                atomization = atomize_to_chars(word, char_lookup)

                insert_token(eng_cur, token_id, word,
                             layer='derivative', subcategory=tag,
                             atomization=atomization)
                seen_words.add(word)
                inserted += 1

                if inserted % 1000 == 0:
                    print(f"    Layer D: {inserted} derivatives...")

    eng_conn.commit()
    return inserted


def ingest_contraction_pos(core_conn, eng_conn, char_lookup: dict, counter: TokenCounter,
                           seen_words: set) -> int:
    """Ingest contractions that are classified by POS."""
    inserted = 0
    layer, sub = LAYER_DERIVATIVE, SUB_CONTRACTION

    with core_conn.cursor() as core_cur, eng_conn.cursor() as eng_cur:
        core_cur.execute("SELECT DISTINCT word FROM kaikki_entries WHERE pos = 'contraction'")

        for (word,) in core_cur.fetchall():
            if word in seen_words:
                continue

            high, low = counter.next(layer, sub)
            token_id = encode_word_token_id(layer, sub, high, low)
            atomization = atomize_to_chars(word, char_lookup)

            insert_token(eng_cur, token_id, word,
                         layer='derivative', subcategory='contraction',
                         atomization=atomization)
            seen_words.add(word)
            inserted += 1

    eng_conn.commit()
    return inserted


def run():
    """Run Phase 1: Establish Token IDs with atomization for all English words."""
    print("Connecting to core database (for Kaikki + char lookup)...")
    core_conn = connect_core()

    print("Connecting to English shard...")
    eng_conn = connect_english()
    init_schema(eng_conn)

    print("Building character lookup from core...")
    char_lookup = build_char_lookup(core_conn)
    print(f"  {len(char_lookup)} characters in lookup")

    counter = TokenCounter()
    seen_words = set()

    print("\nIngesting Layer A (Affixes)...")
    count_a = ingest_layer_a(core_conn, eng_conn, char_lookup, counter)
    print(f"  {count_a} affixes")

    print("\nIngesting Layer C (Words)...")
    count_c = ingest_layer_c(core_conn, eng_conn, char_lookup, counter, seen_words)
    print(f"  {count_c} words")

    print("\nBuilding word lookup for phrase atomization...")
    word_lookup = build_word_lookup(eng_conn)
    print(f"  {len(word_lookup)} words in lookup")

    print("\nIngesting Layer E (Multi-word)...")
    count_e = ingest_layer_e(core_conn, eng_conn, char_lookup, word_lookup, counter, seen_words)
    print(f"  {count_e} phrases")

    print("\nIngesting Layer D (Derivatives)...")
    count_d = ingest_layer_d(core_conn, eng_conn, char_lookup, counter, seen_words)
    print(f"  {count_d} derivatives")

    print("\nIngesting contraction POS entries...")
    count_contr = ingest_contraction_pos(core_conn, eng_conn, char_lookup, counter, seen_words)
    print(f"  {count_contr} contractions")

    print("\n=== Summary ===")
    print(f"Layer A (Affixes):     {count_a:>10,}")
    print(f"Layer C (Words):       {count_c:>10,}")
    print(f"Layer E (Multi-word):  {count_e:>10,}")
    print(f"Layer D (Derivatives): {count_d + count_contr:>10,}")
    print(f"Total:                 {count_a + count_c + count_e + count_d + count_contr:>10,}")

    core_conn.close()
    eng_conn.close()


if __name__ == "__main__":
    run()
