#!/usr/bin/env python3
"""
Compose clean English token database from MorphyNet staging tables.

Destructive consumption: entries are deleted from staging as they're processed.
What remains in staging = what hasn't been processed yet.

Approach:
1. Identify true roots (lemmas that are not derivable from another lemma)
2. Create token entries for roots with token_id coordinates
3. Two-pass rule determination for derivational entries
4. Store derivational deltas as token_variants
5. Store irregular inflections as token_variants
6. Tag roots with spelling exception classes
7. Delete from staging as each entry is processed

PoS mapping: MorphyNet uses N/V/J/R/U → our pos_tag enum
"""

import os
import sys
import subprocess
from collections import defaultdict

DB_ENV = {**os.environ, 'PGPASSWORD': 'hcp_dev'}

def psql(sql):
    result = subprocess.run(
        ['psql', '-h', 'localhost', '-U', 'hcp', '-d', 'hcp_english', '-t', '-A', '-c', sql],
        capture_output=True, text=True, env=DB_ENV
    )
    if result.returncode != 0 and result.stderr.strip():
        print(f"SQL ERROR: {result.stderr.strip()}", file=sys.stderr)
    return result.stdout.strip()

def psql_exec(sql):
    result = subprocess.run(
        ['psql', '-h', 'localhost', '-U', 'hcp', '-d', 'hcp_english', '-c', sql],
        capture_output=True, text=True, env=DB_ENV
    )
    if result.returncode != 0 and result.stderr.strip():
        print(f"SQL ERROR: {result.stderr.strip()}", file=sys.stderr)
    return result.returncode == 0

# PoS mapping: MorphyNet single-letter → HCP pos_tag
POS_MAP = {
    'N': 'N_COMMON',
    'V': 'V_MAIN',
    'J': 'ADJ',
    'R': 'ADV',
    'U': None,  # unknown — skip
}

# ---------------------------------------------------------------------------
# Spelling transformation detection
# ---------------------------------------------------------------------------

def detect_rule(base, derived, morpheme, morph_type):
    """Determine which spelling rule produces derived from base + morpheme."""
    if morph_type == 'prefix':
        if morpheme + base == derived:
            return 'DEFAULT'
        if morpheme + '-' + base == derived:
            return 'HYPHENATED'
        return None

    # Suffix rules
    # 1. Default
    if base + morpheme == derived:
        return 'DEFAULT'

    # 2. Silent-e drop
    if (base.endswith('e') and morpheme and morpheme[0] in 'aeiouy'
            and base[:-1] + morpheme == derived):
        return 'SILENT_E'

    # 3. y→i
    if (base.endswith('y') and len(base) >= 2
            and base[-2] not in 'aeiou'
            and morpheme and morpheme[0] not in 'i'
            and base[:-1] + 'i' + morpheme == derived):
        return 'Y_TO_I'

    # 4. CVC doubling
    if (len(base) >= 3
            and morpheme and morpheme[0] in 'aeiouy'
            and base[-1] not in 'aeiouwxy'
            and base[-2] in 'aeiou'
            and base[-3] not in 'aeiou'
            and base + base[-1] + morpheme == derived):
        return 'CVC_DOUBLE'

    # 5. le→ly
    if (base.endswith('le') and morpheme == 'ly'
            and base[:-2] + 'ly' == derived):
        return 'LE_TO_LY'

    # 6. Latin/Greek stem changes — flag for review but don't block
    return None


# ---------------------------------------------------------------------------
# Token ID minting
# ---------------------------------------------------------------------------

# Base-50 charset for token_id encoding
B50 = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwx'

def int_to_b50(n):
    """Convert integer to 2-char base-50 string."""
    return B50[n // 50] + B50[n % 50]

class TokenIdMinter:
    """
    Mint sequential token_ids in AB namespace.

    Convention: p3 is a 2-value pair encoding (starting_letter, word_length).
      - p3 char 1 = starting letter (a→A, b→B, ... n→N, o→a (since o excluded), p→P, ... z→Z)
      - p3 char 2 = word length (1→A, 2→B, ... using base-50 chars)
    This leaves p4+p5 (4 chars, base-50) = ~6.25M positions per (letter, length) bucket.
    """
    def __init__(self):
        self.buckets = {}  # p3 → sequential counter for p4+p5

    def _letter_to_b50(self, ch):
        """Map starting letter to base-50 char. o→a (lowercase a replaces o).
        Non-ASCII and non-alpha chars get bucketed into positions 26-49."""
        ch_lower = ch.lower() if ch.isalpha() else ch
        if ch_lower >= 'a' and ch_lower <= 'z':
            idx = ord(ch_lower) - ord('a')
            if idx == 14:  # 'o'
                return 'a'  # lowercase a represents o
            return B50[idx]  # A-Z for a-z (minus o)
        # Non-ASCII letters (accented), special chars → bucket at position 27
        return B50[27]  # 'b' — special/accented chars bucket

    def mint(self, name):
        """Mint a token_id for a word. p3=(letter,length), p4+p5=sequential."""
        if not name:
            name = '?'

        first = name[0] if name[0].isalpha() else '?'
        word_len = min(len(name), 50)

        # p3: char1 = starting letter, char2 = length
        p3_c1 = self._letter_to_b50(first)
        p3_c2 = B50[word_len] if word_len < 50 else B50[49]
        p3 = p3_c1 + p3_c2

        # p4+p5: sequential within this (letter, length) bucket
        if p3 not in self.buckets:
            self.buckets[p3] = 0
        self.buckets[p3] += 1
        seq = self.buckets[p3]

        # Encode seq into p4 (2 chars) + p5 (2 chars) = 4 base-50 digits
        p4_val = seq // 2500
        p5_val = seq % 2500
        p4 = B50[p4_val // 50] + B50[p4_val % 50]
        p5 = B50[p5_val // 50] + B50[p5_val % 50]

        return f"AB.AA.{p3}.{p4}.{p5}"


# ---------------------------------------------------------------------------
# Main processing
# ---------------------------------------------------------------------------

def main():
    print("="*60)
    print("COMPOSE FROM MORPHYNET")
    print("="*60)

    # --- Step 1: Identify true roots ---
    print("\nStep 1: Identifying true roots...")

    # Get all words that appear as derived in derivational table
    derived_words = set()
    rows = psql("SELECT DISTINCT derived FROM staging_morphynet_deriv")
    for line in rows.split('\n'):
        if line.strip():
            derived_words.add(line.strip())
    print(f"  Words appearing as derived forms: {len(derived_words)}")

    # Get all unique lemmas from inflectional table
    all_lemmas = set()
    rows = psql("SELECT DISTINCT lemma FROM staging_morphynet_infl")
    for line in rows.split('\n'):
        if line.strip():
            all_lemmas.add(line.strip())
    print(f"  Unique inflectional lemmas: {len(all_lemmas)}")

    # Get all base words from derivational table
    base_words = set()
    rows = psql("SELECT DISTINCT base FROM staging_morphynet_deriv")
    for line in rows.split('\n'):
        if line.strip():
            base_words.add(line.strip())
    print(f"  Words appearing as derivational bases: {len(base_words)}")

    # True roots: lemmas that are NOT derived from another word in the dataset
    # A word is a root if it appears in lemmas but not in derived_words,
    # OR if it appears in derived_words but its source chain ends at a word
    # that IS in lemmas and not derived.
    # For simplicity: root = (all_lemmas | base_words) - derived_words + pure derivational bases
    pure_roots = (all_lemmas | base_words) - derived_words
    print(f"  True roots (not derived from anything): {len(pure_roots)}")

    # --- Step 2: Resolve derivation chains ---
    print("\nStep 2: Loading derivation chains...")

    # Build derivation map: derived → (base, morpheme, morph_type, base_pos, derived_pos)
    deriv_map = {}
    rows = psql("SELECT base, derived, base_pos, derived_pos, morpheme, morph_type FROM staging_morphynet_deriv")
    for line in rows.split('\n'):
        if '|' not in line:
            continue
        parts = line.split('|')
        if len(parts) < 6:
            continue
        base, derived, bpos, dpos, morph, mtype = [p.strip() for p in parts]
        deriv_map[derived] = (base, morph, mtype, bpos, dpos)

    print(f"  Derivation entries loaded: {len(deriv_map)}")

    # Trace chains to ultimate roots
    def find_root(word, visited=None):
        if visited is None:
            visited = set()
        if word in visited:
            return word, []  # cycle — treat as root
        visited.add(word)
        if word not in deriv_map:
            return word, []
        base, morph, mtype, bpos, dpos = deriv_map[word]
        root, chain = find_root(base, visited)
        chain.append((morph, mtype))
        return root, chain

    # --- Step 3: Create new token tables ---
    print("\nStep 3: Preparing new token tables...")

    # We'll build in new_tokens, new_token_pos, then swap
    psql_exec("""
        DROP TABLE IF EXISTS new_token_variants CASCADE;
        DROP TABLE IF EXISTS new_token_glosses CASCADE;
        DROP TABLE IF EXISTS new_token_pos CASCADE;
        DROP TABLE IF EXISTS new_tokens CASCADE;

        CREATE TABLE new_tokens (
            ns          CHAR(2) NOT NULL DEFAULT 'AB',
            p2          CHAR(2) NOT NULL DEFAULT 'AA',
            p3          CHAR(2) NOT NULL,
            p4          CHAR(2) NOT NULL,
            p5          CHAR(2) NOT NULL,
            token_id    TEXT GENERATED ALWAYS AS (
                ns || '.' || p2 || '.' || p3 || '.' || p4 || '.' || p5
            ) STORED NOT NULL,
            name        TEXT NOT NULL,
            freq_rank   INTEGER,
            proper_common TEXT,
            characteristics INTEGER NOT NULL DEFAULT 0,
            morph_exception TEXT,  -- SILENT_E, Y_TO_I, CVC_DOUBLE, LE_TO_LY, or NULL (default rules)
            particle_key TEXT GENERATED ALWAYS AS (
                CASE
                    WHEN name ~ '''' THEN '''' || length(name)
                    WHEN name ~ '-'  THEN '-' || length(name)
                    ELSE left(name, 1) || length(name)
                END
            ) STORED,
            PRIMARY KEY (ns, p2, p3, p4, p5),
            UNIQUE (token_id)
        );
        CREATE INDEX idx_nt_name ON new_tokens (name);
        CREATE INDEX idx_nt_particle_key ON new_tokens (particle_key);

        CREATE TABLE new_token_pos (
            id          SERIAL PRIMARY KEY,
            token_id    TEXT NOT NULL,
            pos         pos_tag NOT NULL,
            is_primary  BOOLEAN NOT NULL DEFAULT false,
            morpheme_accept INTEGER NOT NULL DEFAULT 0,
            characteristics INTEGER NOT NULL DEFAULT 0,
            UNIQUE (token_id, pos)
        );

        CREATE TABLE new_token_variants (
            id           SERIAL PRIMARY KEY,
            canonical_id TEXT NOT NULL,
            name         TEXT NOT NULL,
            morpheme     TEXT,
            morph_type   TEXT,  -- 'prefix' or 'suffix'
            rule         TEXT,  -- DEFAULT, SILENT_E, Y_TO_I, CVC_DOUBLE, LE_TO_LY, IRREGULAR
            characteristics INTEGER NOT NULL DEFAULT 0,
            note         TEXT
        );
        CREATE INDEX idx_ntv_canonical ON new_token_variants (canonical_id);
        CREATE INDEX idx_ntv_name ON new_token_variants (name);
    """)
    print("  New tables created.")

    # --- Step 4: Insert roots ---
    print("\nStep 4: Inserting roots...")
    minter = TokenIdMinter()

    # Collect PoS info for roots from derivational base_pos
    root_pos = defaultdict(set)
    for line in psql("SELECT DISTINCT base, base_pos FROM staging_morphynet_deriv").split('\n'):
        if '|' in line:
            word, pos = line.split('|', 1)
            word, pos = word.strip(), pos.strip()
            if word in pure_roots and pos in POS_MAP and POS_MAP[pos]:
                root_pos[word].add(POS_MAP[pos])

    # Also get PoS from inflectional features
    for line in psql("""
        SELECT DISTINCT lemma,
            CASE
                WHEN features LIKE 'N%' THEN 'N_COMMON'
                WHEN features LIKE 'V%' THEN 'V_MAIN'
                WHEN features LIKE 'ADJ%' THEN 'ADJ'
                WHEN features LIKE 'R%' THEN 'ADV'
                ELSE NULL
            END as pos
        FROM staging_morphynet_infl
        WHERE features NOT LIKE '%SG' OR features LIKE 'N%'
    """).split('\n'):
        if '|' in line:
            word, pos = line.split('|', 1)
            word, pos = word.strip(), pos.strip()
            if word in pure_roots and pos and pos != '':
                root_pos[word].add(pos)

    # Insert roots in batches
    root_token_ids = {}  # name → token_id
    batch = []
    batch_pos = []
    inserted = 0

    for word in sorted(pure_roots):
        if not word or len(word) > 48:
            continue

        tid = minter.mint(word)
        parts = tid.split('.')
        ns, p2, p3, p4, p5 = parts

        safe_word = word.replace("'", "''")
        batch.append(f"('{ns}', '{p2}', '{p3}', '{p4}', '{p5}', '{safe_word}')")
        root_token_ids[word] = tid

        # PoS entries
        pos_set = root_pos.get(word, set())
        if not pos_set:
            pos_set = {'N_COMMON'}  # default if unknown
        primary_set = False
        for pos in sorted(pos_set):
            is_primary = 'true' if not primary_set else 'false'
            primary_set = True
            batch_pos.append(f"('{tid}', '{pos}', {is_primary})")

        if len(batch) >= 500:
            psql_exec(f"INSERT INTO new_tokens (ns, p2, p3, p4, p5, name) VALUES {','.join(batch)} ON CONFLICT DO NOTHING;")
            if batch_pos:
                psql_exec(f"INSERT INTO new_token_pos (token_id, pos, is_primary) VALUES {','.join(batch_pos)} ON CONFLICT DO NOTHING;")
            inserted += len(batch)
            batch = []
            batch_pos = []
            if inserted % 10000 == 0:
                print(f"  Inserted {inserted} roots...")

    # Final batch
    if batch:
        psql_exec(f"INSERT INTO new_tokens (ns, p2, p3, p4, p5, name) VALUES {','.join(batch)} ON CONFLICT DO NOTHING;")
        if batch_pos:
            psql_exec(f"INSERT INTO new_token_pos (token_id, pos, is_primary) VALUES {','.join(batch_pos)} ON CONFLICT DO NOTHING;")
        inserted += len(batch)

    print(f"  Inserted {inserted} roots total.")

    # --- Step 5: Process derivational entries ---
    print("\nStep 5: Processing derivational entries (two-pass rule determination)...")

    stats = defaultdict(int)
    variant_batch = []
    exception_updates = defaultdict(set)
    processed_deriv_ids = []

    rows = psql("SELECT id, base, derived, base_pos, derived_pos, morpheme, morph_type FROM staging_morphynet_deriv")
    for line in rows.split('\n'):
        if '|' not in line:
            continue
        parts = line.split('|')
        if len(parts) < 7:
            continue
        row_id, base, derived, bpos, dpos, morpheme, mtype = [p.strip() for p in parts]

        # Find ultimate root
        ultimate_root, chain = find_root(derived)

        # Get root token_id
        canon_tid = root_token_ids.get(ultimate_root)
        if not canon_tid:
            # Root might not be in our set — use direct base if available
            canon_tid = root_token_ids.get(base)
        if not canon_tid:
            stats['no_root'] += 1
            continue

        # Determine spelling rule
        rule = detect_rule(base, derived, morpheme, mtype)
        if rule is None:
            rule = 'UNRESOLVED'

        stats[rule] += 1

        # Tag root with exception if needed
        if rule in ('SILENT_E', 'Y_TO_I', 'CVC_DOUBLE', 'LE_TO_LY'):
            exception_updates[ultimate_root].add(rule)

        safe_derived = derived.replace("'", "''")
        safe_morpheme = morpheme.replace("'", "''")
        safe_rule = rule.replace("'", "''")

        variant_batch.append(
            f"('{canon_tid}', '{safe_derived}', '{safe_morpheme}', '{mtype}', '{safe_rule}', 0)"
        )
        processed_deriv_ids.append(row_id)

        if len(variant_batch) >= 500:
            psql_exec(f"""INSERT INTO new_token_variants
                (canonical_id, name, morpheme, morph_type, rule, characteristics)
                VALUES {','.join(variant_batch)} ON CONFLICT DO NOTHING;""")
            # Delete processed entries from staging
            id_list = ','.join(processed_deriv_ids)
            psql_exec(f"DELETE FROM staging_morphynet_deriv WHERE id IN ({id_list});")
            variant_batch = []
            processed_deriv_ids = []

    # Final batch
    if variant_batch:
        psql_exec(f"""INSERT INTO new_token_variants
            (canonical_id, name, morpheme, morph_type, rule, characteristics)
            VALUES {','.join(variant_batch)} ON CONFLICT DO NOTHING;""")
        if processed_deriv_ids:
            id_list = ','.join(processed_deriv_ids)
            psql_exec(f"DELETE FROM staging_morphynet_deriv WHERE id IN ({id_list});")

    print(f"  Rule distribution:")
    for rule, count in sorted(stats.items(), key=lambda x: -x[1]):
        print(f"    {rule}: {count}")

    # --- Step 6: Update root exception tags ---
    print(f"\nStep 6: Tagging {len(exception_updates)} roots with morph exceptions...")
    for word, tags in exception_updates.items():
        tid = root_token_ids.get(word)
        if tid:
            # Use the most common exception for this root
            tag = sorted(tags)[0]  # alphabetical — deterministic
            safe_tag = tag.replace("'", "''")
            psql_exec(f"UPDATE new_tokens SET morph_exception = '{safe_tag}' WHERE token_id = '{tid}';")

    # --- Step 7: Process irregular inflections ---
    print("\nStep 7: Processing irregular inflections...")

    irr_batch = []
    irr_ids = []
    irr_count = 0

    rows = psql("""
        SELECT id, lemma, form, features FROM staging_morphynet_infl
        WHERE segmentation = '-'
        AND features NOT IN ('N;SG', 'ADJ', 'V;NFIN;IMP+SBJV')
    """)
    for line in rows.split('\n'):
        if '|' not in line:
            continue
        parts = line.split('|')
        if len(parts) < 4:
            continue
        row_id, lemma, form, features = [p.strip() for p in parts]

        canon_tid = root_token_ids.get(lemma)
        if not canon_tid:
            continue

        # Map feature to morpheme label
        if 'PST' in features and 'PTCP' in features:
            morpheme = 'PAST_PARTICIPLE'
        elif 'PST' in features:
            morpheme = 'PAST'
        elif 'PL' in features:
            morpheme = 'PLURAL'
        elif 'CMPR' in features:
            morpheme = 'COMPARATIVE'
        elif 'SPRL' in features:
            morpheme = 'SUPERLATIVE'
        elif 'PRS' in features and '3' in features:
            morpheme = '3RD_SING'
        elif 'PTCP' in features and 'PRS' in features:
            morpheme = 'PROGRESSIVE'
        else:
            morpheme = 'IRREGULAR'

        safe_form = form.replace("'", "''")
        irr_batch.append(
            f"('{canon_tid}', '{safe_form}', '{morpheme}', 'suffix', 'IRREGULAR', 0)"
        )
        irr_ids.append(row_id)
        irr_count += 1

        if len(irr_batch) >= 500:
            psql_exec(f"""INSERT INTO new_token_variants
                (canonical_id, name, morpheme, morph_type, rule, characteristics)
                VALUES {','.join(irr_batch)} ON CONFLICT DO NOTHING;""")
            id_list = ','.join(irr_ids)
            psql_exec(f"DELETE FROM staging_morphynet_infl WHERE id IN ({id_list});")
            irr_batch = []
            irr_ids = []

    if irr_batch:
        psql_exec(f"""INSERT INTO new_token_variants
            (canonical_id, name, morpheme, morph_type, rule, characteristics)
            VALUES {','.join(irr_batch)} ON CONFLICT DO NOTHING;""")
        if irr_ids:
            id_list = ','.join(irr_ids)
            psql_exec(f"DELETE FROM staging_morphynet_infl WHERE id IN ({id_list});")

    print(f"  Irregular inflections stored: {irr_count}")

    # --- Step 8: Delete regular inflections from staging (they follow rules) ---
    print("\nStep 8: Clearing regular inflections from staging...")
    psql_exec("""
        DELETE FROM staging_morphynet_infl
        WHERE segmentation != '-'
        OR features IN ('N;SG', 'ADJ', 'V;NFIN;IMP+SBJV');
    """)

    # --- Final report ---
    remaining_deriv = psql("SELECT count(*) FROM staging_morphynet_deriv")
    remaining_infl = psql("SELECT count(*) FROM staging_morphynet_infl")
    new_tokens_count = psql("SELECT count(*) FROM new_tokens")
    new_pos_count = psql("SELECT count(*) FROM new_token_pos")
    new_variants_count = psql("SELECT count(*) FROM new_token_variants")

    print("\n" + "="*60)
    print("COMPOSITION COMPLETE")
    print("="*60)
    print(f"New tokens (roots): {new_tokens_count}")
    print(f"New token_pos entries: {new_pos_count}")
    print(f"New token_variants: {new_variants_count}")
    print(f"Remaining in staging (deriv): {remaining_deriv}")
    print(f"Remaining in staging (infl): {remaining_infl}")
    print(f"\nRoot exception tags applied: {len(exception_updates)}")
    print(f"Irregular inflections stored: {irr_count}")

if __name__ == '__main__':
    main()
