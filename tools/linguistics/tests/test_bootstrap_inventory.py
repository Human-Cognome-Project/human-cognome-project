"""Checks for the twenty-language bulk population preparation batch."""
import copy
import json
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from cldr import Cldr, ROLES
from icu import Icu
from manifest import component_key, generate, validate_manifest
from sources import Sources, HERE
from unicode_values import UnicodeData, character_address, key
from test_inventory import fixture_cldr, characters


class BulkBootstrapInventoryTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        sources = Sources()
        cls.sources, cls.icu, cls.ucd = sources, Icu(sources), UnicodeData(sources)
        hierarchy = json.loads((HERE / 'bootstrap-groups.json').read_text())
        profile = json.loads((HERE / 'bootstrap-locales.json').read_text())
        cls.manifest, cls.coverage = generate(sources, Cldr(sources.cldr_files()), Icu(sources),
                                              UnicodeData(sources), profile['locales'], ROLES, hierarchy)

    def test_twenty_language_source_coverage(self):
        self.assertEqual(len(self.coverage['coverage']), 20)
        self.assertTrue(all(not row['issues'] for row in self.coverage['coverage']))
        scripts = {s for row in self.coverage['coverage'] for s in row['observed_scripts']}
        self.assertTrue({'Latin', 'Greek', 'Cyrillic', 'Hebrew', 'Arabic', 'Armenian', 'Georgian'} <= scripts)

    def test_population_batch_preserves_shared_and_distinct_values(self):
        values = {v['key']: v for v in self.manifest['values']}
        self.assertEqual(len(values), len(self.manifest['values']))
        for locale in ('en', 'fr'):
            self.assertEqual(set(self.manifest['member_of'][component_key(locale, 'A')]),
                             {'language:' + locale, 'alphabet:' + locale})
        self.assertEqual(len({key('A'), key('Α'), key('א')}), 3)
        self.assertTrue({component_key('en', 'A'), component_key('el', 'Α'), component_key('he', 'א')} <= values.keys())
        composed = values[component_key('nl', 'íj́')]
        self.assertEqual(composed['scalars'], ['U+00ED', 'U+006A', 'U+0301'])
        self.assertEqual([p['ordinal'] for p in composed['parents']], [0, 1, 2])
        self.assertIn(component_key('cs', 'ch'), self.manifest['members']['alphabet:cs'])
        self.assertNotIn(component_key('cs', 'CH'), values)

    def test_shared_core_endpoint_lists_each_language_letter_as_a_child(self):
        values = {v['key']: v for v in self.manifest['values']}
        en_a, fr_a = component_key('en', 'A'), component_key('fr', 'A')
        self.assertNotEqual(en_a, fr_a)
        self.assertEqual(values[en_a]['parents'], values[fr_a]['parents'])
        update = next(u for u in self.manifest['core_child_updates'] if u['address'] == character_address(0x41))
        self.assertTrue({en_a, fr_a} <= {c['key'] for c in update['children']})
        self.assertNotIn('alphabet:en', {c['key'] for c in update['children']})
        self.assertEqual(update['mode'], 'add')

    def test_missing_core_child_listing_is_rejected(self):
        bad = copy.deepcopy(self.manifest)
        update = next(u for u in bad['core_child_updates'] if u['address'] == character_address(0x41))
        update['children'].pop()
        with self.assertRaisesRegex(ValueError, 'Core child reciprocal mismatch'):
            validate_manifest(bad)

    def test_unrelated_or_unknown_child_listing_is_rejected(self):
        for identity in (component_key('en', 'B'), 'missing-endpoint'):
            bad = copy.deepcopy(self.manifest)
            update = next(u for u in bad['core_child_updates'] if u['address'] == character_address(0x41))
            update['children'].append({'key': identity, 'store': 'linguistics', 'address': None})
            with self.assertRaisesRegex(ValueError, 'reciprocal mismatch|unknown language endpoint'):
                validate_manifest(bad)

    def test_duplicate_child_listing_is_rejected(self):
        bad = copy.deepcopy(self.manifest)
        update = bad['core_child_updates'][0]
        update['children'].append(copy.deepcopy(update['children'][0]))
        with self.assertRaisesRegex(ValueError, 'Duplicate core child edge'):
            validate_manifest(bad)

    def test_replacement_of_existing_core_children_is_rejected(self):
        bad = copy.deepcopy(self.manifest)
        bad['core_child_updates'][0]['mode'] = 'replace'
        with self.assertRaisesRegex(ValueError, 'must be additive'):
            validate_manifest(bad)

    def test_scalar_language_letter_requires_its_core_parent(self):
        bad = copy.deepcopy(self.manifest)
        next(v for v in bad['values'] if v['key'] == component_key('en', 'A'))['parents'] = []
        with self.assertRaisesRegex(ValueError, 'core parent order/repetition mismatch'):
            validate_manifest(bad)

    def test_repeated_parents_keep_ordinals_and_one_child_listing(self):
        cldr = fixture_cldr({'root': characters('[]'), 'x': characters('[{aá}]')})
        manifest, _ = generate(self.sources, cldr, self.icu, self.ucd, ['x'], ['main'],
                               {'provenance': 'test source', 'groups': {}, 'memberships': []})
        value = manifest['values'][0]
        self.assertEqual(value['form'], 'aá')
        self.assertEqual([p['ordinal'] for p in value['parents']], [0, 1, 2])
        self.assertEqual([p['address'] for p in value['parents']],
                         [character_address(0x61), character_address(0x61), character_address(0x301)])
        self.assertEqual(len(manifest['core_child_updates']), 2)
        self.assertTrue(all(len(u['children']) == 1 for u in manifest['core_child_updates']))
        bad = copy.deepcopy(manifest)
        bad['values'][0]['parents'].pop(1)
        with self.assertRaisesRegex(ValueError, 'core parent order/repetition mismatch'):
            validate_manifest(bad)

    def test_core_identity_cannot_replace_a_language_endpoint_in_memberships(self):
        bad = copy.deepcopy(self.manifest)
        edge = next(e for e in bad['memberships'] if e['member'] == component_key('en', 'A'))
        edge['member'] = key('A')
        with self.assertRaisesRegex(ValueError, 'Unknown membership endpoint'):
            validate_manifest(bad)

    def test_incorrect_source_scalar_sequence_is_rejected(self):
        bad = copy.deepcopy(self.manifest)
        next(v for v in bad['source_values'] if v['form'] == 'A')['scalars'] = ['U+0042']
        with self.assertRaisesRegex(ValueError, 'Source scalar sequence conflicts'):
            validate_manifest(bad)


if __name__ == '__main__':
    unittest.main()
