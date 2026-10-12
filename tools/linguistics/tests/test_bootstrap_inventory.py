"""Checks for the twenty-language bulk population preparation batch."""
import json
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from cldr import Cldr, ROLES
from icu import Icu
from manifest import generate
from sources import Sources, HERE
from unicode_values import UnicodeData, key


class BulkBootstrapInventoryTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        sources = Sources()
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
        self.assertTrue({'language:en', 'language:fr', 'alphabet:en', 'alphabet:fr'}
                        <= set(self.manifest['member_of'][key('A')]))
        self.assertEqual(len({key('A'), key('Α'), key('א')}), 3)
        self.assertTrue({key('A'), key('Α'), key('א')} <= values.keys())
        composed = values[key('íj́')]
        self.assertEqual(composed['scalars'], ['U+00ED', 'U+006A', 'U+0301'])
        self.assertEqual([p['ordinal'] for p in composed['parents']], [0, 1, 2])
        self.assertIn(key('ch'), self.manifest['members']['alphabet:cs'])
        self.assertNotIn(key('CH'), self.manifest['members']['alphabet:cs'])


if __name__ == '__main__':
    unittest.main()
