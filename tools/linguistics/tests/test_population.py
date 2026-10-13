"""Allocation and mass checks for the approved bulk population batch."""
import copy
import json
from pathlib import Path
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from populate import ROOT, allocate, batch_rows, digest, durable_json, literal
from unicode_values import key
from validate_core import expected_definitions


class AllocationTests(unittest.TestCase):
    def setUp(self):
        self.manifest = json.loads((ROOT / 'data/linguistics/previews/english-pilot/manifest.json').read_text())
        self.mapping = json.loads((ROOT / 'data/linguistics/bootstrap-address-map.json').read_text())

    def test_initial_slots_and_reserved_frames(self):
        ledger = allocate(self.manifest, self.mapping)
        self.assertEqual(ledger['entries']['linguistics:en:0041'], '02.00.00.00.03')
        self.assertEqual(ledger['entries']['language:en'], '02.00.00.00.00')
        self.assertEqual(ledger['entries']['alphabet:en'], '02.00.00.00.01')
        self.assertEqual(ledger['entries']['punctuation:en'], '02.00.00.00.02')
        self.assertEqual(ledger['reserved_frames'], ['02.00.zz', '02.0z.zz'])
        self.assertEqual(len(ledger['entries']), 92)
        self.assertEqual(allocate(self.manifest, self.mapping, ledger), ledger)

    def test_existing_component_does_not_move_when_order_changes(self):
        ledger = allocate(self.manifest, self.mapping)
        # A valid retained allocation need not be recomputed from current sort order.
        entries = ledger['entries']
        entries['linguistics:en:0041'], entries['linguistics:en:0042'] = (
            entries['linguistics:en:0042'], entries['linguistics:en:0041'])
        self.assertEqual(allocate(self.manifest, self.mapping, ledger), ledger)

    def test_append_keeps_all_prior_addresses(self):
        full = allocate(self.manifest, self.mapping)
        previous = copy.deepcopy(full)
        removed = previous['entries'].pop('linguistics:en:0041')
        extended = allocate(self.manifest, self.mapping, previous)
        self.assertNotEqual(extended['entries']['linguistics:en:0041'], removed)
        self.assertEqual({k: extended['entries'][k] for k in previous['entries']}, previous['entries'])

    def test_colliding_or_foreign_ledger_rejected(self):
        ledger = allocate(self.manifest, self.mapping)
        ledger['entries']['linguistics:en:0041'] = ledger['entries']['language:en']
        with self.assertRaises(ValueError):
            allocate(self.manifest, self.mapping, ledger)
        ledger = allocate(self.manifest, self.mapping)
        ledger['map_sha256'] = 'unreviewed'
        with self.assertRaises(ValueError):
            allocate(self.manifest, self.mapping, ledger)

    def test_capacity_and_reserved_group_frame_rejected(self):
        ledger = allocate(self.manifest, self.mapping)
        ledger['entries']['linguistics:en:0041'] = '02.00.00.00.zz'
        ledger['entries'].pop('linguistics:en:0042')
        with self.assertRaises(ValueError):
            allocate(self.manifest, self.mapping, ledger)
        self.mapping['language_prefixes']['en'] = '02.00.zz'
        with self.assertRaises(ValueError):
            allocate(self.manifest, self.mapping)

    def test_repeated_parents_keep_mass_occurrences_and_null_cells(self):
        ledger = allocate(self.manifest, self.mapping)
        # Isolate the row materializer's ordered-occurrence convention.
        value = copy.deepcopy(self.manifest['values'][0])
        value.update(key='repeated', form='AA', structural_mass_derived=4,
                     parents=[{'ordinal': i, 'address': '00.00.00.00.23'} for i in range(2)])
        ledger['entries']['repeated'] = '02.00.00.00.zz'
        self.manifest.update(values=[value], groups=[], memberships=[],
                             core_child_updates=[{'address': '00.00.00.00.23', 'children': [{'key': 'repeated'}]}])
        rows, children = batch_rows(self.manifest, ledger, {'00.00.00.00.23': {'mass': 2}})
        self.assertEqual(rows['token'][0][-1], 4)
        self.assertEqual([r[1] for r in rows['token_parent']], [0, 1])
        self.assertEqual([r[-1] for r in rows['token_parent']], [None, None])
        self.assertEqual(len(children), 1)
        with self.assertRaises(ValueError):
            batch_rows(self.manifest, ledger, {'00.00.00.00.23': {'mass': 3}})

    def test_exact_notation_and_mass_policy(self):
        ledger = allocate(self.manifest, self.mapping)
        core = expected_definitions(self.manifest)
        rows, children = batch_rows(self.manifest, ledger, core)
        self.assertEqual(len(rows['token']), 92)
        self.assertEqual(len(rows['token_parent']), 84)
        self.assertEqual(len(children), 84)
        self.assertTrue(all(r[-1] is None for r in rows['token_parent']))
        ellipsis = next(r for r in rows['token'] if r[2] == '…')
        self.assertEqual(ellipsis[-1], 6)
        self.assertEqual(sum(r[0] == ellipsis[0] for r in rows['token_parent']), 1)

    def test_safe_literal_and_durable_json_preserve_exact_text(self):
        self.assertEqual(literal("'\\\n…"), "'''\\\n…'")
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'journal.json'
            data = {'notation': "'\\\n…"}
            durable_json(path, data)
            self.assertEqual(json.loads(path.read_text()), data)
            self.assertEqual(path.stat().st_mode & 0o777, 0o600)


if __name__ == '__main__':
    unittest.main()
