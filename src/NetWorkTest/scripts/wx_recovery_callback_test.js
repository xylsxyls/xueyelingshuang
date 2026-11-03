'use strict';

// Exercise public recovery lifecycle with a deterministic host clock. No socket
// or Wasm state is fabricated: each fake attempt supplies only the host contract.
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const { createRequire } = require('node:module');
const tick = async () => { for (let i = 0; i < 12; ++i) await Promise.resolve(); };

async function checkReadyDisconnect() {
    const filename = path.resolve(__dirname, 'host/NetWxServiceClient.js');
    const module = { exports: {} }, timers = new Map();
    let serial = 0;
    vm.runInNewContext(fs.readFileSync(filename, 'utf8'), { module, require: createRequire(filename),
        Uint8Array, DataView, console, setTimeout(fn, ms) { timers.set(++serial, { fn, ms }); return serial; },
        clearTimeout(id) { timers.delete(id); }
    }, { filename });
    const attempts = [];
    let readyCalls = 0, resolution = 'pending';
    const client = new module.exports.NetWxServiceClient({
        address: '127.0.0.1', port: 5203, runtime: { randomSource: () => new Uint8Array(4) },
        onReady(owner) { if (++readyCalls === 1) owner.fail(new Error('Disconnected during ready callback')); }
    }, options => {
        const attempt = { ready: false, disposed: false,
            connect() { this.ready = true; return Promise.resolve(); },
            fail(error) { this.ready = false; this.disposed = true; options.onClose(error); },
            dispose() { this.ready = false; this.disposed = true; }
        };
        attempts.push(attempt);
        return attempt;
    });
    try {
        const connected = client.connect().then(() => { resolution = 'ready'; }, () => { resolution = 'failed'; });
        await tick();
        assert.equal(resolution, 'pending', 'connect must not resolve for a disconnected generation');
        assert.equal(client.ready, false);
        assert.equal([...timers.values()].filter(t => t.ms === 30000).length, 1,
            'A disconnect inside onReady must retain the total recovery deadline');
        const retry = [...timers.entries()].find(([, t]) => t.ms < 30000);
        assert.ok(retry, 'Recovery must still schedule another attempt');
        timers.delete(retry[0]); retry[1].fn();
        await tick();
        await connected;
        assert.equal(resolution, 'ready');
        assert.equal(attempts.length, 2);
        assert.equal(timers.size, 0, 'Successful replacement clears all recovery timers');
    } finally { client.dispose(); }
}

checkReadyDisconnect().then(() => console.log('PASS recovery ready-callback disconnect, pending connect and deadline'))
    .catch(error => { console.error(error.stack || error); process.exitCode = 1; });