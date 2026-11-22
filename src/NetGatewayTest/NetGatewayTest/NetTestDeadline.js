'use strict';

// 每个异步步骤共享同一截止时间；过期后拒绝所有后续步骤，不留下仍会启动子进程的旧任务。
const assert = require('node:assert/strict');
const { performance } = require('node:perf_hooks');

class NetTestDeadline {
    constructor(milliseconds, label) {
        assert.ok(Number.isSafeInteger(milliseconds) && milliseconds > 0 && milliseconds <= 2147483647);
        this.label = String(label || 'Test');
        this.end = performance.now() + milliseconds;
        this.closed = false;
        this.failure = null;
        this.deadline = new Promise((resolve, reject) => { this.reject = reject; });
        // 同步异常也会进入close，拒绝通知必须始终有人观察。
        this.deadline.catch(() => {});
        this.timer = setTimeout(() => this.expire(), milliseconds);
    }

    expire() {
        if (this.closed) return;
        this.failure = new Error(this.label + ' deadline exceeded');
        this.close();
    }

    check() {
        if (!this.closed && performance.now() >= this.end) this.expire();
        if (this.closed) throw this.failure || new Error(this.label + ' already closed');
    }

    async wait(pending) {
        // 即使调用前已过期，也要观察传入Promise的拒绝，避免未处理的异步异常。
        const observed = Promise.resolve(pending);
        observed.catch(() => {});
        this.check();
        const value = await Promise.race([observed, this.deadline]);
        this.check();
        return value;
    }

    close() {
        if (this.closed) return;
        this.closed = true;
        clearTimeout(this.timer);
        this.reject(this.failure || new Error(this.label + ' closed'));
    }
}

module.exports = NetTestDeadline;