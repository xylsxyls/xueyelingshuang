'use strict';
const { readPointer } = require('./WasmMemory');

// Thin host bridge: C++ owns framing, tokens, hashes, offset/window checks and
// completion. io is borrowed and implements async read/write/flush/commit/close.
// Never run synchronous filesystem calls in socket callbacks.
class NetWxFileFlow {
    constructor(client, metadata, product, sender, io) {
        this.client = typeof client.connection === 'function' ? client.connection() : client;
        this.module = this.client.runtime.module;
        this.io = io;
        this.sender = !!sender;
        this.handle = 0;
        this.memory = 0;
        this.busy = false;
        this.closed = false;
        this.terminal = null;
        this.error = null;
        this.timer = null;
        this.pollQueued = false;
        this.primeOffset = 0;
        this.completed = new Promise(resolve => { this.resolveCompleted = resolve; });
        if (!(metadata instanceof Uint8Array) || metadata.length !== 160 || !io || typeof io.read !== 'function' ||
            typeof io.close !== 'function' || (!sender && (typeof io.write !== 'function' || typeof io.flush !== 'function' ||
            typeof io.commit !== 'function'))) throw new Error('File host adapter is incomplete');
        try {
            this.memory = this.module._malloc(66000);
            if (!this.memory) throw new Error('File memory budget exceeded');
            this.module.HEAPU8.set(metadata, this.memory + 128);
            this.check(this.module._netInterfaceFileInfoFields(this.memory + 128, 160, this.memory,
                this.memory + 48, this.memory + 64, this.memory + 96));
            const fields = this.module.HEAPU32.slice(this.memory >>> 2, (this.memory + 48) >>> 2);
            this.resume = this.safeOffset(fields[6], fields[7]);
            this.stream = [fields[8], fields[9]];
            this.product = product;
            const now = this.time();
            this.check(this.module._netInterfaceFileFlowCreateWire(this.memory + 128, 160, product, sender ? 1 : 0,
                now[0], now[1], 15000, this.memory));
            this.handle = readPointer(this.module, this.memory);
            // Optional host capability. Old cores and hosts keep single-block
            // writes; DATA and ACK retain the same wire format and durability.
            this.batchWrites = false;
            const batch = Math.min(fields[3], Math.floor((Number(io.maxWriteBytes) || fields[2]) / fields[2]));
            if (!sender && batch > 1 && typeof this.module._netInterfaceFileFlowSetWriteBatchChunks === 'function') {
                this.check(this.module._netInterfaceFileFlowSetWriteBatchChunks(this.handle, batch));
                this.batchWrites = true;
            }
        } catch (error) {
            this.destroy();
            throw error;
        }
    }

    check(result) {
        if (result < 0) {
            const error = new Error('File operation failed: ' + result);
            error.code = result;
            throw error;
        }
        return result;
    }

    time() {
        const now = Math.floor(this.client.options.clock());
        if (!Number.isSafeInteger(now) || now < 0) throw new Error('Invalid monotonic file clock');
        return [now >>> 0, Math.floor(now / 4294967296)];
    }

    safeOffset(low, high) {
        const value = low + high * 4294967296;
        if (!Number.isSafeInteger(value)) throw new Error('Host filesystem offset exceeds its safe range');
        return value;
    }

    // Return false without side effects when C++ has an unfinished durable write;
    // NetWxClient retains the current complete frame and retries it later.
    accept(frame) {
        if (!this.stream || frame.product !== this.product || frame.requestLow !== this.stream[0] || frame.requestHigh !== this.stream[1]) return false;
        if (this.closed || !this.handle) return true;
        const bytes = frame.payload;
        if (!(bytes instanceof Uint8Array) || bytes.length > 65624) throw new Error('File frame exceeds budget');
        this.module.HEAPU32.set([frame.kind, frame.product, frame.requestLow, frame.requestHigh, frame.type, bytes.length], this.memory >>> 2);
        this.module.HEAPU8.set(bytes, this.memory + 320);
        const result = this.module._netInterfaceFileFlowAccept(this.handle, this.memory, this.memory + 320);
        if (result === -3) return false;
        this.check(result);
        // Drain this socket delivery before crossing the host file bridge.
        // A timer can be clamped to one second in the simulator/background;
        // partial windows must also progress without waiting for that timer.
        if (this.batchWrites) this.queuePoll();
        else this.poll();
        return true;
    }

    queuePoll() {
        if (this.closed || this.pollQueued) return;
        this.pollQueued = true;
        Promise.resolve().then(() => { this.pollQueued = false; this.poll(); });
    }

    schedule() {
        if (!this.closed && this.timer === null) this.timer = setTimeout(() => { this.timer = null; this.poll(); }, 25);
    }

    job(work, completed) {
        this.busy = true;
        Promise.resolve().then(work).then(value => {
            if (!this.closed) completed(value);
        }).catch(error => { this.error = error; this.cancel(error.code < 0 ? error.code : -4); }).finally(() => {
            this.busy = false;
            if (!this.closed) { this.client.schedulePump(); this.poll(); }
            else if (this.finishing) this.closeFinished();
        });
    }

    poll() {
        if (this.closed || !this.handle) return;
        try {
            const now = this.time();
            const progress = this.module._netInterfaceFileFlowProgress(this.handle, now[0], now[1]);
            this.check(progress);
            if (!this.busy && this.primeOffset < this.resume) {
                const offset = this.primeOffset;
                const size = Math.min(65536, this.resume - offset);
                this.job(() => this.io.read(offset, size), bytes => {
                    if (!(bytes instanceof Uint8Array) || bytes.length !== size) throw new Error('Short file prefix');
                    this.module.HEAPU8.set(bytes, this.memory + 320);
                    this.check(this.module._netInterfaceFileFlowPrime(this.handle, offset >>> 0, Math.floor(offset / 4294967296),
                        this.memory + 320, size));
                    this.primeOffset += size;
                });
            } else if (!this.busy && this.sender) {
                const result = this.module._netInterfaceFileFlowRead(this.handle, this.memory);
                this.check(result);
                if (result === 2) {
                    const fields = this.module.HEAPU32.slice(this.memory >>> 2, (this.memory + 12) >>> 2);
                    const offset = this.safeOffset(fields[0], fields[1]);
                    this.job(() => this.io.read(offset, fields[2]), bytes => {
                        if (!(bytes instanceof Uint8Array) || bytes.length !== fields[2]) throw new Error('Short file block');
                        this.module.HEAPU8.set(bytes, this.memory + 320);
                        this.check(this.module._netInterfaceFileFlowProduced(this.handle, fields[0], fields[1], this.memory + 320, bytes.length));
                    });
                }
            } else if (!this.busy) {
                const written = this.module._netInterfaceFileFlowWrite(this.handle, this.memory, this.memory + 24);
                this.check(written);
                if (written === 2) {
                    const fields = this.module.HEAPU32.slice(this.memory >>> 2, (this.memory + 12) >>> 2);
                    const pointer = readPointer(this.module, this.memory + 24);
                    const bytes = this.module.HEAPU8.slice(pointer, pointer + fields[2]);
                    const offset = this.safeOffset(fields[0], fields[1]);
                    this.job(async () => { await this.io.write(offset, bytes); await this.io.flush(); }, () => {
                        bytes.fill(0);
                        this.check(this.module._netInterfaceFileFlowWritten(this.handle, fields[0], fields[1], fields[2], 0));
                    });
                } else if (this.module._netInterfaceFileFlowTakeCommit(this.handle) === 2) {
                    this.job(async () => {
                        await this.io.flush();
                        if (this.closed) return;
                        await this.io.close();
                        this.ioClosed = true;
                        if (this.closed) return;
                        await this.io.commit();
                    }, () => {
                        this.check(this.module._netInterfaceFileFlowCommitted(this.handle, 0));
                    });
                }
            }
            const output = this.module._netInterfaceFileFlowPeek(this.handle, this.memory, this.memory + 24);
            this.check(output);
            if (output === 2) {
                const fields = Array.from(this.module.HEAPU32.slice(this.memory >>> 2, (this.memory + 24) >>> 2));
                const pointer = readPointer(this.module, this.memory + 24);
                const bytes = this.module.HEAPU8.slice(pointer, pointer + fields[5]);
                try {
                    this.client.sendFrame(fields.slice(0, 5), bytes);
                    this.check(this.module._netInterfaceFileFlowConsume(this.handle));
                } catch (error) {
                    if (error.code !== -3) throw error;
                } finally { bytes.fill(0); }
            } else if (progress === 2 && !this.busy) {
                this.terminal = 0;
                this.finish();
                return;
            }
            this.schedule();
        } catch (error) { this.error = error; this.cancel(error.code < 0 ? error.code : -4); }
    }

    cancel(reason = -10) {
        if (this.closed) return;
        this.terminal = reason < 0 ? reason : -10;
        if (this.handle) this.module._netInterfaceFileFlowCancel(this.handle, this.terminal);
        if (this.client.ready && !this.client.disposed && this.stream) {
            try { this.client.sendFrame([6, this.product, this.stream[0], this.stream[1], 60005], new Uint8Array(0)); }
            catch (_) { this.client.fail(new Error('File cancellation could not be admitted')); }
        }
        this.finish();
    }

    finish() {
        if (this.finishing) return;
        this.finishing = true;
        clearTimeout(this.timer);
        this.timer = null;
        // An in-flight host job invokes closeFinished from its own settlement.
        // Do not wait for a clamped timer to release the file/network slot.
        this.closed = true;
        this.closeFinished();
    }

    closeFinished() {
        if (this.busy || this.closing) return;
        this.closing = true;
        Promise.resolve().then(() => this.ioClosed ? null : this.io.close()).catch(error => {
            if (!this.terminal) { this.error = error; this.terminal = -7; }
        })
            .finally(() => this.destroy());
    }

    destroy() {
        if (this.busy) { this.cancel(); return; }
        this.closed = true;
        clearTimeout(this.timer);
        if (this.handle) this.module._netInterfaceFileFlowDestroy(this.handle);
        if (this.memory) {
            this.module.HEAPU8.fill(0, this.memory, this.memory + 66000);
            this.module._free(this.memory);
        }
        this.handle = this.memory = 0;
        if (this.resolveCompleted) {
            this.resolveCompleted(this.terminal);
            this.resolveCompleted = null;
        }
    }

    done() {
        return this.closed && !this.busy && !this.handle;
    }

    wait() { return this.completed; }
}

module.exports = { NetWxFileFlow };