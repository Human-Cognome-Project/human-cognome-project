import {chromium} from 'playwright';
import {readFile, mkdir, writeFile} from 'node:fs/promises';
import {resolve} from 'node:path';
import {pathToFileURL} from 'node:url';
import {createHash} from 'node:crypto';
import {existsSync} from 'node:fs';
import assert from 'node:assert/strict';
import {massColour, rangesFor, boundsFor} from '../model.mjs';

const root = resolve(import.meta.dirname, '..');
const snapshotPath = resolve(process.argv[2] || `${root}/examples/letter-study.json`);
const resultsPath = resolve(process.argv[3] || `${root}/tests/results`);
const inMemory = process.env.HCP_VIEWPORT_IN_MEMORY === '1';
await mkdir(resultsPath, {recursive: true});
const nativeText = await readFile(snapshotPath, 'utf8'), native = JSON.parse(nativeText);
assert.equal(native.source.kind, 'retained_cold_to_warm_to_active');
const browser = await chromium.launch({executablePath: process.env.HCP_CHROMIUM || (existsSync('/usr/bin/chromium') ? '/usr/bin/chromium' : chromium.executablePath()), headless: true,
  args: ['--no-sandbox', '--use-angle=swiftshader', '--enable-unsafe-swiftshader']});
const context = await browser.newContext({offline: true, viewport: {width: 1440, height: 1000}, deviceScaleFactor: 1});
const page = await context.newPage(), errors = [], externalRequests = [];
page.on('pageerror', e => errors.push(e.message));
page.on('request', r => { if (/^https?:/.test(r.url())) externalRequests.push(r.url()); });
let checks = 0;
const check = (ok, label) => { assert.ok(ok, label); checks++; };
const state = () => page.evaluate(() => window.HCPViewport.inspect());
async function load(value, name = 'fixture.json') {
  await page.locator('#file').setInputFiles({name, mimeType: 'application/json', buffer: Buffer.from(typeof value === 'string' ? value : JSON.stringify(value))});
}
async function waitStudy(study, generation, tick) {
  await page.waitForFunction(({study, generation, tick}) => {
    const c = window.HCPViewport?.inspect().snapshot?.capture;
    return c?.study === study && c.generation === generation && c.tick === tick;
  }, {study, generation, tick});
  await page.evaluate(() => new Promise(requestAnimationFrame));
}
async function openViewport(target) {
  if (inMemory) {
    // For managed cloud browsers that disallow file navigation. Supply the
    // exact page assets as document content without changing browser policy.
    const html = await readFile(`${root}/index.html`, 'utf8');
    const css = await readFile(`${root}/viewport.css`, 'utf8');
    await target.setContent(html.replace('<link rel="stylesheet" href="viewport.css">', `<style>${css}</style>`)
      .replace('<script defer src="vendor/viewer.bundle.js"></script>', ''));
    await target.addScriptTag({content: await readFile(`${root}/vendor/viewer.bundle.js`, 'utf8')});
  } else await target.goto(pathToFileURL(`${root}/index.html`).href);
  await target.waitForFunction(() => window.HCPViewport);
}
try {
  await openViewport(page);
  check(await page.locator('#error').isHidden(), 'offline WebGL starts');
  await load(nativeText, 'letter-study.json');
  await waitStudy(native.capture.study, native.capture.generation, native.capture.tick);
  let current = await state();
  check(current.drawn.length === native.particles.length, 'all real native particles drawn');
  const ranges = rangesFor(native);
  for (const p of native.particles) {
    const actual = current.drawn.find(v => v.id === p.id);
    assert.deepEqual(actual.position, p.position); checks++;
    check(actual.radius === p.radius, 'actual native radius matches instance transform');
    check(actual.colour === massColour(p.mass, ranges.get(p.level)), 'instance colour agrees with fixed mass mapping');
  }
  check(current.snapshot.frame.reference.every((v, i) => v === native.frame.reference[i]), 'double reference separate');
  const nativeMetrics = {...current.metrics};
  await page.screenshot({path: `${resultsPath}/letter-study.png`});
  const digest = text => createHash('sha256').update(text).digest('hex');
  const before = digest(JSON.stringify(current.snapshot)), legendBefore = await page.locator('#legends').textContent();
  await page.locator('#level').selectOption('1');
  current = await state();
  check(current.drawn.length === native.particles.filter(p => p.level === 1).length, 'level filtering');
  check(await page.locator('#legends').textContent() === legendBefore, 'legends unchanged by filter');
  await page.locator('#objects button').first().click();
  current = await state();
  check(!!current.selected && await page.locator('#inspection').isVisible(), 'click list inspection');
  await page.locator('#frame-selection').click();
  check((await state()).camera.target.every((v, i) => v === native.particles.find(p => p.id === current.selected).position[i]), 'frame selection');
  await page.locator('#level').selectOption('');
  await page.locator('#frame-all').click();
  const canvas = await page.locator('canvas').boundingBox();
  const beforeOrbit = (await state()).camera;
  await page.mouse.move(canvas.x + canvas.width * 0.65, canvas.y + canvas.height * 0.4);
  await page.mouse.down(); await page.mouse.move(canvas.x + canvas.width * 0.65 + 70, canvas.y + canvas.height * 0.4 + 20, {steps: 5}); await page.mouse.up();
  check(JSON.stringify((await state()).camera.position) !== JSON.stringify(beforeOrbit.position), 'orbit changes camera');
  const beforePan = (await state()).camera;
  await page.mouse.move(canvas.x + canvas.width * 0.6, canvas.y + canvas.height * 0.3);
  await page.mouse.down({button: 'right'}); await page.mouse.move(canvas.x + canvas.width * 0.6 + 30, canvas.y + canvas.height * 0.3 + 20, {steps: 3}); await page.mouse.up({button: 'right'});
  check(JSON.stringify((await state()).camera.target) !== JSON.stringify(beforePan.target), 'pan changes camera target');
  const beforeZoom = (await state()).camera;
  await page.mouse.wheel(0, -120);
  check(JSON.stringify((await state()).camera.position) !== JSON.stringify(beforeZoom.position), 'zoom changes camera distance');
  await page.locator('#focus').click();
  check((await state()).camera.target.every((v, i) => v === native.frame.camera_target[i]), 'study focus uses captured target');
  check(digest(JSON.stringify((await state()).snapshot)) === before, 'camera and filters preserve captured model');
  await page.locator('#frame-all').click();
  current = await state();
  const candidate = current.drawn.find(p => p.screen.z > -1 && p.screen.z < 1 && p.screen.y > canvas.y + 70 && p.screen.y < canvas.y + canvas.height - 90);
  assert.ok(candidate, 'visible sphere for picking');
  await page.mouse.click(canvas.x + 10, canvas.y + canvas.height / 2);
  check((await state()).selected === null, 'empty-space picking clears selection');
  await page.mouse.click(candidate.screen.x, candidate.screen.y);
  check(!!(await state()).selected, 'sphere raycast selects captured occurrence');
  await load('{invalid', 'invalid.json');
  await page.waitForFunction(() => !document.getElementById('error').hidden);
  check(digest(JSON.stringify((await state()).snapshot)) === before, 'invalid file retains identified prior snapshot');

  // Edge cases are mutations of a capture solely for UI validation, not native evidence.
  const synthetic = structuredClone(native);
  synthetic.capture = {...synthetic.capture, generation: '12345678-1234-1234-1234-123456789abc', label: 'UI validation fixture'};
  synthetic.particles[0].radius = null; synthetic.particles[1].radius = 0;
  synthetic.particles[2].mass = null; synthetic.particles[2].exact_mass = '9223372036854775811';
  synthetic.particles[2].label = '<img src=x onerror=alert(1)> "é"\n';
  await load(synthetic); await waitStudy(synthetic.capture.study, synthetic.capture.generation, synthetic.capture.tick);
  current = await state();
  check(current.drawn.length === native.particles.length - 2, 'missing/zero extent not invented');
  check((await page.locator('#omissions').textContent()).includes('1 missing extent · 1 zero radius'), 'visible omitted counts');
  check((await page.locator('#level').inputValue()) === '' && !current.selected, 'generation replacement clears filter/selection');
  await page.locator('#search').fill(synthetic.particles[2].id);
  await page.locator('#objects button').first().click();
  check((await page.locator('#details').textContent()).includes('9223372036854775811'), 'large exact integer inspected');
  check(await page.locator('#objects img').count() === 0, 'labels remain text');
  const unknown = (await state()).drawn[0];
  check(unknown.colour === '#9ba4b4', 'unknown mass grey');
  const empty = structuredClone(synthetic); empty.capture.generation = '22345678-1234-1234-1234-123456789abc'; empty.particles = [];
  await load(empty); await waitStudy(empty.capture.study, empty.capture.generation, empty.capture.tick);
  check((await state()).drawn.length === 0 && await page.locator('#objects button').count() === 0, 'old generation objects fully removed');
  const unsupported = structuredClone(empty); unsupported.version = 999;
  await load(unsupported); await page.waitForFunction(() => !document.getElementById('error').hidden);
  check((await state()).snapshot.version === 1, 'unsupported version rejected');

  // Live watching. The captures are the retained native examples (tick 0 and 8 of one study); "UI fixture" copies
  // mutate tick/positions/identities solely to exercise the viewer and are not native evidence. The native folder
  // picker cannot be driven by automation, so a page-side source with the same getFileHandle()/getFile() shape stands in.
  const sleep = ms => new Promise(resolve => setTimeout(resolve, ms));
  const tick0 = JSON.parse(await readFile(`${root}/examples/letter-study-tick0.json`, 'utf8'));
  const tick8 = JSON.parse(await readFile(`${root}/examples/letter-study.json`, 'utf8'));
  const uiFixture = (tick, shift = 0, drop = null) => {
    const f = structuredClone(tick8); f.capture.tick = String(tick); f.capture.label = 'UI validation fixture';
    for (const p of f.particles) p.position[0] = Math.fround(p.position[0] + shift);
    f.particles = f.particles.filter(p => p.id !== drop); return f;
  };
  const picker = await page.evaluate(() => ({show_directory_picker: typeof window.showDirectoryPicker, secure_context: isSecureContext, protocol: location.protocol}));
  if (!inMemory) check(picker.show_directory_picker === 'function' && picker.secure_context && picker.protocol === 'file:', 'showDirectoryPicker is exposed on a direct file:// page in Chromium');
  check((await page.locator('#watch').isEnabled()) === (picker.show_directory_picker === 'function'), 'Watch folder control follows API support');
  check((await page.locator('#mode').textContent()) === 'STATIC' && (await page.locator('#frame-note').textContent()).startsWith('STATIC CAPTURE ·'), 'opened files are labelled static');
  await page.evaluate(() => {
    const s = window.__src = {text: null, mod: 1e12, reads: 0, calls: []};
    s.getFileHandle = async name => {
      s.calls.push(performance.now());
      if (name !== 'latest.json' || s.text === null) throw new DOMException('absent', 'NotFoundError');
      const text = s.text, lastModified = s.mod;
      return {getFile: async () => ({lastModified, size: new Blob([text]).size, text: async () => { ++s.reads; return text; }})};
    };
  });
  const src = {set: value => page.evaluate(text => { window.__src.text = text; window.__src.mod += 1000; }, typeof value === 'string' ? value : JSON.stringify(value)),
    remove: () => page.evaluate(() => { window.__src.text = null; }),
    stats: () => page.evaluate(() => ({reads: window.__src.reads, calls: window.__src.calls.length})),
    times: () => page.evaluate(() => [...window.__src.calls])};
  const waitTick = tick => page.waitForFunction(t => window.HCPViewport.inspect().snapshot?.capture.tick === t, String(tick));
  const waitMode = mode => page.waitForFunction(m => document.getElementById('mode').textContent === m, mode);
  const watchInfo = async () => (await state()).watch;
  const same = async (a, label) => check(JSON.stringify((await state()).camera) === JSON.stringify(a), label);
  const setPoll = async value => { await page.locator('#interval').fill(String(value)); await page.locator('#interval').dispatchEvent('change'); };
  await setPoll(200);
  check(await page.locator('#interval').inputValue() === '200', 'poll interval is configurable');
  await src.set(tick0);
  const previousCamera = (await state()).camera;
  await page.evaluate(() => window.HCPViewport.watchWith(window.__src, 'fixture-dir'));
  await waitTick('0'); await waitMode('LIVE');
  current = await state();
  check(current.drawn.length === tick0.particles.length && current.drawn.every(v => v.position.every((x, i) => x === tick0.particles.find(p => p.id === v.id).position[i])), 'first watched load shows the capture');
  check(JSON.stringify(current.camera.target) === JSON.stringify(boundsFor(tick0.particles).centre) && JSON.stringify(current.camera.target) !== JSON.stringify(previousCamera.target), 'first watched load frames like a new file');
  check((await page.locator('#frame-note').textContent()).startsWith('LIVE CAPTURE · tick 0') && (await page.locator('#live-status').textContent()).includes('fixture-dir'), 'footer and status identify live capture');

  // Same study: the person's filters, selection and camera survive.
  await page.locator('#level').selectOption('1'); await page.locator('#exposure').selectOption('aggregate');
  await page.locator('#objects button').first().click();
  const chosen = (await state()).selected, canvasBox = await page.locator('canvas').boundingBox();
  await page.mouse.move(canvasBox.x + canvasBox.width * 0.65, canvasBox.y + canvasBox.height * 0.4);
  await page.mouse.down(); await page.mouse.move(canvasBox.x + canvasBox.width * 0.65 + 90, canvasBox.y + canvasBox.height * 0.4 + 30, {steps: 5}); await page.mouse.up();
  const orbited = (await state()).camera, level1 = tick8.particles.filter(p => p.level === 1);
  check(!!chosen && JSON.stringify(orbited.position) !== JSON.stringify(current.camera.position), 'selected an object and orbited before the update');
  await src.set(tick8); await waitTick('8');
  current = await state();
  await same(orbited, 'same-study update preserves camera position and target');
  check(await page.locator('#level').inputValue() === '1' && await page.locator('#exposure').inputValue() === 'aggregate', 'same-study update preserves filters');
  check(current.selected === chosen && (await page.locator('#details').textContent()).includes(tick8.particles.find(p => p.id === chosen).position.join(', ')), 'selection preserved by occurrence ID and inspected at new values');
  check(current.drawn.length === level1.length && level1.every(p => current.drawn.find(v => v.id === p.id).position.every((x, i) => x === p.position[i])), 'positions updated for the filtered view');
  check((await page.locator('#capture-meta').textContent()).includes('8 · post_tick') && (await watchInfo()).applied === 2, 'tick and metadata updated');
  check(JSON.stringify(current.ranges) === JSON.stringify([...rangesFor(tick8)]) && current.drawn.every(v => v.colour === massColour(tick8.particles.find(p => p.id === v.id).mass, rangesFor(tick8).get(1))), 'colour ranges are recomputed per capture');
  check((await page.locator('#mode').textContent()) === 'LIVE' && (await page.locator('#frame-note').textContent()).startsWith('LIVE CAPTURE · tick 8') && (await page.locator('#live-status').textContent()).includes('Since last change'), 'live status shows tick and elapsed time');

  // Polling: an unchanged file is read once, at about the configured interval.
  const quiet = await src.stats(); await sleep(900);
  const after = await src.stats(), gaps = (await src.times()).slice(-(after.calls - quiet.calls + 1)).map((t, i, all) => t - all[i - 1]).slice(1).sort((a, b) => a - b);
  check(after.reads === quiet.reads && after.calls - quiet.calls >= 2, 'unchanged file is polled but not re-read or re-parsed');
  check(gaps[Math.floor(gaps.length / 2)] >= 190 && gaps[Math.floor(gaps.length / 2)] <= 500, 'poll interval honoured approximately');
  await setPoll(30);
  check(await page.locator('#interval').inputValue() === '100' && (await watchInfo()).interval === 100, 'poll interval has an enforced minimum');
  await setPoll(200);
  if (!inMemory) check(await page.evaluate(() => localStorage.getItem('hcp-viewport-poll-ms')) === '200', 'poll interval remembered locally');

  // Bad updates keep the prior capture, show the error, and recovery is automatic.
  const beforeBad = await src.stats();
  await src.set('{broken'); await waitMode('ERROR'); await sleep(700);
  check((await src.stats()).reads === beforeBad.reads + 1, 'invalid file is read once, not re-parsed every poll');
  current = await state();
  check(current.snapshot.capture.tick === '8' && current.selected === chosen && (await page.locator('#error').textContent()).includes('Capture at tick 8 remains displayed') && (await page.locator('#error').textContent()).includes('Still watching'), 'invalid update retains and identifies prior capture, selection and error');
  await same(orbited, 'invalid update leaves camera alone');
  const bad = structuredClone(tick8); bad.version = 999;
  await src.set(bad); await sleep(500);
  check((await state()).snapshot.capture.tick === '8' && (await watchInfo()).error.includes('Unsupported') && (await watchInfo()).mode === 'ERROR', 'unsupported update is refused while polling continues');
  await src.set(uiFixture(9, 0.05)); await waitTick('9'); await waitMode('LIVE');
  check(await page.locator('#error').isHidden() && (await state()).selected === chosen, 'valid capture recovers automatically');
  await same(orbited, 'recovery keeps camera');

  // A selected occurrence absent from the new capture is cleared; filters stay.
  await src.set(uiFixture(10, 0.1, chosen)); await waitTick('10');
  current = await state();
  check(current.selected === null && await page.locator('#inspection').isHidden() && !current.matched.includes(chosen) && await page.locator('#level').inputValue() === '1', 'selection cleared only because its ID is gone');
  await same(orbited, 'removal keeps camera');

  // Missing latest.json (run not started or cleaned up) is a waiting state, not an error.
  await src.remove(); await waitMode('WAITING');
  check(await page.locator('#error').isHidden() && (await state()).snapshot.capture.tick === '10' && (await page.locator('#live-status').textContent()).includes('not found'), 'missing latest.json waits and keeps polling');
  await src.set(uiFixture(11)); await waitTick('11'); await waitMode('LIVE');

  // Pause stops applying updates and polling; resume catches up.
  await page.locator('#pause').click(); await waitMode('PAUSED'); await sleep(400);
  const paused = await src.stats(); await src.set(uiFixture(12)); await sleep(800);
  check((await src.stats()).calls === paused.calls && (await state()).snapshot.capture.tick === '11' && (await page.locator('#pause').textContent()) === 'Resume', 'pause stops polling and applying');
  await page.locator('#pause').click(); await waitTick('12'); await waitMode('LIVE');

  // A different study replaces everything, like opening a new file.
  const other = structuredClone(tick8), ids = ['0123abcd-0123-4123-8123-0123456789ab', '1123abcd-0123-4123-8123-0123456789ab'];
  other.capture = {...other.capture, session: ids[0], study: ids[1], generation: ids[1], tick: '3', label: 'UI validation fixture, other study'};
  for (const p of other.particles) { p.id = ids[1] + p.id.slice(36); p.aggregate = ids[1] + p.aggregate.slice(36); }
  await page.locator('#objects button').first().click();
  check(!!(await state()).selected, 'selection exists before study change');
  await src.set(other); await waitTick('3');
  current = await state();
  check(current.snapshot.capture.study === ids[1] && current.selected === null && await page.locator('#level').inputValue() === '' && await page.locator('#exposure').inputValue() === '', 'different study replaces generation and clears selection and filters');
  check(JSON.stringify(current.camera.target) === JSON.stringify(boundsFor(other.particles).centre), 'different study is framed like a new file');

  // Stop watching keeps the capture as a static display.
  await page.locator('#stop').click();
  current = await state(); const stopped = await src.stats(); await sleep(500);
  check(current.snapshot.capture.tick === '3' && !current.watch.active && (await page.locator('#mode').textContent()) === 'STATIC' && (await page.locator('#frame-note').textContent()).startsWith('STATIC CAPTURE ·') && (await src.stats()).calls === stopped.calls && await page.locator('#stop').isDisabled(), 'stop watching leaves a static capture and ends polling');

  // Opening a file while watching ends the watch; Open snapshot itself is unchanged.
  await src.set(tick0); await page.evaluate(() => window.HCPViewport.watchWith(window.__src, 'fixture-dir')); await waitTick('0');
  await load(tick8, 'letter-study.json'); await waitTick('8'); await waitMode('STATIC');
  await src.set(uiFixture(13)); await sleep(600);
  check((await state()).snapshot.capture.tick === '8' && !(await watchInfo()).active, 'opening a file stops watching');

  // A real FileSystemDirectoryHandle (origin-private) exercises the genuine getFileHandle/getFile path, including replacement by a new file.
  const opfs = await page.evaluate(async text => {
    try {
      const dir = await (await navigator.storage.getDirectory()).getDirectoryHandle('hcp-watch-test', {create: true});
      window.__opfsWrite = async t => { const w = await (await dir.getFileHandle('latest.json', {create: true})).createWritable(); await w.write(t); await w.close(); };
      await window.__opfsWrite(text); window.HCPViewport.watchWith(dir, 'opfs-dir'); return 'ok';
    } catch (e) { return `unavailable: ${e.name}`; }
  }, JSON.stringify(tick0));
  if (opfs === 'ok') {
    await waitTick('0'); await page.evaluate(text => window.__opfsWrite(text), JSON.stringify(uiFixture(14))); await waitTick('14');
    check((await state()).snapshot.capture.label === 'UI validation fixture' && (await page.locator('#mode').textContent()) === 'LIVE', 'real directory handle watched and replaced file applied');
    await page.locator('#stop').click();
    await page.evaluate(async () => (await navigator.storage.getDirectory()).removeEntry('hcp-watch-test', {recursive: true}));
  }

  // Without the File System Access API the control is disabled and says Open snapshot still works.
  const plain = await context.newPage(); plain.on('pageerror', e => errors.push(e.message));
  await plain.addInitScript(() => { Object.defineProperty(window, 'showDirectoryPicker', {value: undefined, configurable: true}); });
  await openViewport(plain);
  const remembered = await plain.locator('#interval').inputValue();
  check(await plain.locator('#watch').isDisabled() && (await plain.locator('#live-note').textContent()).includes('Open snapshot still works'), 'unsupported browsers see a disabled control and explanation');
  await plain.locator('#file').setInputFiles({name: 'tick0.json', mimeType: 'application/json', buffer: Buffer.from(JSON.stringify(tick0))});
  await plain.waitForFunction(() => window.HCPViewport.inspect().snapshot?.capture.tick === '0');
  check(await plain.locator('#mode').textContent() === 'STATIC', 'Open snapshot works without watch support');
  await plain.close();
  if (!inMemory) check(remembered === '200', 'remembered poll interval restored on a new page');

  check(errors.length === 0, 'no browser exceptions'); check(externalRequests.length === 0, 'no network requests');
  const evidence = {checks, browser: browser.version(), transport: inMemory ? 'in-memory exact page assets, offline browser context' : 'file://, offline browser context',
    direct_file_navigation: inMemory ? 'unexecuted here: managed Chromium blocks file:// navigation' : 'passed',
    renderer: current.renderer, engine_backend: 'native CPU capture; Taichi GPU unexecuted',
    particles: native.particles.length, snapshot_sha256: digest(nativeText), snapshot_bytes: Buffer.byteLength(nativeText),
    live_updates: {...picker, page: inMemory ? 'in-memory document (not a file:// page)' : 'direct file:// page', real_directory_handle: opfs === 'ok' ? 'origin-private directory handle watched and file replacement applied' : opfs,
      source: 'page-side source with the picker handle shape; the native folder picker is not automatable', captures: 'retained native tick 0 and 8; UI fixtures for other ticks, identities and invalid files'},
    metrics: nativeMetrics, network_requests: externalRequests.length, browser_errors: errors};
  await writeFile(`${resultsPath}/browser-evidence.json`, JSON.stringify(evidence, null, 2) + '\n');
  console.log(JSON.stringify(evidence, null, 2));
} finally { await browser.close(); }
