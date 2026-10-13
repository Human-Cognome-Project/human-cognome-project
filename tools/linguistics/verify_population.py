"""Isolated PostgreSQL acceptance for the letter bootstrap; never reset a DB.

Creates fresh, uniquely named fixture databases and preserves them for inspection.
Fixture character definitions come from the accepted UTF-8 export. Its closed
character dependency subset excludes unrelated encoding-table group memberships.
--full-restore additionally loads the complete snapshot into one new empty core.
"""
import argparse
import copy
import gzip
import json
import os
from pathlib import Path
import subprocess
import sys
import time
import uuid

from populate import (BootstrapSQL, CORE_TRIGGER_BODY, ROOT, TABLES, allocate,
                      digest, durable_json, insert_rows, populate, prepared,
                      recover, schema, snapshot)
from validate_core import expected_definitions, file_hash, read_export
from manifest import component_key, validate_manifest
from unicode_values import key

SNAPSHOT_SHA = '4408fd55e8884efa6a1b575b7e317aea17bdea922b09fab193cf9f7f65456dc8'


def create_databases(psql):
    suffix = uuid.uuid4().hex[:14]
    names = {'core': 'hcp_seed_core_' + suffix, 'linguistics': 'hcp_seed_ling_' + suffix}
    admin = BootstrapSQL(psql, 'postgres')
    try:
        for name in names.values():
            admin.sql('CREATE DATABASE ' + name + " ENCODING 'UTF8' TEMPLATE template0")
    finally:
        admin.close()
    return names


def initialize(psql, names, fixture, trigger=False):
    for store, name in names.items():
        db = BootstrapSQL(psql, name)
        try:
            create_schema(db)
            if store == 'core':
                for table in TABLES:
                    insert_rows(db, table, fixture[table])
                if trigger:
                    db.sql('CREATE FUNCTION public.token_parent_structural_mass() RETURNS trigger '
                           'LANGUAGE plpgsql AS $hcp$' + CORE_TRIGGER_BODY + '$hcp$; '
                           'CREATE TRIGGER token_parent_structural_mass AFTER INSERT ON public.token_parent '
                           'FOR EACH ROW EXECUTE FUNCTION public.token_parent_structural_mass()')
        finally:
            db.close()


def create_schema(db):
    # The reviewed schema's unqualified CREATEs belong in public, never pg_catalog.
    db.sql('SET search_path = public;\n' + (ROOT / 'kernels/database/schema/schema.sql').read_text()
           + '\nSET search_path = pg_catalog, public;')


def repeated_fixture(english):
    """Test-only sequence: two parent occurrences need one reverse child edge."""
    manifest = copy.deepcopy(english)
    source = copy.deepcopy(next(v for v in manifest['source_values'] if v['form'] == 'A'))
    source.update(key=key('AA'), form='AA', scalars=['U+0041', 'U+0041'], utf8_hex='41 41',
                  structural_mass_derived=4, scalar_metadata=source['scalar_metadata'] * 2,
                  kind='source-sequence', store=None, address=None,
                  parents=[{'ordinal': i, 'store': 'hcp_core', 'address': '00.00.00.00.23'} for i in range(2)])
    manifest['source_values'].append(source)
    value = {k: source[k] for k in ('form', 'scalars', 'utf8_hex', 'structural_mass_derived', 'scalar_metadata', 'parents')}
    identity = component_key('en', 'AA')
    value.update(key=identity, locale='en', source_value=source['key'],
                 kind='language-component-candidate', store='linguistics', address=None)
    manifest['values'].append(value)
    for group in ('alphabet:en', 'language:en'):
        manifest['memberships'].append({'member': identity, 'group': group, 'evidence': ['test-only-repeated-occurrence']})
        manifest['members'][group] = sorted(manifest['members'][group] + [identity])
    manifest['member_of'][identity] = ['alphabet:en', 'language:en']
    update = next(u for u in manifest['core_child_updates'] if u['address'] == '00.00.00.00.23')
    update['children'].append({'key': identity, 'store': 'linguistics', 'address': None})
    validate_manifest(manifest)
    return manifest


def state(psql, names):
    result = {}
    for store, name in names.items():
        db = BootstrapSQL(psql, name)
        try:
            result[store] = {'data': snapshot(db), 'schema': schema(db, store, allow_removed=True),
                             'prepared': sorted(prepared({store: db})[store])}
        finally:
            db.close()
    return result


def wait_closed(psql, names):
    # Abrupt child exit closes psql pipes; wait briefly for server rollback/EOF.
    db = BootstrapSQL(psql, 'postgres')
    try:
        for _ in range(100):
            if not db.query("SELECT pid FROM pg_stat_activity WHERE datname IN ('" + names['core']
                            + "','" + names['linguistics'] + "') AND backend_type='client backend'"):
                return
            time.sleep(0.1)
    finally:
        db.close()
    raise AssertionError('Crashed bootstrap connections remained active')


def acceptance(args):
    if file_hash(args.snapshot) != SNAPSHOT_SHA:
        raise ValueError('Acceptance fixtures require the reviewed current full UTF-8 export')
    full = json.loads(args.manifest.read_text())
    english = json.loads((ROOT / 'data/linguistics/previews/english-pilot/manifest.json').read_text())
    mapping = json.loads((ROOT / 'data/linguistics/bootstrap-address-map.json').read_text())
    args.work.mkdir(parents=True, exist_ok=True)
    cache = args.work / 'core-subset.json'
    expected = expected_definitions(full)
    if cache.exists():
        retained = json.loads(cache.read_text())
        if retained['export_sha256'] != SNAPSHOT_SHA or retained['expected_sha256'] != digest(expected):
            raise ValueError('Fixture cache differs from retained snapshot/input')
        fixture = retained['rows']
    else:
        tokens, parents, children = read_export(args.snapshot, expected)
        fixture = {t: [] for t in TABLES}
        fixture['token'] = [[a.split('.'), t['token_text'], t.get('notation'), int(t['mass'])]
                            for a, t in sorted(tokens.items())]
        fixture['token_parent'] = [[a.split('.'), o, p.split('.'), m]
                                   for a, occurrences in sorted(parents.items()) for o, p, m in occurrences]
        fixture['token_child'] = [[p.split('.'), c.split('.')] for p, c in children]
        durable_json(cache, {'export_sha256': SNAPSHOT_SHA, 'expected_sha256': digest(expected), 'rows': fixture})
    args.work = args.work / ('run-' + uuid.uuid4().hex[:14])
    args.work.mkdir()
    reports = []
    def run(manifest, names, label):
        ledger = allocate(manifest, mapping)
        journal = args.work / (label + '.journal.json')
        result = populate(manifest, mapping, ledger, args.psql, names['core'], names['linguistics'], journal)
        rerun = populate(manifest, mapping, ledger, args.psql, names['core'], names['linguistics'], journal)
        assert rerun['rows_added'] == 0
        stores = {s: BootstrapSQL(args.psql, n) for s, n in names.items()}
        try:
            assert not any(prepared(stores).values())
        finally:
            for db in stores.values():
                db.close()
        reports.append({'check': label, 'status': 'passed', 'result': result, 'rerun_rows_added': 0})
        print(label + ': passed', flush=True)
        return journal
    names = create_databases(args.psql)
    initialize(args.psql, names, fixture, trigger=True)
    run(english, names, 'english-sample-core-trigger-preserved')

    names = create_databases(args.psql)
    initialize(args.psql, names, fixture)
    run(full, names, 'twenty-language-subset')

    names = create_databases(args.psql)
    initialize(args.psql, names, fixture, trigger=True)
    repeated = repeated_fixture(english)
    run(repeated, names, 'repeated-ordered-core-parent')

    # Partial unrelated existing data fails before FK changes or seed inserts.
    names = create_databases(args.psql)
    initialize(args.psql, names, fixture)
    db = BootstrapSQL(args.psql, names['linguistics'])
    db.sql("INSERT INTO public.token VALUES (ARRAY['02','00','00','00','03'],'02.00.00.00.03','conflict',999)")
    db.close()
    before = state(args.psql, names)
    try:
        run(english, names, 'conflict')
        raise AssertionError('Conflicting batch accepted')
    except ValueError as error:
        assert 'Conflicting or partial' in str(error), error
        assert state(args.psql, names) == before
        reports.append({'check': 'pre-existing-conflict', 'status': 'passed', 'data_and_schema': 'unchanged'})
        print('pre-existing-conflict: passed', flush=True)

    # Unknown local-parent trigger in Linguistics is rejected, never disabled.
    names = create_databases(args.psql)
    initialize(args.psql, names, fixture)
    db = BootstrapSQL(args.psql, names['linguistics'])
    db.sql("CREATE FUNCTION public.wrong_mass() RETURNS trigger LANGUAGE plpgsql AS $$BEGIN RETURN NULL; END;$$; "
           "CREATE TRIGGER wrong_mass AFTER INSERT ON public.token_parent FOR EACH ROW EXECUTE FUNCTION public.wrong_mass()")
    data_before = snapshot(db)
    db.close()
    try:
        run(english, names, 'unexpected-trigger')
        raise AssertionError('Unexpected trigger accepted')
    except ValueError as error:
        assert 'Unexpected destination trigger' in str(error), error
        db = BootstrapSQL(args.psql, names['linguistics'])
        assert snapshot(db) == data_before
        assert db.query("SELECT tgenabled FROM pg_trigger WHERE tgname='wrong_mass'") == [{'tgenabled': 'O'}]
        db.close()
        reports.append({'check': 'unexpected-linguistic-trigger', 'status': 'passed', 'trigger': 'preserved enabled'})
        print('unexpected-linguistic-trigger: passed', flush=True)

    names = create_databases(args.psql)
    initialize(args.psql, names, fixture)
    db = BootstrapSQL(args.psql, names['core'])
    db.sql("UPDATE public.token SET mass=9 WHERE token_id=ARRAY['00','00','00','00','23']")
    db.close()
    before = state(args.psql, names)
    try:
        run(english, names, 'invalid-core')
        raise AssertionError('Invalid actual core mass accepted')
    except ValueError as error:
        assert 'mass/rendered address conflict' in str(error), error
        assert state(args.psql, names) == before
        reports.append({'check': 'invalid-core-mass', 'status': 'passed', 'data_and_schema': 'unchanged'})
        print('invalid-core-mass: passed', flush=True)

    names = create_databases(args.psql)
    initialize(args.psql, names, fixture)
    db = BootstrapSQL(args.psql, names['linguistics'])
    role = 'hcp_other_writer_' + uuid.uuid4().hex[:14]
    db.sql('CREATE ROLE ' + role + ' LOGIN; GRANT INSERT ON public.token TO ' + role)
    db.close()
    before = state(args.psql, names)
    try:
        run(english, names, 'other-writer')
        raise AssertionError('Another writable login role accepted')
    except ValueError as error:
        assert 'Other login roles can mutate' in str(error), error
        assert state(args.psql, names) == before
        reports.append({'check': 'other-writable-login-role', 'status': 'passed', 'data_and_schema': 'unchanged'})
        print('other-writable-login-role: passed', flush=True)

    if args.unsupported_psql:
        names = create_databases(args.unsupported_psql)
        initialize(args.unsupported_psql, names, fixture)
        before = state(args.unsupported_psql, names)
        try:
            populate(english, mapping, allocate(english, mapping), args.unsupported_psql,
                     names['core'], names['linguistics'], args.work / 'unsupported.journal.json')
            raise AssertionError('Unsupported prepared transaction configuration accepted')
        except ValueError as error:
            assert 'at least two prepared transactions' in str(error), error
            assert state(args.unsupported_psql, names) == before
            reports.append({'check': 'unsupported-two-phase-configuration', 'status': 'passed', 'data_and_schema': 'unchanged'})
            print('unsupported-two-phase-configuration: passed', flush=True)

    for point in ('core-prepared', 'both-prepared', 'commit-decided', 'core-committed', 'linguistics-committed'):
        names = create_databases(args.psql)
        initialize(args.psql, names, fixture, trigger=True)
        before = state(args.psql, names)
        label = 'crash-' + point
        request = args.work / (label + '.request.json')
        journal = args.work / (label + '.journal.json')
        durable_json(request, {'psql': args.psql, 'names': names, 'manifest': english,
                              'mapping': mapping, 'ledger': allocate(english, mapping),
                              'journal': str(journal), 'point': point})
        child = subprocess.run([sys.executable, str(Path(__file__).resolve()), '--worker', str(request)],
                               capture_output=True, text=True)
        assert child.returncode == 75, child.stderr
        wait_closed(args.psql, names)
        retained = json.loads(journal.read_text())
        # Wrong input must not act on even our own prepared work.
        try:
            populate(full, mapping, allocate(full, mapping), args.psql,
                     names['core'], names['linguistics'], journal)
            raise AssertionError('Recovery accepted a different batch')
        except ValueError as error:
            assert 'different inputs or target' in str(error), error
        if point in ('core-prepared', 'both-prepared'):
            stores = {s: BootstrapSQL(args.psql, n) for s, n in names.items()}
            try:
                recover(stores, retained, journal, english, allocate(english, mapping))
            finally:
                for db in stores.values():
                    db.close()
            assert state(args.psql, names) == before, 'DDL/data not rolled back together'
        # Commit-decision cases restart the ordinary writer directly, proving
        # recovery runs before its conflict/partial-batch preflight.
        run(english, names, label)
        assert json.loads(journal.read_text())['phase'] == 'complete'
        reports[-1]['decision_recovery'] = 'abort then same-batch retry' if point in (
            'core-prepared', 'both-prepared') else 'complete original commit decision'
        reports[-1]['wrong_input_recovery'] = 'rejected'

    names = create_databases(args.psql)
    initialize(args.psql, names, fixture, trigger=True)
    journal = args.work / 'automatic-abort-retry.journal.json'
    request = args.work / 'automatic-abort-retry.request.json'
    durable_json(request, {'psql': args.psql, 'names': names, 'manifest': english,
                          'mapping': mapping, 'ledger': allocate(english, mapping),
                          'journal': str(journal), 'point': 'core-prepared'})
    child = subprocess.run([sys.executable, str(Path(__file__).resolve()), '--worker', str(request)],
                           capture_output=True, text=True)
    assert child.returncode == 75, child.stderr
    wait_closed(args.psql, names)
    run(english, names, 'automatic-abort-retry')

    if args.full_restore:
        names = create_databases(args.psql)
        # Restore only into the explicitly NEW empty fixture core. No reset/drop.
        restore = subprocess.Popen([args.psql, '-Xq', '-v', 'ON_ERROR_STOP=1', '--dbname', names['core']],
                                   stdin=subprocess.PIPE, stdout=subprocess.DEVNULL)
        with gzip.open(args.snapshot, 'rb') as source:
            while block := source.read(1024 * 1024):
                restore.stdin.write(block)
        restore.stdin.close()
        assert restore.wait() == 0, 'New isolated core restore failed'
        db = BootstrapSQL(args.psql, names['linguistics'])
        create_schema(db)
        db.close()
        journal = run(full, names, 'full-retained-core-twenty-language-load')
        retained = json.loads(journal.read_text())
        reports[-1]['core_before_counts'] = {t: r['count'] for t, r in retained['before']['core'].items()}
        after = state(args.psql, names)
        reports[-1]['core_after_counts'] = {t: after['core']['data'][t]['count'] for t in TABLES}
        durable_json(args.work / 'full-targets.json', names)
    report = {'format': 'hcp-letter-population-acceptance-v1', 'snapshot_sha256': SNAPSHOT_SHA,
              'manifest_sha256': digest(full), 'address_map_sha256': digest(mapping),
              'postgres_major': 16, 'isolation': 'fresh owned fixture databases; no existing user store writes',
              'checks': reports}
    durable_json(args.out, report)


def main():
    if len(sys.argv) == 3 and sys.argv[1] == '--worker':
        request = json.loads(Path(sys.argv[2]).read_text())
        populate(request['manifest'], request['mapping'], request['ledger'], request['psql'],
                 request['names']['core'], request['names']['linguistics'], request['journal'], request['point'])
        raise AssertionError('Requested interruption did not execute')
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--psql', default='psql')
    parser.add_argument('--snapshot', required=True, type=Path)
    parser.add_argument('--manifest', required=True, type=Path)
    parser.add_argument('--work', required=True, type=Path)
    parser.add_argument('--out', required=True, type=Path)
    parser.add_argument('--full-restore', action='store_true')
    parser.add_argument('--unsupported-psql', help='Optional second private test cluster with max_prepared_transactions=0')
    acceptance(parser.parse_args())


if __name__ == '__main__':
    main()
