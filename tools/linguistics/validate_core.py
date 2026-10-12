"""Stream a plain/gzip pg_dump COPY export. Never restore or execute its SQL."""
from collections import defaultdict
import gzip
import hashlib
from pathlib import Path
import re
from unicode_values import character_address, byte_address, nibble_address

COPY = re.compile(r'^COPY (?:"?public"?\.)?"?(token|token_parent|token_child)"? \(([^)]+)\) FROM stdin;$')
ADDRESS = re.compile(r'\{(?:"?[0-9A-Za-z]{2}"?,)*"?[0-9A-Za-z]{2}"?\}')
COLUMNS = {'token': {'token_id', 'token_text', 'mass'},
           'token_parent': {'token_id', 'ordinal', 'parent_token_id', 'mass'},
           'token_child': {'token_id', 'child_token_id'}}


def address(value):
    if not ADDRESS.fullmatch(value):
        raise ValueError('Not an HCP address array in COPY export')
    return '.'.join(x.strip('"') for x in value[1:-1].split(','))


def cell(value):
    if value == r'\N':
        return None
    escapes = {'b': '\b', 'f': '\f', 'n': '\n', 'r': '\r', 't': '\t', 'v': '\v'}
    def replace(match):
        token = match[1]
        if token.startswith('x'):
            return chr(int(token[1:], 16))
        if token[0] in '01234567':
            return chr(int(token, 8))
        return escapes.get(token, token)
    return re.sub(r'\\(x[0-9A-Fa-f]{1,2}|[0-7]{1,3}|.)', replace, value)


def file_hash(path):
    digest = hashlib.sha256()
    with open(path, 'rb') as source:
        while block := source.read(1024 * 1024):
            digest.update(block)
    return digest.hexdigest()


def expected_definitions(manifest, all_floor_bytes=True):
    expected = {}
    def nibble(n):
        identity = nibble_address(n)
        expected[identity] = {'mass': 1, 'parents': [], 'kind': 'nibble'}
        return identity
    def byte(b):
        identity = byte_address(b)
        expected[identity] = {'mass': 2, 'parents': [nibble(b >> 4), nibble(b & 15)],
                              'kind': 'byte', 'notation': f'{b:02X}'}
        return identity
    if all_floor_bytes:
        for b in range(256):
            byte(b)
    for value in manifest.get('source_values', manifest['values']):
        for c in value['form']:
            cp = ord(c)
            identity = character_address(cp)
            if cp < 128:
                byte(cp)
            else:
                raw = c.encode('utf-8')
                expected[identity] = {'mass': 2 * len(raw), 'parents': [byte(b) for b in raw],
                                      'kind': 'unicode-endpoint'}
    return expected


def read_export(path, wanted):
    path = Path(path)
    with path.open('rb') as src:
        compressed = src.read(2) == b'\x1f\x8b'
    opener = gzip.open if compressed else open
    tokens, parents, children = {}, defaultdict(list), []
    seen, table, fields = set(), None, None
    with opener(path, 'rt', encoding='utf-8') as source:
        for lineno, raw in enumerate(source, 1):
            line = raw.rstrip('\n')
            if table is not None:
                if line == r'\.':
                    table = None
                    continue
                parts = line.split('\t')
                if len(parts) != len(fields):
                    raise ValueError(f'COPY field count mismatch at line {lineno}')
                row = dict(zip(fields, parts))
                identity = address(row['token_id'])
                if table == 'token' and identity in wanted:
                    if identity in tokens:
                        raise ValueError('Duplicate token in export: ' + identity)
                    tokens[identity] = {k: cell(v) for k, v in row.items()}
                elif table == 'token_parent' and identity in wanted:
                    mass = cell(row['mass'])
                    parents[identity].append((int(row['ordinal']), address(row['parent_token_id']),
                                              None if mass is None else int(mass)))
                elif table == 'token_child':
                    child = address(row['child_token_id'])
                    if child in wanted:
                        children.append((identity, child))
                continue
            match = COPY.fullmatch(line)
            if match:
                table, columns = match.groups()
                fields = [field.strip().strip('"') for field in columns.split(',')]
                if len(set(fields)) != len(fields) or not COLUMNS[table].issubset(fields):
                    raise ValueError('Missing/duplicate COPY columns for ' + table)
                if table in seen:
                    raise ValueError('Repeated COPY table: ' + table)
                seen.add(table)
    if table is not None:
        raise ValueError('Truncated COPY data for ' + table)
    if seen != set(COLUMNS):
        raise ValueError('Export must contain token, token_parent and token_child COPY sections')
    return tokens, parents, children


def validate_export(path, manifest, all_floor_bytes=True):
    expected = expected_definitions(manifest, all_floor_bytes)
    tokens, parents, children = read_export(path, expected)
    missing = sorted(expected.keys() - tokens.keys())
    errors, null_cells, valid = [], 0, set()
    childset = set(children)
    if len(childset) != len(children):
        errors.append({'issue': 'duplicate-child-reciprocal'})
    wanted_childset = {(parent, identity) for identity, definition in expected.items()
                       if identity in tokens for parent in definition['parents']}
    for parent, child in sorted(childset - wanted_childset):
        errors.append({'address': child, 'issue': 'unexpected-child-reciprocal', 'parent': parent})
    for identity, definition in sorted(expected.items()):
        if identity not in tokens:
            continue
        before = len(errors)
        token = tokens[identity]
        try:
            mass = int(token['mass']) if token['mass'] is not None else None
        except ValueError:
            mass = None
        if mass != definition['mass']:
            errors.append({'address': identity, 'issue': 'token-mass', 'actual': token['mass'],
                           'expected': definition['mass']})
        if token['token_text'] != identity:
            errors.append({'address': identity, 'issue': 'token-text'})
        if 'notation' in definition and token.get('notation') != definition['notation']:
            errors.append({'address': identity, 'issue': 'byte-notation'})
        stored = sorted(parents[identity], key=lambda row: (row[0], row[1]))
        wanted = list(enumerate(definition['parents']))
        if [(ordinal, parent) for ordinal, parent, _ in stored] != wanted:
            errors.append({'address': identity, 'issue': 'ordered-parents',
                           'actual': [[o, p] for o, p, _ in stored], 'expected': wanted})
        for ordinal, parent, parent_mass in stored:
            if parent not in tokens or tokens[parent]['mass'] != str(expected.get(parent, {}).get('mass')):
                errors.append({'address': identity, 'issue': 'unverified-constituent', 'parent': parent})
            if parent_mass is None:
                null_cells += 1  # NULL is a deferred cell; never treat it as zero.
            elif parent not in expected or parent_mass != expected[parent]['mass']:
                errors.append({'address': identity, 'issue': 'parent-mass', 'ordinal': ordinal})
        for parent in set(definition['parents']):
            if (parent, identity) not in childset:
                errors.append({'address': identity, 'issue': 'missing-child-reciprocal', 'parent': parent})
        if before == len(errors) and not any(child == identity for _, child in childset - wanted_childset):
            valid.add(identity)
    return {'status': 'invalid' if errors else 'partial' if missing else 'valid',
            'export_sha256': file_hash(path), 'expected_tokens': len(expected),
            'present_tokens': len(tokens), 'validated_tokens': len(valid),
            'validated_floor_bytes': sum(expected[i]['kind'] == 'byte' for i in valid),
            'missing_endpoints_or_dependencies': missing, 'errors': errors,
            'null_parent_mass_cells': null_cells,
            'null_parent_mass_policy': 'retained as deferred; verified constituent token.mass supplies expected mass',
            'scope': 'referenced endpoints and dependencies in supplied export, not a live database',
            'database_writes': 0}
