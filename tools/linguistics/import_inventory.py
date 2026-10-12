#!/usr/bin/env python3
"""Bulk bootstrap population preparation and validation. No database writer."""
import argparse
import json
from pathlib import Path
import sys
from cldr import Cldr, ROLES
from icu import Icu
from manifest import generate, validate_manifest, write_json
from sources import Sources, DEFAULT_CACHE, HERE
from unicode_values import UnicodeData
from validate_core import validate_export


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cache', type=Path, default=DEFAULT_CACHE)
    commands = parser.add_subparsers(dest='command', required=True)
    commands.add_parser('fetch', help='fetch and verify pinned public inputs')
    preview = commands.add_parser('preview', help='emit candidate manifest and coverage, without DB writes')
    locale_input = preview.add_mutually_exclusive_group(required=True)
    locale_input.add_argument('--locales', nargs='+')
    locale_input.add_argument('--profile', type=Path, help='JSON profile with a locales list')
    preview.add_argument('--roles', nargs='+', default=list(ROLES), choices=ROLES)
    preview.add_argument('--hierarchy', type=Path, default=HERE / 'bootstrap-groups.json')
    preview.add_argument('--out', type=Path, required=True)
    preview.add_argument('--allow-gaps', action='store_true', help='retain gaps but permit a successful preview exit')
    validate = commands.add_parser('validate-core', help='inspect COPY export without restoring it')
    validate.add_argument('--manifest', type=Path, required=True)
    validate.add_argument('--export', type=Path, required=True)
    validate.add_argument('--out', type=Path, required=True)
    args = parser.parse_args()
    try:
        if args.command == 'fetch':
            Sources(args.cache).fetch()
            print('All pinned source hashes verified.')
            return 0
        if args.command == 'preview':
            sources = Sources(args.cache)
            profile = json.loads(args.profile.read_text()) if args.profile else None
            locales = (profile.get('locales') if isinstance(profile, dict) else None) if args.profile else args.locales
            if not isinstance(locales, list) or not locales or not all(isinstance(x, str) for x in locales):
                raise ValueError('Profile locales must be a nonempty list of locale IDs')
            manifest, coverage = generate(sources, Cldr(sources.cldr_files()), Icu(sources),
                                          UnicodeData(sources), locales, args.roles,
                                          json.loads(args.hierarchy.read_text()))
            write_json(args.out / 'manifest.json', manifest)
            write_json(args.out / 'coverage.json', coverage)
            print(json.dumps(coverage['counts'], sort_keys=True))
            return 0 if args.allow_gaps or not coverage['counts']['coverage_gaps'] else 2
        manifest = json.loads(args.manifest.read_text())
        validate_manifest(manifest)
        report = validate_export(args.export, manifest)
        write_json(args.out, report)
        print(json.dumps({k: report[k] for k in ('status', 'expected_tokens', 'validated_tokens',
                                               'validated_floor_bytes', 'database_writes')}))
        return 0 if report['status'] == 'valid' else 2
    except (ValueError, OSError) as error:
        print(f'Inventory error: {error}', file=sys.stderr)
        return 1


if __name__ == '__main__':
    sys.exit(main())
