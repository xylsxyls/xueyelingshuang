'use strict';

const fs = require('node:fs');
const path = require('node:path');
const net = require('node:net');
const tls = require('node:tls');
const { spawn } = require('node:child_process');
const { attachApplicationEcho } = require('./application_echo');

async function main() {
    const args = {};
    const keys = new Set(['--module', '--output', '--cli', '--helper', '--appid', '--automation-port', '--bootstrap']);
    for (const argument of process.argv.slice(2)) {
        const split = argument.indexOf('=');
        const key = argument.slice(0, split);
        if (split < 0 || !keys.has(key) || args[key]) throw new Error('Unknown or repeated argument: ' + argument);
        args[key] = argument.slice(split + 1);
    }
    for (const key of ['--module', '--output', '--cli', '--helper']) {
        if (!path.isAbsolute(args[key] || '')) throw new Error(key + ' must be an absolute path');
    }
    const port = Number(args['--automation-port'] || 49431);
    if (!Number.isInteger(port) || port < 1024 || port > 65535) throw new Error('Invalid automation port');
    const portProbe = net.createServer();
    await new Promise((resolve, reject) => { portProbe.once('error', reject); portProbe.listen(port, '127.0.0.1', resolve); });
    await new Promise(resolve => portProbe.close(resolve));
    const output = args['--output'];
    if (args['--bootstrap'] && args['--bootstrap'] !== 'true') throw new Error('Invalid bootstrap flag');
    const project = path.join(output, 'project');
    if (fs.existsSync(output)) throw new Error('Use a new owned output directory for this run');
    fs.mkdirSync(project, { recursive: true });
    fs.writeFileSync(path.join(output, 'owner.json'), JSON.stringify({ owner: 'NetWorkTest', pid: process.pid }));
    const root = path.resolve(__dirname, '../../..');
    fs.cpSync(path.resolve(__dirname, '../wx'), project, { recursive: true });
    if (args['--bootstrap']) {
        fs.writeFileSync(path.join(project, 'app.js'),
            "App({globalData:{netInterfaceReport:{state:'BOOTSTRAP_READY',scope:'automation-only'}}});");
    }
    fs.mkdirSync(path.join(project, 'net'));
    const factory = require(args['--module']);
    const wasmName = factory.sdk ? factory.sdk.wasm : 'NetInterface.wasm';
    fs.copyFileSync(args['--module'], path.join(project, 'net/NetInterfaceAPI.js'));
    fs.writeFileSync(path.join(project, 'net', wasmName), require('./host/read_wasm').readWasm(args['--module']));
    const adapters = path.join(root, 'src/NetWorkTest/scripts/host');
    for (const name of fs.readdirSync(adapters).filter(name => name.startsWith('Net') && name !== 'NetAccountClient.js' && name.endsWith('.js'))) {
        let source = fs.readFileSync(path.join(adapters, name), 'utf8');
        source = source.replace('../../../../include/NetInterface/NetInterfaceGatewayProtocol', './NetInterfaceGatewayProtocol');
        source = source.replace('../../../../include/Account/AccountProtocol', './AccountProtocol');
        fs.writeFileSync(path.join(project, 'net', name), source);
    }
    for (const [library, file] of [['Account', 'AccountProtocol.js'], ['NetInterface', 'NetInterfaceGatewayProtocol.js']])
        fs.copyFileSync(path.join(root, 'include', library, file), path.join(project, 'net', file));
    const certRoot = path.join(root, 'src/NetWorkTest/testdata/tls');
    const peers = new Set();
    const evidence = { startedUtc: new Date().toISOString(), state: 'RUNNING', tlsConnections: 0, receivedBytes: 0 };
    const server = tls.createServer({
        cert: fs.readFileSync(path.join(certRoot, 'server2-sha256.crt')),
        key: fs.readFileSync(path.join(certRoot, 'server2.key')),
        minVersion: 'TLSv1.2', maxVersion: 'TLSv1.2'
    }, socket => {
        peers.add(socket);
        ++evidence.tlsConnections;
        socket.on('close', () => peers.delete(socket));
        socket.on('error', () => {});
        attachApplicationEcho(socket, {
            onHello: hello => { evidence.applicationHello = hello; },
            onApplicationBytes: size => { evidence.receivedBytes += size; }
        });
    });
    server.on('tlsClientError', error => { evidence.peerTlsError = error.message; });
    await new Promise((resolve, reject) => { server.once('error', reject); server.listen(0, '127.0.0.1', resolve); });
    fs.writeFileSync(path.join(project, 'test-config.js'), 'module.exports = ' + JSON.stringify({
        address: '127.0.0.1', port: server.address().port,
        authorityPem: fs.readFileSync(path.join(certRoot, 'test-ca-sha256.crt'), 'ascii')
    }));
    fs.writeFileSync(path.join(project, 'project.config.json'), JSON.stringify({
        appid: args['--appid'] || 'touristappid', compileType: 'miniprogram', miniprogramRoot: './',
        projectname: 'NetWorkTest', libVersion: '3.17.0',
        setting: { es6: true, enhance: true, urlCheck: false, minified: false }
    }, null, 2));
    const { connectBounded } = require(args['--helper']);
    const cancelled = new AbortController();
    const cancel = () => cancelled.abort();
    process.once('SIGINT', cancel);
    let mini;
    async function cli(argumentsList, name, timeoutMs) {
        // Match the official cli.bat: the launcher resolves its package relative
        // to the bundled node.exe, not the test runner's arbitrary Node version.
        const executable = process.platform === 'win32' ? path.join(path.dirname(args['--cli']), 'node.exe') : process.execPath;
        if (!fs.existsSync(executable)) throw new Error('Missing official CLI runtime: ' + executable);
        const log = fs.openSync(path.join(output, name + '.log'), 'w');
        const child = spawn(executable, [args['--cli']].concat(argumentsList),
            { windowsHide: true, stdio: ['ignore', log, log] });
        fs.closeSync(log);
        await new Promise((resolve, reject) => {
            const timer = setTimeout(() => { child.kill(); reject(new Error(name + ' timeout')); }, timeoutMs);
            child.once('error', error => { clearTimeout(timer); reject(error); });
            child.once('exit', code => { clearTimeout(timer); code === 0 ? resolve() : reject(new Error(name + ' exit ' + code)); });
        });
    }
    try {
        await cli(['auto', '--project', project, '--auto-port', String(port), '--trust-project'], 'auto', 45000);
        const connectDeadline = Date.now() + 30000;
        let connectError;
        while (!mini && Date.now() < connectDeadline && !cancelled.signal.aborted) {
            try {
                mini = await connectBounded('ws://127.0.0.1:' + port,
                    Math.min(4000, connectDeadline - Date.now()), cancelled.signal, 30000);
            } catch (error) {
                connectError = error;
                await new Promise(resolve => setTimeout(resolve, 250));
            }
        }
        if (!mini) throw connectError || new Error('Automation connection cancelled');
        mini.on('exception', error => {
            evidence.exceptions = (evidence.exceptions || []).concat([error]);
        });
        evidence.tool = await mini.send('Tool.getInfo');
        const deadline = Date.now() + 30000;
        while (Date.now() < deadline && !cancelled.signal.aborted) {
            evidence.app = await mini.evaluate(() => getApp().globalData.netInterfaceReport);
            if (evidence.app && evidence.app.state !== 'RUNNING') break;
            await new Promise(resolve => setTimeout(resolve, 250));
        }
        if (!evidence.app || evidence.app.state === 'RUNNING') throw new Error('Actual mini-program did not finish before timeout');
        evidence.state = evidence.app.state;
        if (evidence.state === 'PASS' && (evidence.tlsConnections !== 1 || evidence.receivedBytes !== 65568)) {
            throw new Error('Independent TCP server evidence does not match mini-program result');
        }
        if (evidence.exceptions && evidence.exceptions.length) throw new Error('Uncaught mini-program exceptions');
    } catch (error) {
        evidence.state = 'FAIL';
        evidence.error = error.stack;
    } finally {
        if (mini) mini.disconnect();
        try { await cli(['close', '--project', project], 'close', 15000); }
        catch (error) { evidence.closeError = error.message; evidence.state = 'FAIL'; }
        peers.forEach(socket => socket.destroy());
        await new Promise(resolve => server.close(resolve));
        process.removeListener('SIGINT', cancel);
        fs.writeFileSync(path.join(output, 'report.json'), JSON.stringify(evidence, null, 2));
    }
    console.log(JSON.stringify(evidence, null, 2));
    process.exitCode = evidence.state === 'PASS' ||
        (args['--bootstrap'] && evidence.state === 'BOOTSTRAP_READY') ? 0 : 1;
}

main().catch(error => { console.error(error.stack); process.exitCode = 2; });