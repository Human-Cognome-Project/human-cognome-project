#!/usr/bin/env python3
"""
Resolve the ~18K UNRESOLVED derivational variants by adding Latin/Greek
stem-change transformation rules.

Patterns identified:
  -ia → -ic    (mania→manic, anemia→anemic)
  -a → -ic     (panorama→panoramic)
  -y → -ic     (anarchy→anarchic, allergy→allergic)
  -is → -ic    (amaurosis→amaurotic — actually -is→-tic)
  -gy → -ist   (geology→geologist — drop -gy, add -gist)
  -sm → -st    (organism→organist — drop -sm, add -st)
  -y → -ist    (botany→botanist)
  -y → -ism    (botany→botanism)
  -y → -ical   (history→historical)
  -le → -ility (able→ability)
  -e → -ity    (rare→rarity... wait, that's silent-e)
  -ble → -bility (possible→possibility)
  -ous → -osity (viscous→viscosity)
  -an → -anic  (organ→organic type patterns)
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
    return result.stdout.strip()

def psql_exec(sql):
    result = subprocess.run(
        ['psql', '-h', 'localhost', '-U', 'hcp', '-d', 'hcp_english', '-c', sql],
        capture_output=True, text=True, env=DB_ENV
    )
    if result.returncode != 0 and result.stderr.strip():
        print(f"  SQL ERROR: {result.stderr.strip()}", file=sys.stderr)
    return result.returncode == 0


# Additional stem-change rules beyond the basic 6
# Each rule: (base_ending, suffix_morpheme, base_strip, replacement) → derived = base[:-strip] + replacement
# Tried in order; first match wins.

STEM_RULES = [
    # -ia endings
    ('ia', 'ic', 2, 'ic'),          # mania→manic
    ('ia', 'ical', 2, 'ical'),      # mania→maniacal? — check
    ('ia', 'ist', 2, 'ist'),        # malaria→malarist (rare)

    # -a endings (non -ia)
    ('ma', 'ic', 1, 'tic'),         # stigma→stigmatic, panorama→panoramic...
    ('a', 'ic', 1, 'ic'),           # panorama→panoramic (drop a, add ic)
    ('a', 'tic', 1, 'tic'),         # stigma→stigmatic
    ('a', 'ist', 1, 'ist'),         # drama→dramatist

    # -y endings (very common: -gy, -hy, -py, -my, -ny, -ry)
    ('y', 'ic', 1, 'ic'),           # anarchy→anarchic, allergy→allergic
    ('y', 'ical', 1, 'ical'),       # history→historical, whimsy→whimsical
    ('y', 'ist', 1, 'ist'),         # botany→botanist, biology→biologist
    ('y', 'ism', 1, 'ism'),         # alcohol→alcoholism... well, alchemy→alchemism
    ('y', 'ize', 1, 'ize'),         # colony→colonize
    ('y', 'ity', 1, 'ity'),         # oddity type patterns

    # -gy → -gist (special case of -y→-ist where g is retained)
    ('gy', 'ist', 2, 'gist'),       # geology→geologist
    ('gy', 'ize', 2, 'gize'),       # geology→geologize

    # -is endings (Greek/Latin)
    ('is', 'ic', 2, 'ic'),          # genesis→genetic? No: gene→genetic
    ('sis', 'tic', 3, 'tic'),       # amaurosis→amaurotic, neurosis→neurotic
    ('sis', 'st', 3, 'st'),         # analysis→analyst
    ('is', 'tic', 2, 'tic'),        # arthritis→arthritic (drop -is, add -tic)

    # -us endings (Latin)
    ('us', 'ic', 2, 'ic'),          # focus→focal? No...
    ('us', 'al', 2, 'al'),          # stimulus→stimulal? No...
    ('us', 'ous', 2, 'ous'),        # glamour/glamorous type

    # -um endings (Latin)
    ('um', 'al', 2, 'al'),          # spectrum→spectral
    ('um', 'ic', 2, 'ic'),          # algorithm→algorithmic
    ('um', 'a', 2, 'a'),            # datum→data, corrigendum→corrigenda

    # -on endings (Greek)
    ('on', 'al', 2, 'al'),          # phenomenon→phenomenal
    ('on', 'ic', 2, 'ic'),          # demon→demonic

    # -ble → -bility
    ('ble', 'ity', 3, 'bility'),    # possible→possibility, able→ability
    ('ble', 'ly', 3, 'bly'),        # possible→possibly (already handled?)

    # -ous → -osity/-ity
    ('ous', 'ity', 3, 'osity'),     # viscous→viscosity, curious→curiosity

    # -ive → -ivity/-tion
    ('ive', 'ity', 3, 'ivity'),     # creative→creativity, active→activity

    # -al → -ality
    ('al', 'ity', 2, 'ality'),      # national→nationality

    # -ent/-ant → -ence/-ance
    ('ent', 'ence', 3, 'ence'),     # different→difference
    ('ant', 'ance', 3, 'ance'),     # distant→distance
    ('ent', 'cy', 3, 'ency'),       # frequent→frequency
    ('ant', 'cy', 3, 'ancy'),       # vacant→vacancy

    # -ic → -ical (already a suffix, but stem doesn't change)
    ('ic', 'al', 0, 'al'),          # magic→magical, music→musical
    ('ic', 'ally', 0, 'ally'),      # magic→magically
    ('ic', 'ist', 0, 'ist'),        # classic→classicist
    ('ic', 'ism', 0, 'ism'),        # classic→classicism
    ('ic', 'ize', 0, 'ize'),        # romantic→romanticize

    # -ate → -ation (drop e, add ion — but also ate→ation)
    ('ate', 'ion', 3, 'ation'),     # create→creation, educate→education
    ('ate', 'ive', 3, 'ative'),     # create→creative
    ('ate', 'or', 3, 'ator'),       # create→creator

    # -fy/-ify → -fication
    ('fy', 'ication', 2, 'fication'),   # simplify→simplification
    ('fy', 'ier', 2, 'fier'),           # simplify→simplifier

    # -ism → -ist (drop m, add t... or just replace)
    ('sm', 'st', 2, 'st'),          # organism→organist, journalism→journalist

    # -ing → -er (for compound derivations: storytelling→storyteller)
    ('ing', 'er', 3, 'er'),         # bookkeeping→bookkeeper

    # -ne → -nic
    ('ne', 'ic', 2, 'nic'),         # hygiene→hygienic

    # -oid patterns
    ('oid', 'al', 0, 'al'),         # spheroid→spheroidal
]


def try_stem_rules(base, derived, morpheme):
    """Try each stem-change rule. Return rule name if one matches."""
    for base_ending, morph, strip_len, replacement in STEM_RULES:
        if not base.endswith(base_ending):
            continue
        if morph != morpheme:
            continue
        if strip_len > 0:
            candidate = base[:-strip_len] + replacement
        else:
            candidate = base + replacement
        if candidate == derived:
            return f'STEM_{base_ending.upper()}_{morph.upper()}'
    return None


def main():
    print("Resolving UNRESOLVED derivational variants...")

    # Load unresolved entries from staging
    rows = psql("""
        SELECT id, base, derived, morpheme
        FROM staging_morphynet_deriv
    """)

    entries = []
    for line in rows.split('\n'):
        if '|' not in line:
            continue
        parts = line.split('|')
        if len(parts) < 4:
            continue
        var_id, base, derived, morpheme = [p.strip() for p in parts]
        entries.append((var_id, base, derived, morpheme))

    print(f"  Total unresolved: {len(entries)}")

    resolved = defaultdict(int)
    still_unresolved = 0
    updates = []

    for var_id, base, derived, morpheme in entries:
        rule = try_stem_rules(base, derived, morpheme)
        if rule:
            resolved[rule] += 1
            updates.append((var_id, rule))
        else:
            still_unresolved += 1

    print(f"\n  Resolved: {sum(resolved.values())}")
    print(f"  Still unresolved: {still_unresolved}")

    print(f"\n  Rules matched:")
    for rule, count in sorted(resolved.items(), key=lambda x: -x[1]):
        print(f"    {rule}: {count}")

    # For resolved entries: look up the root token_id, insert into new_token_variants,
    # delete from staging. Destructive consumption.
    if updates:
        print(f"\n  Processing {len(updates)} resolved entries...")
        inserted = 0
        for var_id, rule in updates:
            # Find the entry in staging
            row = psql(f"SELECT base, derived, morpheme FROM staging_morphynet_deriv WHERE id = {var_id}")
            if '|' not in row:
                continue
            base, derived, morpheme = [p.strip() for p in row.split('|')]

            # Find root token_id
            canon_tid = psql(f"SELECT token_id FROM new_tokens WHERE name = '{base.replace(chr(39), chr(39)+chr(39))}' LIMIT 1")
            if not canon_tid:
                continue

            safe_derived = derived.replace("'", "''")
            safe_morpheme = morpheme.replace("'", "''")
            safe_rule = rule.replace("'", "''")

            psql_exec(f"""
                INSERT INTO new_token_variants (canonical_id, name, morpheme, morph_type, rule, characteristics)
                VALUES ('{canon_tid}', '{safe_derived}', '{safe_morpheme}', 'suffix', '{safe_rule}', 0)
                ON CONFLICT DO NOTHING;
            """)
            psql_exec(f"DELETE FROM staging_morphynet_deriv WHERE id = {var_id};")
            inserted += 1

            if inserted % 1000 == 0:
                print(f"    Processed {inserted}...")

        print(f"  Inserted {inserted} resolved variants, deleted from staging.")

        # Tag roots with STEM_CHANGE exception
        print("\n  Tagging roots with STEM_CHANGE exception...")
        psql_exec("""
            UPDATE new_tokens t
            SET morph_exception = COALESCE(morph_exception || '+STEM_CHANGE', 'STEM_CHANGE')
            WHERE token_id IN (
                SELECT DISTINCT canonical_id FROM new_token_variants
                WHERE rule LIKE 'STEM_%'
            )
            AND (morph_exception IS NULL OR morph_exception NOT LIKE '%STEM%');
        """)

        tagged = psql("SELECT count(*) FROM new_tokens WHERE morph_exception LIKE '%STEM%'")
        print(f"  Roots tagged with STEM_CHANGE: {tagged}")

    # Final count
    remaining = psql("SELECT count(*) FROM staging_morphynet_deriv")
    print(f"\n  Remaining in staging: {remaining}")


if __name__ == '__main__':
    main()
