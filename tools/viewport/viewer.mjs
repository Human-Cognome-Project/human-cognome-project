import * as THREE from 'three';
import {OrbitControls} from 'three/addons/controls/OrbitControls.js';
import {parseSnapshot, rangesFor, massColour, selectParticles, boundsFor, classifyUpdate, referenceShift, pollInterval, fileStamp, resolveSelection, watchState, defaultPollMs} from './model.mjs';

const $ = id => document.getElementById(id);
const space = $('space');
const scene = new THREE.Scene();
const camera = new THREE.PerspectiveCamera(45, 1, 0.001, 10000);
camera.position.set(5, 4, 7);
let renderer, controls, snapshot = null, ranges, mesh, drawn = [], selection = null, matched = [];
let parseMs = 0, buildMs = 0, renderMs = 0, renderQueued = false, loadSerial = 0;
const geometry = new THREE.SphereGeometry(1, 32, 24);
const material = new THREE.MeshBasicMaterial({color: 0xffffff});
// A separate wire selection indicator adds no particle and carries no mass.
const highlight = new THREE.Mesh(geometry, new THREE.MeshBasicMaterial({color: 0xffffff, wireframe: true, transparent: true, opacity: 0.35}));
highlight.visible = false;
scene.add(highlight);
function error(message = '') { $('error').textContent = message; $('error').hidden = !message; }
function row(parent, label, value) {
  const dt = document.createElement('dt'), dd = document.createElement('dd');
  dt.textContent = label; dd.textContent = value; parent.append(dt, dd);
}
function metrics() {
  $('metrics').replaceChildren();
  for (const [label, value] of [['Parse + validate', parseMs], ['Build display', buildMs], ['Last render submission', renderMs]])
    row($('metrics'), label, `${value.toFixed(2)} ms`);
}
function render() {
  if (!renderer || renderQueued) return;
  renderQueued = true;
  requestAnimationFrame(() => {
    renderQueued = false;
    const start = performance.now();
    renderer.render(scene, camera);
    renderMs = performance.now() - start;
    metrics();
  });
}
function resize() {
  if (!renderer) return;
  const width = space.clientWidth, height = space.clientHeight;
  renderer.setSize(width, height);
  camera.aspect = width / Math.max(1, height); camera.updateProjectionMatrix(); render();
}
// Display frame: camera and scene use local positions plus `shift` (reference minus the display reference, a per-axis
// double difference), so a reference change between captures never reads as motion. No absolute origin is added.
let displayReference = [0, 0, 0], shift = [0, 0, 0];
const place = point => shift.every(v => v === 0) ? point : point.map((v, i) => v + shift[i]);
function frame(particles, target = null) {
  particles = particles.map(p => ({position: place(p.position), radius: p.radius}));
  const bounds = boundsFor(particles), centre = target ?? bounds.centre;
  let radius = 0.001;
  for (const p of particles) radius = Math.max(radius, Math.hypot(...p.position.map((v, i) => v - centre[i])) + (p.radius ?? 0));
  const distance = radius * 1.8 / Math.sin(THREE.MathUtils.degToRad(camera.fov / 2)) / Math.min(1, camera.aspect);
  const direction = camera.position.clone().sub(controls.target).normalize();
  if (!direction.lengthSq()) direction.set(1, 0.7, 1).normalize();
  controls.target.fromArray(centre);
  camera.position.copy(controls.target).addScaledVector(direction, distance);
  camera.near = Math.max(distance / 100000, 1e-7);
  camera.far = Math.max(distance * 1000, radius * 1000);
  camera.updateProjectionMatrix(); controls.update(); render();
}
function inspect(p) {
  selection = p;
  $('inspection').hidden = !p; $('frame-selection').disabled = !p;
  $('details').replaceChildren();
  if (p) {
    const binding = snapshot.bindings[p.binding];
    for (const [label, value] of Object.entries({Label: p.label || 'Not supplied', Occurrence: p.id,
      Definition: p.definition, 'Local position': p.position.join(', '), Radius: p.radius ?? 'Unavailable',
      'Numerical mass': p.mass ?? 'Unavailable', 'Exact structural mass': p.exact_mass,
      'Conceptual LoD': p.level, Binding: `${p.binding} · ${binding.role} · ${binding.units} units`,
      Exposure: p.exposure, 'Aggregate context': p.aggregate})) row($('details'), label, value);
    highlight.visible = p.radius !== null && p.radius > 0 && drawn.includes(p);
    highlight.position.fromArray(place(p.position)); highlight.scale.setScalar(p.radius ?? 0);
  } else highlight.visible = false;
  for (const button of $('objects').children) button.setAttribute('aria-pressed', String(button.dataset.id === p?.id));
  render();
}
function legends() {
  $('legends').replaceChildren();
  for (const [level, range] of [...ranges].sort(([a], [b]) => a - b)) {
    const block = document.createElement('div'); block.className = 'legend';
    const title = document.createElement('strong'); title.textContent = `LoD ${level} · ${range.total} captured`;
    const bar = document.createElement('div'); bar.className = 'spectrum';
    bar.style.background = !range.known ? '#9ba4b4' : range.min === range.max ? massColour(range.min, range) :
      `linear-gradient(to right, ${[0, 1 / 3, 2 / 3, 1].map(t => massColour(range.min + t * (range.max - range.min), range)).join(',')})`;
    const labels = document.createElement('div'); labels.className = 'range';
    for (const v of [range.min, range.max]) { const span = document.createElement('span'); span.textContent = v ?? 'Unavailable'; labels.append(span); }
    block.append(title, bar, labels);
    const note = document.createElement('p'); note.className = 'muted';
    note.textContent = `${range.min === range.max && range.known ? 'Equal mass: midpoint colour. ' : ''}${range.unknown} unavailable (grey).`;
    block.append(note); $('legends').append(block);
  }
}
function updateDisplay() {
  if (!snapshot || !renderer) return;
  const start = performance.now();
  const result = selectParticles(snapshot, {level: $('level').value, exposure: $('exposure').value, text: $('search').value});
  matched = result.matched; drawn = result.drawable;
  if (mesh) { scene.remove(mesh); mesh.dispose(); mesh = null; }
  if (drawn.length) {
    mesh = new THREE.InstancedMesh(geometry, material, drawn.length);
    const transform = new THREE.Object3D(), colour = new THREE.Color();
    drawn.forEach((p, i) => {
      transform.position.fromArray(place(p.position)); transform.scale.setScalar(p.radius); transform.updateMatrix();
      mesh.setMatrixAt(i, transform.matrix); mesh.setColorAt(i, colour.set(massColour(p.mass, ranges.get(p.level))));
    });
    mesh.computeBoundingSphere(); scene.add(mesh);
  }
  $('counts').textContent = `${snapshot.particles.length} total · ${matched.length} matched · ${drawn.length} drawable`;
  $('omissions').textContent = `${snapshot.particles.length - matched.length} filtered out · ${result.unavailable} missing extent · ${result.zero} zero radius. Camera culling is separate; no display cap.`;
  $('objects').replaceChildren();
  for (const p of matched) {
    const button = document.createElement('button'); button.dataset.id = p.id; button.setAttribute('role', 'listitem');
    button.textContent = `${p.label || 'Unlabelled'} · LoD ${p.level}`;
    const small = document.createElement('small'); small.textContent = `${p.id} · ${p.radius === null ? 'extent unavailable' : p.radius === 0 ? 'zero radius' : p.exposure}`;
    button.append(small); button.addEventListener('click', () => inspect(p)); $('objects').append(button);
  }
  // Re-resolve by occurrence ID: a live update supplies new objects for the same identity.
  inspect(selection ? resolveSelection(matched, selection.id) : null);
  buildMs = performance.now() - start; metrics(); render();
}
function deepFreeze(value) { if (value && typeof value === 'object') { Object.values(value).forEach(deepFreeze); Object.freeze(value); } return value; }
function replaceSnapshot(next, keep = false) {
  const level = $('level').value;
  const fixed = keep ? displayReference : next.frame.reference; // a new capture starts a new display frame
  snapshot = deepFreeze(next); ranges = rangesFor(snapshot); if (!keep) selection = null;
  displayReference = [...fixed]; shift = referenceShift(displayReference, snapshot.frame.reference);
  $('capture-title').textContent = snapshot.capture.label || 'Untitled study'; $('capture-meta').replaceChildren();
  for (const [label, value] of Object.entries({'Schema': 'HCP snapshot v1', Study: snapshot.capture.study,
    Generation: snapshot.capture.generation, Tick: `${snapshot.capture.tick} · ${snapshot.capture.phase}`,
    'Double frame reference': snapshot.frame.reference.join(', '), 'Captured camera target': snapshot.frame.camera_target.join(', '),
    Coordinates: 'Reference-local · operative model units'})) row($('capture-meta'), label, value);
  if (shift.some(v => v !== 0)) row($('capture-meta'), 'Display offset', `${shift.join(', ')} (camera follows reference changes)`);
  $('level').replaceChildren(new Option('All captured levels', ''));
  for (const level of [...ranges.keys()].sort((a, b) => a - b)) $('level').add(new Option(`LoD ${level}`, String(level)));
  // A live update inside one session+study keeps filters and camera; a new capture clears and frames.
  if (keep) { if (level !== '' && ranges.has(Number(level))) $('level').value = level; }
  else { $('exposure').value = ''; $('search').value = ''; }
  $('empty').hidden = true; $('frame-all').disabled = false; $('focus').disabled = false;
  legends(); updateDisplay(); if (!keep) frame(snapshot.particles);
  ui();
}
// Live watching: the page only looks at a user-chosen folder's latest.json at its own interval.
// Each poll re-resolves the name, so the native writer's atomic rename replacement is always seen whole.
const pollKey = 'hcp-viewport-poll-ms', canWatch = typeof window.showDirectoryPicker === 'function';
let watch = null, ticker = 0;
const store = {get: () => { try { return localStorage.getItem(pollKey); } catch { return null; } },
  set: v => { try { localStorage.setItem(pollKey, String(v)); } catch { /* optional convenience */ } }};
const clock = ms => ms < 60000 ? `${Math.floor(ms / 1000)} s` : `${Math.floor(ms / 60000)} min ${Math.floor(ms % 60000 / 1000)} s`;
function ui() {
  const w = watch, now = Date.now();
  const mode = w ? watchState({paused: w.paused, error: w.error, missing: w.missing, hasCapture: !!w.last, idleMs: now - w.changedAt, intervalMs: w.interval}) : 'STATIC';
  $('mode').textContent = mode; $('mode').dataset.mode = mode;
  $('pause').disabled = $('stop').disabled = !w; $('pause').textContent = w?.paused ? 'Resume' : 'Pause';
  $('live-status').replaceChildren();
  if (w) for (const [label, value] of Object.entries({Folder: w.name, State: mode,
    'Latest file': w.missing ? 'latest.json not found; waiting' : w.checks ? 'latest.json' : 'Not yet checked',
    'Applied tick': w.last ? `${w.last.capture.tick} · ${w.last.capture.phase}` : 'None yet',
    'Last change': w.changedAt ? new Date(w.changedAt).toLocaleTimeString() : 'None yet',
    'Since last change': w.changedAt ? clock(now - w.changedAt) : 'None yet',
    'Last check': w.checkedAt ? new Date(w.checkedAt).toLocaleTimeString() : 'None yet',
    'Checks · captures applied': `${w.checks} · ${w.applied}`})) row($('live-status'), label, value);
  if (!snapshot) return;
  const base = `tick ${snapshot.capture.tick} · actual model radii · ${snapshot.particles.length} objects`;
  $('frame-note').textContent = !w ? `STATIC CAPTURE · ${base}` : !w.last ? `STATIC CAPTURE (opened file) · ${base} · watching ${w.name}, ${mode.toLowerCase()}` :
    `${mode} CAPTURE · ${base} · polling ${w.name}/latest.json every ${w.interval} ms · colour ranges per capture`;
}
function schedule(w) { clearTimeout(w.timer); w.timer = setTimeout(() => poll(w), w.interval); }
async function poll(w) {
  if (watch !== w || w.paused || w.busy) return;
  w.busy = true; ++w.checks; w.checkedAt = Date.now();
  let stamped = false;
  try {
    let file;
    try { file = await (await w.src.getFileHandle('latest.json')).getFile(); w.missing = false; }
    catch (e) { if (e?.name !== 'NotFoundError') throw e; w.missing = true; w.error = null; w.stamp = null; w.racing = 0; if (watch === w) error(); return; }
    if (fileStamp(file) === w.stamp) { w.racing = 0; return; }
    const text = await file.text();
    w.racing = 0;
    if (watch !== w || w.paused) return;
    w.stamp = fileStamp(file); stamped = true;
    const start = performance.now(), next = parseSnapshot(text), elapsed = performance.now() - start;
    if (!renderer) throw new Error('WebGL renderer is unavailable.');
    // The displayed capture is the baseline, so a (re)started watch of the same session and study keeps the person's frame.
    const kind = classifyUpdate(snapshot, next);
    if (kind !== 'same') { parseMs = elapsed; replaceSnapshot(next, kind === 'update'); }
    if (kind !== 'same' || !w.last) { w.last = snapshot; w.changedAt = Date.now(); ++w.applied; }
    w.error = null; error();
  } catch (e) {
    // A failed read forgets the stamp so the next check re-reads (and clears the error if the file is unchanged);
    // a parse failure keeps it, so an invalid file stays reported without being re-parsed.
    // A read that races the writer's replacement can fail transiently and is retried.
    // Three consecutive such failures stay silent; the fourth is reported like any read error.
    if (!stamped) w.stamp = null;
    if (e?.name === 'NotReadableError' && ++w.racing <= 3) return;
    if (watch === w) {
      w.error = e.message;
      error(`Could not read latest.json: ${e.message}${w.last ? ` Capture at tick ${w.last.capture.tick} remains displayed.` : snapshot ? ' Previous capture remains displayed.' : ''} Still watching.`);
    }
  } finally { if (watch === w) { w.busy = false; if (!w.paused) schedule(w); ui(); } }
}
function stopWatch() {
  if (!watch) return;
  clearTimeout(watch.timer); clearInterval(ticker); watch = null; error(); ui();
}
// source: anything with getFileHandle(name) -> {getFile()}; the picker's directory handle in normal use.
function startWatch(source, name) {
  stopWatch(); ticker = setInterval(ui, 1000);
  const w = watch = {src: source, name, interval: pollInterval($('interval').value), paused: false, busy: false, racing: 0, timer: 0,
    stamp: null, last: null, checks: 0, applied: 0, changedAt: 0, checkedAt: 0, missing: false, error: null};
  error(); ui(); poll(w);
}
$('interval').value = pollInterval(store.get());
$('interval').addEventListener('change', () => {
  const v = pollInterval($('interval').value, watch?.interval ?? defaultPollMs);
  $('interval').value = v; store.set(v);
  if (watch) { watch.interval = v; if (!watch.paused && !watch.busy) schedule(watch); ui(); }
});
$('pause').addEventListener('click', () => {
  if (!watch) return;
  watch.paused = !watch.paused; watch.racing = 0; clearTimeout(watch.timer);
  if (!watch.paused) poll(watch); else ui();
});
$('stop').addEventListener('click', stopWatch);
if (canWatch) $('watch').addEventListener('click', async () => {
  try { const dir = await window.showDirectoryPicker({mode: 'read'}); startWatch(dir, dir.name); }
  catch (e) { if (e?.name !== 'AbortError') error(`Could not watch folder: ${e.message}`); }
});
else {
  $('watch').disabled = true;
  $('live-note').textContent = 'Watching a folder needs the File System Access API (Chromium-based browsers on a secure page, including a local file). This browser does not provide it. Open snapshot still works.';
  $('watch').title = 'Not supported in this browser; Open snapshot still works.';
}
$('file').addEventListener('change', async event => {
  const serial = ++loadSerial, file = event.target.files?.[0];
  if (!file) return;
  try {
    const text = await file.text();
    const start = performance.now(), next = parseSnapshot(text), elapsed = performance.now() - start;
    if (serial !== loadSerial) return;
    if (!renderer) throw new Error('WebGL renderer is unavailable.');
    parseMs = elapsed; stopWatch(); replaceSnapshot(next); error();
  } catch (e) { if (serial === loadSerial) error(`Could not load ${file.name}: ${e.message}${snapshot ? ' Previous capture remains displayed.' : ''}`); }
  event.target.value = '';
});
for (const id of ['level', 'exposure']) $(id).addEventListener('change', updateDisplay);
$('search').addEventListener('input', updateDisplay);
$('frame-all').addEventListener('click', () => snapshot && frame(matched));
$('frame-selection').addEventListener('click', () => selection && frame([selection]));
$('focus').addEventListener('click', () => snapshot && frame(snapshot.particles, place(snapshot.frame.camera_target)));
try {
  renderer = new THREE.WebGLRenderer({antialias: true, alpha: true});
  renderer.setPixelRatio(Math.min(devicePixelRatio, 2)); renderer.outputColorSpace = THREE.SRGBColorSpace;
  space.prepend(renderer.domElement); controls = new OrbitControls(camera, renderer.domElement);
  controls.addEventListener('change', render); new ResizeObserver(resize).observe(space); resize();
  const raycaster = new THREE.Raycaster(), pointer = new THREE.Vector2(); let down;
  renderer.domElement.addEventListener('pointerdown', e => { down = [e.clientX, e.clientY]; });
  renderer.domElement.addEventListener('pointerup', e => {
    if (!snapshot || !mesh || !down || Math.hypot(e.clientX - down[0], e.clientY - down[1]) > 4 || e.button !== 0) return;
    const rect = renderer.domElement.getBoundingClientRect();
    pointer.set((e.clientX - rect.left) / rect.width * 2 - 1, -(e.clientY - rect.top) / rect.height * 2 + 1);
    raycaster.setFromCamera(pointer, camera); const hit = raycaster.intersectObject(mesh)[0];
    inspect(hit ? drawn[hit.instanceId] : null);
  });
} catch (e) { error(`WebGL could not start: ${e.message}`); }
// Read-only diagnostics, plus a test-only watchWith that polls a supplied object; no engine or network access.
window.HCPViewport = {inspect: () => ({snapshot: snapshot ? structuredClone(snapshot) : null,
  matched: matched.map(p => p.id), drawn: drawn.map((p, i) => {
    const matrix = new THREE.Matrix4(); mesh.getMatrixAt(i, matrix);
    const position = new THREE.Vector3(), scale = new THREE.Vector3(), rotation = new THREE.Quaternion();
    matrix.decompose(position, rotation, scale);
    const colour = new THREE.Color(); mesh.getColorAt(i, colour);
    const projected = position.clone().project(camera), rect = renderer.domElement.getBoundingClientRect();
    return {id: p.id, position: position.toArray(), radius: scale.x, colour: '#' + colour.getHexString(THREE.SRGBColorSpace),
      screen: {x: rect.left + (projected.x + 1) * rect.width / 2, y: rect.top + (1 - projected.y) * rect.height / 2, z: projected.z}};
  }), selected: selection?.id ?? null, ranges: ranges ? [...ranges] : [],
  camera: {position: camera.position.toArray(), target: controls?.target.toArray()}, display: {reference: [...displayReference], shift: [...shift]},
  metrics: {parse_ms: parseMs, build_ms: buildMs, render_submit_ms: renderMs},
  watch: {supported: canWatch, secure: isSecureContext, mode: $('mode').textContent, active: !!watch, paused: watch?.paused ?? null,
    interval: watch?.interval ?? null, checks: watch?.checks ?? 0, applied: watch?.applied ?? 0, error: watch?.error ?? null, missing: watch?.missing ?? null},
  renderer: renderer?.getContext().getParameter(renderer.getContext().VERSION) ?? null}),
  // Test-only: browser automation cannot operate the native folder picker, so it supplies a source object
  // with the same getFileHandle(name) -> {getFile()} shape. Only the supplied object is read; no engine access.
  watchWith: (source, name = 'test source') => startWatch(source, name)};
