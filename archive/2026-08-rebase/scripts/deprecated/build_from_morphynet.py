#!/usr/bin/env python3
"""
Build clean English token database from MorphyNet + Kaikki sources.

Two-pass morphological rule determination:
  Pass 1: Does base + morpheme produce the derived form by default rules?
  Pass 2: If not, which spelling transformation (y→i, silent-e, CVC doubling) works?

Source data:
  - MorphyNet derivational: root→derived pairs with morpheme labels
  - MorphyNet inflectional: lemma→inflected pairs with feature tags
  - Kaikki: glosses, etymology, archaic/dialect forms (supplementary)
"""

import csv
import sys
import os
import subprocess
import re
from collections import defaultdict

# ---------------------------------------------------------------------------
# DB helpers
# ---------------------------------------------------------------------------

DB_ENV = {**os.environ, 'PGPASSWORD': 'hcp_dev'}

def psql(sql, db='hcp_english'):
    result = subprocess.run(
        ['psql', '-h', 'localhost', '-U', 'hcp', '-d', db, '-t', '-A', '-c', sql],
        capture_output=True, text=True, env=DB_ENV
    )
    if result.returncode != 0:
        print(f"SQL ERROR: {result.stderr.strip()}", file=sys.stderr)
    return result.stdout.strip()

def psql_exec(sql, db='hcp_english'):
    result = subprocess.run(
        ['psql', '-h', 'localhost', '-U', 'hcp', '-d', db, '-c', sql],
        capture_output=True, text=True, env=DB_ENV
    )
    if result.returncode != 0:
        print(f"SQL ERROR: {result.stderr.strip()}", file=sys.stderr)
    return result.returncode == 0

# ---------------------------------------------------------------------------
# Spelling transformation rules
# ---------------------------------------------------------------------------

def default_suffixed(base, suffix):
    """Default: just concatenate."""
    return base + suffix

def silent_e_suffixed(base, suffix):
    """Drop terminal e before vowel-initial suffix."""
    if base.endswith('e') and suffix and suffix[0] in 'aeiouy':
        return base[:-1] + suffix
    return None

def y_to_i_suffixed(base, suffix):
    """Change terminal y to i before suffix (if preceded by consonant)."""
    if (base.endswith('y') and len(base) >= 2
            and base[-2] not in 'aeiou'
            and suffix and suffix[0] not in 'i'):
        return base[:-1] + 'i' + suffix
    return None

def cvc_doubling_suffixed(base, suffix):
    """Double final consonant for CVC pattern before vowel-initial suffix."""
    if (len(base) >= 3
            and suffix and suffix[0] in 'aeiouy'
            and base[-1] not in 'aeiouwxy'
            and base[-2] in 'aeiou'
            and base[-3] not in 'aeiou'):
        return base + base[-1] + suffix
    return None

def default_prefixed(base, prefix):
    """Default: just concatenate."""
    return prefix + base

def determine_rule(base, derived, morpheme, morph_type):
    """
    Determine which spelling rule transforms base+morpheme into derived.
    Returns (rule_name, None) or (None, None) if no rule matches.
    """
    if morph_type == 'prefix':
        # Prefixes rarely change spelling
        if default_prefixed(base, morpheme) == derived:
            return ('DEFAULT', None)
        # Try with hyphen
        if morpheme + '-' + base == derived:
            return ('HYPHENATED', None)
        # Some prefixes modify (e.g., in→il before l)
        return (None, None)

    # Suffix rules — try in order
    # 1. Default (just append)
    if default_suffixed(base, morpheme) == derived:
        return ('DEFAULT', None)

    # 2. Silent-e drop
    result = silent_e_suffixed(base, morpheme)
    if result and result == derived:
        return ('SILENT_E', None)

    # 3. y→i
    result = y_to_i_suffixed(base, morpheme)
    if result and result == derived:
        return ('Y_TO_I', None)

    # 4. CVC doubling
    result = cvc_doubling_suffixed(base, morpheme)
    if result and result == derived:
        return ('CVC_DOUBLE', None)

    # 5. Silent-e drop + y→i combo (rare)
    # 6. Other patterns we haven't identified

    # Try: base ends in 'e', suffix starts with consonant (keep e)
    # This is default and should have matched above

    # Try: base ends in 'ic', suffix is 'ally' → 'ically'
    if base.endswith('ic') and morpheme == 'ally' and derived == base + 'ally':
        return ('DEFAULT', None)

    # Try: base ends in 'le', suffix is 'y' → drop le, add ly
    if base.endswith('le') and morpheme == 'ly' and derived == base[:-2] + 'ly':
        return ('LE_TO_LY', None)

    return (None, None)

# ---------------------------------------------------------------------------
# Load MorphyNet derivational
# ---------------------------------------------------------------------------

def load_derivational(filepath):
    """Load MorphyNet derivational TSV. Returns list of dicts."""
    entries = []
    with open(filepath, 'r', encoding='utf-8') as f:
        reader = csv.reader(f, delimiter='\t')
        for row in reader:
            if len(row) < 6:
                continue
            entries.append({
                'base': row[0].strip().lower(),
                'derived': row[1].strip().lower(),
                'base_pos': row[2].strip(),
                'derived_pos': row[3].strip(),
                'morpheme': row[4].strip(),
                'morph_type': row[5].strip(),  # prefix or suffix
            })
    return entries

# ---------------------------------------------------------------------------
# Load MorphyNet inflectional
# ---------------------------------------------------------------------------

def load_inflectional(filepath):
    """Load MorphyNet inflectional TSV. Returns list of dicts."""
    entries = []
    with open(filepath, 'r', encoding='utf-8') as f:
        reader = csv.reader(f, delimiter='\t')
        for row in reader:
            if len(row) < 4:
                continue
            entries.append({
                'lemma': row[0].strip().lower(),
                'form': row[1].strip().lower(),
                'features': row[2].strip(),
                'segmentation': row[3].strip(),
            })
    return entries

# ---------------------------------------------------------------------------
# Identify roots
# ---------------------------------------------------------------------------

def identify_roots(deriv_entries):
    """
    A root is a word that appears as a base in derivational entries
    but whose ultimate source is itself (tracing the chain back).
    Also include words that appear ONLY as bases, never as derived forms.
    """
    # All words that appear as derived forms
    derived_set = {e['derived'] for e in deriv_entries}
    # All words that appear as base forms
    base_set = {e['base'] for e in deriv_entries}

    # Pure roots: appear as base but never as derived
    pure_roots = base_set - derived_set

    # Words that appear as both base and derived (intermediate nodes)
    intermediates = base_set & derived_set

    # Words that appear only as derived (terminal leaves)
    terminal_derived = derived_set - base_set

    return pure_roots, intermediates, terminal_derived

# ---------------------------------------------------------------------------
# Two-pass rule determination
# ---------------------------------------------------------------------------

def analyze_rules(deriv_entries):
    """
    Pass 1: Try default concatenation.
    Pass 2: Try spelling transformations.
    Returns categorized results.
    """
    results = {
        'DEFAULT': [],
        'SILENT_E': [],
        'Y_TO_I': [],
        'CVC_DOUBLE': [],
        'LE_TO_LY': [],
        'HYPHENATED': [],
        'UNRESOLVED': [],
    }

    root_exceptions = defaultdict(set)  # root → set of exception tags

    for entry in deriv_entries:
        rule, _ = determine_rule(
            entry['base'], entry['derived'],
            entry['morpheme'], entry['morph_type']
        )

        if rule is None:
            results['UNRESOLVED'].append(entry)
        else:
            results[rule].append(entry)
            if rule != 'DEFAULT' and rule != 'HYPHENATED':
                root_exceptions[entry['base']].add(rule)

    return results, root_exceptions

# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    deriv_path = '/tmp/morphynet_deriv.tsv'
    infl_path = '/tmp/morphynet_infl.tsv'

    if not os.path.exists(deriv_path):
        print("ERROR: MorphyNet derivational file not found at", deriv_path)
        sys.exit(1)

    # --- Load data ---
    print("Loading MorphyNet derivational...")
    deriv = load_derivational(deriv_path)
    print(f"  Loaded {len(deriv)} derivational entries")

    print("Loading MorphyNet inflectional...")
    infl = load_inflectional(infl_path)
    print(f"  Loaded {len(infl)} inflectional entries")

    # --- Identify roots ---
    print("\nIdentifying roots...")
    pure_roots, intermediates, terminal_derived = identify_roots(deriv)
    print(f"  Pure roots (never derived from another): {len(pure_roots)}")
    print(f"  Intermediate (both base and derived): {len(intermediates)}")
    print(f"  Terminal derived (never a base for others): {len(terminal_derived)}")

    # Also get all unique lemmas from inflectional
    infl_lemmas = {e['lemma'] for e in infl}
    print(f"  Inflectional unique lemmas: {len(infl_lemmas)}")

    # Roots are: pure_roots + inflectional lemmas that aren't derived
    all_roots = pure_roots | (infl_lemmas - terminal_derived)
    print(f"  Combined root set: {len(all_roots)}")

    # --- Two-pass rule analysis ---
    print("\nRunning two-pass rule analysis on derivational entries...")
    results, root_exceptions = analyze_rules(deriv)

    print(f"\n  Pass 1 (DEFAULT): {len(results['DEFAULT'])} ({len(results['DEFAULT'])/len(deriv)*100:.1f}%)")
    print(f"  Pass 2 results:")
    for rule in ['SILENT_E', 'Y_TO_I', 'CVC_DOUBLE', 'LE_TO_LY', 'HYPHENATED']:
        count = len(results[rule])
        if count > 0:
            print(f"    {rule}: {count} ({count/len(deriv)*100:.1f}%)")
    print(f"  UNRESOLVED: {len(results['UNRESOLVED'])} ({len(results['UNRESOLVED'])/len(deriv)*100:.1f}%)")

    # --- Root exception tags ---
    print(f"\n  Roots needing exception tags: {len(root_exceptions)}")
    tag_counts = defaultdict(int)
    for tags in root_exceptions.values():
        for tag in tags:
            tag_counts[tag] += 1
    for tag, count in sorted(tag_counts.items(), key=lambda x: -x[1]):
        print(f"    {tag}: {count} roots")

    # --- Sample unresolved ---
    print(f"\n  Sample UNRESOLVED entries:")
    for entry in results['UNRESOLVED'][:20]:
        print(f"    {entry['base']} → {entry['derived']} ({entry['morpheme']}, {entry['morph_type']})")

    # --- Inflectional irregular analysis ---
    print("\nAnalyzing inflectional irregulars...")
    irregular_count = 0
    irregular_by_type = defaultdict(int)
    for entry in infl:
        if entry['segmentation'] == '-' and entry['features'] not in ('N;SG', 'ADJ', 'V;NFIN;IMP+SBJV'):
            irregular_count += 1
            irregular_by_type[entry['features']] += 1

    print(f"  Truly irregular inflections: {irregular_count}")
    for feat, count in sorted(irregular_by_type.items(), key=lambda x: -x[1]):
        print(f"    {feat}: {count}")

    # --- Summary ---
    print("\n" + "="*60)
    print("SUMMARY")
    print("="*60)
    print(f"Total roots: {len(all_roots)}")
    print(f"Total derivational pairs: {len(deriv)}")
    print(f"  Resolved by default rules: {len(deriv) - len(results['UNRESOLVED'])} ({(len(deriv) - len(results['UNRESOLVED']))/len(deriv)*100:.1f}%)")
    print(f"  Unresolved: {len(results['UNRESOLVED'])} ({len(results['UNRESOLVED'])/len(deriv)*100:.1f}%)")
    print(f"Roots needing exception tags: {len(root_exceptions)} ({len(root_exceptions)/len(all_roots)*100:.1f}%)")
    print(f"Irregular inflections: {irregular_count}")
    print(f"Combined vocabulary (roots + all forms): {len(all_roots | {e['derived'] for e in deriv} | {e['form'] for e in infl})}")

if __name__ == '__main__':
    main()
