#!/usr/bin/env python3
"""
Phase 2: Apply existing transformation rules to unresolved MorphyNet entries,
then iteratively discover new rules from remaining patterns.

Process:
1. Apply all existing inflection_rules transformations to staging entries
2. For each remaining entry: derive a rule or declare unique
3. Run new rules against remaining entries
4. Repeat until everything is ruled or unique
"""

import os
import sys
import re
import subprocess
from collections import defaultdict

DB_ENV = {**os.environ, 'PGPASSWORD': 'hcp_dev'}

def psql(sql):
    r = subprocess.run(['psql','-h','localhost','-U','hcp','-d','hcp_english','-t','-A','-c',sql],
                       capture_output=True, text=True, env=DB_ENV)
    return r.stdout.strip()

def psql_exec(sql):
    r = subprocess.run(['psql','-h','localhost','-U','hcp','-d','hcp_english','-c',sql],
                       capture_output=True, text=True, env=DB_ENV)
    if r.returncode != 0 and r.stderr.strip():
        print(f"  ERR: {r.stderr.strip()}", file=sys.stderr)


def apply_suffix_rule(base, strip_suffix, add_suffix):
    """Apply a suffix transformation rule to a base word."""
    if strip_suffix == '__DOUBLING__':
        # CVC doubling: duplicate last consonant, then add suffix
        if (len(base) >= 3 and base[-1] not in 'aeiouwxy'
                and base[-2] in 'aeiou' and base[-3] not in 'aeiou'):
            return base + base[-1] + add_suffix
        return None
    if strip_suffix:
        if base.endswith(strip_suffix):
            return base[:-len(strip_suffix)] + add_suffix
        return None
    return base + add_suffix


def apply_prefix_rule(base, strip_prefix, add_prefix):
    """Apply a prefix transformation rule."""
    return add_prefix + base


def load_existing_rules():
    """Load existing inflection_rules from DB."""
    rules = []
    rows = psql("""
        SELECT morpheme, condition, strip_suffix, add_suffix,
               strip_prefix, add_prefix, rule_type, priority
        FROM inflection_rules
        ORDER BY morpheme, priority
    """)
    for line in rows.split('\n'):
        if '|' not in line: continue
        parts = [p.strip() for p in line.split('|')]
        if len(parts) < 8: continue
        rules.append({
            'morpheme': parts[0],
            'condition': parts[1],
            'strip_suffix': parts[2],
            'add_suffix': parts[3],
            'strip_prefix': parts[4],
            'add_prefix': parts[5],
            'rule_type': parts[6],
            'priority': int(parts[7]) if parts[7] else 99,
        })
    return rules


def load_staging():
    """Load all remaining staging entries."""
    entries = []
    rows = psql("SELECT id, base, derived, base_pos, derived_pos, morpheme, morph_type FROM staging_morphynet_deriv")
    for line in rows.split('\n'):
        if '|' not in line: continue
        parts = [p.strip() for p in line.split('|')]
        if len(parts) < 7: continue
        entries.append({
            'id': parts[0],
            'base': parts[1],
            'derived': parts[2],
            'base_pos': parts[3],
            'derived_pos': parts[4],
            'morpheme': parts[5],
            'morph_type': parts[6],
        })
    return entries


def try_rules_on_entry(entry, rules):
    """Try all existing rules on an entry. Return rule description if one matches."""
    base = entry['base']
    derived = entry['derived']

    for rule in rules:
        # Check condition regex against base
        try:
            if not re.search(rule['condition'], base):
                continue
        except re.error:
            continue

        if rule['rule_type'] == 'SUFFIX':
            result = apply_suffix_rule(base, rule['strip_suffix'], rule['add_suffix'])
            if result == derived:
                return f"{rule['morpheme']}_{rule['priority']}"
        elif rule['rule_type'] == 'PREFIX':
            result = apply_prefix_rule(base, rule['strip_prefix'], rule['add_prefix'])
            if result == derived:
                return f"{rule['morpheme']}_{rule['priority']}"

    return None


def insert_resolved(entry, rule_name):
    """Insert resolved entry into new_token_variants and delete from staging."""
    base = entry['base']
    derived = entry['derived']
    morpheme = entry['morpheme']
    morph_type = entry['morph_type']

    safe_base = base.replace("'", "''")
    canon = psql(f"SELECT token_id FROM new_tokens WHERE name = '{safe_base}' LIMIT 1")
    if not canon:
        return False

    safe_derived = derived.replace("'", "''")
    safe_morph = morpheme.replace("'", "''")
    safe_rule = rule_name.replace("'", "''")

    psql_exec(f"""
        INSERT INTO new_token_variants (canonical_id, name, morpheme, morph_type, rule, characteristics)
        VALUES ('{canon}', '{safe_derived}', '{safe_morph}', '{morph_type}', '{safe_rule}', 0)
        ON CONFLICT DO NOTHING;
    """)
    psql_exec(f"DELETE FROM staging_morphynet_deriv WHERE id = {entry['id']};")
    return True


def analyze_remaining_patterns(entries):
    """Look at remaining entries and identify potential new rules."""
    # Group by: what transformation actually happened?
    patterns = defaultdict(list)

    for entry in entries:
        base = entry['base']
        derived = entry['derived']
        morph_type = entry['morph_type']
        morpheme = entry['morpheme']

        if morph_type == 'suffix':
            # Find what was stripped from base end and what was added
            # Try different strip lengths
            for strip_len in range(0, min(5, len(base))):
                stripped_base = base[:len(base)-strip_len] if strip_len > 0 else base
                if derived.startswith(stripped_base):
                    added = derived[len(stripped_base):]
                    removed = base[len(base)-strip_len:] if strip_len > 0 else ''
                    key = (f'strip:{removed}', f'add:{added}', f'morph:{morpheme}')
                    patterns[key].append(entry)
                    break
            else:
                # No simple prefix match — complex stem change
                patterns[('COMPLEX', morpheme, base[-3:] if len(base)>=3 else base)].append(entry)

        elif morph_type == 'prefix':
            if derived.endswith(base):
                added = derived[:len(derived)-len(base)]
                key = (f'prefix:{added}', f'morph:{morpheme}')
                patterns[key].append(entry)
            else:
                patterns[('COMPLEX_PFX', morpheme)].append(entry)

    return patterns


def main():
    print("="*60)
    print("PHASE 2: Rule-based resolution of remaining MorphyNet entries")
    print("="*60)

    # --- Step 1: Load existing rules and staging ---
    print("\nStep 1: Loading existing rules and staging data...")
    rules = load_existing_rules()
    print(f"  Existing rules: {len(rules)}")

    entries = load_staging()
    print(f"  Staging entries: {len(entries)}")

    # --- Step 2: Apply existing rules ---
    print("\nStep 2: Applying existing transformation rules...")
    resolved_count = 0
    unresolved = []

    for entry in entries:
        rule_name = try_rules_on_entry(entry, rules)
        if rule_name:
            if insert_resolved(entry, rule_name):
                resolved_count += 1
                if resolved_count % 500 == 0:
                    print(f"  Resolved: {resolved_count}")
        else:
            unresolved.append(entry)

    print(f"  Resolved by existing rules: {resolved_count}")
    print(f"  Remaining: {len(unresolved)}")

    # --- Step 3: Analyze remaining patterns ---
    print("\nStep 3: Analyzing remaining patterns...")
    patterns = analyze_remaining_patterns(unresolved)

    # Sort by frequency — most common patterns become rules
    sorted_patterns = sorted(patterns.items(), key=lambda x: -len(x[1]))

    print(f"  Distinct patterns found: {len(sorted_patterns)}")
    print(f"\n  Top 30 patterns:")
    for key, examples in sorted_patterns[:30]:
        base_ex = examples[0]['base']
        derived_ex = examples[0]['derived']
        print(f"    {key}: {len(examples)} entries (e.g. {base_ex}→{derived_ex})")

    # --- Step 4: For patterns with 3+ entries, create and apply new rules ---
    print(f"\nStep 4: Creating new rules from patterns with 3+ entries...")
    new_rules_created = 0
    newly_resolved = 0

    for key, examples in sorted_patterns:
        if len(examples) < 3:
            continue  # Too few to justify a rule — these will be unique

        # Extract the transformation
        if len(key) >= 2 and isinstance(key[0], str) and key[0].startswith('strip:'):
            strip_part = key[0].replace('strip:', '')
            add_part = key[1].replace('add:', '')
            morpheme = key[2].replace('morph:', '') if len(key) > 2 else 'UNKNOWN'

            rule_name = f"DERIV_{strip_part.upper() or 'NONE'}_{add_part.upper()}"

            # Apply this rule to all entries in this pattern group
            for entry in examples:
                if insert_resolved(entry, rule_name):
                    newly_resolved += 1

            new_rules_created += 1

        elif len(key) >= 2 and isinstance(key[0], str) and key[0].startswith('prefix:'):
            prefix_part = key[0].replace('prefix:', '')
            morpheme = key[1].replace('morph:', '') if len(key) > 1 else 'UNKNOWN'

            rule_name = f"DERIV_PFX_{prefix_part.upper()}"

            for entry in examples:
                if insert_resolved(entry, rule_name):
                    newly_resolved += 1

            new_rules_created += 1

    print(f"  New rules created: {new_rules_created}")
    print(f"  Newly resolved: {newly_resolved}")

    # --- Step 5: Remaining are unique forms ---
    remaining = psql("SELECT count(*) FROM staging_morphynet_deriv")
    print(f"\nStep 5: Remaining unique/complex forms in staging: {remaining}")

    if int(remaining) > 0:
        # Show what's left
        print("\n  Sample remaining entries:")
        rows = psql("SELECT base, derived, morpheme, morph_type FROM staging_morphynet_deriv LIMIT 20")
        for line in rows.split('\n'):
            if line.strip():
                print(f"    {line}")

    # --- Final summary ---
    total_variants = psql("SELECT count(*) FROM new_token_variants")
    print(f"\n{'='*60}")
    print(f"PHASE 2 COMPLETE")
    print(f"{'='*60}")
    print(f"Total variants in clean DB: {total_variants}")
    print(f"Remaining in staging: {remaining}")
    print(f"Resolved by existing rules: {resolved_count}")
    print(f"Resolved by new rules: {newly_resolved}")


if __name__ == '__main__':
    main()
