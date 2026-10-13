"""Approved letter/punctuation bulk bootstrap only; no runtime DB interface.

Both stores stay offline for mutation except through this reviewed procedure.
The private journal must survive until read-back and every prepared transaction
finish. Restart this exact command to recover an interrupted batch.
"""
import argparse
from collections import defaultdict
import fcntl
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile
import uuid

from manifest import validate_manifest
from unicode_values import ALPHABET, pair
from validate_core import expected_definitions

ROOT = Path(__file__).resolve().parents[2]
TABLES = ('token', 'token_parent', 'token_child', 'members', 'member_of')
TARGET_FK = {'core': ('token_child', 'token_child_child_token_id_fkey'),
             'linguistics': ('token_parent', 'token_parent_parent_token_id_fkey')}
COLUMNS = {
    'token': [('token_id', 'text[]', True), ('token_text', 'text', False),
              ('notation', 'text', False), ('mass', 'integer', False)],
    'token_parent': [('token_id', 'text[]', True), ('ordinal', 'integer', True),
                     ('parent_token_id', 'text[]', True), ('mass', 'integer', False)],
    'token_child': [('token_id', 'text[]', True), ('child_token_id', 'text[]', True)],
    'members': [('token_id', 'text[]', True), ('member_token_id', 'text[]', True)],
    'member_of': [('token_id', 'text[]', True), ('group_token_id', 'text[]', True)],
}
# Exact authorized retained-core trigger body; never installed by this writer.
CORE_TRIGGER_BODY = """
DECLARE
    parent_mass integer;
BEGIN
    SELECT mass INTO parent_mass FROM token WHERE token_id = NEW.parent_token_id;
    IF parent_mass IS NULL THEN
        RAISE EXCEPTION
            'parent % has unknown (NULL) mass: not available for composition',
            NEW.parent_token_id;
    END IF;

    UPDATE token
    SET mass = (SELECT SUM(p.mass)::integer
                FROM token_parent tp
                JOIN token p ON p.token_id = tp.parent_token_id
                WHERE tp.token_id = NEW.token_id)
    WHERE token_id = NEW.token_id;
    RETURN NULL;
END;
"""


def canonical(value):
    return json.dumps(value, ensure_ascii=False, sort_keys=True, separators=(',', ':'))


def digest(value):
    return hashlib.sha256(canonical(value).encode('utf-8')).hexdigest()


def durable_json(path, value):
    """Same-filesystem replacement, file and directory fsync before proceeding."""
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    fd, name = tempfile.mkstemp(prefix=path.name + '.', dir=path.parent)
    try:
        with os.fdopen(fd, 'w', encoding='utf-8') as out:
            out.write(json.dumps(value, ensure_ascii=False, sort_keys=True, indent=2) + '\n')
            out.flush()
            os.fsync(out.fileno())
        os.replace(name, path)
        directory = os.open(path.parent, os.O_RDONLY | os.O_DIRECTORY)
        try:
            os.fsync(directory)
        finally:
            os.close(directory)
    finally:
        if os.path.exists(name):
            os.unlink(name)


def address(value, root='02', width=5):
    parts = value.split('.')
    if (len(parts) != width or parts[0] != root
            or any(len(p) != 2 or any(c not in ALPHABET for c in p) for p in parts)):
        raise ValueError('Invalid bootstrap address: ' + value)
    return parts


def allocate(manifest, mapping, previous=None):
    validate_manifest(manifest)
    if mapping['namespace'] != '02' or mapping['word_component_couplet'] != '00':
        raise ValueError('Expected accepted linguistic root/component frame')
    if mapping['reserved_component_slots'] != {'language': '00', 'alphabet': '01', 'punctuation': '02'}:
        raise ValueError('Unexpected grouping slot map')
    if mapping['first_component_slot'] != '03':
        raise ValueError('Unexpected initial component slot')
    prefixes = mapping['language_prefixes']
    if len(set(prefixes.values())) != len(prefixes):
        raise ValueError('Duplicate language frame')
    for prefix in prefixes.values():
        address(prefix, width=3)
    reserved = sorted({'.'.join(address(a)[:3]) for a in mapping['broader_group_addresses'].values()})
    if set(prefixes.values()) & set(reserved):
        raise ValueError('Language frame consumes reserved broader-group frame')
    ledger = {'format': 'hcp-letter-seed-ledger-v1', 'map_sha256': digest(mapping),
              'reserved_frames': reserved, 'entries': {}}
    if previous is not None:
        if any(previous.get(k) != ledger[k] for k in ('format', 'map_sha256', 'reserved_frames')):
            raise ValueError('Ledger does not match reviewed address map')
        ledger['entries'] = dict(previous['entries'])
    entries = ledger['entries']
    if len(set(entries.values())) != len(entries):
        raise ValueError('Ledger address collision')
    # Validate retained entries too, including entries absent from this batch.
    for key, rendered in entries.items():
        parts = address(rendered)
        if key in mapping['broader_group_addresses']:
            wanted = mapping['broader_group_addresses'][key]
        elif key.startswith('linguistics:'):
            locale = key.split(':')[1]
            wanted = prefixes[locale] + '.00.' + parts[-1]
            if parts[-1] < '03':
                raise ValueError('Ledger component consumes grouping slot')
        else:
            kind, locale = key.split(':')
            if kind == 'language':
                kind = 'language'
            wanted = prefixes[locale] + '.00.' + mapping['reserved_component_slots'][kind]
        if rendered != wanted:
            raise ValueError('Ledger entry changed its accepted frame/slot')
    def assign(key, rendered):
        if key in entries and entries[key] != rendered:
            raise ValueError('Cannot renumber a retained endpoint')
        if rendered in entries.values() and entries.get(key) != rendered:
            raise ValueError('Address already allocated')
        entries[key] = rendered
    for group in manifest['groups']:
        key = group['key']
        if key in mapping['broader_group_addresses']:
            rendered = mapping['broader_group_addresses'][key]
        else:
            kind, locale = key.split(':')
            rendered = prefixes[locale] + '.00.' + mapping['reserved_component_slots'][kind]
        assign(key, rendered)
    by_locale = defaultdict(list)
    for value in manifest['values']:
        by_locale[value['locale']].append(value)
    for locale, values in sorted(by_locale.items()):
        prefix = prefixes[locale] + '.00.'
        occupied = [ALPHABET.index(a[-2]) * 62 + ALPHABET.index(a[-1])
                    for a in entries.values() if a.startswith(prefix)]
        next_slot = max([2] + occupied) + 1
        values.sort(key=lambda v: ('alphabet:' + locale not in manifest['member_of'][v['key']],
                                   tuple(map(ord, v['form']))))
        for value in values:
            if value['key'] not in entries:
                assign(value['key'], prefix + pair(next_slot))
                next_slot += 1
    return ledger


def batch_rows(manifest, ledger, core_tokens):
    entries = ledger['entries']
    rows = {t: [] for t in TABLES}
    for group in manifest['groups']:
        a = entries[group['key']]
        rows['token'].append([address(a), a, group['label'], None])
    for value in manifest['values']:
        a = entries[value['key']]
        # Verified actual parent masses, including each repeated occurrence.
        mass = sum(core_tokens[p['address']]['mass'] for p in value['parents'])
        if mass != value['structural_mass_derived']:
            raise ValueError('Derived whole mass disagrees with seed evidence')
        rows['token'].append([address(a), a, value['form'], mass])
        for p in value['parents']:
            rows['token_parent'].append([address(a), p['ordinal'], address(p['address'], '00'), None])
    for edge in manifest['memberships']:
        m, g = address(entries[edge['member']]), address(entries[edge['group']])
        rows['members'].append([g, m])
        rows['member_of'].append([m, g])
    children = [[address(u['address'], '00'), address(entries[c['key']])]
                for u in manifest['core_child_updates'] for c in u['children']]
    for table in rows:
        rows[table].sort(key=canonical)
    return rows, sorted(children, key=canonical)


def literal(value):
    if value is None:
        return 'NULL'
    if isinstance(value, int):
        return str(value)
    if isinstance(value, list):
        return 'ARRAY[' + ','.join(literal(v) for v in value) + ']::text[]'
    return "'" + value.replace("'", "''") + "'"


class BootstrapSQL:
    """Private psql pipe for this bulk operation; not a runtime transport."""
    def __init__(self, psql, database):
        if not re.fullmatch(r'[a-zA-Z_][a-zA-Z0-9_]*', database):
            raise ValueError('Select an explicit database name, not a connection string')
        self.errors = tempfile.TemporaryFile(mode='w+t', encoding='utf-8')
        self.process = subprocess.Popen(
            [psql, '-X', '-qAt', '-v', 'ON_ERROR_STOP=1', '--dbname', database],
            stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=self.errors,
            text=True, encoding='utf-8')
        self.sql("SET standard_conforming_strings = on; SET client_encoding = 'UTF8'; "
                 "SET search_path = pg_catalog, public; SET statement_timeout = '180s';")

    def sql(self, sql):
        fence = 'hcp_' + uuid.uuid4().hex
        try:
            self.process.stdin.write(sql + ';\n\\echo ' + fence + '\n')
            self.process.stdin.flush()
            lines = []
            while True:
                line = self.process.stdout.readline()
                if not line:
                    raise ValueError('PostgreSQL rejected bootstrap SQL; no completion reported')
                if line.rstrip('\n') == fence:
                    return '\n'.join(lines)
                lines.append(line.rstrip('\n'))
        except (BrokenPipeError, OSError) as error:
            raise ValueError('Bootstrap SQL connection failed; restart with the retained journal') from error

    def query(self, sql):
        result = self.sql("SELECT COALESCE(jsonb_agg(to_jsonb(q)), '[]'::jsonb) FROM (" + sql + ') q')
        return json.loads(result)

    def close(self):
        self.process.stdin.close()
        self.process.wait(timeout=15)
        self.process.stdout.close()
        self.errors.close()


def store_identity(db):
    return db.query("SELECT current_database() AS database, current_user AS role, "
                    "(SELECT oid FROM pg_database WHERE datname=current_database()) AS oid, "
                    "(SELECT system_identifier::text FROM pg_control_system()) AS cluster")[0]


def schema(db, store, allow_removed=False):
    cols = db.query("SELECT c.relname AS t, a.attname AS n, format_type(a.atttypid,a.atttypmod) AS ty, "
                    "a.attnotnull AS nn, co.collname AS co, a.attnum AS pos, "
                    "a.atthasdef AS has_default, a.attgenerated AS generated, "
                    "c.relkind AS kind, c.relpersistence AS persistence, c.relrowsecurity AS rls "
                    "FROM pg_class c JOIN pg_namespace ns ON ns.oid=c.relnamespace "
                    "JOIN pg_attribute a ON a.attrelid=c.oid "
                    "LEFT JOIN pg_collation co ON co.oid=a.attcollation "
                    "WHERE ns.nspname='public' AND c.relname IN (" + ','.join(map(literal, TABLES)) + ') '
                    "AND a.attnum>0 AND NOT a.attisdropped ORDER BY c.relname,a.attnum")
    actual = defaultdict(list)
    for c in cols:
        actual[c['t']].append((c['n'], c['ty'], c['nn']))
        if (c['has_default'] or c['generated'] or c['kind'] != 'r' or c['persistence'] != 'p'
                or c['rls'] or c['ty'] == 'text[]' and c['co'] != 'C'):
            raise ValueError('Unexpected destination column/table configuration')
    if dict(actual) != COLUMNS:
        raise ValueError('Destination columns differ from reviewed five-table schema')
    constraints = db.query("SELECT c.relname AS t, k.conname AS n, k.contype AS ty, "
                           "k.convalidated AS valid, k.condeferrable AS deferred, "
                           "pg_get_constraintdef(k.oid) AS def FROM pg_constraint k "
                           "JOIN pg_class c ON c.oid=k.conrelid JOIN pg_namespace ns ON ns.oid=c.relnamespace "
                           "WHERE ns.nspname='public' AND c.relname IN (" + ','.join(map(literal, TABLES)) + ') '
                           'ORDER BY c.relname,k.conname')
    expected = {}
    for t, definitions in COLUMNS.items():
        pk = ('token_id', 'ordinal') if t == 'token_parent' else tuple(c[0] for c in definitions[:2])
        if t == 'token':
            pk = ('token_id',)
            expected[t, 'token_id_nonempty'] = 'CHECK ((cardinality(token_id) > 0))'
        expected[t, t + '_pkey'] = 'PRIMARY KEY (' + ', '.join(pk) + ')'
        if t != 'token':
            for name, ty, _ in definitions:
                if ty == 'text[]':
                    expected[t, t + '_' + name + '_fkey'] = 'FOREIGN KEY (' + name + ') REFERENCES token(token_id)'
    found = {(r['t'], r['n']): r['def'].replace('public.token(', 'token(') for r in constraints}
    destination = TARGET_FK[store]
    if destination not in found and allow_removed:
        expected.pop(destination)
    if found != expected or any(not r['valid'] or r['deferred'] for r in constraints):
        raise ValueError('Unexpected destination constraints, including removed foreign key without journal')
    indexes = db.query("SELECT c.relname AS t, i.indisprimary AS pk, i.indisvalid AS valid "
                       "FROM pg_index i JOIN pg_class c ON c.oid=i.indrelid "
                       "JOIN pg_namespace ns ON ns.oid=c.relnamespace WHERE ns.nspname='public' "
                       "AND c.relname IN (" + ','.join(map(literal, TABLES)) + ') ORDER BY c.relname')
    if len(indexes) != 5 or any(not i['pk'] or not i['valid'] for i in indexes):
        raise ValueError('Expected only the five direct-address primary indexes')
    triggers = db.query("SELECT c.relname AS t, tg.tgname AS n, tg.tgtype AS ty, tg.tgenabled AS enabled, "
                        "tg.tgnargs AS args, tg.tgqual IS NOT NULL AS qual, "
                        "p.proname AS function, p.prosrc AS body, p.prosecdef AS security_definer, "
                        "p.proconfig AS config, l.lanname AS language FROM pg_trigger tg "
                        "JOIN pg_class c ON c.oid=tg.tgrelid JOIN pg_namespace ns ON ns.oid=c.relnamespace "
                        "JOIN pg_proc p ON p.oid=tg.tgfoid JOIN pg_language l ON l.oid=p.prolang "
                        "WHERE ns.nspname='public' AND NOT tg.tgisinternal "
                        "AND c.relname IN (" + ','.join(map(literal, TABLES)) + ') ORDER BY c.relname,tg.tgname')
    if triggers:
        t = triggers[0]
        if (store != 'core' or len(triggers) != 1 or t['t'] != 'token_parent'
                or t['n'] != 'token_parent_structural_mass' or t['ty'] != 5 or t['enabled'] != 'O'
                or t['args'] or t['qual'] or t['function'] != 'token_parent_structural_mass'
                or t['body'].strip() != CORE_TRIGGER_BODY.strip() or t['security_definer']
                or t['config'] or t['language'] != 'plpgsql'):
            raise ValueError('Unexpected destination trigger; never disable or clone it')
    rules = db.query("SELECT r.rulename FROM pg_rewrite r JOIN pg_class c ON c.oid=r.ev_class "
                     "JOIN pg_namespace ns ON ns.oid=c.relnamespace WHERE ns.nspname='public' "
                     "AND c.relname IN (" + ','.join(map(literal, TABLES)) + ')')
    if rules:
        raise ValueError('Unexpected destination write rules')
    return {'columns': cols, 'constraints': constraints, 'triggers': triggers,
            'indexes': indexes, 'destination_fk_present': destination in found}


def verify_core(db, manifest):
    expected = expected_definitions(manifest)
    keys = [address(a, '00') for a in sorted(expected)]
    wanted = '(VALUES ' + ','.join('(' + literal(a) + ')' for a in keys) + ') AS wanted(token_id)'
    # VALUES drives direct PK probes even on a freshly restored, unanalyzed core.
    tokens = db.query('SELECT t.* FROM ' + wanted
                      + ' JOIN public.token t USING (token_id) ORDER BY t.token_id')
    indexed = {'.'.join(t['token_id']): t for t in tokens}
    parents = db.query('SELECT t.* FROM ' + wanted
                       + ' JOIN public.token_parent t USING (token_id) ORDER BY t.token_id,t.ordinal')
    by_owner = defaultdict(list)
    for p in parents:
        by_owner['.'.join(p['token_id'])].append(p)
    # Follow owner lists, restricting output to the closed verified dependency set.
    # Byte lists in the full snapshot also contain millions of other characters.
    # Two small addressed sets avoid a many-thousand-way OR per retained row.
    children = db.query('SELECT t.* FROM ' + wanted + ' JOIN public.token_child t USING (token_id) '
                        'JOIN ' + wanted.replace('wanted(token_id)', 'children(child_token_id)')
                        + ' USING (child_token_id)')
    present = {('.'.join(p['token_id']), '.'.join(p['child_token_id'])) for p in children}
    if set(indexed) != set(expected):
        raise ValueError('Missing verified core endpoint/dependency')
    wanted_children = {(p, a) for a, definition in expected.items() for p in definition['parents']}
    if present != wanted_children:
        raise ValueError('Core closed-dependency child reciprocals conflict')
    for a, definition in expected.items():
        token, stored = indexed[a], by_owner[a]
        if token['mass'] != definition['mass'] or token['token_text'] != a:
            raise ValueError('Core endpoint mass/rendered address conflict: ' + a)
        if 'notation' in definition and token['notation'] != definition['notation']:
            raise ValueError('Core byte notation conflict: ' + a)
        if [(p['ordinal'], '.'.join(p['parent_token_id'])) for p in stored] != list(enumerate(definition['parents'])):
            raise ValueError('Core ordered parent conflict: ' + a)
        for p in stored:
            parent = '.'.join(p['parent_token_id'])
            if (parent, a) not in present or p['mass'] not in (None, indexed[parent]['mass']):
                raise ValueError('Core parent mass/child reciprocal conflict: ' + a)
    return indexed


def snapshot(db, exclude_children=None):
    """Counts + order-independent 128-bit row digest sums, with no huge output."""
    result = {}
    for t in TABLES:
        where = ''
        if t == 'token_child' and exclude_children:
            values = ','.join('(' + literal(a) + ')' for a in exclude_children)
            where = ' WHERE NOT EXISTS (SELECT 1 FROM (VALUES ' + values + ') AS seed(child_id) '
            where += 'WHERE seed.child_id=r.child_token_id)'
        result[t] = db.query("SELECT count(*) AS count, "
                             "COALESCE(sum(('x'||substr(md5(r::text),1,16))::bit(64)::bigint),0)::text AS h1, "
                             "COALESCE(sum(('x'||substr(md5(r::text),17,16))::bit(64)::bigint),0)::text AS h2 "
                             'FROM public.' + t + ' r' + where)[0]
    return result


def read_seed(db, addresses):
    keys = ','.join(literal(a) for a in addresses)
    result = {}
    for t in TABLES:
        # Detect unrelated incoming links to occupied seed addresses as conflicts.
        target = {'token_parent': 'parent_token_id', 'token_child': 'child_token_id',
                  'members': 'member_token_id', 'member_of': 'group_token_id'}.get(t)
        where = 'token_id IN (' + keys + ')'
        if target:
            where += ' OR ' + target + ' IN (' + keys + ')'
        records = db.query('SELECT * FROM public.' + t + ' WHERE ' + where)
        result[t] = sorted([[r[n] for n, _, _ in COLUMNS[t]] for r in records], key=canonical)
    return result


def read_core_children(db, addresses):
    return sorted([[r['token_id'], r['child_token_id']] for r in db.query(
        'SELECT t.* FROM (VALUES ' + ','.join('(' + literal(a) + ')' for a in addresses)
        + ') AS seed(child_token_id) JOIN public.token_child t USING (child_token_id)')], key=canonical)


def insert_rows(db, table, rows):
    # Bounded bulk statements; exact notation (quotes/newlines/backslashes) kept.
    for start in range(0, len(rows), 400):
        block = rows[start:start + 400]
        db.sql('INSERT INTO public.' + table + ' VALUES '
               + ','.join('(' + ','.join(map(literal, row)) + ')' for row in block))


def lock_tables(db):
    db.sql("BEGIN; SET LOCAL lock_timeout = '10s'; LOCK TABLE "
           + ','.join('public.' + t for t in TABLES) + ' IN ACCESS EXCLUSIVE MODE')


def configuration(stores):
    pids = [db.query('SELECT pg_backend_pid() AS pid')[0]['pid'] for db in stores.values()]
    identities = {s: store_identity(db) for s, db in stores.items()}
    if identities['core']['cluster'] != identities['linguistics']['cluster']:
        # This bounded offline bootstrap validates one private cluster only.
        raise ValueError('This bootstrap requires two explicit databases in the same PostgreSQL cluster')
    if identities['core']['oid'] == identities['linguistics']['oid']:
        raise ValueError('Core and Linguistics must be distinct databases')
    for db in stores.values():
        settings = db.query("SELECT current_setting('max_prepared_transactions')::int AS prepared, "
                            "current_setting('server_version_num')::int AS version")[0]
        if settings['prepared'] < 2 or not 160000 <= settings['version'] < 170000:
            raise ValueError('Require PostgreSQL 16 with at least two prepared transactions')
        other = db.query("SELECT r.rolname FROM pg_roles r WHERE r.rolcanlogin AND r.rolname<>current_user "
                         "AND EXISTS (SELECT 1 FROM pg_class c JOIN pg_namespace ns ON ns.oid=c.relnamespace "
                         "WHERE ns.nspname='public' AND c.relname IN (" + ','.join(map(literal, TABLES)) + ') '
                         "AND has_table_privilege(r.oid,c.oid,'INSERT,UPDATE,DELETE,TRUNCATE,TRIGGER'))")
        if other:
            raise ValueError('Other login roles can mutate selected stores; restrict to bootstrap owner first')
        owners = db.query("SELECT c.relname FROM pg_class c JOIN pg_namespace ns ON ns.oid=c.relnamespace "
                          "WHERE ns.nspname='public' AND c.relname IN (" + ','.join(map(literal, TABLES)) + ') '
                          "AND NOT pg_has_role(c.relowner,'USAGE') AND NOT "
                          "(SELECT rolsuper FROM pg_roles WHERE rolname=current_user)")
        if owners:
            raise ValueError('Bootstrap role must own the selected tables')
        others = db.query("SELECT pid FROM pg_stat_activity WHERE datname=current_database() "
                          "AND backend_type='client backend' AND pid NOT IN (" + ','.join(map(str, pids)) + ')')
        if others:
            raise ValueError('Selected store has another client; take exclusive bootstrap access first')
    if not stores['core'].query('SELECT pg_try_advisory_lock(132, 2) AS acquired')[0]['acquired']:
        raise ValueError('Another linguistic bootstrap holds this core store')
    return identities


def validate_readback(stores, manifest, ledger, journal):
    tokens = verify_core(stores['core'], manifest)
    rows, children = batch_rows(manifest, ledger, tokens)
    addresses = [r[0] for r in rows['token']]
    if read_seed(stores['linguistics'], addresses) != rows:
        raise ValueError('Linguistic notation/mass/parent/membership read-back mismatch')
    if read_core_children(stores['core'], addresses) != children:
        raise ValueError('Core reverse child read-back mismatch')
    if snapshot(stores['core'], addresses) != journal['core_preserved']:
        raise ValueError('Pre-existing core definitions or child entries changed')
    for s, db in stores.items():
        actual = schema(db, s, allow_removed=True)
        before = journal['schemas'][s]
        expected = dict(before)
        expected['destination_fk_present'] = False
        expected['constraints'] = [c for c in before['constraints'] if (c['t'], c['n']) != TARGET_FK[s]]
        if actual != expected:
            raise ValueError('Unexpected schema/trigger change during population')
    return {'tokens': len(rows['token']), 'language_components': len(manifest['values']),
            'groups': len(manifest['groups']), 'ordered_parents': len(rows['token_parent']),
            'core_child_additions': len(children), 'members': len(rows['members']),
            'member_of': len(rows['member_of']), 'readback': 'exact',
            'parent_mass_cells': 'NULL', 'group_mass': 'NULL',
            'whole_mass': 'sum of verified core masses per ordered occurrence',
            'pre_existing_core': 'unchanged'}


def prepared(stores):
    return {s: {r['gid'] for r in db.query("SELECT gid FROM pg_prepared_xacts WHERE database=current_database()")}
            for s, db in stores.items()}


def recover(stores, journal, path, manifest, ledger):
    pending = prepared(stores)
    for s, gids in pending.items():
        if gids - {journal['gids'][s]}:
            raise ValueError('Unrelated prepared transaction in selected store; leave it untouched')
    decision = journal.get('decision')
    if decision not in (None, 'abort', 'commit'):
        raise ValueError('Invalid durable batch decision')
    command = 'COMMIT' if decision == 'commit' else 'ROLLBACK'
    if decision != 'commit':
        journal['decision'] = 'abort'
        durable_json(path, journal)
    for s, db in stores.items():
        if journal['gids'][s] in pending[s]:
            db.sql(command + ' PREPARED ' + literal(journal['gids'][s]))
    for db in stores.values():
        lock_tables(db)
    try:
        if decision == 'commit':
            journal['result'] = validate_readback(stores, manifest, ledger, journal)
            journal['phase'] = 'complete'
        else:
            for s, db in stores.items():
                if schema(db, s, allow_removed=True) != journal['schemas'][s]:
                    raise ValueError('Abort recovery did not restore pre-batch schema')
                if snapshot(db) != journal['before'][s]:
                    raise ValueError('Abort recovery did not restore pre-batch data')
            journal['phase'] = 'aborted'
        durable_json(path, journal)
    finally:
        for db in stores.values():
            db.sql('ROLLBACK')


def populate(manifest, mapping, ledger, psql, core_db, linguistics_db, journal_path, interrupt=None):
    # Verify inputs even on recovery; never accept unrelated input or target.
    if allocate(manifest, mapping, ledger) != ledger:
        raise ValueError('Ledger must be allocated and retained before the database load')
    inputs = {'manifest': digest(manifest), 'map': digest(mapping), 'ledger': digest(ledger)}
    path = Path(journal_path)
    path.parent.mkdir(parents=True, exist_ok=True)
    with open(str(path) + '.lock', 'a') as guard:
        fcntl.flock(guard, fcntl.LOCK_EX | fcntl.LOCK_NB)
        stores = {}
        try:
            stores['core'] = BootstrapSQL(psql, core_db)
            stores['linguistics'] = BootstrapSQL(psql, linguistics_db)
            identities = configuration(stores)
            previous = json.loads(path.read_text()) if path.exists() else None
            if previous:
                if (previous.get('format') != 'hcp-letter-seed-batch-v1'
                        or previous['inputs'] != inputs or previous['stores'] != identities):
                    raise ValueError('Retained batch belongs to different inputs or target stores')
                if previous['phase'] not in ('complete', 'aborted'):
                    recover(stores, previous, path, manifest, ledger)
            if any(prepared(stores).values()):
                raise ValueError('Unresolved prepared transaction; use its original batch journal')
            for db in stores.values():
                lock_tables(db)
            schemas = {s: schema(db, s, allow_removed=bool(previous and previous['phase'] == 'complete'))
                       for s, db in stores.items()}
            core = verify_core(stores['core'], manifest)
            rows, children = batch_rows(manifest, ledger, core)
            addresses = [r[0] for r in rows['token']]
            existing = read_seed(stores['linguistics'], addresses)
            existing_children = read_core_children(stores['core'], addresses)
            if existing == rows and existing_children == children:
                if not previous or previous['phase'] != 'complete':
                    raise ValueError('Existing batch lacks its completed durable identity')
                result = validate_readback(stores, manifest, ledger, previous)
                result['rows_added'] = 0
                return result
            if previous and previous['phase'] == 'complete' or any(existing.values()) or existing_children:
                raise ValueError('Conflicting or partial pre-existing seed batch; no writes')
            before = {s: snapshot(db) for s, db in stores.items()}
            journal = {'format': 'hcp-letter-seed-batch-v1', 'batch_id': digest(inputs),
                       'inputs': inputs, 'stores': identities, 'phase': 'intent', 'decision': None,
                       'schemas': schemas, 'before': before,
                       'core_preserved': before['core'],
                       'payload_sha256': digest({'rows': rows, 'core_children': children}),
                       'gids': {s: 'hcp_letters_' + uuid.uuid4().hex + '_' + s for s in stores}}
            durable_json(path, journal)
            for s, db in stores.items():
                table, fk = TARGET_FK[s]
                if schemas[s]['destination_fk_present']:
                    db.sql('ALTER TABLE public.' + table + ' DROP CONSTRAINT ' + fk)
            for t in TABLES:
                insert_rows(stores['linguistics'], t, rows[t])
            insert_rows(stores['core'], 'token_child', children)
            result = validate_readback(stores, manifest, ledger, journal)
            def checkpoint(point):
                if interrupt == point:
                    os._exit(75)  # Deliberate test crash, no finally/rollback.
            for s, db in stores.items():
                db.sql('PREPARE TRANSACTION ' + literal(journal['gids'][s]))
                journal['phase'] = s + '-prepared'
                durable_json(path, journal)
                checkpoint(journal['phase'])
            checkpoint('both-prepared')
            journal.update(decision='commit', phase='commit-decided')
            durable_json(path, journal)
            checkpoint('commit-decided')
            for s, db in stores.items():
                db.sql('COMMIT PREPARED ' + literal(journal['gids'][s]))
                journal['phase'] = s + '-committed'
                durable_json(path, journal)
                checkpoint(journal['phase'])
            for db in stores.values():
                lock_tables(db)
            result = validate_readback(stores, manifest, ledger, journal)
            result['rows_added'] = sum(len(r) for r in rows.values()) + len(children)
            journal.update(phase='complete', result=result)
            durable_json(path, journal)
            return result
        finally:
            # EOF rolls back only unprepared work; prepared work belongs to journal.
            for db in reversed(list(stores.values())):
                db.close()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('operation', choices=('allocate', 'load'))
    parser.add_argument('--manifest', required=True, type=Path)
    parser.add_argument('--map', type=Path, default=ROOT / 'data/linguistics/bootstrap-address-map.json')
    parser.add_argument('--ledger', required=True, type=Path)
    parser.add_argument('--psql', default='psql')
    parser.add_argument('--core-db')
    parser.add_argument('--linguistics-db')
    parser.add_argument('--journal', type=Path)
    parser.add_argument('--out', type=Path)
    parser.add_argument('--exclusive-bootstrap', action='store_true',
                        help='Confirm these explicit stores remain offline for all other mutation paths')
    args = parser.parse_args()
    manifest, mapping = json.loads(args.manifest.read_text()), json.loads(args.map.read_text())
    previous = json.loads(args.ledger.read_text()) if args.ledger.exists() else None
    ledger = allocate(manifest, mapping, previous)
    if args.operation == 'allocate':
        durable_json(args.ledger, ledger)
        print(canonical({'allocated_entries': len(ledger['entries']), 'ledger_sha256': digest(ledger)}))
        return
    if not args.exclusive_bootstrap or not all((args.core_db, args.linguistics_db, args.journal, previous)):
        parser.error('load requires retained ledger, both explicit databases, journal and --exclusive-bootstrap')
    if ledger != previous:
        parser.error('allocate and retain updated ledger before loading')
    result = populate(manifest, mapping, ledger, args.psql, args.core_db, args.linguistics_db, args.journal)
    if args.out:
        durable_json(args.out, result)
    print(canonical(result))


if __name__ == '__main__':
    main()
