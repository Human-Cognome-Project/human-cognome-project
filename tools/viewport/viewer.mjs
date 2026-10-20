import * as THREE from 'three';
import {OrbitControls} from 'three/addons/controls/OrbitControls.js';
import {parseSnapshot, rangesFor, massColour, selectParticles, boundsFor} from './model.mjs';

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
function frame(particles, target = null) {
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
    highlight.position.fromArray(p.position); highlight.scale.setScalar(p.radius ?? 0);
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
      transform.position.fromArray(p.position); transform.scale.setScalar(p.radius); transform.updateMatrix();
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
  if (selection && !matched.includes(selection)) inspect(null); else inspect(selection);
  buildMs = performance.now() - start; metrics(); render();
}
function deepFreeze(value) { if (value && typeof value === 'object') { Object.values(value).forEach(deepFreeze); Object.freeze(value); } return value; }
function replaceSnapshot(next) {
  snapshot = deepFreeze(next); selection = null; ranges = rangesFor(snapshot);
  $('capture-title').textContent = snapshot.capture.label || 'Untitled study'; $('capture-meta').replaceChildren();
  for (const [label, value] of Object.entries({'Schema': 'HCP snapshot v1', Study: snapshot.capture.study,
    Generation: snapshot.capture.generation, Tick: `${snapshot.capture.tick} · ${snapshot.capture.phase}`,
    'Double frame reference': snapshot.frame.reference.join(', '), 'Captured camera target': snapshot.frame.camera_target.join(', '),
    Coordinates: 'Reference-local · operative model units'})) row($('capture-meta'), label, value);
  $('level').replaceChildren(new Option('All captured levels', ''));
  for (const level of [...ranges.keys()].sort((a, b) => a - b)) $('level').add(new Option(`LoD ${level}`, String(level)));
  $('exposure').value = ''; $('search').value = ''; $('empty').hidden = true;
  $('frame-all').disabled = false; $('focus').disabled = false;
  legends(); updateDisplay(); frame(snapshot.particles);
  $('frame-note').textContent = `STATIC CAPTURE · tick ${snapshot.capture.tick} · actual model radii · ${snapshot.particles.length} objects`;
}
$('file').addEventListener('change', async event => {
  const serial = ++loadSerial, file = event.target.files?.[0];
  if (!file) return;
  try {
    const text = await file.text();
    const start = performance.now(), next = parseSnapshot(text), elapsed = performance.now() - start;
    if (serial !== loadSerial) return;
    if (!renderer) throw new Error('WebGL renderer is unavailable.');
    parseMs = elapsed; replaceSnapshot(next); error();
  } catch (e) { if (serial === loadSerial) error(`Could not load ${file.name}: ${e.message}${snapshot ? ' Previous capture remains displayed.' : ''}`); }
  event.target.value = '';
});
for (const id of ['level', 'exposure']) $(id).addEventListener('change', updateDisplay);
$('search').addEventListener('input', updateDisplay);
$('frame-all').addEventListener('click', () => snapshot && frame(matched));
$('frame-selection').addEventListener('click', () => selection && frame([selection]));
$('focus').addEventListener('click', () => snapshot && frame(snapshot.particles, snapshot.frame.camera_target));
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
// Read-only diagnostics for reproducible browser verification, no execution API.
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
  camera: {position: camera.position.toArray(), target: controls?.target.toArray()},
  metrics: {parse_ms: parseMs, build_ms: buildMs, render_submit_ms: renderMs},
  renderer: renderer?.getContext().getParameter(renderer.getContext().VERSION) ?? null})};
