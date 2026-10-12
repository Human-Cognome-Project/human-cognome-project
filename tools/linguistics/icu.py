"""Batch bridge to pinned ICU. System/Python Unicode versions are not used."""
import base64
import subprocess
from sources import HERE


def encoded(text):
    return base64.b64encode(text.encode('utf-8')).decode('ascii')


class Icu:
    def __init__(self, sources):
        self.jar = sources.path('icu4j.jar')
        self.expected_icu = sources.lock['icu_version']
        self.expected_unicode = sources.lock['unicode_version']

    def batch(self, requests):
        if not requests:
            return []
        data = ''.join('\t'.join([op, *map(encoded, args)]) + '\n' for op, *args in requests)
        run = subprocess.run(['java', '--class-path', str(self.jar), str(HERE / 'IcuBridge.java')],
                             input=data, text=True, capture_output=True, check=True)
        lines = run.stdout.splitlines()
        header = lines[0].split('\t')
        if header != ['VERSION', self.expected_icu + '.0.0', self.expected_unicode + '.0']:
            raise ValueError(f'ICU/Unicode version mismatch: {header}')
        if len(lines) != len(requests) + 1:
            raise ValueError('Incomplete ICU response')
        results = []
        for index, line in enumerate(lines[1:]):
            ident, status, payload = line.split('\t')
            if ident != str(index):
                raise ValueError('ICU response order mismatch')
            if status != 'OK':
                raise ValueError('ICU rejected source: ' + base64.b64decode(payload).decode('utf-8'))
            results.append([base64.b64decode(s).decode('utf-8') for s in payload.split(',')]
                           if payload else [])
        return results
