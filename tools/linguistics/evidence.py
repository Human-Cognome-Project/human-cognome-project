#!/usr/bin/env python3
"""Reproduce the checked-in pilot/coverage evidence. Defaults to comparison only."""
import argparse
import json
from pathlib import Path
from cldr import Cldr, ROLES
from icu import Icu
from manifest import generate, write_json
from sources import Sources, HERE, DEFAULT_CACHE
from unicode_values import UnicodeData
from validate_core import validate_export

EVIDENCE = HERE.parents[1] / 'data/linguistics/previews'


def selected_values(manifest):
    forms = {'A', '…', 'ch', 'CH', 'İ', 'ı', '؟', '\u200c', '\u200d'}
    return [{'value': value, 'groups': manifest['member_of'].get(value['key'], [])}
            for value in manifest['values'] if value['form'] in forms]


def reproduce(sources, floor_export=None, utf8_export=None, bootstrap_only=False):
    cldr, icu, ucd = Cldr(sources.cldr_files()), Icu(sources), UnicodeData(sources)
    hierarchy = json.loads((HERE / 'bootstrap-groups.json').read_text())
    if bootstrap_only:
        profile = json.loads((HERE / 'bootstrap-locales.json').read_text())
        manifest, coverage = generate(sources, cldr, icu, ucd, profile['locales'], ROLES, hierarchy)
        # A flat review catalog of the entire bulk population candidate batch.
        rows = []
        for locale in sorted(profile['locales']):
            rows.append({'locale': locale, 'language': 'language:' + locale,
                         'alphabet': manifest['members'].get('alphabet:' + locale, []),
                         'punctuation': manifest['members'].get('punctuation:' + locale, [])})
        applicable = {v for row in rows for role in ('alphabet', 'punctuation') for v in row[role]}
        catalog = {'encoding': 'UTF-8', 'inventories': rows,
                   'values': [{k: v[k] for k in ('key', 'locale', 'form', 'scalars', 'utf8_hex',
                                                'source_value', 'store', 'address', 'parents', 'structural_mass_derived')}
                              for v in manifest['values'] if v['key'] in applicable],
                   'core_child_updates': manifest['core_child_updates']}
        results = {'bootstrap-20/coverage.json': coverage, 'bootstrap-20/inventories.json': catalog}
        if utf8_export:
            results['bootstrap-20/utf8-validation.json'] = validate_export(utf8_export, manifest)
        return results
    en, en_report = generate(sources, cldr, icu, ucd, ['en'], ['main', 'punctuation'], hierarchy)
    # A new resolver avoids unrelated used-source hashes leaking between runs.
    six, six_report = generate(sources, Cldr(cldr.files), icu, ucd,
                               ['en', 'cs', 'tr', 'ar', 'fa', 'ur'], ROLES, hierarchy)
    results = {'english-pilot/manifest.json': en, 'english-pilot/coverage.json': en_report,
               'six-locales/coverage.json': six_report,
               'six-locales/selected-values.json': selected_values(six)}
    if floor_export:
        results['english-pilot/floor-validation.json'] = validate_export(floor_export, en)
    if utf8_export:
        results['english-pilot/utf8-validation.json'] = validate_export(utf8_export, en)
        results['six-locales/utf8-validation.json'] = validate_export(utf8_export, six)
    return results


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cache', type=Path, default=DEFAULT_CACHE)
    parser.add_argument('--write', action='store_true', help='replace local evidence files for a reviewed change')
    parser.add_argument('--core-export', type=Path, help='also reproduce floor report from its retained export')
    parser.add_argument('--utf8-export', type=Path, help='also reproduce English/six-locale reports from the full UTF-8 export')
    parser.add_argument('--bootstrap-only', action='store_true', help='reproduce the 20-language bootstrap evidence')
    args = parser.parse_args()
    results = reproduce(Sources(args.cache), args.core_export, args.utf8_export, args.bootstrap_only)
    for name, result in results.items():
        path = EVIDENCE / name
        if args.write:
            write_json(path, result)
        elif json.loads(path.read_text()) != result:
            raise SystemExit('Evidence differs: ' + name)
    print(f'{len(results)} evidence files ' + ('written.' if args.write else 'reproduced exactly.'))
