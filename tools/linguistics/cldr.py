"""LDML exemplar inheritance. CLDR ancestry is source provenance, not HCP membership."""
import re
import xml.etree.ElementTree as ET
from sources import sha256

ROLES = ('main', 'auxiliary', 'index', 'punctuation', 'punctuation-auxiliary')
INHERIT = '↑↑↑'
BLOCK = '∅∅∅'
SEGMENT = re.compile(r"([A-Za-z][\w-]*)(.*)")
PREDICATE = re.compile(r"\[@([\w-]+)=['\"]([^'\"]*)['\"]\]")


def locale_id(value):
    value = value.replace('-', '_')
    if not re.fullmatch(r'[A-Za-z0-9]+(?:_[A-Za-z0-9]+)*', value):
        raise ValueError(f'Invalid locale ID: {value}')
    return value


def field_path(role):
    return (('characters', ()), ('exemplarCharacters', () if role == 'main' else (('type', role),)))


def render(path):
    return '/ldml/' + '/'.join(tag + ''.join(f"[@{a}='{v}']" for a, v in attrs)
                              for tag, attrs in path)


def alias_path(base, expression):
    if not expression:
        return base
    parts = [] if expression.startswith('/') else list(base)
    terms = expression.split('/')
    if expression.startswith('/'):
        if terms[:2] != ['', 'ldml']:
            raise ValueError(f'Unsupported absolute LDML alias: {expression}')
        terms = terms[2:]
    for term in terms:
        if term == '.':
            continue
        if term == '..':
            if not parts:
                raise ValueError(f'Alias leaves LDML root: {expression}')
            parts.pop()
            continue
        match = SEGMENT.fullmatch(term)
        if not match:
            raise ValueError(f'Unsupported LDML alias path: {expression}')
        tag, predicates = match.groups()
        attrs = PREDICATE.findall(predicates)
        if PREDICATE.sub('', predicates):
            raise ValueError(f'Unsupported LDML alias predicate: {expression}')
        parts.append((tag, tuple(sorted(attrs))))
    return tuple(parts)


class Cldr:
    def __init__(self, files):
        self.files, self.docs, self.used = files, {}, {}
        supplemental = 'common/supplemental/supplementalData.xml'
        root = ET.fromstring(files[supplemental])
        self.used[supplemental] = sha256(files[supplemental])
        self.parents = {}
        for node in root.findall('./parentLocales'):
            if 'component' in node.attrib:
                continue  # Collation/etc. ancestry does not govern characters.
            for relation in node.findall('./parentLocale'):
                if 'component' not in relation.attrib:
                    for child in relation.attrib['locales'].split():
                        self.parents[child] = relation.attrib['parent']

    def exists(self, locale):
        return f'common/main/{locale}.xml' in self.files

    def parent(self, locale):
        if locale == 'root':
            return None
        return self.parents.get(locale, locale.rsplit('_', 1)[0] if '_' in locale else 'root')

    def doc(self, locale):
        if locale not in self.docs:
            filename = f'common/main/{locale}.xml'
            if filename not in self.files:
                return None
            self.used[filename] = sha256(self.files[filename])
            doc = ET.fromstring(self.files[filename])
            # Keep only the relevant sections; full CLDR locale docs are large.
            for child in list(doc):
                if child.tag not in ('identity', 'alias', 'characters'):
                    doc.remove(child)
            self.docs[locale] = doc
        return self.docs[locale]

    def context(self, locale):
        doc = self.doc(locale)
        if doc is None:
            return {}
        return {child.tag: child.attrib.get('type') for child in doc.findall('./identity/*')
                if child.tag in ('language', 'script', 'territory', 'variant')}

    def resolve(self, locale, role):
        if role not in ROLES:
            raise ValueError(f'Unsupported inventory role: {role}')
        if not self.exists(locale):
            return None
        return self._resolve(locale, field_path(role), set(), [])

    def _resolve(self, locale, path, seen, trace):
        state = (locale, path)
        if state in seen:
            raise ValueError(f'CLDR inheritance/alias cycle: {locale} {render(path)}')
        seen = seen | {state}
        doc = self.doc(locale)
        node = doc
        draft = 'approved'
        if doc is not None:
            for depth in range(len(path) + 1):
                draft = node.attrib.get('draft', draft)
                aliases = node.findall('./alias')
                if len(aliases) > 1:
                    raise ValueError('Ambiguous CLDR alias')
                if aliases:
                    alias = aliases[0]
                    target = alias.attrib.get('source', 'locale')
                    target = locale if target == 'locale' else locale_id(target)
                    newpath = alias_path(path[:depth], alias.attrib.get('path', '')) + path[depth:]
                    hop = {'locale': locale, 'path': render(path[:depth]), 'via': 'alias',
                           'target_locale': target, 'target_path': render(newpath)}
                    return self._resolve(target, newpath, seen, trace + [hop])
                if depth == len(path):
                    text = (node.text or '').strip()
                    if text == BLOCK:
                        return None
                    if text and text != INHERIT:
                        return {'pattern': text, 'source_locale': locale, 'path': render(path),
                                'draft': draft,
                                'attributes': dict(sorted(node.attrib.items())), 'trace': trace,
                                'source_sha256': self.used[f'common/main/{locale}.xml']}
                    break
                tag, attrs = path[depth]
                required = dict(attrs)
                matches = [n for n in node.findall('./' + tag)
                           if all(n.attrib.get(a) == v for a, v in required.items())
                           and ('alt' in required or 'alt' not in n.attrib)
                           and (tag != 'exemplarCharacters' or 'type' in required or 'type' not in n.attrib)]
                if len(matches) > 1:
                    raise ValueError(f'Ambiguous CLDR field: {locale} {render(path)}')
                if not matches:
                    break
                node = matches[0]
        parent = self.parent(locale)
        if parent is None:
            return None
        return self._resolve(parent, path, seen, trace + [{'locale': locale, 'via': 'parent',
                                                         'target_locale': parent}])
