"""Bulk linguistic seed endpoints, core child additions and memberships. No writes."""
from collections import defaultdict
import json
from sources import sha256
from cldr import ROLES, locale_id
from unicode_values import character_address, key

ALPHABET_ROLES = {'main', 'auxiliary'}
PUNCTUATION_ROLES = {'punctuation', 'punctuation-auxiliary'}


def component_key(locale, form):
    key(form)  # Reject empty forms and invalid scalars before making a seed key.
    return 'linguistics:' + locale_id(locale) + ':' + '-'.join(f'{ord(c):04X}' for c in form)


def write_json(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, ensure_ascii=False, sort_keys=True, indent=2) + '\n', encoding='utf-8')


def validate_manifest(manifest):
    if manifest.get('format') != 'hcp-language-inventory-v2':
        raise ValueError('Expected language endpoint seed format v2')
    source_values = {v['key']: v for v in manifest['source_values']}
    values = {v['key']: v for v in manifest['values']}
    groups = {g['key']: g for g in manifest['groups']}
    if (len(source_values) != len(manifest['source_values'])
            or len(values) != len(manifest['values']) or len(groups) != len(manifest['groups'])):
        raise ValueError('Duplicate value/group identity')
    if values.keys() & groups.keys():
        raise ValueError('Value and group identities overlap')
    edges = [(e['member'], e['group']) for e in manifest['memberships']]
    if len(edges) != len(set(edges)):
        raise ValueError('Duplicate membership edge')
    up, down = defaultdict(set), defaultdict(set)
    for member, group in edges:
        if member not in values and member not in groups or group not in groups:
            raise ValueError('Unknown membership endpoint')
        up[member].add(group)
        down[group].add(member)
    if {k: sorted(v) for k, v in up.items()} != manifest['member_of']:
        raise ValueError('member_of reciprocal mismatch')
    if {k: sorted(v) for k, v in down.items()} != manifest['members']:
        raise ValueError('members reciprocal mismatch')
    visited, active = set(), set()
    def visit(group):
        if group in active:
            raise ValueError('Membership hierarchy cycle')
        if group in visited:
            return
        active.add(group)
        for parent in up[group]:
            visit(parent)
        active.remove(group)
        visited.add(group)
    for group in groups:
        visit(group)
        descriptor = groups[group]
        if descriptor.get('centroid') != {'key': 'centroid:' + group, 'state': 'native-assembly-pending'}:
            raise ValueError('Group needs its distinct pending centroid representation')
        if 'parents' in descriptor or 'structural_mass_derived' in descriptor:
            raise ValueError('Group containment must not create literal parents/mass')
    for value in source_values.values():
        if value['key'] != key(value['form']):
            raise ValueError('Shared value identity conflicts with exact form')
        raw = value['form'].encode('utf-8')
        if value['scalars'] != [f'U+{ord(c):04X}' for c in value['form']]:
            raise ValueError('Source scalar sequence conflicts with exact form')
        if value['utf8_hex'] != raw.hex(' ').upper() or value['structural_mass_derived'] != 2 * len(raw):
            raise ValueError('Literal byte/mass mismatch')
        if len(value['form']) == 1:
            if value['kind'] != 'existing-core-value' or 'parents' in value:
                raise ValueError('Single character must reference existing core value')
            if value.get('store') != 'hcp_core' or value.get('address') != character_address(ord(value['form'])):
                raise ValueError('Core reference conflicts with scalar identity')
        else:
            expected = [{'ordinal': i, 'store': 'hcp_core', 'address': character_address(ord(c))}
                        for i, c in enumerate(value['form'])]
            if value.get('parents') != expected:
                raise ValueError('Literal parent order/repetition mismatch')
            if value.get('kind') != 'source-sequence' or value.get('store') is not None or value.get('address') is not None:
                raise ValueError('Source sequences are evidence, not shared linguistic endpoints')
    expected_children = set()
    for value in values.values():
        locale, form = value['locale'], value['form']
        if value['key'] != component_key(locale, form):
            raise ValueError('Language endpoint identity conflicts with locale/form')
        if (value.get('kind') != 'language-component-candidate'
                or value.get('store') != 'linguistics' or value.get('address') is not None):
            raise ValueError('Language endpoint needs its own unallocated linguistic identity')
        source_value = source_values.get(value.get('source_value'))
        fields = ('form', 'scalars', 'utf8_hex', 'structural_mass_derived', 'scalar_metadata')
        if source_value is None or any(value[field] != source_value[field] for field in fields):
            raise ValueError('Language endpoint conflicts with source form')
        expected = [{'ordinal': i, 'store': 'hcp_core', 'address': character_address(ord(c))}
                    for i, c in enumerate(form)]
        if value.get('parents') != expected:
            raise ValueError('Language endpoint core parent order/repetition mismatch')
        language, alphabet, punctuation = 'language:' + locale, 'alphabet:' + locale, 'punctuation:' + locale
        memberships = up[value['key']]
        if (language not in memberships or not memberships & {alphabet, punctuation}
                or memberships - {language, alphabet, punctuation}):
            raise ValueError('Language endpoint has incorrect language/group membership')
        expected_children.update((p['address'], value['key']) for p in expected)
    children, core_addresses = set(), set()
    for update in manifest['core_child_updates']:
        if update.get('store') != 'hcp_core' or update.get('mode') != 'add':
            raise ValueError('Core child updates must be additive to existing listings')
        address = update['address']
        if address in core_addresses or not update['children']:
            raise ValueError('Duplicate or empty core child update')
        core_addresses.add(address)
        for child in update['children']:
            target = values.get(child.get('key'))
            if target is None or child.get('store') != 'linguistics' or child.get('address') != target['address']:
                raise ValueError('Core child update references an unknown language endpoint')
            edge = address, child['key']
            if edge in children:
                raise ValueError('Duplicate core child edge')
            children.add(edge)
    if children != expected_children:
        raise ValueError('Core child reciprocal mismatch with language endpoint parents')


def generate(sources, cldr, icu, ucd, locales, roles, hierarchy):
    locales = sorted({locale_id(x) for x in locales})
    if not roles or set(roles) - set(ROLES):
        raise ValueError('Select supported inventory roles')
    roles = [r for r in ROLES if r in roles]
    inventories, coverage = [], []
    for locale in locales:
        row = {'locale': locale, 'script_context': cldr.context(locale), 'roles': {}, 'issues': []}
        if not cldr.exists(locale):
            row['issues'].append('missing-locale-source')
        for role in roles:
            try:
                inventory = cldr.resolve(locale, role)
                if inventory is None:
                    row['roles'][role] = {'status': 'missing'}
                    row['issues'].append('missing-inventory:' + role)
                    continue
                inventory.update(id=f'{locale}:{role}', locale=locale, role=role,
                                 script_context=row['script_context'])
                inventories.append(inventory)
            except ValueError as error:
                row['roles'][role] = {'status': 'error', 'error': str(error)}
                row['issues'].append('source-resolution-error:' + role)
        coverage.append(row)
    sets = icu.batch([('SET', i['pattern']) for i in inventories])
    source_forms, forms_for_case = [], []
    for inventory, forms in zip(inventories, sets):
        forms = sorted(set(forms), key=lambda s: tuple(map(ord, s)))
        inventory['forms'] = [key(form) for form in forms]
        for form in forms:
            source_forms.append({'inventory': inventory['id'], 'value': key(form)})
            if inventory['role'] in ALPHABET_ROLES:
                forms_for_case.append((inventory, form))
    case_sets = icu.batch([('CASE', i['locale'], f) for i, f in forms_for_case])
    groups, source_values, values, edges = {}, {}, {}, defaultdict(set)
    def group(identity, label, kind):
        descriptor = {'key': identity, 'label': label, 'kind': kind, 'store': 'linguistics',
                      'address': None, 'centroid': {'key': 'centroid:' + identity,
                                                  'state': 'native-assembly-pending'}}
        if identity in groups and groups[identity]['kind'] != kind:
            raise ValueError('Conflicting group description: ' + identity)
        groups.setdefault(identity, descriptor)
    def value(form):
        identity = key(form)
        candidate = ucd.value(form)
        if identity in source_values and source_values[identity] != candidate:
            raise ValueError('Conflicting shared construction: ' + identity)
        source_values[identity] = candidate
        return identity
    for identity, descriptor in hierarchy['groups'].items():
        group(identity, descriptor['label'], descriptor['kind'])
    for member, parent in hierarchy['memberships']:
        if member not in groups or parent not in groups:
            raise ValueError('Hierarchy references undefined group')
        edges[member, parent].add('bootstrap-hierarchy')
    for locale in locales:
        identity = 'language:' + locale
        if identity not in groups:
            group(identity, locale, 'language' if '_' not in locale else 'language-subgroup')
    def memberships(inventory, identity, evidence):
        role, locale = inventory['role'], inventory['locale']
        if role in ALPHABET_ROLES:
            kind = 'alphabet'
        elif role in PUNCTUATION_ROLES:
            kind = 'punctuation'
        else:
            return  # Index forms retain evidence without creating alphabet membership.
        source = source_values[identity]
        identity = component_key(locale, source['form'])
        candidate = {field: source[field] for field in ('form', 'scalars', 'utf8_hex',
                                                        'structural_mass_derived', 'scalar_metadata')}
        candidate.update(key=identity, locale=locale, source_value=source['key'],
                         kind='language-component-candidate', store='linguistics', address=None,
                         parents=[{'ordinal': i, 'store': 'hcp_core', 'address': character_address(ord(c))}
                                  for i, c in enumerate(source['form'])])
        if identity in values and values[identity] != candidate:
            raise ValueError('Conflicting language endpoint construction: ' + identity)
        values[identity] = candidate
        language, grouping = 'language:' + locale, kind + ':' + locale
        group(grouping, kind.capitalize() + ' (' + locale + ')', kind)
        edges[grouping, language].add('inventory-group:' + inventory['id'])
        edges[identity, grouping].add(evidence)
        edges[identity, language].add(evidence)
    for inventory, forms in zip(inventories, sets):
        for form in forms:
            identity = value(form)
            memberships(inventory, identity, 'source:' + inventory['id'] + ':' + identity)
    case_candidates = []
    for (inventory, original), variants in zip(forms_for_case, case_sets):
        for variant in sorted(set(variants) - {original}, key=lambda s: tuple(map(ord, s))):
            # Only scalar-to-scalar casing receives candidate memberships. A
            # multi-scalar case mapping remains explicit evidence for review;
            # it cannot fabricate a new construction (e.g. ß -> SS).
            applied = len(original) == len(variant) == 1
            record = {'inventory': inventory['id'], 'from': key(original), 'form': variant,
                      'value': key(variant), 'membership_candidate': applied,
                      'rule': 'ICU locale lower/upper/title, exact sequence preserved'}
            case_candidates.append(record)
            if applied:
                identity = value(variant)
                memberships(inventory, identity, 'case:' + inventory['id'] + ':' + key(original))
    by_locale = {row['locale']: row for row in coverage}
    for inventory in inventories:
        by_locale[inventory['locale']]['roles'][inventory['role']] = {
            'status': 'present' if inventory['forms'] else 'empty',
            'source_locale': inventory['source_locale'], 'inherited': bool(inventory['trace']),
            'source_memberships': len(inventory['forms']), 'draft': inventory['draft']}
        if not inventory['forms'] and inventory['role'] == 'main':
            by_locale[inventory['locale']]['issues'].append('empty-main-inventory')
    up, down = defaultdict(set), defaultdict(set)
    for member, parent in edges:
        up[member].add(parent)
        down[parent].add(member)
    core_children = defaultdict(set)
    for candidate in values.values():
        for parent in candidate['parents']:
            core_children[parent['address']].add(candidate['key'])
    child_updates = [{'store': 'hcp_core', 'address': address, 'mode': 'add',
                      'children': [{'key': identity, 'store': 'linguistics', 'address': values[identity]['address']}
                                   for identity in sorted(children)]}
                     for address, children in sorted(core_children.items())]
    manifest = {'format': 'hcp-language-inventory-v2', 'status': 'candidate-no-persistent-writes',
                'core_validation': 'not-run', 'provenance': sources.provenance(),
                'cldr_files_sha256': dict(sorted(cldr.used.items())),
                'hierarchy_provenance': hierarchy['provenance'],
                'hierarchy_sha256': sha256(json.dumps(hierarchy, sort_keys=True).encode('utf-8')),
                'inventories': inventories, 'source_forms': source_forms,
                'case_candidates': case_candidates,
                'source_values': [source_values[k] for k in sorted(source_values)],
                'values': [values[k] for k in sorted(values)],
                'core_child_updates': child_updates,
                'groups': [groups[k] for k in sorted(groups)],
                'memberships': [{'member': m, 'group': g, 'evidence': sorted(edges[m, g])}
                                for m, g in sorted(edges)],
                'member_of': {k: sorted(v) for k, v in sorted(up.items())},
                'members': {k: sorted(v) for k, v in sorted(down.items())}}
    validate_manifest(manifest)
    for row in coverage:
        locale = row['locale']
        own = [i for i in inventories if i['locale'] == locale]
        originals = {v for i in own for v in i['forms']}
        row.update(source_memberships=sum(len(i['forms']) for i in own),
                   distinct_source_sequences=len(originals),
                   multi_scalar_source_memberships=sum(v.startswith('sequence:') for i in own for v in i['forms']),
                   direct_value_memberships=len([m for m in down['language:' + locale] if m in values]),
                   case_expanded_candidates=sum(c['inventory'].startswith(locale + ':') for c in case_candidates),
                   observed_scripts=sorted({meta['script'] for v in originals
                                            for meta in source_values[v]['scalar_metadata']}))
    counts = {'source_memberships': len(source_forms), 'distinct_source_sequences': len({r['value'] for r in source_forms}),
              'source_values_including_case_candidates': len(source_values),
              'language_component_endpoints': len(values), 'groups': len(groups),
              'membership_edges': len(edges), 'case_expanded_candidates': len(case_candidates),
              'multi_scalar_source_values': sum(len(v['form']) > 1 for v in source_values.values()),
              'multi_scalar_language_endpoints': sum(len(v['form']) > 1 for v in values.values()),
              'ordered_core_parent_occurrences': sum(len(v['parents']) for v in values.values()),
              'core_child_edges': sum(len(c) for c in core_children.values()),
              'core_endpoints_receiving_children': len(core_children),
              'coverage_gaps': sum(bool(row['issues']) for row in coverage)}
    report = {'format': manifest['format'], 'counts': counts, 'coverage': coverage,
              'core_validation': 'not-run', 'database_writes': 0,
              'cldr_files_sha256': manifest['cldr_files_sha256'],
              'source_pins': sources.provenance(), 'requested_locales': locales,
              'requested_roles': roles}
    return manifest, report
