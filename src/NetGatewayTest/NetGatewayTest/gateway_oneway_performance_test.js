'use strict';

// 真实单向链路：NetWork -> TLS/TCP -> 网关 -> ProcessWork -> 业务TLS解密校验。
// 开始和结束取同一Windows主机的QPC；stdout只传统计，不参与结束时间。
const fs = require('fs');
const path = require('path');
const assert = require('assert').strict;
const { start } = require('./local_link_gateway_process_test');
const validate = require('./NetPerformanceAssertions');
const Deadline = require('./NetTestDeadline');

async function main() {
    assert.equal(process.platform, 'win32', 'Cross-process QPC verified on Windows only');
    const options = {};
    for (const argument of process.argv.slice(2)) {
        const split = argument.indexOf('=');
        assert.ok(split > 0);
        const key = argument.slice(0, split);
        assert.ok(['--server', '--client', '--output', '--sizes', '--count', '--large-count', '--repeats', '--producers', '--methods', '--sessions', '--workers', '--queue-initial', '--producer-cpu', '--owner-cpu'].includes(key) && !Object.prototype.hasOwnProperty.call(options, key));
        assert.ok(argument.length > split + 1, 'Empty option: ' + key);
        options[key] = argument.slice(split + 1);
    }
    for (const key of ['--server', '--client', '--output']) assert.ok(path.isAbsolute(options[key] || ''), key);
    const list = (key, fallback, allowed) => {
        const values = (options[key] || fallback).split(',');
        assert.ok(values.length && values.every(value => allowed.includes(value)) && new Set(values).size === values.length, key);
        return values;
    };
    const sizes = list('--sizes', '6', ['0', '6', '1024', '65536']).map(Number);
    const producers = list('--producers', '1,4', ['1', '4']).map(Number);
    const sessionsList = list('--sessions', '1', ['1', '4']).map(Number);
    const workers = Number(options['--workers'] || 4);
    assert.ok(Number.isInteger(workers) && workers >= 1 && workers <= 16);
    const queueInitial = Number(options['--queue-initial'] || 128);
    assert.ok(Number.isInteger(queueInitial) && queueInitial >= 128 && queueInitial <= 65536);
    const pinned = options['--producer-cpu'] !== undefined || options['--owner-cpu'] !== undefined;
    const producerCpu = pinned ? Number(options['--producer-cpu']) : null;
    const ownerCpu = pinned ? Number(options['--owner-cpu']) : null;
    if (pinned) {
        assert.ok(Number.isInteger(producerCpu) && producerCpu >= 0 && producerCpu < 32 &&
            Number.isInteger(ownerCpu) && ownerCpu >= 0 && ownerCpu < 32 && producerCpu !== ownerCpu);
        assert.ok(producers.length === 1 && producers[0] === 1 && sessionsList.length === 1 && sessionsList[0] === 1 && workers === 1);
    }
    const methods = list('--methods', 'send,post', ['send', 'post']);
    const count = Number(options['--count'] || 65536);
    const repeats = Number(options['--repeats'] || 3);
    const largeCount = Number(options['--large-count'] || 256);
    assert.ok(Number.isInteger(count) && count >= 8 && count <= 10000000);
    assert.ok(Number.isInteger(largeCount) && largeCount >= 8 && largeCount <= 100000);
    assert.ok(Number.isInteger(repeats) && repeats >= 1 && repeats <= 9);
    const output = path.resolve(options['--output']);
    assert.ok(!fs.existsSync(output), 'Use a fresh isolated output');
    fs.mkdirSync(output, { recursive: true });
    const results = [];
    const failures = [];
    for (let repeat = 0; repeat < repeats; ++repeat) {
        for (const size of sizes) for (const threads of producers) for (const method of methods) for (const sessions of sessionsList) {
            if (sessions > threads) continue;
            const measuredCount = size >= 65536 ? Math.min(count, largeCount) : count;
            const directory = path.join(output, `${repeat}-${size}-${threads}-${method}-${sessions}`);
            fs.mkdirSync(directory);
            const children = [];
            const round = new Deadline(160000, 'One-way round');
            try {
                await (async () => {
                    const server = start(options['--server'], ['--protocol=3', '--listen=127.0.0.1', '--port=0',
                        '--identity-directory=' + path.join(directory, 'identity'), '--stop-file=' + path.join(directory, 'server.stop'),
                        '--run-for-ms=150000'], 'server', directory, /GATEWAY_READY port=(\d+) pid=(\d+)/);
                    children.push(server);
                    const ready = await round.wait(server.ready);
                    const previous = process.env.NET_GATEWAY_ONEWAY;
                    let service;
                    try {
                        process.env.NET_GATEWAY_ONEWAY = `${measuredCount},${size},${threads}`;
                        service = start(options['--server'], ['--self-test-service=' + ready[2],
                            '--stop-file=' + path.join(directory, 'service.stop'), '--run-for-ms=150000'],
                            'service', directory, /SERVICE_PROBE_READY pid=/);
                    } finally {
                        if (previous === undefined) delete process.env.NET_GATEWAY_ONEWAY;
                        else process.env.NET_GATEWAY_ONEWAY = previous;
                    }
                    children.push(service);
                    const received = service.record('ONEWAY_RECEIVED');
                    received.catch(() => {});
                    await round.wait(service.ready);
                    let client;
                    const previousSessions = process.env.NET_GATEWAY_PERF_SESSIONS;
                    const previousWorkers = process.env.NET_GATEWAY_PERF_WORKERS;
                    const previousQueue = process.env.NET_GATEWAY_PERF_QUEUE_INITIAL;
                    const previousProducerCpu = process.env.NET_GATEWAY_PERF_PRODUCER_CPU;
                    const previousOwnerCpu = process.env.NET_GATEWAY_PERF_OWNER_CPU;
                    try {
                        process.env.NET_GATEWAY_PERF_SESSIONS = String(sessions);
                        process.env.NET_GATEWAY_PERF_WORKERS = String(workers);
                        process.env.NET_GATEWAY_PERF_QUEUE_INITIAL = String(queueInitial);
                        if (pinned) {
                            process.env.NET_GATEWAY_PERF_PRODUCER_CPU = String(producerCpu);
                            process.env.NET_GATEWAY_PERF_OWNER_CPU = String(ownerCpu);
                        } else {
                            delete process.env.NET_GATEWAY_PERF_PRODUCER_CPU;
                            delete process.env.NET_GATEWAY_PERF_OWNER_CPU;
                        }
                        client = start(options['--client'], [ready[1], String(size), String(measuredCount), String(threads),
                            'data-' + method, path.join(directory, 'client.stop')], 'client', directory, /ONEWAY_SUBMITTED /);
                    } finally {
                        if (previousSessions === undefined) delete process.env.NET_GATEWAY_PERF_SESSIONS;
                        else process.env.NET_GATEWAY_PERF_SESSIONS = previousSessions;
                        if (previousWorkers === undefined) delete process.env.NET_GATEWAY_PERF_WORKERS;
                        else process.env.NET_GATEWAY_PERF_WORKERS = previousWorkers;
                        if (previousQueue === undefined) delete process.env.NET_GATEWAY_PERF_QUEUE_INITIAL;
                        else process.env.NET_GATEWAY_PERF_QUEUE_INITIAL = previousQueue;
                        if (previousProducerCpu === undefined) delete process.env.NET_GATEWAY_PERF_PRODUCER_CPU;
                        else process.env.NET_GATEWAY_PERF_PRODUCER_CPU = previousProducerCpu;
                        if (previousOwnerCpu === undefined) delete process.env.NET_GATEWAY_PERF_OWNER_CPU;
                        else process.env.NET_GATEWAY_PERF_OWNER_CPU = previousOwnerCpu;
                    }
                    children.push(client);
                    const submitted = client.record('ONEWAY_SUBMITTED');
                    const [tx, rx] = await round.wait(Promise.all([submitted, received]));
                    const elapsedUs = validate.oneWay(tx, rx, { count: measuredCount, bodyBytes: size, producers: threads });
                    fs.writeFileSync(path.join(directory, 'client.stop'), 'stop');
                    const stoppedClient = await round.wait(client.completed);
                    assert.equal(stoppedClient.code, 0, stoppedClient.stderr);
                    if (pinned) {
                        assert.ok(stoppedClient.stdout.includes(`ONEWAY_CPU NET_GATEWAY_PERF_PRODUCER_CPU=${producerCpu} `));
                        assert.ok(stoppedClient.stdout.includes(`ONEWAY_CPU NET_GATEWAY_PERF_OWNER_CPU=${ownerCpu} `));
                    }
                    const wireMatch = /^ONEWAY_WIRE (.+)\r?$/m.exec(stoppedClient.stdout);
                    assert.ok(wireMatch, 'Missing actual TCP byte totals');
                    const wire = JSON.parse(wireMatch[1]);
                    validate.wire(wire);
                    fs.writeFileSync(path.join(directory, 'service.stop'), 'stop');
                    const stoppedService = await round.wait(service.completed);
                    assert.equal(stoppedService.code, 0, stoppedService.stderr);
                    assert.ok(!/SERVICE_PROBE_ECHO/.test(stoppedService.stdout), 'Measured one-way must not send business responses');
                    assert.equal((stoppedService.stdout.match(/^ONEWAY_WARMUP_ECHO /gm) || []).length, (size >= 65536 ? 8 : 64) * sessions, 'Warmup excluded from measured data');
                    assert.equal((stoppedService.stdout.match(/^ONEWAY_RECEIVED /gm) || []).length, 1);
                    assert.equal((stoppedClient.stdout.match(/^ONEWAY_SUBMITTED /gm) || []).length, 1);
                    assert.equal((stoppedClient.stdout.match(/^ONEWAY_WIRE /gm) || []).length, 1);
                    fs.writeFileSync(path.join(directory, 'server.stop'), 'stop');
                    assert.equal((await round.wait(server.completed)).code, 0);
                    results.push({ repeat, bodyBytes: size, count: measuredCount, producers: threads, sessions, workers, queueInitial, producerCpu, ownerCpu, method, elapsedUs,
                        messagesPerSecond: measuredCount * 1000000 / elapsedUs, sender: tx, receiver: rx,
                        ...wire, clientSentBodyBytes: measuredCount * size, serverReceivedBodyBytes: measuredCount * size,
                        serverSentBodyBytes: 0, businessResponses: 0, rpcWindow: null, compression: false, warmupRequests: (size >= 65536 ? 8 : 64) * sessions,
                        timing: 'client-before-first-encryption to service-after-last-decryption-and-validation (same-host QPC)' });
                    console.log(`${repeat} body=${size} producers=${threads} sessions=${sessions} ${method}: ${results[results.length - 1].messagesPerSecond.toFixed(3)} validated one-way/s`);
                })();
            } catch (error) {
                failures.push({ repeat, bodyBytes: size, producers: threads, method, error: String(error.stack || error) });
                console.error(`FAILED body=${size} producers=${threads} sessions=${sessions} ${method}: ${error.message || error}`);
            } finally {
                round.close();
                for (const item of children) if (item.child.exitCode === null && item.child.signalCode === null) item.child.kill();
                await Promise.all(children.map(item => item.completed.catch(() => {})));
                fs.writeFileSync(path.join(output, 'results.json'), JSON.stringify({ complete: false, results, failures }, null, 2));
            }
        }
    }
    fs.writeFileSync(path.join(output, 'results.json'), JSON.stringify({ complete: true, passed: failures.length === 0, results, failures }, null, 2));
    if (failures.length) process.exitCode = 1;
}

if (require.main === module) main().catch(error => { console.error(error.stack || error); process.exitCode = 1; });