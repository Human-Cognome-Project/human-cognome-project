"""Offline membership selection for iconographic component bundles."""
from collections import defaultdict
import json
from manifest import validate_manifest
from sources import sha256
from validate_core import expected_definitions
from unicode_values import byte_address, key


def select_components(manifest, selectors):
    validate_manifest(manifest)
    selectors = sorted(set(selectors))
    if not selectors:
        raise ValueError('Select at least one language/alphabet/punctuation group')
    groups = {g['key']: g for g in manifest['groups']}
    values = {v['key']: v for v in manifest['values']}
    for edge in manifest.get('boundary_memberships', []):
        if edge.get('member') not in groups and edge.get('member') not in values:
            raise ValueError('Unknown boundary membership member')
        if not edge.get('group') or edge['group'] in groups:
            raise ValueError('Boundary membership must reference a group outside the input view')
    for selector in selectors:
        if selector not in groups:
            raise ValueError('Unknown membership selector: ' + selector)
    selected_groups, selected_values = set(), set()
    def visit(group):
        if group in selected_groups:
            return
        selected_groups.add(group)
        for member in manifest['members'].get(group, []):
            if member in groups:
                visit(member)
            else:
                selected_values.add(member)
    for selector in selectors:
        visit(selector)
    selected = selected_groups | selected_values
    # Narrowing an exported bundle preserves links back to the larger view.
    all_edges = {(e['member'], e['group']): e
                 for e in manifest['memberships'] + manifest.get('boundary_memberships', [])}
    all_edges = [all_edges[k] for k in sorted(all_edges)]
    edges = [edge for edge in all_edges
             if edge['member'] in selected and edge['group'] in selected_groups]
    boundary = [edge for edge in all_edges
                if edge['member'] in selected and edge['group'] not in selected_groups]
    up, down = defaultdict(set), defaultdict(set)
    for edge in edges:
        up[edge['member']].add(edge['group'])
        down[edge['group']].add(edge['member'])
    bundle = {'format': 'hcp-iconographic-selection-v1',
              'status': 'offline-bundle-native-assembly-pending', 'encoding': 'UTF-8',
              'selectors': selectors,
              'source_manifest_sha256': sha256(json.dumps(manifest, sort_keys=True, ensure_ascii=False).encode('utf-8')),
              'provenance': manifest['provenance'],
              'cldr_files_sha256': manifest['cldr_files_sha256'],
              'values': [values[k] for k in sorted(selected_values)],
              'groups': [groups[k] for k in sorted(selected_groups)],
              'memberships': edges, 'boundary_memberships': boundary,
              'members': {k: sorted(v) for k, v in sorted(down.items())},
              'member_of': {k: sorted(v) for k, v in sorted(up.items())},
              'core_validation': 'not-run'}
    # Selected forms retain their shared identities. Include construction
    # expectations for the complete referenced value trees, without adding
    # byte/nibble dependencies to alphabet or punctuation memberships.
    definitions = expected_definitions(bundle, all_floor_bytes=False)
    bundle['construction_expectations'] = [
        {'key': 'core:' + identity, 'store': 'hcp_core', 'address': identity,
         'structural_mass_derived': definition['mass'],
         'parents': [{'ordinal': ordinal, 'value': 'core:' + parent,
                      'structural_count_derived': definitions[parent]['mass']}
                     for ordinal, parent in enumerate(definition['parents'])]}
        for identity, definition in sorted(definitions.items())]
    locales = {g.split(':', 1)[1] for g in selected_groups
               if g.startswith(('language:', 'alphabet:', 'punctuation:'))}
    inventories = [i for i in manifest.get('inventories', manifest.get('inventory_refs', []))
                   if i['locale'] in locales
                   and ('forms' not in i or any(v in selected_values for v in i['forms']))]
    bundle['inventory_refs'] = [{k: i[k] for k in ('id', 'locale', 'role', 'source_locale', 'path',
                                                  'source_sha256', 'draft', 'trace')}
                                for i in inventories]
    bundle['counts'] = {'values': len(selected_values), 'groups': len(selected_groups),
                        'membership_edges': len(edges), 'boundary_memberships': len(boundary),
                        'core_construction_definitions': len(definitions),
                        'multi_scalar_literals': sum(len(values[k]['form']) > 1 for k in selected_values)}
    if not selected_values:
        bundle['status'] = 'empty-selection'
    validate_manifest(bundle)
    return bundle


def inventory_index(manifest):
    """Compact memberships/glyph references for every supplied locale."""
    result = []
    for language in sorted(g['key'] for g in manifest['groups']
                           if g['kind'] in ('language', 'language-subgroup')):
        locale = language.removeprefix('language:')
        alphabet, punctuation = 'alphabet:' + locale, 'punctuation:' + locale
        if alphabet not in manifest['members'] and punctuation not in manifest['members']:
            continue
        result.append({'locale': locale, 'language': language,
                       'alphabet': manifest['members'].get(alphabet, []),
                       'punctuation': manifest['members'].get(punctuation, [])})
    active_values = {v for row in result for kind in ('alphabet', 'punctuation') for v in row[kind]}
    return {'encoding': 'UTF-8', 'inventories': result,
            'values': [{'key': v['key'], 'form': v['form'], 'scalars': v['scalars'],
                        'utf8_hex': v['utf8_hex']} for v in manifest['values']
                       if v['key'] in active_values]}


def selection_summary(bundle):
    """Reviewable samples plus a digest covering the entire generated bundle."""
    forms = {'A', 'Α', 'א', '…'}
    addresses = {key('…'), 'core:' + byte_address(0xE2)}
    return {'encoding': bundle['encoding'], 'status': bundle['status'],
            'selectors': bundle['selectors'], 'counts': bundle['counts'],
            'source_manifest_sha256': bundle['source_manifest_sha256'],
            'bundle_sha256': sha256(json.dumps(bundle, sort_keys=True, ensure_ascii=False).encode('utf-8')),
            'groups': bundle['groups'],
            'sample_values': [v for v in bundle['values'] if v['form'] in forms],
            'sample_constructions': [d for d in bundle['construction_expectations'] if d['key'] in addresses]}
