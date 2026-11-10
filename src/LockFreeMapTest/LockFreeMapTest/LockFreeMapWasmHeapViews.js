// 仅用于隔离诊断：刷新旧工具链遗漏的 64 位堆视图，不改原始产物或编译器安装。
const fs = require('node:fs');
const path = require('node:path');
const crypto = require('node:crypto');

try {
    if (process.argv.length !== 4) {
        throw new Error('expected input.js output.js');
    }
    const input = path.resolve(process.argv[2]);
    const output = path.resolve(process.argv[3]);
    if (input === output || path.dirname(input) !== path.dirname(output) ||
        fs.existsSync(output) || fs.existsSync(output + '.json')) {
        throw new Error('output must be a new sibling file');
    }
    const original = fs.readFileSync(input, 'utf8');
    let replacements = 0;
    const patched = original.replace(/\bHEAP(U?)64\[/g, (match, unsigned) => {
        ++replacements;
        return `LockFreeMapTestHeap${unsigned ? 'U' : 'I'}64()[`;
    });
    if (replacements === 0) {
        throw new Error('no direct 64-bit heap access found');
    }
    const helpers = '\nfunction LockFreeMapTestHeapI64(){if(wasmMemory.buffer!==HEAP64.buffer){updateMemoryViews();}return HEAP64;}\n' +
        'function LockFreeMapTestHeapU64(){if(wasmMemory.buffer!==HEAPU64.buffer){updateMemoryViews();}return HEAPU64;}\n';
    const result = patched + helpers;
    fs.writeFileSync(output, result, {flag: 'wx'});
    const digest = value => crypto.createHash('sha256').update(value).digest('hex');
    fs.writeFileSync(output + '.json', JSON.stringify({input, output, replacements,
        input_sha256: digest(original), output_sha256: digest(result), diagnostic_only: true}, null, 2), {flag: 'wx'});
} catch (error) {
    process.stderr.write(String(error) + '\n');
    process.exitCode = 1;
}
