"""Ingest NSM (Natural Semantic Metalanguage) primitives into the core shard.

The 65 semantic primes are the irreducible conceptual atoms — the mesh vertices
that all meaning decomposes to. They occupy AA.AA.AA.AB.{n} in the core shard.

Each prime is cross-referenced to its English exponent(s) in hcp_english.
Primes are language-universal; the English words are just one texture pack's
rendering of them.

English exponents include all surface forms: articles, prepositions, copula
forms, pronouns — these are prime translations through the English grammar
codec, not a separate class. Without them the decomposition graph cannot
propagate.

Reference: Goddard & Wierzbicka (2014), NSM chart v19 (April 2017).
"""

from ..core.token_id import encode_token_id
from ..db.postgres import connect as connect_core, insert_token
from ..db.english import connect as connect_english


# The 65 NSM semantic primes, organized by category.
# Each entry: (canonical_name, category, [english_exponents])
#
# Exponents include all common English surface forms that express the prime:
# - Base forms and inflections
# - Grammatical function words (articles, prepositions, copula)
# - Pro-forms (pronouns, pro-adverbs)
# - Common definition metalanguage that maps to the prime
#
# The "/" in NSM notation means allolexes (same prime, different surface forms).

NSM_PRIMES = [
    # Substantives (6)
    ("I", "substantive", ["i", "me", "my", "myself"]),
    ("YOU", "substantive", ["you", "your", "yourself"]),
    ("SOMEONE", "substantive", ["someone", "person", "somebody",
                                 "anyone", "anybody", "who", "whom", "whose"]),
    ("SOMETHING", "substantive", ["something", "thing", "what", "it",
                                   "itself", "which", "whatever"]),
    ("PEOPLE", "substantive", ["people"]),
    ("BODY", "substantive", ["body"]),

    # Relational substantives (2)
    ("KIND", "relational_substantive", ["kind", "type", "sort", "form"]),
    ("PART", "relational_substantive", ["part", "of"]),

    # Determiners (3)
    ("THIS", "determiner", ["this", "the", "that", "these", "those"]),
    ("THE SAME", "determiner", ["same", "synonym"]),
    ("OTHER", "determiner", ["other", "else", "another", "alternative", "or"]),

    # Quantifiers (5)
    ("ONE", "quantifier", ["one", "a", "an", "single", "singular"]),
    ("TWO", "quantifier", ["two"]),
    ("SOME", "quantifier", ["some", "any", "certain"]),
    ("ALL", "quantifier", ["all", "every", "each", "both", "plural"]),
    ("MUCH", "quantifier", ["much", "many"]),

    # Evaluators (2)
    ("GOOD", "evaluator", ["good", "well"]),
    ("BAD", "evaluator", ["bad", "badly", "ill"]),

    # Descriptors (2)
    ("BIG", "descriptor", ["big", "large", "great"]),
    ("SMALL", "descriptor", ["small", "little"]),

    # Mental predicates (7)
    ("THINK", "mental_predicate", ["think", "thought"]),
    ("KNOW", "mental_predicate", ["know", "known"]),
    ("WANT", "mental_predicate", ["want", "wanted"]),
    ("DON'T WANT", "mental_predicate", ["want"]),
    ("FEEL", "mental_predicate", ["feel", "felt", "feeling"]),
    ("SEE", "mental_predicate", ["see", "seen", "saw"]),
    ("HEAR", "mental_predicate", ["hear", "heard"]),

    # Speech (3)
    ("SAY", "speech", ["say", "said"]),
    ("WORDS", "speech", ["words", "word"]),
    ("TRUE", "speech", ["true"]),

    # Actions, events, movement, contact (4)
    ("DO", "action", ["do", "does", "did", "done", "doing", "used"]),
    ("HAPPEN", "action", ["happen", "happened"]),
    ("MOVE", "action", ["move", "to"]),
    ("TOUCH", "action", ["touch"]),

    # Existence and possession (2)
    ("THERE IS", "existence", ["there", "exist", "exists"]),
    ("MINE", "possession", ["mine", "have", "has", "had", "having", "own"]),

    # Life and death (2)
    ("LIVE", "life_death", ["live", "living", "alive"]),
    ("DIE", "life_death", ["die", "dead", "death"]),

    # Time (8)
    ("WHEN", "time", ["when", "time"]),
    ("NOW", "time", ["now", "present", "currently"]),
    ("BEFORE", "time", ["before", "past", "ago"]),
    ("AFTER", "time", ["after", "then", "later"]),
    ("A LONG TIME", "time", ["long", "time"]),
    ("A SHORT TIME", "time", ["short", "time"]),
    ("FOR SOME TIME", "time", ["some", "time"]),
    ("MOMENT", "time", ["moment"]),

    # Space (8)
    ("WHERE", "space", ["where", "place", "at"]),
    ("HERE", "space", ["here"]),
    ("ABOVE", "space", ["above", "over", "up", "on", "upon"]),
    ("BELOW", "space", ["below", "under", "down", "beneath"]),
    ("FAR", "space", ["far", "away", "from"]),
    ("NEAR", "space", ["near", "by", "beside", "next"]),
    ("SIDE", "space", ["side"]),
    ("INSIDE", "space", ["inside", "in", "into", "within"]),

    # Logical concepts (5)
    ("NOT", "logical", ["not", "no", "neither", "nor", "without", "but"]),
    ("MAYBE", "logical", ["maybe", "perhaps"]),
    ("CAN", "logical", ["can", "could", "able"]),
    ("BECAUSE", "logical", ["because", "for", "so", "since", "therefore"]),
    ("IF", "logical", ["if", "whether"]),

    # Intensifier, augmentor (2)
    ("VERY", "intensifier", ["very", "especially", "particularly"]),
    ("MORE", "intensifier", ["more", "than", "also", "too", "and", "with", "plus"]),

    # Similarity (2)
    ("LIKE", "similarity", ["like", "as"]),
    ("WAY", "similarity", ["way", "how"]),

    # Specification (2)
    ("BE (SOMEONE)", "specification", ["be", "is", "are", "was", "were",
                                        "been", "being"]),
    ("BE (SOMEWHERE)", "specification", ["be", "is", "are", "was", "were",
                                          "been", "being"]),
]

assert len(NSM_PRIMES) == 65, f"Expected 65 primes, got {len(NSM_PRIMES)}"


def build_english_lookup(eng_conn) -> dict[str, list[str]]:
    """Build lookup: lowercase word -> list of token_ids from english shard."""
    lookup = {}
    with eng_conn.cursor() as cur:
        cur.execute("SELECT token_id, name FROM tokens")
        for token_id, name in cur.fetchall():
            key = name.lower() if name else None
            if key:
                if key not in lookup:
                    lookup[key] = []
                lookup[key].append(token_id)
    return lookup


def run():
    """Ingest the 65 NSM primes into the core shard."""
    print("Connecting to core database...")
    core_conn = connect_core()

    print("Connecting to English shard (for exponent cross-references)...")
    eng_conn = connect_english()

    print("Building English word lookup...")
    eng_lookup = build_english_lookup(eng_conn)
    print(f"  {len(eng_lookup)} words in lookup")

    print(f"\nIngesting {len(NSM_PRIMES)} NSM primes...")
    inserted = 0
    linked = 0
    total_exponent_tokens = 0

    with core_conn.cursor() as cur:
        for idx, (name, category, exponents) in enumerate(NSM_PRIMES):
            # Token ID: AA.AA.AA.AB.{idx}
            token_id = encode_token_id(0, 0, 0, 1, idx)

            # Find English exponent token IDs
            english_refs = {}
            for exp_word in exponents:
                matches = eng_lookup.get(exp_word, [])
                if matches:
                    english_refs[exp_word] = matches
                    linked += 1
                    total_exponent_tokens += len(matches)

            metadata = {
                "nsm_category": category,
                "canonical": name,
                "english_exponents": exponents,
                "english_token_ids": english_refs,
                "prime_index": idx,
            }

            insert_token(cur, token_id, name,
                         category="nsm_prime",
                         subcategory=category,
                         metadata=metadata)
            inserted += 1

    core_conn.commit()

    print(f"\n=== Summary ===")
    print(f"Primes inserted:       {inserted}")
    print(f"Exponent words linked: {linked}")
    print(f"Total english tokens:  {total_exponent_tokens}")

    # Show a few examples
    print(f"\nSample tokens:")
    with core_conn.cursor() as cur:
        cur.execute("""
            SELECT token_id, name, subcategory,
                   jsonb_object_keys(metadata->'english_token_ids') as exponent
            FROM tokens
            WHERE category = 'nsm_prime'
            ORDER BY token_id
            LIMIT 15
        """)
        for row in cur.fetchall():
            print(f"  {row[0]}  {row[1]:<20s}  [{row[2]}]  exponent: {row[3]}")

    core_conn.close()
    eng_conn.close()


if __name__ == "__main__":
    run()
