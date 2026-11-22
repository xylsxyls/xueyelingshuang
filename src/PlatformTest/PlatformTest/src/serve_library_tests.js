// 在本机浏览器打开 Wasm 测试页面，为 HttpRequestTest 提供同源的字节样本。
'use strict';
const fs = require('node:fs');
const http = require('node:http');
const path = require('node:path');
const {pipeline} = require('node:stream');

function serveLibraryTests(directory, port) {
    const root = fs.realpathSync(directory);
    const types = {'.html': 'text/html; charset=utf-8', '.js': 'text/javascript; charset=utf-8',
        '.wasm': 'application/wasm', '.json': 'application/json'};
    const server = http.createServer(async (request, response) => {
        try {
            const name = decodeURIComponent(new URL(request.url, 'http://localhost').pathname);
            if (request.method === 'POST' && name === '/echo') {
                response.writeHead(200, {'Content-Type': 'application/octet-stream'});
                pipeline(request, response, () => {});
                return;
            }
            if (request.method === 'GET' && name === '/video/sample') {
                response.writeHead(200, {'Content-Type': 'application/octet-stream'}).end(Buffer.from([65, 0, 66, 67, 68]));
                return;
            }
            if (request.method === 'GET' && name === '/missing') {
                response.writeHead(404, {'Content-Type': 'text/plain'}).end('missing');
                return;
            }
            if (request.method !== 'GET' && request.method !== 'HEAD') {
                response.writeHead(405, {Allow: 'GET, HEAD'}).end();
                return;
            }
            const location = await fs.promises.realpath(path.resolve(root, '.' + name));
            const relative = path.relative(root, location);
            const type = types[path.extname(location)];
            if (!relative || relative === '..' || relative.startsWith('..' + path.sep) ||
                path.isAbsolute(relative) || !type || !(await fs.promises.stat(location)).isFile()) {
                response.writeHead(404).end();
                return;
            }
            response.writeHead(200, {'Content-Type': type, 'Cache-Control': 'no-cache'});
            if (request.method === 'HEAD') {
                response.end();
                return;
            }
            pipeline(fs.createReadStream(location), response, () => {});
        } catch {
            response.writeHead(404).end();
        }
    });
    server.on('error', error => {
        console.error(error.message);
        process.exitCode = 1;
    });
    return server.listen(port, '127.0.0.1', () => {
        console.log('http://127.0.0.1:' + server.address().port + '/<project>/LibraryTestHost.html');
    });
}

module.exports = serveLibraryTests;

if (require.main === module) {
    const port = Number(process.argv[3] || '58124');
    if (!process.argv[2] || !Number.isInteger(port) || port < 1 || port > 65535) {
        console.error('Usage: node serve_library_tests.js <package directory> [port]');
        process.exitCode = 2;
    } else {
        serveLibraryTests(process.argv[2], port);
    }
}
