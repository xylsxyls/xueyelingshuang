'use strict';

// Same executable source, linked separately against each SDK. Count only validated RPC replies.
const fs = require('fs');
const path = require('path');
const assert = require('assert').strict;
const { start } = require('./local_link_gateway_process_test');
const validate = require('./NetPerformanceAssertions');
const Deadline = require('./NetTestDeadline');

async function main() {
    const options = {};
    for (const argument of process.argv.slice(2)) {
        const split = argument.indexOf('=');
        assert.ok(split > 0);
        const key = argument.slice(0, split);
        assert.ok(['--server', '--client', '--output', '--sizes', '--count', '--repeats', '--producers', '--methods'].includes(key) && !Object.prototype.hasOwnProperty.call(options, key));
        assert.ok(argument.length > split + 1, 'Empty option: ' + key);
        options[key] = argument.slice(split + 1);
    }
    for (const key of ['--server', '--client', '--output']) assert.ok(path.isAbsolute(options[key] || ''), key);
    const list = (key, fallback, allowed) => {
        const values = (options[key] || fallback).split(',');
        assert.ok(values.length && values.every(value => allowed.includes(value)) && new Set(values).size === values.length, key);
        return values;
    };
    const sizes = list('--sizes', '6,1024,65536,1048560', ['0', '6', '1024', '65536', '1048560']).map(Number);
    const producers = list('--producers', '1,4', ['1', '4']).map(Number);
    const methods = list('--methods', 'send,post', ['send', 'post']);
    const count = Number(options['--count'] || '256');
    const repeats = Number(options['--repeats'] || '3');
    assert.ok(Number.isInteger(count) && count >= 8 && count <= 100000);
    assert.ok(Number.isInteger(repeats) && repeats >= 1 && repeats <= 9);
    const output = path.resolve(options['--output']);
    assert.ok(!fs.existsSync(output), 'Use a fresh isolated output');
    fs.mkdirSync(output, { recursive: true });
    const results = [];
    const failures = [];
    for (let repeat = 0; repeat < repeats; ++repeat) {
        for (const size of sizes) for (const threads of producers) for (const method of methods) {
            const requests = size >= 1048560 ? Math.min(count, 32) : (size >= 65536 ? Math.min(count, 256) : count);
            const warmup = size >= 65536 ? 8 : 64;
            const directory = path.join(output, `${repeat}-${size}-${threads}-${method}`);
            fs.mkdirSync(directory);
            const children = [];
            const round = new Deadline(160000, 'Performance round');
            try {
                await (async () => {
                    const server = start(options['--server'], ['--protocol=3', '--listen=127.0.0.1', '--port=0',
                        '--identity-directory=' + path.join(directory, 'identity'), '--stop-file=' + path.join(directory, 'server.stop'),
                        '--run-for-ms=150000'], 'server', directory, /GATEWAY_READY port=(\d+) pid=(\d+)/);
                    children.push(server);
                    const ready = await round.wait(server.ready);
                    const service = start(options['--server'], ['--self-test-service=' + ready[2],
                        '--stop-file=' + path.join(directory, 'service.stop'), '--run-for-ms=150000'],
                        'service', directory, /SERVICE_PROBE_READY pid=/, { NET_GATEWAY_RPC_SUMMARY: '1' });
                    children.push(service);
                    await round.wait(service.ready);
                    const client = start(options['--client'], [ready[1], String(size), String(requests), String(threads), method],
                        'client', directory, /PERFORMANCE_RESULT /);
                    children.push(client);
                    const completed = await round.wait(client.completed);
                    assert.equal(completed.code, 0, completed.stderr || completed.stdout);
                    const match = /^PERFORMANCE_RESULT (.+)$/m.exec(completed.stdout);
                    assert.ok(match, 'No complete performance record');
                    const result = JSON.parse(match[1]);
                    validate.rpc(result, { count: requests, bodyBytes: size, producers: threads, method });
                    assert.equal((completed.stdout.match(/^PERFORMANCE_RESULT /gm) || []).length, 1);
                    // Stop service first so its terminal callback remains part of the validation.
                    fs.writeFileSync(path.join(directory, 'service.stop'), 'stop');
                    const stoppedService = await round.wait(service.completed);
                    assert.equal(stoppedService.code, 0, stoppedService.stderr);
                    validate.rpcService(stoppedService.stdout, requests + warmup, size);
                    fs.writeFileSync(path.join(directory, 'server.stop'), 'stop');
                    assert.equal((await round.wait(server.completed)).code, 0);
                    Object.assign(result, { repeat, serverReceivedBodyBytes: requests * size, serverSentBodyBytes: requests * size,
                        serverRequests: requests, serverResponses: requests, warmupRequestsExcluded: warmup,
                        totalBodyBytesBothDirections: requests * size * 2, window: result.window,
                        compression: { request: false, responseWhitelist: true, responseMessageType: 0x7345,
                            note: 'Response compression is conditional on size and savings; small replies remain uncompressed.' } });
                    results.push(result);
                    fs.writeFileSync(path.join(output, 'results.json'), JSON.stringify({ complete: false, results, failures }, null, 2));
                    console.log(`${repeat} body=${size} producers=${threads} ${method}: ${result.requestsPerSecond} validated RPC/s`);
                })();
            } catch (error) {
                failures.push({ repeat, bodyBytes: size, producers: threads, method, requests,
                    error: String(error.stack || error) });
                console.error(`FAILED body=${size} producers=${threads} ${method}: ${error.message || error}`);
            } finally {
                round.close();
                for (const item of children) if (item.child.exitCode === null && item.child.signalCode === null) item.child.kill();
                await Promise.all(children.map(item => item.completed.catch(() => {})));
                fs.writeFileSync(path.join(output, 'results.json'), JSON.stringify({ complete: false, results, failures }, null, 2));
            }
        }
    }
    fs.writeFileSync(path.join(output, 'results.json'), JSON.stringify({ complete: true, passed: failures.length === 0,
        results, failures }, null, 2));
    if (failures.length) process.exitCode = 1;
}

if (require.main === module) main().catch(error => { console.error(error.stack || error); process.exitCode = 1; });