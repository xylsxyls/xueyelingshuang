'use strict';

const fs = require('node:fs');
const path = require('node:path');
const {parseArgs} = require('node:util');

// Read memory zero from an imported or defined memory without requiring the packaging host to support memory64.
function memoryWidth(filename) {
    const bytes = fs.readFileSync(filename);
    let offset = 8;
    let end = bytes.length;
    function fail(message) {
        throw new Error(message + ': ' + filename);
    }
    function byte() {
        if (offset >= end) fail('Truncated Wasm declaration');
        return bytes[offset++];
    }
    function integer(maxBytes = 5) {
        let value = 0;
        for (let index = 0; index < maxBytes; ++index) {
            const part = byte();
            value += (part & 127) * 2 ** (7 * index);
            if (part < 128) return value;
        }
        fail('Invalid Wasm integer');
    }
    function valueType() {
        const type = byte();
        if (type === 0x63 || type === 0x64) integer();
    }
    function limits() {
        const flags = byte();
        const width = flags & 4 ? 64 : 32;
        // Limits themselves are skipped. Only lengths and counts (u32) are used as JS array offsets.
        integer(width === 64 ? 10 : 5);
        if (flags & 1) integer(width === 64 ? 10 : 5);
        return width;
    }
    if (!bytes.subarray(0, 8).equals(Buffer.from([0, 97, 115, 109, 1, 0, 0, 0]))) {
        fail('Expected a core Wasm module');
    }
    while (offset < bytes.length) {
        const section = byte();
        const length = integer();
        end = offset + length;
        if (end > bytes.length) fail('Truncated Wasm section');
        if (section === 5) {
            if (integer()) return limits();
        } else if (section === 2) {
            const count = integer();
            for (let index = 0; index < count; ++index) {
                for (let name = 0; name < 2; ++name) {
                    const size = integer();
                    offset += size;
                    if (offset > end) fail('Truncated Wasm import name');
                }
                const kind = byte();
                switch (kind) {
                case 0: integer(); break;
                case 1: valueType(); limits(); break;
                case 2: return limits();
                case 3: valueType(); byte(); break;
                case 4: byte(); integer(); break;
                default: fail('Unknown Wasm import kind');
                }
            }
        }
        offset = end;
        end = bytes.length;
    }
    fail('Missing Wasm linear memory');
}

function validateBuild(test, build, bits, option) {
    const cache = fs.readFileSync(path.join(build, 'CMakeCache.txt'), 'utf8').split(/\r?\n/);
    if (!cache.some(line => new RegExp('^BIT:[^=]+=wasm' + bits + '$').test(line))) {
        throw new Error('Expected a wasm' + bits + ' CMake build in ' + build);
    }
    const enabled = new RegExp('^' + option + ':[^=]+=(ON|TRUE|YES|1)$', 'i');
    if (!cache.some(line => enabled.test(line))) throw new Error('Configure ' + option + '=ON for ' + build);
    const script = path.join(build, test + '.js');
    if (!fs.statSync(script).isFile()) throw new Error('Expected a file: ' + script);
    const module = path.join(build, test + '.wasm');
    const actual = memoryWidth(module);
    if (actual !== bits) throw new Error('Expected wasm' + bits + ', found wasm' + actual + ' linear memory in ' + module);
}

function fixtureOrigin(origin) {
    if (!origin) throw new Error('HttpWorkTest requires --origin pointing to HttpWorkFixture --serve');
    const url = new URL(origin);
    if (!/^https?:\/\//i.test(origin) || !url.hostname || /^https?:\/\/[^/?#]*@/i.test(origin) ||
        url.search || url.hash) {
        throw new Error('Expected an HTTP or HTTPS fixture URL without credentials, query or fragment');
    }
    return origin.replace(/\/+$/, '');
}

function nodeEntry(test, host, origin) {
    const settings = origin ? JSON.stringify(origin) + ', settings' : 'settings';
    return `const run = require('./${host}');
const runPair = require('./LibraryTestWidth').runPair;
function start(bits) {
    const base = require('path').join(__dirname, 'wasm' + bits, '${test}');
    const args = process.argv.slice(3);
    const settings = { wasmBinary: require('fs').readFileSync(base + '.wasm'), benchmark: args.length === 1 && args[0] === '--benchmark' };
    if (args.length && !settings.benchmark) settings.args = args;
    return run(require(base + '.js'), ${settings});
}
runPair(() => start(32), () => start(64), process.argv[2] || 'auto').then(
    result => console.log('${test} wasm' + result.bits + ' PASS'),
    error => { console.error(error); process.exitCode = 1; }
);`;
}

function wechatPage(test, host, origin) {
    function run(bits) {
        const stem = 'wasm' + bits + '/' + test;
        const server = origin ? JSON.stringify(origin) + ', ' : '';
        return `() => run(require('../${stem}'), ${server}{wasmPath: "/${stem}.wasm", benchmark: query.benchmark === '1', args: query.args === undefined ? undefined : JSON.parse(query.args), print: printLine})`;
    }
    return `const run = require('../${host}');
const runPair = require('../LibraryTestWidth').runPair;
Page({
    data: {result: 'Running'},
    async onLoad(query) {
        try {
            const samples = [];
            function printLine(line) {
                console.log(line);
                if (line.startsWith('[BENCH]')) samples.push(line);
            }
            const result = await runPair(${run(32)}, ${run(64)}, query.bits || 'auto');
            this.setData({result: ['${test} wasm' + result.bits + ' PASS', ...samples].join('\\n')});
        } catch (error) {
            this.setData({result: '${test} FAIL ' + String(error)});
        }
    }
});`;
}

function browserPage(test, origin) {
    if (!origin) {
        return fs.readFileSync(path.join(__dirname, 'LibraryTestHost.html'), 'utf8')
            .replace('<html lang="zh-CN">', '<html lang="zh-CN" data-paired-test="' + test + '">');
    }
    const server = JSON.stringify(origin).replace(/</g, '\\u003c');
    return `<!doctype html><html lang="zh-CN"><meta charset="utf-8"><title>HttpWorkTest</title>
<pre id="result">Running</pre><script src="LibraryTestWidth.js"></script><script src="HttpWorkHost.js"></script>
<script>
(async () => {
    const result = document.getElementById('result');
    try {
        const bits = await selectLibraryTestWidth(new URLSearchParams(location.search).get('bits') || 'auto');
        const script = document.createElement('script');
        script.src = './wasm' + bits + '/HttpWorkTest.js';
        await new Promise((ok, fail) => { script.onload = ok; script.onerror = fail; document.body.appendChild(script); });
        const samples = [];
        await runHttpWorkTest(createHttpWorkTest, ${server}, {
            benchmark: new URLSearchParams(location.search).get('benchmark') === '1',
            args: new URLSearchParams(location.search).has('args') ? JSON.parse(new URLSearchParams(location.search).get('args')) : undefined,
            print(line) { console.log(line); if (line.startsWith('[BENCH]')) samples.push(line); }
        });
        result.textContent = ['HttpWorkTest wasm' + bits + ' PASS', ...samples].join('\\n');
    } catch (error) { result.textContent = 'HttpWorkTest FAIL ' + error; }
})();
</script></html>`;
}

function packageTest(test, options) {
    if (!/^[A-Za-z][A-Za-z0-9]*Test$/.test(test)) throw new Error('Expected the CMake test target name');
    const http = test === 'HttpWorkTest';
    const origin = http ? fixtureOrigin(options.origin) : '';
    const host = http ? 'HttpWorkHost' : 'LibraryTestHost';
    const option = http ? 'HTTPWORK_WASM_HOST' : 'LIBRARY_TEST_HOST';
    const copies = new Map();
    copies.set(host + '.js', path.join(http ? options.wasm32 : __dirname, host + '.js'));
    for (const name of ['LibraryTestWidth.js', 'LibraryTestMemory64.wasm']) {
        copies.set(name, path.join(__dirname, name));
    }
    for (const bits of [32, 64]) {
        const tag = 'wasm' + bits;
        const build = options[tag];
        validateBuild(test, build, bits, option);
        for (const extension of ['.js', '.wasm']) {
            const file = test + extension;
            copies.set(tag + '/' + file, path.join(build, file));
        }
    }
    for (const file of copies.values()) {
        if (!fs.statSync(file).isFile()) throw new Error('Expected a file: ' + file);
    }
    const text = {
        'LibraryTestHost.html': browserPage(test, origin),
        'run-node.js': nodeEntry(test, host, origin),
        'app.json': JSON.stringify({pages: ['pages/result']}),
        'app.js': 'App({})',
        'project.config.json': JSON.stringify({appid: 'touristappid', projectname: test, compileType: 'miniprogram',
            setting: {es6: true, urlCheck: false}}),
        'pages/result.js': wechatPage(test, host, origin),
        'pages/result.wxml': '<view>{{result}}</view>'
    };
    const destination = path.resolve(options.output);
    fs.mkdirSync(destination, {recursive: true});
    if (fs.readdirSync(destination).length) throw new Error('Choose an empty output directory: ' + destination);
    // Reserve the empty directory exclusively. On failure remove only this invocation's own partial files.
    const lock = path.join(destination, '.packaging');
    const descriptor = fs.openSync(lock, 'wx');
    const created = [];
    try {
        for (const directory of ['wasm32', 'wasm64', 'pages']) {
            const dir = path.join(destination, directory);
            fs.mkdirSync(dir);
            created.push(dir);
        }
        for (const [name, source] of copies) {
            const target = path.join(destination, name);
            fs.copyFileSync(source, target, fs.constants.COPYFILE_EXCL);
            created.push(target);
        }
        for (const [name, content] of Object.entries(text)) {
            const target = path.join(destination, name);
            fs.writeFileSync(target, content, {flag: 'wx'});
            created.push(target);
        }
    } catch (error) {
        for (const target of created.reverse()) {
            if (fs.statSync(target).isDirectory()) fs.rmdirSync(target);
            else fs.unlinkSync(target);
        }
        throw error;
    } finally {
        fs.closeSync(descriptor);
        fs.unlinkSync(lock);
    }
}

const usage = 'node package_library_test.js <Test> --wasm32 <build32> --wasm64 <build64> --output <empty directory> [--origin <URL>]';
try {
    const {values, positionals} = parseArgs({allowPositionals: true, options: {
        wasm32: {type: 'string'}, wasm64: {type: 'string'}, output: {type: 'string'}, origin: {type: 'string'},
        help: {type: 'boolean', short: 'h'}
    }});
    if (values.help) console.log(usage);
    else if (positionals.length !== 1 || !values.wasm32 || !values.wasm64 || !values.output) {
        console.error(usage);
        process.exitCode = 2;
    } else {
        packageTest(positionals[0], values);
    }
} catch (error) {
    console.error(error.message);
    process.exitCode = 1;
}