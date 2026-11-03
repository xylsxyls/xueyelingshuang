'use strict';
// Basic portable host-bridge evidence in Node WebAssembly. This is not a
// WXWebAssembly, TCPSocket, WeChat filesystem or device qualification result.
const assert = require('node:assert/strict');
const fs = require('node:fs');
const fsp = require('node:fs/promises');
const path = require('node:path');
const crypto = require('node:crypto');
const { performance } = require('node:perf_hooks');
const { setModuleBits } = require('./host/WasmMemory');

async function main() {
    const args = Object.fromEntries(process.argv.slice(2).map(value => {
        const split = value.indexOf('=');
        if (split < 0) throw new Error('Expected key=value');
        return [value.slice(0, split), value.slice(split + 1)];
    }));
    if (!path.isAbsolute(args['--module'] || '') || !path.isAbsolute(args['--report'] || '')) throw new Error('Absolute module/report required');
    const directory = path.join(path.dirname(args['--report']), 'wasm-file-owned-' + crypto.randomBytes(8).toString('hex'));
    await fsp.mkdir(directory, { recursive: true });
    const factory = require(args['--module']);
    const module = setModuleBits(await factory({ wasmBinary: require('./host/read_wasm').readWasm(args['--module']) }), factory);
    const NetWxFileFlow = module.NetFileTransfer;
    assert.equal(typeof NetWxFileFlow, 'function', 'Use the generated NetPeer file facade');
    const expected = Buffer.from(Array.from({ length: 2357 }, (_, index) => (index * 17) & 255));
    const hash = crypto.createHash('sha256').update(expected).digest();
    const source = path.join(directory, 'source.bin');
    await fsp.writeFile(source, expected, { flag: 'wx' });
    const reports = [];
    for (const direction of [1, 2]) for (const batch of [false, true]) {
        const start = performance.now();
        const part = path.join(directory, 'direction-' + direction + '-' + batch + '.part');
        const final = path.join(directory, 'direction-' + direction + '-' + batch + '.bin');
        const reading = await fsp.open(source, 'r');
        const writing = await fsp.open(part, 'wx+');
        let readClosed = false;
        let writeClosed = false;
        const metadata = Buffer.alloc(160);
        metadata.write('NFI2');
        for (const [offset, value] of [[4, 2], [8, direction], [12, 2], [16, 1024], [20, 2]]) metadata.writeUInt32BE(value, offset);
        metadata.writeBigUInt64BE(BigInt(expected.length), 24);
        metadata.writeBigUInt64BE(0xfedcba9876543200n + BigInt(direction), 40);
        metadata.writeBigUInt64BE(BigInt(Math.floor(Date.now() / 1000) + 60), 48);
        crypto.randomBytes(16).copy(metadata, 56);
        crypto.randomBytes(32).copy(metadata, 72);
        hash.copy(metadata, 104);
        const queue = [];
        let sender;
        let receiver;
        const writes = [];
        let durable = 0;
        let written = 0;
        const makeClient = target => ({ runtime: { module }, ready: true, disposed: false,
            options: { clock: () => Math.floor(performance.now()) }, schedulePump() {},
            sendFrame(fields, bytes) {
                if (target === 'sender' && fields[0] === 10) assert.equal(durable, written, 'ACK waits for actual flush');
                queue.push({ target, frame: { kind: fields[0], product: fields[1],
                requestLow: fields[2], requestHigh: fields[3], type: fields[4], payload: bytes.slice() } }); },
            fail(error) { throw error; } });
        sender = new NetWxFileFlow(makeClient('receiver'), new Uint8Array(metadata), 4, true, {
            async read(offset, size) {
                const bytes = Buffer.alloc(size);
                const result = await reading.read(bytes, 0, size, offset);
                assert.equal(result.bytesRead, size);
                return new Uint8Array(bytes);
            },
            async close() { if (!readClosed) { await reading.close(); readClosed = true; } }
        });
        receiver = new NetWxFileFlow(makeClient('sender'), new Uint8Array(metadata), 4, false, {
            maxWriteBytes: batch ? 2048 : 1024,
            async read(offset, size) {
                const bytes = Buffer.alloc(size);
                const result = await writing.read(bytes, 0, size, offset);
                assert.equal(result.bytesRead, size);
                return new Uint8Array(bytes);
            },
            async write(offset, bytes) {
                writes.push(bytes.length);
                const result = await writing.write(bytes, 0, bytes.length, offset);
                assert.equal(result.bytesWritten, bytes.length);
                written = offset + bytes.length;
            },
            async flush() {
                await new Promise(resolve => setTimeout(resolve, 3));
                await writing.sync();
                durable = written;
            },
            async close() { if (!writeClosed) { await writing.close(); writeClosed = true; } },
            async commit() { await fsp.rename(part, final); }
        });
        try {
            sender.poll(); receiver.poll();
            while (!sender.done() || !receiver.done()) {
                if (performance.now() - start > 5000) throw new Error('Basic file host test deadline');
                for (let count = 0; queue.length && count < 16; ++count) {
                    const item = queue[0];
                    // Deliver a complete negotiated window in one socket turn,
                    // followed by the short tail, as a coalesced TCP read does.
                    if (batch && item.target === 'receiver' && item.frame.payload.length === 1112 && queue.length < 2 && count === 0) break;
                    if (!(item.target === 'receiver' ? receiver : sender).accept(item.frame)) break;
                    queue.shift();
                }
                sender.poll();
                if (!batch) receiver.poll();
                await new Promise(resolve => setTimeout(resolve, 1));
            }
            assert.equal(sender.terminal, 0);
            assert.equal(receiver.terminal, 0);
            assert.deepEqual(await Promise.all([sender.wait(), receiver.wait()]), [0, 0]);
            assert.deepEqual(await fsp.readFile(final), expected);
            assert.equal(queue.length, 0);
            assert.ok(writes.every(size => size <= (batch ? 2048 : 1024)));
            if (batch) assert.ok(writes.some(size => size > 1024), 'The shared core combines multiple DATA frames');
            reports.push({ direction, batch, writes, state: 'PASS', elapsed_ms: Math.round(performance.now() - start) });
        } finally {
            sender.cancel(); receiver.cancel();
            while (!sender.done() || !receiver.done()) await new Promise(resolve => setTimeout(resolve, 5));
        }
        let releaseRead;
        let closed = 0;
        const cancelled = new NetWxFileFlow(makeClient('receiver'), new Uint8Array(metadata), 4, true, {
            read: () => new Promise(resolve => { releaseRead = resolve; }),
            async close() { ++closed; }
        });
        cancelled.poll();
        await Promise.resolve();
        assert.equal(typeof releaseRead, 'function');
        cancelled.cancel();
        assert.equal(cancelled.done(), false, 'Cancellation must wait for the outstanding host read');
        assert.equal(closed, 0);
        releaseRead(new Uint8Array(1024));
        assert.equal(await cancelled.wait(), -10);
        assert.equal(closed, 1);
        assert.equal(cancelled.done(), true);
        reports.push({ direction, name: 'cancel during host read settles directly after close', state: 'PASS' });
        let failedClosed = 0;
        const hostError = new Error('ENOSPC: host filesystem quota exceeded');
        const failed = new NetWxFileFlow(makeClient('unused'), new Uint8Array(metadata), 4, true, {
            async read() { throw hostError; },
            async close() { ++failedClosed; }
        });
        failed.poll();
        assert.equal(await failed.wait(), -4);
        assert.equal(failed.error, hostError, 'Preserve the actual host cause for bounded quota recovery');
        assert.equal(failedClosed, 1);
        assert.equal(failed.done(), true);
        reports.push({ direction, name: 'host filesystem failure retains cause after close', state: 'PASS' });
        if (direction === 2) {
            // Empty immutable file reaches the real C++ END/commit handshake
            // without unrelated data writes. Hold either flush or close, cancel,
            // then release the host operation: the final name must not appear.
            const emptyInfo = Buffer.from(metadata);
            emptyInfo.writeBigUInt64BE(0n, 24);
            crypto.createHash('sha256').update(Buffer.alloc(0)).digest().copy(emptyInfo, 104);
            const cancelPart = path.join(directory, 'cancel-commit-' + batch + '.part');
            const cancelFinal = path.join(directory, 'cancel-commit-' + batch + '.bin');
            await fsp.writeFile(cancelPart, Buffer.alloc(0), { flag: 'wx' });
            let release, enter, commits = 0, closes = 0;
            const entered = new Promise(resolve => { enter = resolve; });
            const gate = () => new Promise(resolve => { release = resolve; enter(); });
            const emptySender = new NetWxFileFlow(makeClient('receiver'), new Uint8Array(emptyInfo), 4, true, {
                async read() { throw new Error('Empty file must not read data'); }, async close() {}
            });
            const emptyReceiver = new NetWxFileFlow(makeClient('sender'), new Uint8Array(emptyInfo), 4, false, {
                async read() { throw new Error('Empty file must not read data'); }, async write() {},
                async flush() { if (!batch) await gate(); },
                async close() { ++closes; if (batch) await gate(); },
                async commit() { ++commits; await fsp.rename(cancelPart, cancelFinal); }
            });
            try {
                queue.length = 0;
                emptySender.poll();
                assert.equal(queue.length, 1);
                assert.equal(emptyReceiver.accept(queue.shift().frame), true);
                await entered;
                emptyReceiver.cancel();
                release();
                assert.equal(await emptyReceiver.wait(), -10);
                assert.equal(commits, 0, 'Cancellation before publication must not call commit');
                assert.equal(fs.existsSync(cancelFinal), false);
                assert.equal(closes, 1, 'Cancellation closes the host exactly once');
                reports.push({ name: 'cancel during final ' + (batch ? 'close' : 'flush') + ' prevents publication', state: 'PASS' });
            } finally {
                if (release) release();
                emptySender.cancel(); emptyReceiver.cancel();
                await Promise.all([emptySender.wait(), emptyReceiver.wait()]);
                queue.length = 0;
            }
        }
    }
    await fsp.writeFile(args['--report'], JSON.stringify({ engine: 'Node WebAssembly + shared C++ file flow + async native test filesystem',
        artifacts: directory, cases: reports }, null, 2));
    console.log('Wasm file host bridge: ' + reports.length + '/' + reports.length + ' PASS');
}

main().catch(error => { console.error(error.stack); process.exitCode = 1; });