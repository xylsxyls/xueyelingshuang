'use strict';

// Real gateway process + independent Node TLS verification. Never logs private identity bytes.
const fs = require('fs');
const path = require('path');
const tls = require('tls');
const crypto = require('crypto');
const { start } = require('./local_link_gateway_process_test');
const NetTestDeadline = require('./NetTestDeadline');

function requireCondition(condition, message) {
    if (!condition) throw new Error(message);
}

function compiledAuthority(source) {
    const text = fs.readFileSync(source, 'utf8');
    const begin = text.indexOf('static const char g_networkAuthority[] =');
    const end = text.indexOf(';', begin);
    requireCondition(begin >= 0 && end > begin, 'Compiled public authority is missing');
    return text.slice(begin, end).split(/\r?\n/).filter(line => line.startsWith('"'))
        .map(line => JSON.parse(line)).join('');
}

function startGateway(executable, directory, output, name) {
    const stop = path.join(output, name + '.stop');
    const entry = start(executable, ['--protocol=2', '--identity-directory=' + directory, '--listen=127.0.0.1',
        '--port=0', '--stop-file=' + stop, '--run-for-ms=10000'], name, output,
        /^GATEWAY_READY port=(\d+) pid=(\d+)\r?$/m);
    return Object.assign(entry, { stop });
}

function authenticate(port, authority) {
    return new Promise((resolve, reject) => {
        const socket = tls.connect({ host: '127.0.0.1', port, servername: 'node.gateway', ca: authority,
            rejectUnauthorized: true, minVersion: 'TLSv1.2', maxVersion: 'TLSv1.2' });
        socket.setTimeout(3000, () => socket.destroy(new Error('Gateway TLS verification timed out')));
        socket.once('error', reject);
        socket.once('secureConnect', () => {
            try {
                const peer = socket.getPeerCertificate();
                requireCondition(socket.authorized && peer.subject && /^node\.[0-9a-f]{32}$/.test(peer.subject.CN) &&
                    /^[0-9A-F]{2}(?::[0-9A-F]{2}){31}$/.test(peer.fingerprint256),
                    'Gateway did not authenticate a generated node identity');
                resolve({ name: peer.subject.CN, fingerprint: peer.fingerprint256 });
            } catch (error) { reject(error); }
            finally { socket.destroy(); }
        });
    });
}

async function main() {
    const options = {};
    for (const argument of process.argv.slice(2)) {
        const split = argument.indexOf('=');
        const key = argument.slice(0, split);
        requireCondition(split > 0 && split < argument.length - 1 &&
            ['--server', '--output'].includes(key) && !Object.hasOwn(options, key), 'Expected unique named test arguments');
        options[key] = argument.slice(split + 1);
    }
    requireCondition(options['--server'] && options['--output'], 'Required: --server=<exe> --output=<fresh owned directory>');
    const server = path.resolve(options['--server']);
    const output = path.resolve(options['--output']);
    requireCondition(!fs.existsSync(output), 'Use a fresh test output directory');
    fs.mkdirSync(output, { recursive: true });
    const directory = path.join(output, '中文身份');
    const identity = path.join(directory, 'identity.dat');
    const root = path.resolve(__dirname, '../../..');
    const authority = compiledAuthority(path.join(root, 'src/NetInterface/NetInterface/src/NetInterface.cpp'));
    requireCondition(authority.includes('BEGIN CERTIFICATE') && !authority.includes('PRIVATE KEY'), 'Invalid public authority');
    const results = [];
    let originalBytes;
    let originalPeer;
    for (let index = 0; index < 3; ++index) {
        if (index === 2) fs.unlinkSync(identity);
        const round = new NetTestDeadline(12000, 'Gateway identity ' + index);
        const running = startGateway(server, directory, output, 'run-' + index);
        let peer;
        try {
            const ready = await round.wait(running.ready);
            const port = Number(ready[1]);
            requireCondition(Number(ready[2]) === running.child.pid && Number.isInteger(port) && port > 0 && port <= 65535,
                'Readiness does not identify this owned gateway');
            peer = await round.wait(authenticate(port, authority));
            fs.writeFileSync(running.stop, '', { flag: 'wx' });
            const stopped = await round.wait(running.completed);
            requireCondition(stopped.code === 0 && !stopped.signal && /^GATEWAY_STOPPED\b/m.test(stopped.stdout),
                'Gateway did not stop normally');
        } finally {
            round.close();
            if (running.child.exitCode === null && running.child.signalCode === null) running.child.kill();
            await running.completed.catch(() => {});
        }
        const bytes = fs.readFileSync(identity);
        requireCondition(fs.readdirSync(directory).join(',') === 'identity.dat', 'Identity creation retained temporary files');
        if (index === 0) { originalBytes = bytes; originalPeer = peer; }
        if (index === 1) requireCondition(bytes.equals(originalBytes) && peer.name === originalPeer.name &&
            peer.fingerprint === originalPeer.fingerprint, 'Restart replaced the persistent identity');
        if (index === 2) requireCondition(!bytes.equals(originalBytes) && peer.name !== originalPeer.name &&
            peer.fingerprint !== originalPeer.fingerprint, 'Deleting the identity did not create a new independently trusted identity');
        results.push({ case: ['bootstrap', 'restart', 'delete-and-rebuild'][index], pass: true,
            identityHash: crypto.createHash('sha256').update(bytes).digest('hex'), peer });
    }
    const corrupt = fs.readFileSync(identity);
    requireCondition(corrupt.length > 16, 'Identity corruption offset is outside the stored identity');
    corrupt[16] ^= 1;
    fs.writeFileSync(identity, corrupt);
    const deadline = new NetTestDeadline(12000, 'Corrupt identity');
    const broken = startGateway(server, directory, output, 'corrupt');
    try {
        const rejected = await deadline.wait(broken.completed);
        requireCondition(rejected.code === 2 && !rejected.signal && !rejected.stdout.includes('GATEWAY_READY') &&
            fs.readFileSync(identity).equals(corrupt), 'Damaged identity was accepted or silently overwritten');
    } finally {
        deadline.close();
        if (broken.child.exitCode === null && broken.child.signalCode === null) broken.child.kill();
        await broken.completed.catch(() => {});
    }
    results.push({ case: 'reject-corruption-without-overwrite', pass: true });
    fs.writeFileSync(path.join(output, 'result.json'), JSON.stringify({ total: results.length, pass: results.length, results }, null, 2));
    console.log('Gateway identity: ' + results.length + '/' + results.length + ' PASS');
}

main().catch(error => { console.error(error.stack || String(error)); process.exitCode = 1; });