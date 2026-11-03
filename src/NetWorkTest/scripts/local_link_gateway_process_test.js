'use strict';

// Actual desktop path: endpoint -> ProcessWork -> client gateway -> TCP -> server gateway.
const fs = require('fs');
const path = require('path');
const { spawn } = require('child_process');

function check(value, message) { if (!value) throw new Error(message); }

function start(executable, args, name, output, readiness) {
    let stdout = '';
    let stderr = '';
    let readyResolve;
    let readyReject;
    const ready = new Promise((resolve, reject) => { readyResolve = resolve; readyReject = reject; });
    ready.catch(() => {});
    const child = spawn(executable, args, { windowsHide: true, stdio: ['ignore', 'pipe', 'pipe'] });
    child.stdout.on('data', bytes => {
        stdout += bytes.toString('utf8');
        const match = readiness.exec(stdout);
        if (match) readyResolve(match);
    });
    child.stderr.on('data', bytes => { stderr += bytes.toString('utf8'); });
    const completed = new Promise((resolve, reject) => {
        child.once('error', error => { readyReject(error); reject(error); });
        child.once('close', (code, signal) => {
            readyReject(new Error(name + ' exited before readiness'));
            fs.writeFileSync(path.join(output, name + '.json'), JSON.stringify({ code, signal, stdout, stderr }, null, 2));
            resolve({ code, signal, stdout, stderr });
        });
    });
    return { child, ready, completed };
}

async function main() {
    const options = {};
    for (const argument of process.argv.slice(2)) {
        const split = argument.indexOf('=');
        check(split > 0, 'Expected --name=value');
        options[argument.slice(0, split)] = argument.slice(split + 1);
    }
    for (const key of ['--server', '--client', '--test', '--output']) check(options[key], 'Missing ' + key);
    const output = path.resolve(options['--output']);
    check(!fs.existsSync(output), 'Output must be a fresh owned directory');
    fs.mkdirSync(output, { recursive: true });
    const children = [];
    let timer;
    try {
        // 旧明文入口已移除；只启动本测试自己的宿主，旧实现超时也只清理其子进程。
        let retiredModesRejected = 0;
        for (const [executable, variants, role] of [
            [options['--client'], [['--legacy=127.0.0.1'], ['--legacy'], ['--replace', '--legacy=127.0.0.1']], 'client'],
            [options['--server'], [['--legacy'], ['--legacy=127.0.0.1'], ['--protocol=3', '--legacy']], 'server']
        ]) {
            for (let index = 0; index < variants.length; ++index) {
                const entry = start(path.resolve(executable), variants[index], 'retired-' + role + '-' + index,
                    output, /RETIRED_MODE_MUST_NOT_START/);
                children.push(entry);
                let deadline;
                try {
                    const stopped = await Promise.race([entry.completed, new Promise((resolve, reject) => {
                        deadline = setTimeout(() => reject(new Error('Retired ' + role + ' mode was not rejected')), 3000);
                    })]);
                    check(stopped.code === 2 && !stopped.signal && !/GATEWAY_READY/.test(stopped.stdout) &&
                        /Legacy gateway mode has been removed/.test(stopped.stderr), 'Retired ' + role + ' argument was accepted');
                    ++retiredModesRejected;
                } finally {
                    clearTimeout(deadline);
                }
            }
        }
        const serverStop = path.join(output, 'server.stop');
        const clientStop = path.join(output, 'client.stop');
        const server = start(path.resolve(options['--server']), ['--protocol=3', '--listen=127.0.0.1', '--port=0',
            '--identity-directory=' + path.join(output, 'identity'), '--stop-file=' + serverStop, '--run-for-ms=20000'],
            'server', output, /GATEWAY_READY port=(\d+) pid=(\d+)/);
        children.push(server);
        const bounded = new Promise((resolve, reject) => { timer = setTimeout(() => reject(new Error('Process test deadline')), 18000); });
        await Promise.race([(async () => {
            const serverReady = await server.ready;
            const client = start(path.resolve(options['--client']), ['--protocol=3', '--address=127.0.0.1', '--port=' + serverReady[1],
                '--cache-file=' + path.join(output, 'client-nodes.cache'),
                '--stop-file=' + clientStop, '--run-for-ms=18000'], 'client', output, /CLIENT_GATEWAY_READY pid=(\d+)/);
            children.push(client);
            const clientReady = await client.ready;
            const test = start(path.resolve(options['--test']), ['--managed-local-test=' + clientReady[1]], 'endpoint', output,
                /MANAGED_LOCAL_GATEWAY_PASS/);
            children.push(test);
            const tested = await test.completed;
            check(tested.code === 0 && /MANAGED_LOCAL_GATEWAY_PASS/.test(tested.stdout), 'Desktop endpoint test failed');
            const grantAudit = start(path.resolve(options['--test']), ['--service-grant-test=' + serverReady[2]],
                'service-grant-audit', output, /Service grant actual IPC positions=8 rejected=\d+ PASS/);
            children.push(grantAudit);
            const granted = await grantAudit.completed;
            check(granted.code === 0 && /positions=8 rejected=\d+ PASS/.test(granted.stdout), 'Service grant copy audit failed');
            fs.writeFileSync(clientStop, '');
            fs.writeFileSync(serverStop, '');
            const stopped = await Promise.all([client.completed, server.completed]);
            check(stopped.every(value => value.code === 0 && !value.signal), 'Owned gateways did not stop normally');
            // 仅借用本次测试节点的隔离叶子身份；正式程序和发布包不携带测试密钥。
            for (const [mode, name, marker] of [
                ['--link-auth-audit', 'link-auth-audit', /Link auth checks=\d+ PASS/],
                ['--link-output-audit', 'link-output-audit', /Link output queue checks=\d+ PASS/],
                ['--link-input-audit', 'link-input-audit', /Link input drain checks=\d+ PASS/],
                ['--sender-link-audit', 'sender-link-audit', /Sender link drain checks=\d+ PASS/]
            ]) {
                const audit = start(path.resolve(options['--test']), [mode, path.join(output, 'identity', 'identity.dat')],
                    name, output, marker);
                children.push(audit);
                const result = await audit.completed;
                check(result.code === 0 && !result.signal && marker.test(result.stdout), name + ' failed');
            }
        })(), bounded]);
        fs.writeFileSync(path.join(output, 'result.json'), JSON.stringify({ pass: true,
            path: ['endpoint', 'ProcessWork', 'NetClientManager', 'TCP', 'NetServerManager'],
            requestId: '8123456789abcdef', unregisteredServiceRejected: true, retiredModesRejected, serviceGrantCopyAudited: true, linkInputDrainAudited: true, senderLinkDrainAudited: true, linkOutputQueueAudited: true, linkAuthenticationAudited: true }, null, 2));
        console.log('Actual ProcessWork -> transparent client gateway -> actual server gateway: PASS');
    } finally {
        clearTimeout(timer);
        for (const entry of children) if (entry.child.exitCode === null && entry.child.signalCode === null) entry.child.kill();
        await Promise.all(children.map(entry => entry.completed.catch(() => {})));
    }
}

if (require.main === module) main().catch(error => { console.error(error.stack || String(error)); process.exitCode = 1; });
module.exports = { start };