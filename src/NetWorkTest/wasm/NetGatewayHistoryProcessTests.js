'use strict';
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const net = require('node:net');
const crypto = require('node:crypto');
const { start } = require('../scripts/local_link_gateway_process_test');
const { readWasm } = require('../scripts/host/read_wasm');
const { NetInterfaceRuntime } = require('../scripts/host/NetInterfaceRuntime');
const { NetInterfaceLink } = require('../scripts/host/NetInterfaceLink');
const { NetInterfaceMessage } = require('../scripts/host/NetInterfaceMessage');
const delay = ms => new Promise(resolve => setTimeout(resolve, ms));

async function exercise(modulePath, port, baseline) {
    const runtime = await NetInterfaceRuntime.load(require(modulePath), {
        randomSource: size => crypto.randomBytes(size), moduleOptions: { wasmBinary: readWasm(modulePath) } });
    const link = new NetInterfaceLink(runtime);
    const message = new NetInterfaceMessage(runtime);
    const incoming = [];
    const responses = new Set();
    let connected = false;
    let closed = false;
    let receivedBytes = 0;
    const socket = net.createConnection({ host: '127.0.0.1', port });
    socket.on('connect', () => { connected = true; });
    socket.on('data', bytes => { receivedBytes += bytes.length; incoming.push({ bytes, offset: 0 }); });
    socket.on('error', () => { closed = true; });
    socket.on('close', () => { closed = true; });
    async function pump(predicate, timeout = 10000) {
        const deadline = Date.now() + timeout;
        while (!predicate() && !closed && Date.now() < deadline) {
            if (connected) {
                for (let step = 0; step < 64; ++step) {
                    if (incoming.length) {
                        const first = incoming[0];
                        const consumed = link.feed(first.bytes.subarray(first.offset));
                        first.offset += consumed;
                        receivedBytes -= consumed;
                        if (first.offset === first.bytes.length) incoming.shift();
                    }
                    link.progress(Math.floor(performance.now()));
                    let response;
                    while ((response = link.takeControl())) {
                        assert.equal(response.fields[0], 4);
                        responses.add(response.fields[2]);
                    }
                    const writes = [];
                    let bytes;
                    while ((bytes = link.pullOutput())) writes.push(Buffer.from(bytes));
                    if (writes.length) socket.write(Buffer.concat(writes));
                    if (!incoming.length) break;
                }
            }
            assert.ok(receivedBytes <= 2097152 && incoming.length <= 128);
            await runtime.prepare();
            await delay(1);
        }
        return predicate();
    }
    try {
        assert.ok(await pump(() => connected && link.progress(Math.floor(performance.now())) === 2));
        const count = 8193;
        for (let batch = 1; batch <= count; batch += 32) {
            const last = Math.min(count, batch + 31);
            for (let channel = batch; channel <= last; ++channel) {
                message.clear();
                const value = new Uint8Array(8);
                new DataView(value.buffer).setUint32(0, channel, true);
                message.setValue(0, 8, 1, new Uint8Array([0xcf, 7, 0, 0]));
                message.setValue(0, 9, 2, value);
                assert.ok(link.sendControl([3, 0, channel, 0, 0x4e470002], message.encode()));
            }
            assert.ok(await pump(() => responses.has(last)), 'Authenticated OPEN response missing before history eviction');
            assert.equal(responses.size, last);
        }
        assert.ok(link.sendRecord([4, 1999, 1, 0], new Uint8Array(4)));
        assert.ok(link.sendControl([3, 0, count + 1, 0, 0x4e470001], new Uint8Array()));
        const survived = await pump(() => responses.has(count + 1));
        if (baseline) {
            assert.equal(survived, false);
            assert.equal(closed, true);
            return { status: 'BASELINE_REPRODUCED', authenticatedRejectedOpens: count,
                lateOwnedCloseDisconnectedSharedPeer: true };
        }
        assert.equal(survived, true, 'An obsolete CLOSE must not terminate the authenticated physical connection');
        message.clear();
        message.setValue(0, 9, 2, new Uint8Array([1, 0, 0, 0, 0, 0, 0, 0]));
        message.setValue(0, 14, 1, new Uint8Array([4, 0, 0, 0]));
        assert.ok(link.sendControl([5, 0, 0, 0, 0x4e470005], message.encode()));
        assert.ok(link.sendControl([5, 0, 0, 0, 0x4e470006], (() => {
            message.clear();
            message.setValue(0, 9, 2, new Uint8Array([1, 0, 0, 0, 0, 0, 0, 0]));
            message.setValue(0, 13, 1, new Uint8Array(4));
            return message.encode();
        })()));
        assert.ok(link.sendControl([3, 0, count + 2, 0, 0x4e470001], new Uint8Array()));
        assert.ok(await pump(() => responses.has(count + 2)));
        assert.ok(link.sendRecord([4, 1999, count + 1, 0], new Uint8Array(4)));
        assert.equal(await pump(() => closed), true, 'Unallocated future channels must still be rejected');
        return { status: 'PASS', authenticatedRejectedOpens: count, obsoleteCloseCreditAndEventIgnored: true,
            futureChannelRejected: true };
    } finally { socket.destroy(); message.dispose(); link.dispose(); }
}

async function main() {
    const [serverPath, modulePath, destination, mode] = process.argv.slice(2);
    const output = path.resolve(destination);
    assert.equal(fs.existsSync(output), false);
    fs.mkdirSync(output, { recursive: true });
    const gateway = start(path.resolve(serverPath), ['--protocol=3', '--listen=127.0.0.1', '--port=0',
        '--identity-directory=' + path.join(output, 'identity'), '--stop-file=' + path.join(output, 'gateway.stop'),
        '--run-for-ms=30000'], 'gateway', output, /GATEWAY_READY port=(\d+) pid=(\d+)/);
    let timer;
    try {
        const result = await Promise.race([(async () => {
            const ready = await gateway.ready;
            return exercise(path.resolve(modulePath), Number(ready[1]), mode === 'baseline');
        })(), new Promise((resolve, reject) => { timer = setTimeout(() => reject(new Error('History process deadline')), 25000); })]);
        fs.writeFileSync(path.join(output, 'gateway.stop'), '');
        const stopped = await gateway.completed;
        assert.equal(stopped.code, 0);
        fs.writeFileSync(path.join(output, 'result.json'), JSON.stringify(result, null, 2));
        console.log(JSON.stringify(result));
    } finally {
        clearTimeout(timer);
        if (gateway.child.exitCode === null && gateway.child.signalCode === null) gateway.child.kill();
        await gateway.completed;
    }
}

main().catch(error => { console.error(error.stack); process.exitCode = 1; });
