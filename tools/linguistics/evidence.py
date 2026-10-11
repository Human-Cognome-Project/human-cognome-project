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


def reproduce(sources):
    cldr, icu, ucd = Cldr(sources.cldr_files()), Icu(sources), UnicodeData(sources)
    hierarchy = json.loads((HERE / 'bootstrap-groups.json').read_text())
    en, en_report = generate(sources, cldr, icu, ucd, ['en'], ['main', 'punctuation'], hierarchy)
    # A new resolver avoids unrelated used-source hashes leaking between runs.
    six, six_report = generate(sources, Cldr(cldr.files), icu, ucd,
                               ['en', 'cs', 'tr', 'ar', 'fa', 'ur'], ROLES, hierarchy)
    return {'english-pilot/manifest.json': en, 'english-pilot/coverage.json': en_report,
            'six-locales/coverage.json': six_report,
            'six-locales/selected-values.json': selected_values(six)}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cache', type=Path, default=DEFAULT_CACHE)
    parser.add_argument('--write', action='store_true', help='replace local evidence files for a reviewed change')
    parser.add_argument('--core-export', type=Path, help='also reproduce floor report from its retained export')
    args = parser.parse_args()
    results = reproduce(Sources(args.cache))
    if args.core_export:
        results['english-pilot/floor-validation.json'] = validate_export(
            args.core_export, results['english-pilot/manifest.json'])
    for name, result in results.items():
        path = EVIDENCE / name
        if args.write:
            write_json(path, result)
        elif json.loads(path.read_text()) != result:
            raise SystemExit('Evidence differs: ' + name)
    print(f'{len(results)} evidence files ' + ('written.' if args.write else 'reproduced exactly.'))
