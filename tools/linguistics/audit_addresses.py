#!/usr/bin/env python3
"""Compare all scalars with the pinned PR #111 native pure functions, without a DB."""
import argparse
import json
from pathlib import Path
import subprocess
from manifest import write_json
from sources import Sources, HERE, DEFAULT_CACHE
from unicode_values import character_address


def audit(cache=DEFAULT_CACHE):
    oracle = Sources(Path(cache) / 'native-oracle')
    oracle.lock = json.loads((HERE / 'native-oracle.lock.json').read_text())
    oracle.fetch()
    files = {name: oracle.path(name) for name in oracle.lock['files']}
    binary = oracle.cache / 'address-dump'
    subprocess.run(['c++', '-std=c++17', '-O2', '-ffunction-sections', '-fdata-sections',
                    '-I' + str(oracle.cache / 'kernels/database/bootstrap'),
                    '-I' + str(oracle.cache / 'kernels/database/controller'),
                    '-I' + str(oracle.cache / 'kernels/database/codec'),
                    str(HERE / 'native_address_dump.cpp'),
                    str(files['kernels/database/bootstrap/encoding_populate.cpp']),
                    str(files['kernels/database/codec/codec.cpp']),
                    '-Wl,--gc-sections', '-o', str(binary)], check=True)
    # Controller declarations are required to compile the pinned translation
    # unit, but its uncalled population sections are discarded. No controller
    # implementation, libpq or database connection is linked into this binary.
    count, expected_cp, previous = 0, 0, ''
    mismatches = {'address_mismatches': 0, 'utf8_mismatches': 0, 'address_order_violations': 0}
    with subprocess.Popen([str(binary)], stdout=subprocess.PIPE, text=True) as process:
        for line in process.stdout:
            cp_text, address, encoded = line.rstrip('\n').split('\t')
            cp = int(cp_text)
            if cp != expected_cp:
                raise ValueError('Native scalar enumeration gap/repetition')
            expected_cp += 1
            if expected_cp == 0xD800:
                expected_cp = 0xE000
            mismatches['address_mismatches'] += address != character_address(cp)
            mismatches['utf8_mismatches'] += encoded != chr(cp).encode('utf-8').hex().upper()
            mismatches['address_order_violations'] += address <= previous
            previous = address
            count += 1
        if process.wait() or count != 1112064 or expected_cp != 0x110000:
            raise ValueError('Incomplete native scalar audit')
    return {'valid_scalars': count, **mismatches, 'native_source': oracle.lock,
            'database_rows_verified': False, 'database_writes': 0}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cache', type=Path, default=DEFAULT_CACHE)
    parser.add_argument('--out', type=Path, required=True)
    args = parser.parse_args()
    result = audit(args.cache)
    write_json(args.out, result)
    print(json.dumps({k: v for k, v in result.items() if k != 'native_source'}))
    raise SystemExit(1 if any(result[k] for k in ('address_mismatches', 'utf8_mismatches', 'address_order_violations')) else 0)
