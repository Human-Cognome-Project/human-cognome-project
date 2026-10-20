import {chromium} from 'playwright';
import {readFile, mkdir, writeFile} from 'node:fs/promises';
import {resolve} from 'node:path';
import {pathToFileURL} from 'node:url';
import {createHash} from 'node:crypto';
import {existsSync} from 'node:fs';
import assert from 'node:assert/strict';
import {massColour, rangesFor} from '../model.mjs';

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
try {
  if (inMemory) {
    // For managed cloud browsers that disallow file navigation. Supply the
    // exact page assets as document content without changing browser policy.
    const html = await readFile(`${root}/index.html`, 'utf8');
    const css = await readFile(`${root}/viewport.css`, 'utf8');
    await page.setContent(html.replace('<link rel="stylesheet" href="viewport.css">', `<style>${css}</style>`)
      .replace('<script defer src="vendor/viewer.bundle.js"></script>', ''));
    await page.addScriptTag({content: await readFile(`${root}/vendor/viewer.bundle.js`, 'utf8')});
  } else await page.goto(pathToFileURL(`${root}/index.html`).href);
  await page.waitForFunction(() => window.HCPViewport);
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
  check(errors.length === 0, 'no browser exceptions'); check(externalRequests.length === 0, 'no network requests');
  const evidence = {checks, browser: browser.version(), transport: inMemory ? 'in-memory exact page assets, offline browser context' : 'file://, offline browser context',
    direct_file_navigation: inMemory ? 'unexecuted here: managed Chromium blocks file:// navigation' : 'passed',
    renderer: current.renderer, engine_backend: 'native CPU capture; Taichi GPU unexecuted',
    particles: native.particles.length, snapshot_sha256: digest(nativeText), snapshot_bytes: Buffer.byteLength(nativeText),
    metrics: nativeMetrics, network_requests: externalRequests.length, browser_errors: errors};
  await writeFile(`${resultsPath}/browser-evidence.json`, JSON.stringify(evidence, null, 2) + '\n');
  console.log(JSON.stringify(evidence, null, 2));
} finally { await browser.close(); }
