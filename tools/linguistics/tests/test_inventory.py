import copy
import gzip
import json
from pathlib import Path
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from cldr import Cldr, ROLES
from icu import Icu
from manifest import generate, validate_manifest
from sources import Sources, HERE
from unicode_values import UnicodeData, character_address, byte_address, nibble_address, key
from validate_core import expected_definitions, validate_export


def fixture_cldr(docs, parents=''):
    files = {f'common/main/{locale}.xml': ('<ldml>' + xml + '</ldml>').encode()
             for locale, xml in docs.items()}
    files['common/supplemental/supplementalData.xml'] = (
        '<supplementalData><parentLocales>' + parents + '</parentLocales></supplementalData>').encode()
    return Cldr(files)


def characters(main, auxiliary=None):
    return ('<characters><exemplarCharacters>' + main + '</exemplarCharacters>'
            + (f'<exemplarCharacters type="auxiliary">{auxiliary}</exemplarCharacters>'
               if auxiliary is not None else '') + '</characters>')


class CldrTests(unittest.TestCase):
    def test_explicit_parent_precedes_truncation(self):
        cldr = fixture_cldr({'root': characters('[]'), 'en': characters('[a]'),
                             'en_001': characters('[b]'), 'en_AU': characters('↑↑↑')},
                            '<parentLocale parent="en_001" locales="en_AU"/>')
        result = cldr.resolve('en_AU', 'main')
        self.assertEqual(result['source_locale'], 'en_001')
        self.assertEqual(result['trace'][0]['target_locale'], 'en_001')

    def test_missing_intermediate_follows_documented_parent_chain(self):
        cldr = fixture_cldr({'root': characters('[]'), 'en': characters('[a]'), 'en_AU': ''},
                            '<parentLocale parent="en_001" locales="en_AU"/>')
        self.assertEqual(cldr.resolve('en_AU', 'main')['source_locale'], 'en')

    def test_unknown_requested_locale_stays_missing(self):
        cldr = fixture_cldr({'root': characters('[x]'), 'en': characters('[a]')})
        self.assertIsNone(cldr.resolve('en_XY', 'main'))

    def test_whole_locale_alias(self):
        cldr = fixture_cldr({'root': characters('[]'), 'en': characters('[a]'),
                             'x': '<alias source="en"/>'})
        result = cldr.resolve('x', 'main')
        self.assertEqual(result['pattern'], '[a]')
        self.assertEqual(result['trace'][0]['via'], 'alias')

    def test_section_alias(self):
        cldr = fixture_cldr({'root': characters('[]'), 'en': characters('[a]'),
                             'x': '<characters><alias source="en" path="../characters"/></characters>'})
        self.assertEqual(cldr.resolve('x', 'main')['pattern'], '[a]')

    def test_relative_field_alias_and_draft(self):
        cldr = fixture_cldr({'root': characters('[]'), 'x': '''<characters>
          <exemplarCharacters><alias source="locale" path="../exemplarCharacters[@type='auxiliary']"/></exemplarCharacters>
          <exemplarCharacters type="auxiliary" draft="contributed">[b]</exemplarCharacters>
        </characters>'''})
        result = cldr.resolve('x', 'main')
        self.assertEqual((result['pattern'], result['draft']), ('[b]', 'contributed'))

    def test_alias_cycle_rejected(self):
        cldr = fixture_cldr({'root': characters('[]'), 'x': '<alias source="y"/>',
                             'y': '<alias source="x"/>'})
        with self.assertRaisesRegex(ValueError, 'cycle'):
            cldr.resolve('x', 'main')

    def test_parent_cycle_rejected(self):
        cldr = fixture_cldr({'root': characters('[]'), 'x': '', 'y': ''},
                            '<parentLocale parent="y" locales="x"/><parentLocale parent="x" locales="y"/>')
        with self.assertRaisesRegex(ValueError, 'cycle'):
            cldr.resolve('x', 'main')

    def test_unsupported_alias_rejected(self):
        cldr = fixture_cldr({'root': characters('[]'), 'x':
                             '<characters><alias source="locale" path="//characters"/></characters>'})
        with self.assertRaisesRegex(ValueError, 'Unsupported'):
            cldr.resolve('x', 'main')

    def test_no_inheritance_marker(self):
        cldr = fixture_cldr({'root': characters('[a]'), 'x': characters('∅∅∅')})
        self.assertIsNone(cldr.resolve('x', 'main'))

    def test_ancestor_draft_is_retained(self):
        cldr = fixture_cldr({'root': characters('[]'), 'x':
                             '<characters draft="provisional"><exemplarCharacters>[a]</exemplarCharacters></characters>'})
        self.assertEqual(cldr.resolve('x', 'main')['draft'], 'provisional')

    def test_alt_is_not_selected_implicitly(self):
        cldr = fixture_cldr({'root': characters('[a]'), 'x':
                             '<characters><exemplarCharacters alt="proposed">[z]</exemplarCharacters></characters>'})
        self.assertEqual(cldr.resolve('x', 'main')['pattern'], '[a]')


class SourceAndScalarTests(unittest.TestCase):
    def test_bad_source_hash_fails_closed(self):
        with tempfile.TemporaryDirectory() as directory:
            source = Sources(directory)
            source.lock['files'] = {'icu4j.jar': source.lock['files']['icu4j.jar']}
            (Path(directory) / 'icu4j.jar').write_bytes(b'wrong source')
            with self.assertRaisesRegex(ValueError, 'SHA256 mismatch'):
                source.path('icu4j.jar')
            with self.assertRaisesRegex(ValueError, 'SHA256 mismatch'):
                source.fetch()

    def test_address_examples_and_utf8_boundaries(self):
        expected = {0x41: '00.00.00.00.23', 0x2026: '00.00.00.04.bi',
                    0x131: '00.00.00.01.2r', 0x130: '00.00.00.01.2q',
                    0x61F: '00.00.00.01.ND', 0x80: '00.00.00.01.00',
                    0x800: '00.00.00.03.00', 0x10000: '00.00.00.10.00'}
        for cp, identity in expected.items():
            self.assertEqual(character_address(cp), identity)
        self.assertLess(character_address(0xD7FF), character_address(0xE000))
        self.assertLess(character_address(0xFFFF), character_address(0x10000))
        self.assertEqual(character_address(0), byte_address(0))
        self.assertTrue(character_address(0x10FFFF).startswith('00.00.00.'))

    def test_invalid_scalars_rejected(self):
        for cp in (-1, 0xD800, 0xDFFF, 0x110000):
            with self.assertRaises(ValueError):
                character_address(cp)
        with self.assertRaises(ValueError):
            key('')


class PinnedInventoryTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.sources = Sources()
        cls.icu = Icu(cls.sources)
        cls.ucd = UnicodeData(cls.sources)
        cls.files = cls.sources.cldr_files()
        cls.hierarchy = json.loads((HERE / 'bootstrap-groups.json').read_text())
        cls.manifest, cls.report = generate(cls.sources, Cldr(cls.files), cls.icu, cls.ucd,
                                            ['en', 'cs', 'tr', 'ar', 'fa', 'ur'], ROLES, cls.hierarchy)

    def test_unicode17_properties_ranges_and_strings(self):
        sets = self.icu.batch([('SET', r'[a-c {ch} \u2026]'),
                               ('SET', r'[\p{sc=Tolong_Siki}]'),
                               ('SET', r'[\p{sc=Latn}&\p{Lu}]')])
        self.assertEqual(set(sets[0]), {'a', 'b', 'c', 'ch', '…'})
        self.assertIn(chr(0x11DB0), sets[1])  # Script added in Unicode 17.
        self.assertIn('A', sets[2])
        self.assertNotIn('a', sets[2])
        with self.assertRaisesRegex(ValueError, 'non-scalar'):
            self.icu.batch([('SET', r'[\uD800]')])

    def test_turkish_case_preserves_dotted_and_dotless_i(self):
        sets = self.icu.batch([('CASE', 'tr', 'i'), ('CASE', 'tr', 'I')])
        self.assertIn('İ', sets[0])
        self.assertNotIn('I', sets[0])
        self.assertIn('ı', sets[1])
        self.assertNotIn('i', sets[1])

    def test_ucd_ranges_and_script_extensions(self):
        self.assertEqual(self.ucd.metadata(0x4E01)['category'], 'Lo')
        self.assertEqual(self.ucd.metadata(0x4E01)['script'], 'Han')
        self.assertEqual(set(self.ucd.metadata(0x30FC)['script_extensions']), {'Hiragana', 'Katakana'})
        self.assertEqual(self.ucd.metadata(0x11DB0)['script'], 'Tolong_Siki')

    def test_source_totals_remain_separate_from_edges(self):
        self.assertEqual(self.report['counts']['source_memberships'], 771)
        self.assertEqual(self.report['counts']['distinct_source_sequences'], 249)
        expected = {'en': 122, 'cs': 133, 'tr': 132, 'ar': 111, 'fa': 119, 'ur': 154}
        self.assertEqual({r['locale']: r['source_memberships'] for r in self.report['coverage']}, expected)
        self.assertNotEqual(self.report['counts']['membership_edges'], 771)
        self.assertEqual(self.report['counts']['coverage_gaps'], 0)

    def test_shared_scalar_has_one_identity_across_languages(self):
        identity = key('A')
        self.assertEqual(sum(v['key'] == identity for v in self.manifest['values']), 1)
        self.assertTrue({'language:en', 'language:cs', 'language:tr'}.issubset(self.manifest['member_of'][identity]))
        self.assertNotIn('parents', next(v for v in self.manifest['values'] if v['key'] == identity))

    def test_ellipsis_is_one_endpoint(self):
        value = next(v for v in self.manifest['values'] if v['form'] == '…')
        self.assertEqual(value['address'], '00.00.00.04.bi')
        self.assertEqual(value['utf8_hex'], 'E2 80 A6')
        self.assertNotIn('parents', value)
        self.assertFalse(any(v['form'] == '...' for v in self.manifest['values']))

    def test_literal_order_repetition_and_no_normalization(self):
        value = self.ucd.value('aa\u0301')
        self.assertEqual(value['form'], 'aa\u0301')
        self.assertEqual([p['ordinal'] for p in value['parents']], [0, 1, 2])
        self.assertEqual(value['parents'][0]['address'], value['parents'][1]['address'])
        ch = next(v for v in self.manifest['values'] if v['form'] == 'ch')
        self.assertEqual([p['address'] for p in ch['parents']], ['00.00.00.00.2b', '00.00.00.00.2g'])
        self.assertIsNone(ch['address'])

    def test_index_does_not_assign_native_alphabet_memberships(self):
        manifest, _ = generate(self.sources, Cldr(self.files), self.icu, self.ucd, ['tr'],
                                ['main', 'index'], self.hierarchy)
        for form in ('Q', 'W', 'X'):
            self.assertIn(key(form), [v['key'] for v in manifest['values']])
            self.assertNotIn('alphabet:tr', manifest['member_of'].get(key(form), []))

    def test_distinct_centroids_and_hierarchy_memberships(self):
        groups = {g['key']: g for g in self.manifest['groups']}
        self.assertNotEqual(groups['alphabet:en']['centroid'], groups['punctuation:en']['centroid'])
        self.assertEqual(self.manifest['member_of']['language:en_AU'], ['language:en'])
        self.assertEqual(self.manifest['member_of']['language:en'], ['family:germanic'])
        self.assertTrue(all('parents' not in g for g in groups.values()))

    def test_missing_locale_and_empty_main_report_gaps(self):
        cldr = fixture_cldr({'root': characters('[]'), 'x': ''})
        _, report = generate(self.sources, cldr, self.icu, self.ucd, ['x', 'missing'],
                             ['main'], self.hierarchy)
        self.assertEqual(report['counts']['coverage_gaps'], 2)
        self.assertIn('missing-locale-source', report['coverage'][0]['issues'])
        self.assertIn('empty-main-inventory', report['coverage'][1]['issues'])

    def test_reciprocal_and_identity_conflicts_rejected(self):
        bad = copy.deepcopy(self.manifest)
        bad['members']['alphabet:en'].pop()
        with self.assertRaisesRegex(ValueError, 'reciprocal'):
            validate_manifest(bad)
        bad = copy.deepcopy(self.manifest)
        next(v for v in bad['values'] if v['form'] == 'A')['address'] = byte_address(66)
        with self.assertRaisesRegex(ValueError, 'scalar identity'):
            validate_manifest(bad)

    def test_multiscalar_case_mapping_is_evidence_only(self):
        cldr = fixture_cldr({'root': characters('[]'), 'de': characters('[ß]')})
        manifest, _ = generate(self.sources, cldr, self.icu, self.ucd, ['de'], ['main'], self.hierarchy)
        ss = next(c for c in manifest['case_candidates'] if c['form'] == 'SS')
        self.assertFalse(ss['membership_candidate'])
        self.assertNotIn(key('SS'), {v['key'] for v in manifest['values']})


def export_fixture(definitions, mutation=None, compressed=False):
    tokens = [[identity, str(d['mass']), d.get('notation', '')] for identity, d in sorted(definitions.items())]
    parents = [[identity, str(i), parent, None] for identity, d in sorted(definitions.items())
               for i, parent in enumerate(d['parents'])]
    children = [list(pair) for pair in sorted({(parent, identity) for identity, d in definitions.items()
                                              for parent in d['parents']})]
    if mutation:
        mutation(tokens, parents, children)
    def arr(identity):
        return '{' + ','.join(identity.split('.')) + '}'
    lines = ['COPY public.token (notation, mass, token_text, token_id) FROM stdin;']
    for identity, mass, notation in tokens:
        lines.append('\t'.join([notation, mass if mass is not None else r'\N', identity, arr(identity)]))
    lines.extend([r'\.', 'COPY public.token_parent (token_id, ordinal, parent_token_id, mass) FROM stdin;'])
    for identity, ordinal, parent, mass in parents:
        lines.append('\t'.join([arr(identity), ordinal, arr(parent), r'\N' if mass is None else str(mass)]))
    lines.extend([r'\.', 'COPY public.token_child (token_id, child_token_id) FROM stdin;'])
    lines += ['\t'.join(map(arr, pair)) for pair in children]
    lines.append(r'\.')
    data = ('\n'.join(lines) + '\n').encode()
    return gzip.compress(data) if compressed else data


class CoreExportTests(unittest.TestCase):
    def setUp(self):
        self.manifest = {'values': [{'form': '\x00'}, {'form': 'A'}, {'form': '…'}]}
        self.expected = expected_definitions(self.manifest, all_floor_bytes=False)

    def run_validation(self, mutate=None, compressed=False):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'export'
            path.write_bytes(export_fixture(self.expected, mutate, compressed))
            return validate_export(path, self.manifest, all_floor_bytes=False)

    def test_complete_gzip_and_plain_exports_validate(self):
        for compressed in (True, False):
            report = self.run_validation(compressed=compressed)
            self.assertEqual(report['status'], 'valid')
            self.assertGreater(report['null_parent_mass_cells'], 0)

    def test_ascii_uses_nibbles_and_multibyte_uses_bytes(self):
        self.assertEqual(self.expected[character_address(0x41)]['parents'], [nibble_address(4), nibble_address(1)])
        self.assertEqual(self.expected[character_address(0x2026)]['parents'],
                         [byte_address(0xE2), byte_address(0x80), byte_address(0xA6)])
        self.assertEqual(self.expected[character_address(0x2026)]['mass'], 6)
        self.assertEqual(self.expected[character_address(0)]['parents'], [nibble_address(0)] * 2)

    def test_missing_endpoint_is_partial(self):
        def mutate(tokens, parents, children):
            tokens[:] = [row for row in tokens if row[0] != character_address(0x2026)]
            parents[:] = [row for row in parents if row[0] != character_address(0x2026)]
            children[:] = [row for row in children if row[1] != character_address(0x2026)]
        report = self.run_validation(mutate)
        self.assertEqual(report['status'], 'partial')
        self.assertEqual(report['missing_endpoints_or_dependencies'], [character_address(0x2026)])

    def test_incomplete_token_mass_is_invalid(self):
        def mutate(tokens, parents, children):
            next(row for row in tokens if row[0] == character_address(0x41))[1] = None
        report = self.run_validation(mutate)
        self.assertEqual(report['status'], 'invalid')
        self.assertIn('token-mass', [error['issue'] for error in report['errors']])

    def test_swapped_utf8_parents_rejected(self):
        def mutate(tokens, parents, children):
            rows = [row for row in parents if row[0] == character_address(0x2026)]
            rows[0][2], rows[1][2] = rows[1][2], rows[0][2]
        self.assertIn('ordered-parents', [e['issue'] for e in self.run_validation(mutate)['errors']])

    def test_missing_repeated_parent_occurrence_rejected(self):
        def mutate(tokens, parents, children):
            parents[:] = [row for row in parents if not (row[0] == character_address(0) and row[1] == '1')]
        self.assertIn('ordered-parents', [e['issue'] for e in self.run_validation(mutate)['errors']])

    def test_gap_or_duplicate_ordinal_rejected(self):
        for ordinal in ('0', '2'):
            def mutate(tokens, parents, children):
                next(row for row in parents if row[0] == character_address(0) and row[1] == '1')[1] = ordinal
            self.assertIn('ordered-parents', [e['issue'] for e in self.run_validation(mutate)['errors']])

    def test_zero_parent_mass_is_not_null(self):
        def mutate(tokens, parents, children):
            parents[0][3] = 0
        self.assertIn('parent-mass', [e['issue'] for e in self.run_validation(mutate)['errors']])

    def test_missing_reciprocal_rejected(self):
        def mutate(tokens, parents, children):
            children.pop()
        self.assertIn('missing-child-reciprocal', [e['issue'] for e in self.run_validation(mutate)['errors']])

    def test_unexpected_reciprocal_rejected(self):
        def mutate(tokens, parents, children):
            children.append([nibble_address(0), character_address(0x41)])
        self.assertIn('unexpected-child-reciprocal', [e['issue'] for e in self.run_validation(mutate)['errors']])

    def test_unrelated_address_depth_does_not_change_validation(self):
        def mutate(tokens, parents, children):
            tokens.append(['00.01.02', None, ''])
            tokens.append(['00.00.01.02.03.04', None, ''])
        self.assertEqual(self.run_validation(mutate)['status'], 'valid')

    def test_missing_copy_tables_or_truncation_rejected(self):
        for data in (b'version https://git-lfs.github.com/spec/v1\n',
                     export_fixture(self.expected).removesuffix(b'\\.\n')):
            with tempfile.TemporaryDirectory() as directory:
                path = Path(directory) / 'bad'
                path.write_bytes(data)
                with self.assertRaises(ValueError):
                    validate_export(path, self.manifest, all_floor_bytes=False)


if __name__ == '__main__':
    unittest.main()
