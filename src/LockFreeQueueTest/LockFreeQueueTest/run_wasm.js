'use strict';

// Node host only. Queue operations, FIFO oracles and timed loops stay inside C++/Wasm.
const fs = require('fs');
const path = require('path');
const crypto = require('crypto');
const childProcess = require('child_process');
const root = path.resolve(__dirname, '../../..');
const temporary = path.join(root, 'src/temp/LockFreeQueueTest');
const core = [1, 2, 3, 4, 6, 7, 8, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
    25, 26, 27, 28, 29, 30, 31, 32, 34, 36, 37, 39, 41, 42, 43, 44, 45];
const single = [1, 3, 6, 8, 10, 11, 12, 13, 14, 15, 20, 32, 34, 39, 41, 44, 45];

function main() {
    const options = {};
    const input = process.argv.slice(2);
    if (input.length === 1 && input[0] === '--help') {
        console.log('node run_wasm.js --build NATIVE_OR_PTHREAD_WASM_BUILD --batch UNIQUE_NAME --threads on|off');
        return;
    }
    for (let i = 0; i < input.length; i += 2) {
        const key = input[i];
        if (!['--build', '--batch', '--threads'].includes(key) || options[key] !== undefined || !input[i + 1]) {
            throw new Error('Unknown/duplicate option or missing value: ' + key);
        }
        options[key] = input[i + 1];
    }
    if (!options['--build'] || !options['--batch'] || !['on', 'off'].includes(options['--threads'])) {
        throw new Error('Require --build, --batch and explicit --threads on|off');
    }
    const output = path.resolve(temporary, options['--batch']);
    const relative = path.relative(temporary, output);
    if (!relative || relative.startsWith('..') || path.isAbsolute(relative) || fs.existsSync(output)) {
        throw new Error('Use a fresh batch strictly below src/temp/LockFreeQueueTest');
    }
    let ancestor = path.dirname(output);
    while (!fs.existsSync(ancestor)) ancestor = path.dirname(ancestor);
    if (path.resolve(fs.realpathSync(ancestor)).toLowerCase() !== ancestor.toLowerCase()) {
        throw new Error('Output ancestor resolves through a link');
    }
    fs.mkdirSync(output, {recursive: true});
    const build = path.resolve(options['--build']);
    const ids = options['--threads'] === 'on' ? core : single;
    const cases = [
        {name: 'core', binary: 'LockFreeQueueTest.js', args: [ids.join(',')], ids, seconds: 180},
        {name: 'backlog', binary: 'LockFreeQueueMeasurements.js', args: ['22'], seconds: 180}
    ];
    if (options['--threads'] === 'on') {
        cases.push({name: 'pools', binary: 'LockFreeQueuePerformance.js', args: ['38', '--no-allocation-count'], seconds: 180});
        cases.push({name: 'wave', binary: 'LockFreeQueuePerformance.js', args: ['40', '--no-allocation-count'], seconds: 240});
    }
    const results = [];
    const hashes = {};
    for (const entry of fs.readdirSync(build)) {
        if (/\.(js|wasm)$/.test(entry)) {
            hashes[entry] = crypto.createHash('sha256').update(fs.readFileSync(path.join(build, entry))).digest('hex');
        }
    }
    fs.writeFileSync(path.join(output, 'environment.json'), JSON.stringify({node: process.version,
        platform: process.platform, architecture: process.arch, threads: options['--threads'], hashes,
        scope: 'Node Wasm tests only; not WeChat/device or whole-system acceptance'}, null, 2));
    for (const test of cases) {
        const command = [path.join(build, test.binary), ...test.args];
        const began = process.hrtime.bigint();
        const result = childProcess.spawnSync(process.execPath, command, {
            cwd: build, timeout: test.seconds * 1000, maxBuffer: 32 * 1024 * 1024,
            windowsHide: true
        });
        const raw = Buffer.concat([result.stdout || Buffer.alloc(0), result.stderr || Buffer.alloc(0)]);
        fs.writeFileSync(path.join(output, test.name + '.raw.log'), raw);
        const text = raw.toString('utf8');
        let valid = result.status === 0 && !result.error && !/^FAIL\b/m.test(text) && !/\bvalid=0\b/.test(text);
        if (test.ids) {
            const passed = [...text.matchAll(/^PASS id=(\d+)\b/gm)].map(match => Number(match[1])).sort((a, b) => a - b);
            valid = valid && JSON.stringify(passed) === JSON.stringify(test.ids);
        } else {
            valid = valid && /^PASS\b/m.test(text);
            if (test.name === 'pools') valid = valid && (text.match(/^POLICY_RESULT /gm) || []).length === 42;
            if (test.name === 'wave') {
                valid = valid && (text.match(/^WAVE_RESULT /gm) || []).length === 10 && (text.match(/^WAVE_STAGE /gm) || []).length === 120;
            }
        }
        results.push({name: test.name, command, exit: result.status, error: result.error && result.error.message,
            seconds: Number(process.hrtime.bigint() - began) / 1e9, valid,
            sha256: crypto.createHash('sha256').update(raw).digest('hex')});
        fs.writeFileSync(path.join(output, 'results.json'), JSON.stringify(results, null, 2));
        console.log((valid ? 'PASS ' : 'FAIL ') + test.name);
        if (!valid) throw new Error('Wasm workload failed; retained log: ' + path.join(output, test.name + '.raw.log'));
    }
}

try { main(); } catch (error) { console.error('HARNESS_FAIL', error.message); process.exitCode = 1; }