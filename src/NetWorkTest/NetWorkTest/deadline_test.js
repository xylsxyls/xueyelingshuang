'use strict';

const assert = require('node:assert/strict');
const Deadline = require('./NetTestDeadline');

async function main() {
    let checked = 0;
    for (const value of [0, -1, 0.5, NaN, Infinity, 2147483648]) {
        assert.throws(() => new Deadline(value)); ++checked;
    }
    const ordinary = new Deadline(5000);
    try {
        assert.equal(await ordinary.wait(Promise.resolve(123)), 123); ++checked;
        await assert.rejects(ordinary.wait(Promise.reject(new Error('underlying failure'))), /underlying failure/); ++checked;
    } finally { ordinary.close(); }
    await assert.rejects(ordinary.wait(Promise.resolve(1)), /closed/); ++checked;
    await assert.rejects(ordinary.wait(Promise.reject(new Error('after close'))), /closed/); ++checked;
    const expired = new Deadline(20, 'bounded');
    let release;
    let executedAfterWait = false;
    const operation = (async () => {
        await expired.wait(new Promise(resolve => { release = resolve; }));
        executedAfterWait = true;
    })();
    await assert.rejects(operation, /bounded deadline exceeded/); ++checked;
    release();
    await new Promise(resolve => setImmediate(resolve));
    assert.equal(executedAfterWait, false); ++checked;
    await assert.rejects(expired.wait(Promise.resolve()), /deadline exceeded/); ++checked;
    const cancelled = new Deadline(5000);
    const pending = cancelled.wait(new Promise(() => {}));
    cancelled.close();
    await assert.rejects(pending, /closed/); ++checked;
    cancelled.close();
    assert.equal(cancelled.closed, true); ++checked;
    console.log('DEADLINE_ASSERTIONS checks=' + checked + ' PASS');
}

main().catch(error => { console.error(error.stack || error); process.exitCode = 1; });