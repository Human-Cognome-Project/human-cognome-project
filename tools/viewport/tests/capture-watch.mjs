// Integration harness: watch real atomic native output through a read-only file bridge.
// The bridge substitutes only the browser's directory picker, never capture contents.
// usage: node tests/capture-watch.mjs OUTPUT_DIR omitted|ok|budget_refused COMMAND [ARGS...]
// COMMAND must write OUTPUT_DIR/latest.json, pace captures, and exit successfully.
import assert from 'node:assert/strict';
import {open, mkdir, writeFile, access} from 'node:fs/promises';
import {resolve} from 'node:path';
import {pathToFileURL} from 'node:url';
import {spawn} from 'node:child_process';
import {chromium} from 'playwright';
import {parseSnapshot} from '../model.mjs';

const [directory, expected, command, ...args] = process.argv.slice(2);
assert.ok(directory && command && ['omitted', 'ok', 'budget_refused'].includes(expected),
  'provide output directory, expected intent status and native command');
const output = resolve(directory), latest = resolve(output, 'latest.json');
await mkdir(output, {recursive: true});
await assert.rejects(access(latest), {code: 'ENOENT'}, 'use a fresh output directory');
const browser = await chromium.launch({executablePath: process.env.HCP_CHROMIUM,
  headless: true, args: ['--no-sandbox', '--use-angle=swiftshader', '--enable-unsafe-swiftshader']});
let child;
const errors = [], network = [], observed = new Map();
let log = '', exitCode, processError;
try {
  const context = await browser.newContext({offline: true, viewport: {width: 1440, height: 1000}});
  const page = await context.newPage();
  page.on('pageerror', e => errors.push(e.message));
  page.on('request', r => { if (/^https?:/.test(r.url())) network.push(r.url()); });
  await page.exposeFunction('readNativeCapture', async () => {
    let handle;
    try {
      // Read metadata and bytes from the same opened file, even during rename replacement.
      handle = await open(latest, 'r');
      const stat = await handle.stat(), text = await handle.readFile('utf8');
      const value = parseSnapshot(text);
      assert.equal(value.source.kind, 'bounded_native_current_hierarchy');
      assert.equal(value.intent?.status ?? 'omitted', expected);
      if (value.intent) {
        assert.equal(value.intent.tick, value.capture.tick);
        assert.equal(value.intent.gravity, value.source.parameters.gravity);
      }
      observed.set(value.capture.tick, value);
      return {text, size: stat.size, lastModified: stat.mtimeMs};
    } catch (e) {
      if (e.code === 'ENOENT') return null;
      throw e;
    } finally { await handle?.close(); }
  });
  await page.goto(pathToFileURL(resolve(import.meta.dirname, '../index.html')).href);
  await page.waitForFunction(() => window.HCPViewport?.inspect().renderer);
  await page.locator('#interval').fill('100');
  await page.locator('#interval').dispatchEvent('change');
  await page.evaluate(() => window.HCPViewport.watchWith({
    async getFileHandle(name) {
      if (name !== 'latest.json') throw new Error('unexpected file request');
      return {async getFile() {
        const f = await window.readNativeCapture();
        if (!f) throw new DOMException('not published yet', 'NotFoundError');
        return {...f, text: async () => f.text};
      }};
    }
  }, 'native capture output'));
  child = spawn(command, args, {shell: false, windowsHide: true});
  child.stdout.on('data', b => { log += b; });
  child.stderr.on('data', b => { log += b; });
  child.on('error', e => { processError = e; });
  child.on('exit', code => { exitCode = code; });
  await page.waitForFunction(() => window.HCPViewport.inspect().snapshot?.capture.tick === '0', null, {timeout: 120000});
  await page.locator('#objects button').first().click();
  const selected = await page.evaluate(() => window.HCPViewport.inspect().selected);
  const deadline = Date.now() + 120000;
  while (exitCode === undefined && !processError && Date.now() < deadline)
    await new Promise(r => setTimeout(r, 100));
  assert.ifError(processError);
  assert.equal(exitCode, 0, `native process failed or timed out: ${log}`);
  const finalHandle = await open(latest, 'r');
  const final = parseSnapshot(await finalHandle.readFile('utf8')); await finalHandle.close();
  await page.waitForFunction(t => window.HCPViewport.inspect().snapshot?.capture.tick === t, final.capture.tick);
  const state = await page.evaluate(() => window.HCPViewport.inspect());
  assert.deepEqual(state.snapshot, final);
  assert.equal(state.selected, selected);
  assert.equal(state.watch.error, null);
  assert.ok(observed.size >= 3, 'must observe tick zero, intermediate and final native captures');
  assert.ok(state.watch.applied >= 3, 'watch must apply intermediate captures');
  const details = await page.locator('#inspection').innerText();
  assert.ok(details.includes(expected === 'omitted' ? 'Not requested' :
    expected === 'budget_refused' ? 'Observation budget refused' : 'Supported'));
  assert.deepEqual(errors, []); assert.deepEqual(network, []);
  await page.screenshot({path: resolve(output, 'watch.png')});
  await writeFile(resolve(output, 'watch-evidence.json'), JSON.stringify({
    expected, observed_ticks: [...observed.keys()], final_tick: final.capture.tick,
    applied: state.watch.applied, selected, details, browser: browser.version(),
    bridge: 'read-only filesystem adapter; not a native directory picker', errors, network
  }, null, 2));
  console.log(`PASS native capture-to-watch ${expected}: ${observed.size} ticks`);
} finally {
  if (child && exitCode === undefined) child.kill();
  await writeFile(resolve(output, 'native.log'), log);
  await browser.close();
}
