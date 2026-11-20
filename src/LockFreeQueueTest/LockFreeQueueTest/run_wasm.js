'use strict';

// Node host only. Queue operations, FIFO oracles and timed loops stay inside C++/Wasm.
const fs = require('fs');
const path = require('path');
const crypto = require('crypto');
const childProcess = require('child_process');
const root = path.resolve(__dirname, '../../..');
const temporary = path.join(root, 'src/temp/LockFreeQueueTest');
const core = [1, 2, 3, 4, 6, 7, 8, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
    25, 26, 27, 28, 29, 30, 31, 32, 34, 36, 37, 39, 41, 42, 43, 44, 45, 47, 48, 49, 50];
const single = [1, 3, 6, 8, 10, 11, 12, 13, 14, 15, 20, 32, 34, 39, 41, 44, 45, 47, 48, 49, 50];

/** Check that each requested Wasm benchmark scenario produced its own evidence.
 * @param {string} suite Backlog, pool, or wave workload name
 * @param {string} log Output from a single invocation of the C++ workload
 * @returns {boolean} Whether every expected scenario and phase completed exactly once
 */
function measuredScenarios(suite, log) {
    const pending = new Set();
    const started = new Set();
    if (suite === 'backlog') {
        for (const count of [1, 1024, 65536, 262144]) {
            pending.add(`backlog/${count}`);
            pending.add(`latency/${count}`);
        }
    } else if (suite === 'pools') {
        for (const name of ['default_zero', 'default_positive', 'finite_fixed_zero', 'finite_fixed_positive',
            'finite_proportional', 'shrink_regrow_zero', 'shrink_regrow_positive']) {
            const shrink = name.startsWith('shrink_regrow_');
            const items = name.startsWith('default_') ? 32768 : 4095;
            for (let cycle = 0; cycle < (shrink ? 8 : 4); ++cycle) {
                const id = `${name}/${shrink || cycle === 0 ? 'growth' : 'reuse'}/${cycle}`;
                pending.add(`${id}/result/${items}`);
                pending.add(`${id}/reclaim`);
                if (shrink) pending.add(`${id}/shrink`);
            }
        }
        for (const threshold of [0, 1]) {
            for (const producers of [1, 4, 16]) {
                const id = `bounded${threshold}_p${producers}/pressure/0`;
                pending.add(`${id}/result/200000`);
                pending.add(`${id}/reclaim`);
            }
        }
    } else {
        for (const threshold of [0, 8]) {
            for (const producers of [1, 2, 4, 8, 16]) {
                const id = `wave750_t${threshold}_p${producers}`;
                pending.add(`${id}/result/1182148`);
                for (let cycle = 0; cycle < 4; ++cycle) {
                    for (let phase = 0; phase < 3; ++phase) {
                        pending.add(`${id}/${cycle}/${phase}/${phase === 0 ? 1000 : 500}`);
                    }
                }
            }
        }
    }
    let id = '';
    let firstPop = 0;
    for (const line of log.split(/\r?\n/)) {
        const [tag, ...tokens] = line.trimEnd().split(/\s+/);
        if (!['BACKLOG', 'LATENCY', 'POLICY', 'WAVE', 'POLICY_RESULT', 'WAVE_RESULT',
            'MAINTENANCE', 'WAVE_STAGE'].includes(tag)) continue;
        const fields = Object.create(null);
        for (const token of tokens) {
            const parts = token.split('=');
            if (parts.length !== 2 || !parts[0] || !parts[1] || parts[0] in fields) return false;
            fields[parts[0]] = parts[1];
        }
        // A zero-duration individual sample is legal on a coarse host clock.
        const ticks = key => /^\d+$/.test(fields[key]) && Number.isSafeInteger(Number(fields[key]));
        if (tag === 'BACKLOG') {
            id = fields.count;
            if (fields.valid !== '1' || !ticks('first_pop_ns') || !pending.delete('backlog/' + id)) return false;
            firstPop = Number(fields.first_pop_ns);
        } else if (tag === 'LATENCY') {
            const percentiles = ['p50_ns', 'p95_ns', 'p99_ns', 'max_ns'];
            if (!ticks('samples') || Number(fields.samples) === 0 ||
                !percentiles.every((key, i) => ticks(key) &&
                    (i === 0 || Number(fields[percentiles[i - 1]]) <= Number(fields[key])))) return false;
            if (suite === 'backlog' && (fields.operation !== 'pop_backlog' || fields.samples !== id ||
                Number(fields.max_ns) < firstPop ||
                (id === '1' && !percentiles.every(key => Number(fields[key]) === firstPop)) ||
                !pending.delete('latency/' + id))) return false;
        } else if (tag === 'POLICY') {
            id = `${fields.case}/${fields.phase}/${fields.cycle}`;
            if (suite !== 'pools' || started.has(id) || !pending.has(`${id}/result/${fields.items}`)) return false;
            started.add(id);
        } else if (tag === 'WAVE') {
            id = fields.case;
            if (suite !== 'wave' || started.has(id) || !pending.has(`${id}/result/1182148`)) return false;
            started.add(id);
        } else if (tag === 'WAVE_STAGE') {
            if (!pending.delete(`${id}/${fields.cycle}/${fields.phase}/${fields.backlog}`)) return false;
        } else {
            const key = tag === 'MAINTENANCE' ? fields.phase : 'result/' + fields.operations;
            if (tag === 'MAINTENANCE' && !['reclaim', 'shrink'].includes(key)) return false;
            if (!ticks('elapsed_ns') || (tag !== 'MAINTENANCE' && Number(fields.elapsed_ns) === 0) ||
                (tag === 'WAVE_RESULT') !== (suite === 'wave') ||
                fields.valid !== '1' || !pending.delete(id + '/' + key)) return false;
        }
    }
    return pending.size === 0;
}

/** Check the latency oracle using synthetic evidence, without running a benchmark. */
function selfTest() {
    const rows = [1, 1024, 65536, 262144].map(n => `BACKLOG count=${n} first_pop_ns=0 valid=1\n` +
        `LATENCY operation=pop_backlog samples=${n} p50_ns=0 p95_ns=${n === 1 ? 0 : 1}` +
        ` p99_ns=${n === 1 ? 0 : 2} max_ns=${n === 1 ? 0 : 3}`).join('\n');
    if (!measuredScenarios('backlog', rows)) throw new Error('Valid latency evidence rejected');
    for (const [from, to] of [[' first_pop_ns=0', ''], [' max_ns=3', ''], ['samples=1 ', 'samples=2 '],
        ['p99_ns=2', 'p99_ns=-1'], ['p99_ns=2', 'p99_ns=NaN'], ['p50_ns=0', 'p50_ns=2'],
        ['p99_ns=2', 'p99_ns=2 p99_ns=2'], ['max_ns=3', 'max_ns=9007199254740992'],
        ['max_ns=0', 'max_ns=1'], ['first_pop_ns=0', 'first_pop_ns=9']]) {
        if (measuredScenarios('backlog', rows.replace(from, to))) throw new Error('Invalid evidence accepted: ' + to);
    }
    console.log('PASS Wasm latency oracle: 1 valid, 10 invalid fixtures');
}

/** Run isolated Wasm cases, validate their evidence, and retain logs on failure
 * @returns {void} Throws when arguments, execution, or result evidence are invalid
 */
function main() {
    const options = {};
    const input = process.argv.slice(2);
    if (input.length === 1 && input[0] === '--self-test') return selfTest();
    if (input.length === 1 && input[0] === '--help') {
        console.log('node run_wasm.js --build NATIVE_OR_PTHREAD_WASM_BUILD --batch UNIQUE_NAME --threads on|off [--scope core|all]');
        return;
    }
    for (let i = 0; i < input.length; i += 2) {
        const key = input[i];
        if (!['--build', '--batch', '--threads', '--scope'].includes(key) || options[key] !== undefined || !input[i + 1]) {
            throw new Error('Unknown/duplicate option or missing value: ' + key);
        }
        options[key] = input[i + 1];
    }
    if (!options['--build'] || !options['--batch'] || !['on', 'off'].includes(options['--threads'])) {
        throw new Error('Require --build, --batch and explicit --threads on|off [--scope core|all]');
    }
    const scope = options['--scope'] || 'all';
    if (!['core', 'all'].includes(scope)) throw new Error('Invalid --scope');
    const output = path.resolve(temporary, options['--batch']);
    const relative = path.relative(temporary, output);
    if (!relative || relative.startsWith('..') || path.isAbsolute(relative) || fs.existsSync(output)) {
        throw new Error('Use a fresh batch strictly below src/temp/LockFreeQueueTest');
    }
    let ancestor = path.dirname(output);
    while (!fs.existsSync(ancestor)) ancestor = path.dirname(ancestor);
    const canonicalAncestor = path.resolve(fs.realpathSync(ancestor));
    if (path.relative(ancestor, canonicalAncestor) !== '') {
        throw new Error('Output ancestor resolves through a link');
    }
    fs.mkdirSync(output, {recursive: true});
    const build = path.resolve(options['--build']);
    const ids = options['--threads'] === 'on' ? core : single;
    const cases = [
        {name: 'core', binary: 'LockFreeQueueTest.js', args: [ids.join(',')], ids, seconds: 180}
    ];
    cases.push({name: 'headers', binary: 'LockFreeQueueIntegration.js', args: ['23'], ids: [23], seconds: 180});
    if (options['--threads'] === 'on') {
        cases.push({name: 'notification', binary: 'LockFreeQueueIntegration.js', args: ['9'], ids: [9], seconds: 180});
    }
    if (scope === 'all') cases.push({name: 'backlog', binary: 'LockFreeQueueMeasurements.js', args: ['22'], seconds: 180});
    if (scope === 'all' && options['--threads'] === 'on') {
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
        if (test.name === 'core') {
            const expectedThreads = options['--threads'] === 'on' ? '1' : '0';
            const markers = [...text.matchAll(/^WASM_THREADS enabled=([01])\r?$/gm)];
            const summaries = [...text.matchAll(/^SUMMARY executed=(\d+) failed=(\d+)\r?$/gm)];
            valid = valid && markers.length === 1 && markers[0][1] === expectedThreads &&
                summaries.length === 1 && Number(summaries[0][1]) === test.ids.length && Number(summaries[0][2]) === 0;
        }
        if (test.name === 'notification') {
            const rows = text.match(/^NOTIFICATION\b.*$/gm) || [];
            valid = valid && rows.length === 1 && rows[0].trimEnd() ===
                'NOTIFICATION received=6000 wake_permits=191 coalesced_batch=32 finite_wait_ms=5 valid=1';
        }
        if (test.ids) {
            const passed = [...text.matchAll(/^PASS id=(\d+)(?=\s|$)/gm)].map(match => Number(match[1])).sort((a, b) => a - b);
            valid = valid && (text.match(/^PASS\b/gm) || []).length === test.ids.length &&
                JSON.stringify(passed) === JSON.stringify(test.ids);
        } else {
            const expectedId = test.name === 'backlog' ? 22 : (test.name === 'pools' ? 38 : 40);
            valid = valid && (text.match(/^PASS\b/gm) || []).length === 1 &&
                new RegExp('^PASS id=' + expectedId + '(?=\\s|$)', 'm').test(text);
            valid = valid && measuredScenarios(test.name, text);
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
