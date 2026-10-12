#!/usr/bin/env python3
"""Read-only coverage audit for the retained 2026-10-03 hcp_core COPY export."""
import argparse
import gzip
import hashlib
import json
from pathlib import Path
import re

ALPHABET = '0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz'
DIGITS = {c: i for i, c in enumerate(ALPHABET)}
EXPECTED = {'token': 1112215, 'token_parent': 4382976, 'token_child': 4333104,
            'members': 1112213, 'member_of': 1112213}
COPY = re.compile(r'^COPY public\.(\w+) \(([^)]+)\) FROM stdin;$')


def audit(path):
    counts = {name: 0 for name in EXPECTED}
    scalars = bytearray(0x110000)
    nibbles, bytes_seen, labels = bytearray(16), bytearray(256), bytearray(6)
    errors, error_count, virtual_count = [], 0, 0
    seen_tables, table, columns = set(), None, []
    def error(message):
        nonlocal error_count
        error_count += 1
        if len(errors) < 20:
            errors.append(message)
    with gzip.open(path, 'rt', encoding='utf-8') as source:
        for raw in source:
            line = raw.rstrip('\n')
            if table is None:
                match = COPY.fullmatch(line)
                if match:
                    table, fields = match.groups()
                    if table not in counts or table in seen_tables:
                        raise ValueError('Unexpected/repeated COPY table: ' + table)
                    columns = [c.strip() for c in fields.split(',')]
                    seen_tables.add(table)
                continue
            if line == r'\.':
                table = None
                continue
            counts[table] += 1
            if table != 'token':
                continue
            fields = line.split('\t')
            if len(fields) != len(columns):
                raise ValueError('Malformed token COPY row')
            row = dict(zip(columns, fields))
            parts = row['token_id'][1:-1].split(',')
            address = '.'.join(parts)
            if len(parts) != 5 or row['token_text'] != address:
                error('Bad token address/rendering: ' + address)
                continue
            codes = [DIGITS[p[0]] * 62 + DIGITS[p[1]] for p in parts]
            if codes[:3] == [0, 0, 1] and codes[3] == 0 and 0 <= codes[4] < 6:
                label = codes[4]
                if labels[label]:
                    error('Duplicate label: ' + address)
                labels[label] = 1
                continue  # Placeholder label masses/names are outside this audit.
            if codes[:3] != [0, 0, 0]:
                error('Unexpected token prefix: ' + address)
                continue
            fourth, last = codes[3:]
            if fourth == 0 and last < 16:
                if nibbles[last]:
                    error('Duplicate nibble: ' + address)
                nibbles[last] = 1
                notation, mass = f'{last:X}', 1
            elif fourth == 0 and last == 16:
                virtual_count += 1
                notation, mass = '0x', 0
            elif fourth == 0 and 62 <= last < 318:
                b = last - 62
                if bytes_seen[b]:
                    error('Duplicate byte: ' + address)
                bytes_seen[b] = 1
                if b < 128:
                    scalars[b] = 1
                notation, mass = f'{b:02X}', 2
            else:
                if 1 <= fourth < 3:
                    cp = 0x80 + (fourth - 1) * 3844 + last
                    valid = cp < 0x800
                elif 3 <= fourth < 62:
                    index = (fourth - 3) * 3844 + last
                    cp = 0x800 + index + (0x800 if index >= 0xD000 else 0)
                    valid = cp < 0x10000
                elif fourth >= 62:
                    cp = 0x10000 + (fourth - 62) * 3844 + last
                    valid = cp <= 0x10FFFF
                else:
                    valid = False
                if not valid:
                    error('Endpoint outside allocated scalar range: ' + address)
                    continue
                if scalars[cp]:
                    error('Duplicate scalar endpoint: ' + address)
                scalars[cp] = 1
                encoded = chr(cp).encode('utf-8')
                notation, mass = encoded.hex().upper(), 2 * len(encoded)
            if row['notation'] != notation or row['mass'] != str(mass):
                error('Incorrect UTF-8 notation or whole mass: ' + address)
    if table is not None or seen_tables != set(EXPECTED):
        error('Missing/truncated COPY table')
    if counts != EXPECTED:
        error('Table row totals differ from the retained snapshot manifest')
    missing = sum(not scalars[cp] for cp in range(0x110000) if not 0xD800 <= cp <= 0xDFFF)
    if missing or not all(nibbles) or not all(bytes_seen) or not all(labels) or virtual_count != 1:
        error('Incomplete scalar/floor/label coverage')
    digest = hashlib.sha256()
    with open(path, 'rb') as source:
        while block := source.read(1024 * 1024):
            digest.update(block)
    return {'status': 'valid' if error_count == 0 else 'invalid', 'export_sha256': digest.hexdigest(),
            'row_totals': counts, 'valid_scalar_endpoints': sum(scalars),
            'non_ascii_endpoints': sum(scalars[128:]), 'missing_valid_scalars': missing,
            'nibble_values': sum(nibbles), 'byte_values': sum(bytes_seen),
            'placeholder_labels': sum(labels), 'error_count': error_count, 'errors': errors,
            'scope': 'complete endpoint coverage, exact UTF-8 notation and whole mass; relationship row totals only',
            'database_writes': 0}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('export', type=Path)
    parser.add_argument('--out', type=Path, required=True)
    args = parser.parse_args()
    result = audit(args.export)
    args.out.write_text(json.dumps(result, sort_keys=True, indent=2) + '\n')
    print(json.dumps(result, sort_keys=True))
    raise SystemExit(0 if result['status'] == 'valid' else 2)
