import {test} from 'node:test';
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import {parseSnapshot, validateSnapshot, rangesFor, massColour, selectParticles, boundsFor, classifyUpdate, pollInterval, fileStamp, resolveSelection, watchState} from '../model.mjs';
const study = '12345678-1234-1234-1234-123456789abc';
export function fixture() {
  const particle = (i, mass, radius, binding = 0) => ({id: `${study}/${i}`, definition: `public:${i}`, slot: i,
    position: [i * 2, 0, 0], radius, mass, exact_mass: '9223372036854775811', binding, level: binding,
    exposure: 'aggregate', aggregate: `${study}/${i}`, label: `Letter "é" \\ ${i}\n`});
  return {schema: 'hcp-study-snapshot', version: 1,
    capture: {session: study, study, generation: study, tick: '0', phase: 'published_seeded', label: 'Synthetic validation fixture'},
    frame: {coordinates: 'reference_local', units: 'operative_model_units', reference: [0, 0, 0], camera_target: [0, 0, 0], target_kind: 'legacy_origin', focal_fields: []},
    bindings: [{id: 0, parent: -1, role: 'root', units: 1, level: 0}, {id: 1, parent: 0, role: 'primary_step', units: 64, level: 1}],
    particles: [particle(0, 2, 0.5), particle(1, 10, 1), particle(2, null, null), particle(3, 2, 0), particle(4, 7, 0.5, 1)], source: {kind: 'synthetic'}};
}
test('exact identities/masses and escaped supplied labels survive JSON', () => {
  const parsed = parseSnapshot(JSON.stringify(fixture()));
  assert.equal(parsed.particles[0].exact_mass, '9223372036854775811');
  assert.equal(parsed.particles[0].label, fixture().particles[0].label);
});
test('stable independent per-level ranges, equal/unknown handling, zero is known', () => {
  const f = fixture(), ranges = rangesFor(f);
  assert.deepEqual(ranges.get(0), {min: 2, max: 10, known: 3, unknown: 1, total: 4});
  assert.equal(massColour(7, ranges.get(1)), massColour(6, {min: 2, max: 10}));
  assert.equal(massColour(null, ranges.get(0)), '#9ba4b4');
  const colour = massColour(2, ranges.get(0));
  const selected = selectParticles(f, {text: f.particles[0].id});
  assert.equal(selected.matched.length, 1);
  assert.equal(massColour(selected.matched[0].mass, ranges.get(0)), colour);
  f.particles[0].mass = 0; assert.equal(rangesFor(f).get(0).min, 0);
});
test('filters count missing/zero extent and preserve captured data', () => {
  const f = fixture(), before = JSON.stringify(f), selected = selectParticles(f);
  assert.equal(selected.matched.length, 5); assert.equal(selected.drawable.length, 3);
  assert.equal(selected.zero, 1); assert.equal(selected.unavailable, 1);
  assert.equal(selectParticles(f, {level: '1'}).matched.length, 1);
  assert.equal(selectParticles(f, {exposure: 'parts'}).matched.length, 0);
  assert.equal(JSON.stringify(f), before);
});
test('large reference remains separate; focus uses current weighted fields', () => {
  const f = fixture(); f.frame.reference = [2 ** 60, 0, 0];
  f.frame.focal_fields = [{kind: 'membership', value: 'a', mass: 2, position: [1, 0, 0]},
    {kind: 'parent_value', value: 'a', mass: 6, position: [5, 0, 0]}];
  f.frame.camera_target = [4, 0, 0]; f.frame.target_kind = 'captured_focal_centroid';
  assert.equal(validateSnapshot(f).particles[1].position[0], 2);
  f.frame.camera_target[0] = 3; assert.throws(() => validateSnapshot(f), /Camera target/);
});
test('nested compression/naming inherit levels', () => {
  const f = fixture();
  f.bindings.push({id: 2, parent: 1, role: 'secondary_compression', units: 64, level: 1},
    {id: 3, parent: 2, role: 'naming_equivalent', units: 64, level: 1});
  f.particles[4].binding = 3; assert.doesNotThrow(() => validateSnapshot(f));
  f.bindings[3].level = 3; assert.throws(() => validateSnapshot(f), /Conceptual/);
});
test('malformed, unsupported and mixed identities are rejected', () => {
  const mutations = [f => f.version = 2, f => f.capture.tick = 3, f => f.capture.tick = '18446744073709551616',
    f => f.capture.phase = 'post_tick', f => f.particles[0].radius = -1, f => f.particles[0].position[0] = Infinity,
    f => f.particles[1].id = f.particles[0].id, f => f.particles[1].slot = 0, f => f.particles[0].id = 'other/0',
    f => f.particles[0].binding = 99, f => f.particles[0].level = 9, f => delete f.particles[0].radius,
    f => f.particles[0].position[0] = 1e100, f => f.particles[0].radius = 0.1];
  for (const mutate of mutations) { const f = fixture(); mutate(f); assert.throws(() => validateSnapshot(f)); }
  assert.throws(() => parseSnapshot('{broken'));
});
test('empty captures and point extents have finite framing bounds', () => {
  const f = fixture(); f.particles = [];
  assert.doesNotThrow(() => validateSnapshot(f)); assert.equal(rangesFor(f).size, 0);
  assert.equal(selectParticles(f).drawable.length, 0);
  assert.ok(Number.isFinite(boundsFor([]).radius));
  assert.equal(boundsFor([{position: [3, 4, 5], radius: 0}]).radius, 0);
});
test('retained real native examples satisfy the public schema and phase contract', () => {
  for (const name of ['letter-study.json', 'letter-study-tick0.json']) {
    const value = parseSnapshot(readFileSync(new URL(`../examples/${name}`, import.meta.url), 'utf8'));
    assert.equal(value.source.kind, 'retained_cold_to_warm_to_active');
    assert.equal(value.particles.length, 16);
    assert.equal(value.capture.tick, name.includes('tick0') ? '0' : '8');
    assert.equal(value.frame.target_kind, 'captured_focal_centroid');
  }
});
test('live updates: same session+study updates, anything else replaces, identical capture is no change', () => {
  const a = fixture(), other = '22345678-1234-1234-1234-123456789abc';
  const update = (...m) => { const f = fixture(); m.forEach(fn => fn(f)); return f; };
  assert.equal(classifyUpdate(null, a), 'replace');
  assert.equal(classifyUpdate(a, update(f => f.capture.tick = '8', f => f.capture.phase = 'post_tick')), 'update');
  assert.equal(classifyUpdate(a, update(f => f.capture.generation = other)), 'update');
  assert.equal(classifyUpdate(a, update()), 'same');
  assert.equal(classifyUpdate(a, update(f => f.capture.study = other)), 'replace');
  assert.equal(classifyUpdate(a, update(f => f.capture.session = other)), 'replace');
});
test('live updates: selection re-resolves by occurrence ID and clears when absent or filtered out', () => {
  const a = fixture(), b = fixture(); b.particles[1].position = [9, 9, 9]; b.particles.splice(2, 1);
  const chosen = a.particles[1];
  assert.equal(resolveSelection(selectParticles(b).matched, chosen.id), b.particles[1]);
  assert.notEqual(resolveSelection(selectParticles(b).matched, chosen.id), chosen);
  assert.equal(resolveSelection(selectParticles(b).matched, a.particles[2].id), null);
  assert.equal(resolveSelection(selectParticles(b, {level: '1'}).matched, chosen.id), null);
});
test('live updates: interval is clamped, files are compared by modified time and size', () => {
  assert.equal(pollInterval('250'), 250); assert.equal(pollInterval(50), 100); assert.equal(pollInterval('0'), 100);
  assert.equal(pollInterval(''), 1000); assert.equal(pollInterval('abc'), 1000); assert.equal(pollInterval(null, 400), 400);
  assert.equal(pollInterval(1e12), 3600000); assert.equal(pollInterval(150.4), 150);
  assert.equal(fileStamp({lastModified: 5, size: 7}), fileStamp({lastModified: 5, size: 7}));
  assert.notEqual(fileStamp({lastModified: 5, size: 7}), fileStamp({lastModified: 6, size: 7}));
  assert.notEqual(fileStamp({lastModified: 5, size: 7}), fileStamp({lastModified: 5, size: 8}));
});
test('live updates: watch state precedence and staleness hint', () => {
  const base = {paused: false, error: null, missing: false, hasCapture: true, idleMs: 0, intervalMs: 1000};
  assert.equal(watchState(base), 'LIVE');
  assert.equal(watchState({...base, idleMs: 10001}), 'STALE');
  assert.equal(watchState({...base, intervalMs: 5000, idleMs: 20000}), 'LIVE');
  assert.equal(watchState({...base, hasCapture: false}), 'WAITING');
  assert.equal(watchState({...base, missing: true}), 'WAITING');
  assert.equal(watchState({...base, error: 'x', idleMs: 99999}), 'ERROR');
  assert.equal(watchState({...base, error: 'x', paused: true}), 'PAUSED');
});
