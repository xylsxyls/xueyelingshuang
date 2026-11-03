'use strict';

// Production WX module, real TCP and a separate gateway. All protocol/TLS/protobuf work stays in C++/Wasm.
const fs = require('fs');
const path = require('path');
const net = require('net');
const crypto = require('crypto');
const { spawn } = require('child_process');
const { performance } = require('perf_hooks');
const memory = require('./host/WasmMemory');

function check(condition, message) { if (!condition) throw new Error(message); }

async function createRuntime(modulePath) {
    let instance;
    const factory = require(modulePath);
    instance = await factory({ wasmBinary: require('./host/read_wasm').readWasm(modulePath),
        noInitialRun: true,
        netInterfaceEntropy: (pointer, size) => {
            if (!instance) return -1;
            instance.HEAPU8.set(crypto.randomBytes(size), memory.heapPointer(instance, pointer));
            return 0;
        },
        print: () => {}, printErr: line => process.stderr.write(line + '\n') });
    return memory.setModuleBits(instance, factory);
}

function startGateway(executable, output) {
    let stdout = '';
    let stderr = '';
    let readyResolve;
    let readyReject;
    const ready = new Promise((resolve, reject) => { readyResolve = resolve; readyReject = reject; });
    ready.catch(() => {});
    const stop = path.join(output, 'gateway.stop');
    const child = spawn(executable, ['--protocol=3', '--identity-directory=' + path.join(output, 'identity'),
        '--listen=127.0.0.1', '--port=0', '--stop-file=' + stop, '--run-for-ms=15000'],
        { windowsHide: true, stdio: ['ignore', 'pipe', 'pipe'] });
    child.stdout.on('data', bytes => {
        stdout += bytes.toString('utf8');
        const match = /GATEWAY_READY port=(\d+) pid=(\d+)/.exec(stdout);
        if (match) readyResolve(Number(match[1]));
    });
    child.stderr.on('data', bytes => { stderr += bytes.toString('utf8'); });
    const completed = new Promise((resolve, reject) => {
        child.once('error', error => { readyReject(error); reject(error); });
        child.once('close', (code, signal) => {
            readyReject(new Error('Gateway exited before readiness'));
            fs.writeFileSync(path.join(output, 'gateway.json'), JSON.stringify({ code, signal, stdout, stderr }, null, 2));
            resolve({ code, signal });
        });
    });
    return { child, stop, ready, completed };
}

async function exercise(module, port) {
    const allocations = [];
    const allocate = size => {
        const pointer = memory.heapPointer(module, module._malloc(size));
        check(pointer !== 0, 'Wasm allocation failed');
        allocations.push(pointer);
        return pointer;
    };
    const pointerBytes = memory.pointerBytes(module);
    const handles = allocate(pointerBytes * 2);
    const fields = allocate(24);
    const value = allocate(8 + pointerBytes);
    const payloadPointer = allocate(pointerBytes);
    const wire = allocate(16408);
    const incoming = allocate(65536);
    const messageValue = allocate(4096);
    module.HEAPU8.fill(0, handles, handles + pointerBytes * 2);
    check(module._netInterfaceWasmLinkCreateClient(0, 0, 5000, handles) === 1, 'Create actual production Wasm link failed');
    const link = memory.readPointer(module, handles);
    check(module._netInterfaceTunnelCreate(handles + pointerBytes) === 0, 'Create Wasm outer decoder failed');
    const decoder = memory.readPointer(module, handles + pointerBytes);
    const socket = new net.Socket();
    const chunks = [];
    let offset = 0;
    let inputBytes = 0;
    let writeBlocked = false;
    let requested = false;
    let discovery;
    let unavailable = false;
    let failure;
    const deadline = performance.now() + 10000;
    const epoch = performance.now();
    socket.on('data', bytes => {
        if (inputBytes + bytes.length > 131072) { failure = new Error('TCP input budget exceeded'); return; }
        chunks.push(bytes);
        inputBytes += bytes.length;
        if (inputBytes >= 32768) socket.pause();
    });
    socket.on('error', error => { failure = error; });
    socket.on('drain', () => { writeBlocked = false; });
    socket.on('close', () => { if (!unavailable) failure = failure || new Error('TCP closed before test completion'); });
    await new Promise((resolve, reject) => {
        socket.once('error', reject);
        socket.connect(port, '127.0.0.1', resolve);
    });

    const readValue = (message, group, key) => {
        const result = module._netInterfaceMessageGetValue(message, group, key, value, messageValue, 4096, value + 4);
        check(result === 0, 'Wasm protobuf field missing or invalid');
        const type = module.HEAP32[value >>> 2];
        const size = module.HEAPU32[(value + 4) >>> 2];
        if (type === 1) {
            check(size === 4, 'Invalid protobuf int32 width');
            return module.HEAP32[messageValue >>> 2];
        }
        check(type === 4, 'Unexpected protobuf field type');
        return Buffer.from(module.HEAPU8.subarray(messageValue, messageValue + size)).toString('utf8');
    };

    try {
        while (!unavailable) {
            if (failure) throw failure;
            check(performance.now() < deadline, 'Production Wasm gateway test timed out');
            const now = Math.floor(performance.now() - epoch);
            check(module._netInterfaceLinkProgress(link, now, 0) >= 0, 'Authenticated management link failed');
            for (let step = 0; step < 64; ++step) {
                const decoded = module._netInterfaceTunnelPeek(decoder, fields, payloadPointer);
                if (decoded === 2) {
                    const accepted = module._netInterfaceLinkAccept(link, fields, memory.readPointer(module, payloadPointer));
                    if (accepted === -3) break;
                    check(accepted === 0 && module._netInterfaceTunnelConsume(decoder) === 0, 'Wasm rejected outer record');
                } else {
                    check(decoded === 1, 'Outer decoder failed');
                    if (!chunks.length) break;
                    const chunk = chunks[0];
                    const count = Math.min(chunk.length - offset, 65536);
                    module.HEAPU8.set(chunk.subarray(offset, offset + count), incoming);
                    const result = module._netInterfaceTunnelFeed(decoder, incoming, count, value);
                    const consumed = module.HEAPU32[value >>> 2];
                    check(result >= 0 && consumed > 0, 'TCP fragmentation did not advance decoder');
                    offset += consumed;
                    inputBytes -= consumed;
                    if (offset === chunk.length) { chunks.shift(); offset = 0; }
                    if (inputBytes < 16384) socket.resume();
                }
            }
            check(module._netInterfaceLinkProgress(link, now, 0) >= 0, 'Management proof or TLS verification failed');
            if (!requested && module._netInterfaceLinkStatus(link) === 2) {
                module.HEAPU32.set([3, 0, 0x89abcdef, 0x81234567, 0x4e470001, 0], fields >>> 2);
                check(module._netInterfaceLinkSendControl(link, fields, 0) === 0, 'Discovery request was not accepted');
                requested = true;
            }
            for (let step = 0; step < 64; ++step) {
                const pending = module._netInterfaceLinkPeekControl(link, fields, payloadPointer);
                if (pending === 1) break;
                check(pending === 2, 'Management reply parse failed');
                const header = Array.from(module.HEAPU32.subarray(fields >>> 2, (fields >>> 2) + 6));
                check(header[0] === 4 && header[3] === 0x81234567, 'Management reply request high bits were corrupted');
                check(module._netInterfaceMessageCreate(value + 8) === 0, 'Create protobuf response object failed');
                const message = memory.readPointer(module, value + 8);
                try {
                    check(module._netInterfaceMessageParse(message, memory.readPointer(module, payloadPointer), header[5]) === 0,
                        'Production Wasm protobuf3.3.0 rejected native response');
                    check(module._netInterfaceMessageGetCode(message, value, 0, 0, value + 4) === 0, 'Read management result failed');
                    const code = module.HEAP32[value >>> 2];
                    if (!discovery) {
                        check(header[2] === 0x89abcdef && header[4] === 0x4e470001 && code === 0 &&
                            readValue(message, 0, 1) === 1, 'Gateway discovery failed');
                        discovery = { node: readValue(message, 1, 2), address: readValue(message, 1, 3), port: readValue(message, 1, 4) };
                        check(/^node\.[0-9a-f]{32}$/.test(discovery.node) && discovery.address === '127.0.0.1' && discovery.port === port,
                            'Native discovered identity/address did not match the actual gateway');
                    } else {
                        check(header[2] === 0x89abcdf0 && header[4] === 0x4e470002 && code === -8,
                            'Unregistered service was not rejected');
                        unavailable = true;
                    }
                } finally {
                    module._netInterfaceMessageDestroy(message);
                }
                check(module._netInterfaceLinkConsumeControl(link) === 0, 'Consume management reply failed');
                if (discovery && !unavailable) {
                    check(module._netInterfaceMessageCreate(value + 8) === 0, 'Create service-open protobuf failed');
                    const open = memory.readPointer(module, value + 8);
                    try {
                        module.HEAP32[messageValue >>> 2] = 1;
                        check(module._netInterfaceMessageSetValue(open, 0, 8, 1, messageValue, 4) === 0,
                            'Set service-open target failed');
                        module.HEAPU32.set([1, 0], messageValue >>> 2);
                        check(module._netInterfaceMessageSetValue(open, 0, 9, 2, messageValue, 8) === 0,
                            'Set service-open channel failed');
                        check(module._netInterfaceMessageEncode(open, wire, 16408, value) === 0, 'Encode service-open request failed');
                        module.HEAPU32.set([3, 0, 0x89abcdf0, 0x81234567, 0x4e470002, module.HEAPU32[value >>> 2]], fields >>> 2);
                        check(module._netInterfaceLinkSendControl(link, fields, wire) === 0, 'Service-open request was not accepted');
                    } finally { module._netInterfaceMessageDestroy(open); }
                }
            }
            for (let step = 0; step < 32 && !writeBlocked; ++step) {
                const pending = module._netInterfaceLinkPeekOutput(link, fields, payloadPointer);
                if (pending === 1) break;
                check(pending === 2, 'Management transport output failed');
                check(module._netInterfaceTunnelEncode(fields, memory.readPointer(module, payloadPointer), wire, 16408, value) === 0,
                    'C++ outer record encoding failed');
                const count = module.HEAPU32[value >>> 2];
                const owned = Buffer.from(module.HEAPU8.subarray(wire, wire + count));
                writeBlocked = !socket.write(owned);
                check(module._netInterfaceLinkConsumeOutput(link) === 0, 'Copied output not consumed');
            }
            if (!unavailable) await new Promise(resolve => setTimeout(resolve, 2));
        }
        return { pass: true, discovery, requestHigh: '81234567', requestLow: '89abcdef', unregisteredServiceRejected: true };
    } finally {
        socket.destroy();
        module._netInterfaceLinkDestroy(link);
        module._netInterfaceTunnelDestroy(decoder);
        for (const pointer of allocations) module._free(pointer);
    }
}

async function main() {
    const options = {};
    for (const argument of process.argv.slice(2)) {
        const split = argument.indexOf('=');
        check(split > 0, 'Expected named arguments');
        options[argument.slice(0, split)] = argument.slice(split + 1);
    }
    check(options['--module'] && options['--server'] && options['--output'], 'Required --module, --server and --output');
    const output = path.resolve(options['--output']);
    check(!fs.existsSync(output), 'Use a fresh owned output directory');
    fs.mkdirSync(output, { recursive: true });
    const module = await createRuntime(path.resolve(options['--module']));
    const gateway = startGateway(path.resolve(options['--server']), output);
    let result;
    try {
        const port = await gateway.ready;
        result = await exercise(module, port);
        fs.writeFileSync(gateway.stop, '');
        const stopped = await gateway.completed;
        check(stopped.code === 0 && !stopped.signal, 'Native v3 gateway did not stop normally');
    } catch (error) {
        gateway.child.kill();
        await gateway.completed;
        throw error;
    }
    fs.writeFileSync(path.join(output, 'result.json'), JSON.stringify(result, null, 2));
    console.log('Production WX Wasm -> real TCP -> native v3 gateway: PASS');
}

main().catch(error => { console.error(error.stack || String(error)); process.exitCode = 1; });