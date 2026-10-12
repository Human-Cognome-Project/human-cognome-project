"""Scalar metadata and the accepted native core address layout (PR #111)."""
from bisect import bisect_right

ALPHABET = '0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz'


def pair(number):
    if not 0 <= number < 3844:
        raise ValueError('couplet outside Base62 range')
    return ALPHABET[number // 62] + ALPHABET[number % 62]


def scalar(cp):
    if not 0 <= cp <= 0x10FFFF or 0xD800 <= cp <= 0xDFFF:
        raise ValueError(f'Invalid Unicode scalar: {cp:X}')
    return cp


def nibble_address(n):
    if not 0 <= n < 16:
        raise ValueError('Invalid nibble')
    return '00.00.00.00.' + pair(n)


def byte_address(b):
    if not 0 <= b < 256:
        raise ValueError('Invalid byte')
    return '00.00.00.00.' + pair(62 + b)


def character_address(cp):
    scalar(cp)
    if cp < 128:
        return byte_address(cp)
    if cp < 0x800:
        start, index = 1, cp - 128
    elif cp < 0x10000:
        start, index = 3, cp - 0x800 - (0x800 if cp > 0xDFFF else 0)
    else:
        start, index = 62, cp - 0x10000
    return '00.00.00.' + pair(start + index // 3844) + '.' + pair(index % 3844)


def key(form):
    if not form:
        raise ValueError('Empty form has no literal identity')
    cps = [scalar(ord(c)) for c in form]
    return ('core:' + character_address(cps[0]) if len(cps) == 1
            else 'sequence:' + '-'.join(f'{cp:04X}' for cp in cps))


class RangeMap:
    def __init__(self, rows):
        self.rows = sorted(rows)
        self.starts = [row[0] for row in self.rows]

    def get(self, cp, default):
        index = bisect_right(self.starts, cp) - 1
        if index >= 0 and cp <= self.rows[index][1]:
            return self.rows[index][2]
        return default


def property_ranges(path):
    rows = []
    for line in path.read_text().splitlines():
        line = line.split('#')[0].strip()
        if not line:
            continue
        span, value = [part.strip() for part in line.split(';')]
        bounds = span.split('..')
        rows.append((int(bounds[0], 16), int(bounds[-1], 16), value))
    return RangeMap(rows)


class UnicodeData:
    def __init__(self, sources):
        rows, first = [], None
        for line in sources.path('UnicodeData.txt').read_text().splitlines():
            fields = line.split(';')
            cp, name, category = int(fields[0], 16), fields[1], fields[2]
            if name.endswith(', First>'):
                first = (cp, name.removesuffix(', First>') + '>', category)
            elif name.endswith(', Last>'):
                if first is None or category != first[2]:
                    raise ValueError('Malformed UCD First/Last range')
                rows.append((first[0], cp, (first[1], category)))
                first = None
            else:
                rows.append((cp, cp, (name, category)))
        if first:
            raise ValueError('Unclosed UCD range')
        self.characters = RangeMap(rows)
        self.scripts = property_ranges(sources.path('Scripts.txt'))
        self.extensions = property_ranges(sources.path('ScriptExtensions.txt'))
        self.aliases = {}
        for line in sources.path('PropertyValueAliases.txt').read_text().splitlines():
            fields = [x.strip() for x in line.split('#')[0].split(';')]
            if len(fields) >= 3 and fields[0] == 'sc':
                self.aliases[fields[1]] = fields[2]

    def metadata(self, cp):
        scalar(cp)
        name, category = self.characters.get(cp, ('<unassigned>', 'Cn'))
        script = self.scripts.get(cp, 'Unknown')
        extensions = self.extensions.get(cp, None)
        return {'name': name, 'category': category, 'script': script,
                'script_extensions': ([self.aliases[s] for s in extensions.split()]
                                      if extensions else [script])}

    def value(self, form):
        identity = key(form)
        cps = list(map(ord, form))
        raw = form.encode('utf-8')
        if raw.decode('utf-8') != form:
            raise ValueError('UTF-8 reconstruction failed')
        result = {'key': identity, 'form': form, 'scalars': [f'U+{cp:04X}' for cp in cps],
                  'utf8_hex': raw.hex(' ').upper(), 'structural_mass_derived': 2 * len(raw),
                  'scalar_metadata': [self.metadata(cp) for cp in cps]}
        if len(cps) == 1:
            result.update(kind='existing-core-value', store='hcp_core', address=character_address(cps[0]))
        else:
            result.update(kind='source-sequence', store=None, address=None,
                          parents=[{'ordinal': i, 'store': 'hcp_core', 'address': character_address(cp)}
                                   for i, cp in enumerate(cps)])
        return result
