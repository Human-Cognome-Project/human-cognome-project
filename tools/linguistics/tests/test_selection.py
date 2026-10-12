import copy
import json
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from cldr import Cldr, ROLES
from icu import Icu
from manifest import generate
from selection import inventory_index, select_components
from sources import Sources, HERE
from unicode_values import UnicodeData, byte_address, key


class MembershipSelectionTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        sources = Sources()
        hierarchy = json.loads((HERE / 'bootstrap-groups.json').read_text())
        profile = json.loads((HERE / 'bootstrap-locales.json').read_text())
        cls.manifest, cls.coverage = generate(sources, Cldr(sources.cldr_files()), Icu(sources),
                                              UnicodeData(sources), profile['locales'], ROLES, hierarchy)

    def test_broader_language_coverage_has_no_source_gaps(self):
        self.assertEqual(len(self.coverage['coverage']), 20)
        self.assertTrue(all(not row['issues'] for row in self.coverage['coverage']))
        scripts = {s for row in self.coverage['coverage'] for s in row['observed_scripts']}
        self.assertTrue({'Latin', 'Greek', 'Cyrillic', 'Hebrew', 'Arabic', 'Armenian', 'Georgian'} <= scripts)

    def test_combined_languages_share_one_value_definition(self):
        bundle = select_components(self.manifest, ['language:en', 'language:fr'])
        self.assertEqual(sum(v['key'] == key('A') for v in bundle['values']), 1)
        self.assertTrue({'alphabet:en', 'alphabet:fr', 'language:en', 'language:fr'}
                        <= set(bundle['member_of'][key('A')]))

    def test_selecting_a_group_excludes_other_groups_from_active_memberships(self):
        bundle = select_components(self.manifest, ['punctuation:el'])
        self.assertEqual([g['key'] for g in bundle['groups']], ['punctuation:el'])
        self.assertTrue(all(e['group'] == 'punctuation:el' for e in bundle['memberships']))
        self.assertTrue(all(e['group'] != 'punctuation:el' for e in bundle['boundary_memberships']))
        self.assertTrue(all(i['locale'] == 'el' for i in bundle['inventory_refs']))
        self.assertIn(key('…'), {v['key'] for v in bundle['values']})

    def test_value_tree_keeps_character_byte_and_nibble_layers(self):
        bundle = select_components(self.manifest, ['punctuation:en'])
        definitions = {d['key']: d for d in bundle['construction_expectations']}
        ellipsis = definitions[key('…')]
        self.assertEqual(ellipsis['structural_mass_derived'], 6)
        self.assertEqual([p['value'] for p in ellipsis['parents']],
                         ['core:' + byte_address(b) for b in (0xE2, 0x80, 0xA6)])
        self.assertEqual([p['structural_count_derived'] for p in ellipsis['parents']], [2, 2, 2])
        e2 = definitions['core:' + byte_address(0xE2)]
        self.assertEqual([p['value'] for p in e2['parents']],
                         ['core:00.00.00.00.0E', 'core:00.00.00.00.02'])
        self.assertNotIn('core:' + byte_address(0xE2), bundle['members']['punctuation:en'])

    def test_composed_iconographic_form_keeps_exact_scalar_sequence(self):
        bundle = select_components(self.manifest, ['alphabet:nl'])
        value = next(v for v in bundle['values'] if v['form'] == 'íj́')
        self.assertEqual(value['scalars'], ['U+00ED', 'U+006A', 'U+0301'])
        self.assertEqual([p['ordinal'] for p in value['parents']], [0, 1, 2])
        self.assertIsNone(value['address'])

    def test_index_only_form_is_not_selected_as_alphabet_object(self):
        bundle = select_components(self.manifest, ['alphabet:cs'])
        self.assertIn(key('ch'), {v['key'] for v in bundle['values']})
        self.assertNotIn(key('CH'), {v['key'] for v in bundle['values']})

    def test_all_construction_references_are_closed_and_ordered(self):
        bundle = select_components(self.manifest, ['language:en', 'language:el', 'language:he'])
        definitions = {d['key']: d for d in bundle['construction_expectations']}
        for definition in definitions.values():
            self.assertEqual([p['ordinal'] for p in definition['parents']], list(range(len(definition['parents']))))
            for parent in definition['parents']:
                self.assertIn(parent['value'], definitions)
                self.assertEqual(parent['structural_count_derived'], definitions[parent['value']]['structural_mass_derived'])
        self.assertIn(key('A'), {v['key'] for v in bundle['values']})
        self.assertIn(key('Α'), {v['key'] for v in bundle['values']})  # Greek alpha remains distinct.
        self.assertIn(key('א'), {v['key'] for v in bundle['values']})

    def test_duplicate_or_reordered_selectors_produce_identical_bundle(self):
        first = select_components(self.manifest, ['language:fr', 'language:en', 'language:fr'])
        second = select_components(self.manifest, ['language:en', 'language:fr'])
        self.assertEqual(first, second)

    def test_exported_bundle_can_be_narrowed_without_losing_boundary_links(self):
        broad = select_components(self.manifest, ['language:en', 'language:el', 'language:he'])
        narrowed = select_components(broad, ['punctuation:en'])
        direct = select_components(self.manifest, ['punctuation:en'])
        for field in ('values', 'groups', 'memberships', 'members', 'member_of',
                      'boundary_memberships', 'construction_expectations', 'counts'):
            self.assertEqual(narrowed[field], direct[field])

    def test_invalid_boundary_cannot_add_an_active_membership(self):
        bundle = select_components(self.manifest, ['alphabet:en'])
        bad = copy.deepcopy(bundle)
        bad['boundary_memberships'].append({'member': key('A'), 'group': 'alphabet:en'})
        with self.assertRaisesRegex(ValueError, 'outside the input view'):
            select_components(bad, ['alphabet:en'])

    def test_missing_and_unpopulated_selectors_stay_visible(self):
        with self.assertRaisesRegex(ValueError, 'Unknown membership selector'):
            select_components(self.manifest, ['language:missing'])
        with self.assertRaisesRegex(ValueError, 'at least one'):
            select_components(self.manifest, [])
        empty = select_components(self.manifest, ['language:en_AU'])
        self.assertEqual(empty['status'], 'empty-selection')
        self.assertEqual(empty['counts']['values'], 0)

    def test_inventory_index_contains_only_applicable_group_members(self):
        index = inventory_index(self.manifest)
        self.assertEqual(len(index['inventories']), 20)
        available = {v['key'] for v in index['values']}
        for row in index['inventories']:
            self.assertEqual(row['alphabet'], self.manifest['members']['alphabet:' + row['locale']])
            self.assertEqual(row['punctuation'], self.manifest['members']['punctuation:' + row['locale']])
            self.assertTrue(set(row['alphabet'] + row['punctuation']) <= available)


if __name__ == '__main__':
    unittest.main()
