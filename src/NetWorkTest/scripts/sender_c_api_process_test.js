'use strict';

// Real C++ facade -> ProcessWork -> opaque client gateway -> endpoint TLS service.
const fs = require('fs');
const path = require('path');
const assert = require('assert').strict;
const { start } = require('./local_link_gateway_process_test');

async function main() {
    const options = {};
    for (const argument of process.argv.slice(2)) {
        const split = argument.indexOf('=');
        const key = argument.slice(0, split);
        assert.ok(split > 0 && ['--server', '--client', '--test', '--output'].includes(key) && !options[key]);
        options[key] = argument.slice(split + 1);
    }
    for (const key of ['--server', '--client', '--test', '--output']) assert.ok(options[key], key);
    const output = path.resolve(options['--output']);
    assert.ok(!fs.existsSync(output), 'Use a new owned report directory');
    fs.mkdirSync(output, { recursive: true });
    const diagnostic = path.basename(options['--client']).toLowerCase() === 'networktestclient.exe';
    const children = [];
    const run = (binary, args, name, pattern) => {
        const entry = start(path.resolve(binary), args, name, output, pattern);
        children.push(entry);
        return entry;
    };
    let timer;
    try {
        const deadline = new Promise((resolve, reject) => { timer = setTimeout(() => reject(new Error('C++ facade deadline')), 30000); });
        await Promise.race([(async () => {
            const server = run(options['--server'], ['--protocol=3', '--listen=127.0.0.1', '--port=0',
                '--identity-directory=' + path.join(output, 'identity'), '--stop-file=' + path.join(output, 'server.stop'),
                '--run-for-ms=35000'], 'server', /GATEWAY_READY port=(\d+) pid=(\d+)/);
            const serverReady = await server.ready;
            const service = run(options['--server'], ['--self-test-service=' + serverReady[2],
                '--stop-file=' + path.join(output, 'service.stop'), '--run-for-ms=35000'], 'service', /SERVICE_PROBE_READY pid=(\d+)/);
            await service.ready;
            const client = run(options['--client'], ['--protocol=3', '--address=127.0.0.1', '--port=' + serverReady[1],
                '--cache-file=' + path.join(output, 'client.cache'), '--stop-file=' + path.join(output, 'client.stop'),
                '--run-for-ms=35000'], 'client', /CLIENT_GATEWAY_READY pid=(\d+)/);
            const clientReady = await client.ready;
            for (const [name, pid] of [['fixed', clientReady[1]], [diagnostic ? 'fixed-again' : 'automatic', diagnostic ? clientReady[1] : '0']]) {
                const endpoint = run(options['--test'], ['--facade-service-client=' + pid], name, /CPP_SENDER_CLIENT_PASS/);
                const result = await endpoint.completed;
                assert.equal(result.code, 0, result.stdout);
                assert.match(result.stdout, /CPP_SENDER_CLIENT_PASS/);
            }
            // Diagnostic hosts bypass the production singleton and must never discover or replace it.
            if (!diagnostic) {
            const other = run(options['--client'], ['--protocol=3', '--address=127.0.0.1', '--port=' + serverReady[1],
                '--cache-file=' + path.join(output, 'other.cache'), '--stop-file=' + path.join(output, 'other.stop'),
                '--run-for-ms=35000'], 'other', /CLIENT_GATEWAY_READY pid=(\d+)/);
            assert.equal((await other.completed).code, 0);
            assert.match(fs.readFileSync(path.join(output, 'other.json'), 'utf8'), /NetClientManager_REUSED/);
            }
            const retained = run(options['--test'], ['--facade-service-client=' + (diagnostic ? clientReady[1] : '0')], 'retained', /CPP_SENDER_CLIENT_PASS/);
            assert.equal((await retained.completed).code, 0, 'Second launch must leave the original usable');
            for (const name of ['client', 'service', 'server']) fs.writeFileSync(path.join(output, name + '.stop'), '');
            const stopped = await Promise.all([client.completed, service.completed, server.completed]);
            assert.ok(stopped.every(entry => entry.code === 0 && !entry.signal));
            assert.equal((stopped[1].stdout.match(/SERVICE_PROBE_ECHO pid=\d+ size=65537\b/g) || []).length, 3);
        })(), deadline]);
        fs.writeFileSync(path.join(output, 'result.json'), JSON.stringify({ pass: true, fixedPid: true, discoveredPid: !diagnostic, diagnosticOnly: diagnostic,
            payloadBytes: 65537, wrongTargetRejected: true, duplicateLaunchReused: !diagnostic,
            reentrantDestroyRejected: true, recreateAfterDestroy: true }, null, 2));
        console.log('Real NetSender C API lifecycle and opaque gateway round trip: PASS');
    } catch (error) {
        fs.writeFileSync(path.join(output, 'failure.json'), JSON.stringify({ error: String(error.stack || error) }, null, 2));
        throw error;
    } finally {
        clearTimeout(timer);
        for (const entry of children) if (entry.child.exitCode === null && entry.child.signalCode === null) entry.child.kill();
        await Promise.all(children.map(entry => entry.completed.catch(() => {})));
    }
}

main().catch(error => { console.error(error.stack || String(error)); process.exitCode = 1; });