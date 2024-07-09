/* Copyright Light Source Software, LLC and other contributors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// build.js minifies a $PACK/$PACK.cjs file, takes a non-static snapshot and writes the
// bytes to $PACK/jjs-pack-$PACK-js.c file (similar to xxd, as this version of c does not 
// have #embed).
//
// The script assumes jjs has been built and is in $SRCROOT/build/bin. jjs-snapshot
// can be built with the flag:
//
//   build.py: --snapshot-save on --clean
//   cmake:    -DJJS_SNAPSHOT_SAVE=ON
// 
// The c file is checked into git so that the build does not depend on esbuild and node.
// The downside is that working on js file will require a dev to run this script and 
// recompile to test changes. And, of course, the changed js and c file will both need
// to be committed.
//
// Another issue is that the snapshot format is not stable. If a compile flag like 
// JJS_NUMBER_TYPE_FLOAT64 is on, the vm cannot load a snapshot from a vm that had
// that flag off. Since the snapshots are checked in, the jjs-pack feature will not
// work with all configurations. Fortunately, these destabilizing flags are not common.
// This is a bug in the snapshot generation and needs to be fixed.

import esbuild from 'esbuild';
import { basename, dirname, join } from 'node:path';
import { spawnSync } from 'node:child_process';
import { access, readFile, unlink, writeFile } from 'node:fs/promises';
import { tmpdir } from 'node:os';
import { fileURLToPath } from 'node:url';

const moduleDirname = import.meta.dirname ?? dirname(fileURLToPath(import.meta.url));
const buildDir = tmpdir();
const template = await readFile(join(moduleDirname, 'jjs-pack-embed-js.template'), 'utf8');
const moduleArgumentList = 'module,exports,require';

async function embedPackJS(pack) {
  const jsFile = join(moduleDirname, 'pack', pack, `${pack}.cjs`);

  const esBuildResult = await esbuild.build({
    entryPoints: [jsFile],
    bundle: true,
    minify: true,
    format: 'cjs',
    outdir: buildDir,
    write: false,
    external: ['jjs:*'],
  });

  const snapshotBytes = await snapshot(jsFile, esBuildResult.outputFiles[0].contents);
  let bytes = '';

  snapshotBytes.forEach((b, i) => {
    b = '0x' + b.toString(16).padStart(2, '0').toUpperCase();
    bytes += `${(i % 12) === 0 ? '  ' : ''}${b},${((i + 1) % 12) === 0 ? '\n' : ' '}`;
  });

  const embeddedSnapshot = template
    .replaceAll('${CDEFINE}', `JJS_PACK_${pack.toUpperCase()}`)
    .replaceAll('${CIDENT_PREFIX}', `jjs_pack_${pack}`)
    .replace('${BYTES}', bytes)
    .replace('${BYTES_LENGTH}', snapshotBytes.length.toString());

  const cFile = `jjs-pack-${pack}-js.c`;

  await writeFile(join(moduleDirname, 'pack', pack, cFile), embeddedSnapshot, 'utf8');

  console.log(`⚡ ${pack.padEnd(12, ' ')} -> ${cFile}`);
}

async function findSnapshotCommand() {
  const binDir = join(moduleDirname, '..', 'build', 'bin');
  const exe = 'jjs';
  const paths = [];

  if (process.platform === 'win32') {
    const wexe = `${exe}.exe`;
    paths.push(join(binDir, 'Debug', wexe));
    paths.push(join(binDir, 'Release', wexe));
    paths.push(join(binDir, 'MinSizeRel', wexe));
  } else {
    paths.push(join(binDir, exe));
  }

  for (const path of paths) {
    try {
      await access(path);
      return path;
    } catch {
      throw Error('jjs not found in $SRCROOT/build/bin. '
        + 'Minimal build command/flags to install: ./tools/build.py --snapshot-save on --clean')
    }
  }
}

class SnapshotError extends Error {
  constructor(file, result) {
    super(`jjs snapshot failed (${result.status === null ? result.signal : result.status})`);
    this.stderr = result.stderr.toString();
    this.file = file;
  }

  static isError(result) {
    return (result.status === null || result.status !== 0);
  }
}

async function snapshot(jsFile, jsSource) {
  const snapshotFile = join(dirname(jsFile), basename(jsFile, ".js")) + '.snapshot';
  const cmd = await findSnapshotCommand();
  const args = [
    'snapshot',
    '--vm-heap-size',
    '4096',
    '--loader', 'strict',
    '--function', moduleArgumentList,
    '--outfile', snapshotFile,
    '-',
  ];
  // spawn is a bit of a pain to handle errors. use the slower version instead.
  const jjsSnapshotResult = spawnSync(cmd, args, { input: jsSource });

  if (SnapshotError.isError(jjsSnapshotResult))
  {
    throw new SnapshotError(basename(jsFile), jjsSnapshotResult);
  }

  const snapshotBytes = Uint8Array.from(await readFile(snapshotFile));

  await unlink(snapshotFile);

  return snapshotBytes;
}

const result = await Promise.allSettled([
  embedPackJS('console'),
  embedPackJS('domexception'),
  embedPackJS('fs'),
  embedPackJS('path'),
  embedPackJS('performance'),
  embedPackJS('text'),
  embedPackJS('url'),
]);

result.forEach(result => {
  if (result.status === 'rejected') {
    if (result.reason instanceof SnapshotError)
    {
      console.log(`FILE: ${result.reason.file}`);
      console.log(`STDERR: ${result.reason.stderr}`);
    }
    throw Error("Unhandled Error", { cause: result.reason });
  }
});
