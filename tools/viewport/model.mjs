import Ajv from 'ajv/dist/2020.js';
import schema from './snapshot.schema.json' with {type: 'json'};

const validate = new Ajv({allErrors: true, strict: true}).compile(schema);
const limit = (1n << 64n) - 1n;
function requireValue(ok, message) {
  if (!ok) throw new Error(message);
}
function decimal(value) {
  requireValue(BigInt(value) <= limit, 'Exact integer exceeds uint64 range.');
}
export function validateSnapshot(value) {
  requireValue(validate(value), `Unsupported or malformed snapshot: ${validate.errors?.slice(0, 3).map(e => `${e.instancePath || '/'} ${e.message}`).join('; ')}`);
  const {capture, bindings, particles, frame} = value;
  decimal(capture.tick);
  requireValue((capture.tick === '0') === (capture.phase === 'published_seeded'), 'Tick and capture phase disagree.');
  bindings.forEach((b, i) => {
    requireValue(b.id === i, 'Binding IDs must be contiguous and ordered.');
    if (i === 0) requireValue(b.role === 'root' && b.parent === -1 && b.level === 0, 'Invalid conceptual root.');
    else {
      requireValue(b.role !== 'root' && b.parent >= 0 && b.parent < i, 'Invalid binding ancestry.');
      requireValue(b.level === bindings[b.parent].level + (b.role === 'primary_step' ? 1 : 0), 'Conceptual level disagrees with explicit roles.');
      if (b.role !== 'primary_step') requireValue(b.units === bindings[b.parent].units, 'Compression/naming must inherit units.');
    }
  });
  const ids = new Set(), slots = new Set();
  for (const p of particles) {
    requireValue(!ids.has(p.id) && !slots.has(p.slot), 'Duplicate particle identity or slot.');
    ids.add(p.id); slots.add(p.slot);
    requireValue(p.id.startsWith(`${capture.study}/`) && p.aggregate.startsWith(`${capture.study}/`), 'Mixed study occurrence identities.');
    requireValue(bindings[p.binding] && p.level === bindings[p.binding].level, 'Missing or inconsistent conceptual binding.');
    requireValue([...p.position, p.radius, p.mass].every(v => v === null || Math.fround(v) === v), 'Particle scalars must preserve the native f32 representation.');
    decimal(p.exact_mass);
  }
  const focalIds = new Set();
  let total = 0, target = [0, 0, 0];
  for (const g of frame.focal_fields) {
    const id = `${g.kind}:${g.value}`;
    requireValue(!focalIds.has(id), 'Duplicate focal field.');
    focalIds.add(id);
    if (!total) target = [...g.position];
    const next = total + g.mass;
    target = target.map((v, i) => v + (g.position[i] - v) * (g.mass / next));
    total = next;
  }
  if (!total) target = frame.reference.map(v => -v);
  requireValue(frame.target_kind === (total ? 'captured_focal_centroid' : 'legacy_origin'), 'Focal target kind disagrees with selected fields.');
  requireValue(target.every((v, i) => Number.isFinite(v) && Math.abs(v - frame.camera_target[i]) <= 1e-12 * Math.max(1, Math.abs(v))), 'Camera target disagrees with captured focal fields.');
  return value;
}
export function parseSnapshot(text) { return validateSnapshot(JSON.parse(text)); }
export function rangesFor(snapshot) {
  const ranges = new Map();
  for (const p of snapshot.particles) {
    if (!ranges.has(p.level)) ranges.set(p.level, {min: null, max: null, known: 0, unknown: 0, total: 0});
    const r = ranges.get(p.level); ++r.total;
    if (p.mass === null) ++r.unknown;
    else {
      r.min = r.min === null ? p.mass : Math.min(r.min, p.mass);
      r.max = r.max === null ? p.mass : Math.max(r.max, p.mass); ++r.known;
    }
  }
  return ranges;
}
// A sequential spectrum; numeric RGB is also used directly by the legends.
export function massColour(mass, range) {
  if (mass === null) return '#9ba4b4';
  const t = range.min === range.max ? 0.5 : (mass - range.min) / (range.max - range.min);
  const stops = [[70, 100, 170], [32, 139, 163], [108, 190, 122], [250, 225, 70]];
  const x = Math.max(0, Math.min(1, t)) * (stops.length - 1);
  const i = Math.min(stops.length - 2, Math.floor(x)), f = x - i;
  return '#' + stops[i].map((v, c) => Math.round(v + (stops[i + 1][c] - v) * f).toString(16).padStart(2, '0')).join('');
}
export function selectParticles(snapshot, {level = '', exposure = '', text = ''} = {}) {
  const needle = text.toLocaleLowerCase();
  const matched = snapshot.particles.filter(p => (level === '' || p.level === Number(level)) &&
    (!exposure || p.exposure === exposure) && (!needle || `${p.id} ${p.definition} ${p.label}`.toLocaleLowerCase().includes(needle)));
  return {matched, drawable: matched.filter(p => p.radius !== null && p.radius > 0),
    unavailable: matched.filter(p => p.radius === null).length,
    zero: matched.filter(p => p.radius === 0).length};
}
export function boundsFor(particles) {
  if (!particles.length) return {centre: [0, 0, 0], radius: 1};
  const min = [Infinity, Infinity, Infinity], max = [-Infinity, -Infinity, -Infinity];
  for (const p of particles) for (let i = 0; i < 3; ++i) {
    min[i] = Math.min(min[i], p.position[i] - (p.radius ?? 0));
    max[i] = Math.max(max[i], p.position[i] + (p.radius ?? 0));
  }
  const centre = min.map((v, i) => v + (max[i] - v) / 2);
  return {centre, radius: Math.max(...max.map((v, i) => v - centre[i]))};
}
