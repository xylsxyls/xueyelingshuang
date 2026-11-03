'use strict';

// Real gateway process + independent Node TLS verification. Never logs private identity bytes.
const fs = require('fs');
const path = require('path');
const tls = require('tls');
const crypto = require('crypto');
const { spawn } = require('child_process');

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

function startGateway(executable, directory, output) {
    const child = spawn(executable, ['--protocol=2', '--identity-directory=' + directory, '--listen=127.0.0.1',
        '--port=0', '--run-for-ms=1200'], { windowsHide: true, stdio: ['ignore', 'pipe', 'pipe'] });
    let stdout = '';
    let stderr = '';
    let settleReady;
    let rejectReady;
    let announced = false;
    const ready = new Promise((resolve, reject) => { settleReady = resolve; rejectReady = reject; });
    // Corruption cases intentionally do not wait for readiness.
    ready.catch(() => {});
    child.stdout.on('data', bytes => {
        stdout += bytes.toString('utf8');
        const match = /GATEWAY_READY port=(\d+) pid=(\d+)/.exec(stdout);
        if (match && !announced) { announced = true; settleReady(Number(match[1])); }
    });
    child.stderr.on('data', bytes => { stderr += bytes.toString('utf8'); });
    const completed = new Promise((resolve, reject) => {
        const deadline = setTimeout(() => { child.kill(); reject(new Error('Owned gateway process timed out')); }, 10000);
        child.once('error', error => { clearTimeout(deadline); rejectReady(error); reject(error); });
        child.once('close', (code, signal) => {
            clearTimeout(deadline);
            fs.writeFileSync(output, JSON.stringify({ code, signal, stdout, stderr }, null, 2));
            if (!announced) rejectReady(new Error('Gateway did not become ready'));
            resolve({ code, signal, stdout, stderr });
        });
    });
    return { child, ready, completed };
}

function authenticate(port, authority) {
    return new Promise((resolve, reject) => {
        const socket = tls.connect({ host: '127.0.0.1', port, servername: 'node.gateway', ca: authority,
            rejectUnauthorized: true, minVersion: 'TLSv1.2', maxVersion: 'TLSv1.2' });
        socket.setTimeout(3000, () => socket.destroy(new Error('Gateway TLS verification timed out')));
        socket.once('error', reject);
        socket.once('secureConnect', () => {
            const peer = socket.getPeerCertificate();
            requireCondition(socket.authorized && peer.subject && /^node\.[0-9a-f]{32}$/.test(peer.subject.CN),
                'Gateway did not authenticate a generated node identity');
            socket.end();
            resolve({ name: peer.subject.CN, fingerprint: peer.fingerprint256 });
        });
    });
}

async function main() {
    const options = {};
    for (const argument of process.argv.slice(2)) {
        const split = argument.indexOf('=');
        requireCondition(split > 0, 'Expected named test arguments');
        options[argument.slice(0, split)] = argument.slice(split + 1);
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
        const running = startGateway(server, directory, path.join(output, 'run-' + index + '.json'));
        let peer;
        try {
            const port = await running.ready;
            peer = await authenticate(port, authority);
        } catch (error) {
            running.child.kill();
            await running.completed;
            throw error;
        }
        const stopped = await running.completed;
        requireCondition(stopped.code === 0 && !stopped.signal && stopped.stdout.includes('GATEWAY_STOPPED'), 'Gateway did not stop normally');
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
    corrupt[16] ^= 1;
    fs.writeFileSync(identity, corrupt);
    const broken = startGateway(server, directory, path.join(output, 'corrupt.json'));
    const rejected = await broken.completed;
    requireCondition(rejected.code === 2 && !rejected.stdout.includes('GATEWAY_READY') && fs.readFileSync(identity).equals(corrupt),
        'Damaged identity was accepted or silently overwritten');
    results.push({ case: 'reject-corruption-without-overwrite', pass: true });
    fs.writeFileSync(path.join(output, 'result.json'), JSON.stringify({ total: results.length, pass: results.length, results }, null, 2));
    console.log('Gateway identity: ' + results.length + '/' + results.length + ' PASS');
}

main().catch(error => { console.error(error.stack || String(error)); process.exitCode = 1; });