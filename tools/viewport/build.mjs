import {build} from 'esbuild';
import {copyFile, mkdir} from 'node:fs/promises';
await mkdir('vendor', {recursive: true});
await build({entryPoints: ['viewer.mjs'], bundle: true, minify: true,
  format: 'iife', target: ['chrome110', 'firefox115'], outfile: 'vendor/viewer.bundle.js',
  legalComments: 'linked', logLevel: 'info'});
await copyFile('node_modules/three/LICENSE', 'vendor/THREE-LICENSE.MIT');
await copyFile('node_modules/ajv/LICENSE', 'vendor/AJV-LICENSE.MIT');
