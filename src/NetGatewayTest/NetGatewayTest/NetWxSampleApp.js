'use strict';

const config = require('./test-config');

// 只接入正式NetPeer SDK；微信官方工具和Node宿主模拟使用完全相同的应用代码。
App({
    globalData: { netInterfaceReport: { state: 'RUNNING', cases: [], capabilities: {} } },
    onLaunch() { this.runNetGatewayTest(); },
    async runNetGatewayTest() {
        const report = this.globalData.netInterfaceReport;
        let sender, runtime, clock;
        const delay = () => new Promise(resolve => setTimeout(resolve, 0));
        try {
            const factory = require('./net/NetPeerAPI');
            if (typeof WXWebAssembly === 'undefined' || typeof wx.getPerformance !== 'function') {
                throw new Error('WXWebAssembly and the platform monotonic clock are required');
            }
            clock = wx.getPerformance();
            report.capabilities = {
                wasm: typeof WXWebAssembly, tcp: typeof wx.createTCPSocket,
                random: typeof wx.getRandomValues, clock: typeof clock.now, system: wx.getSystemInfoSync()
            };
            let loadTimer, rejectLoad;
            const failed = new Promise((resolve, reject) => {
                rejectLoad = reject;
                loadTimer = setTimeout(() => reject(new Error('Module load timeout')), 10000);
            });
            let module;
            try {
                module = await Promise.race([failed, factory({
                    wasmApi: WXWebAssembly, performance: clock,
                    instantiateWasm(imports, receive) {
                        WXWebAssembly.instantiate('/net/' + factory.sdk.wasm, imports)
                            .then(result => receive(result.instance, result.module)).catch(rejectLoad);
                        return {};
                    },
                    onAbort: reason => rejectLoad(new Error(String(reason)))
                })]);
            } finally { clearTimeout(loadTimer); }
            const host = module.NetPeerRuntime.wxOptions(wx);
            runtime = await module.NetPeerRuntime.create(host);
            report.cases.push({ id: 400, name: '平台Wasm加载、时钟及强随机源', state: 'PASS' });
            const payload = Uint8Array.from({ length: 65536 }, (_, index) => (index * 31 + 127) & 255);
            let ready = false, received = false, request = 0n, failure = null, closed = 0;
            sender = runtime.createSender({
                onSessionReady() { if (ready) failure = new Error('Duplicate ready'); ready = true; },
                onSessionResult(generation, id, product, type, status, bytes) {
                    if (generation !== sender.gatewayGeneration() || id !== request || product !== 5 || type !== 42 ||
                        status !== 0 || received || bytes.length !== payload.length || bytes.some((value, index) => value !== payload[index])) {
                        failure = new Error('Response identity, body or count mismatch');
                    } else { received = true; }
                },
                onSessionFrame() { failure = new Error('Unexpected non-RPC frame'); return true; },
                onSessionClosed(generation, reason) {
                    ++closed;
                    if (!received || reason !== 0) failure = new Error('Unexpected close: ' + reason);
                }
            });
            if (sender.prepareGateway() !== 0) throw new Error('Prepare failed');
            const authority = Uint8Array.from(config.authorityPem + '\0', value => value.charCodeAt(0));
            const connected = sender.connectGateway({
                address: config.address, port: config.port, platform: 3, device: await host.randomSource(16),
                tls: { hostname: 'localhost', authority }, timeoutMs: 10000
            });
            if (connected !== 0) throw new Error('Connect rejected: ' + connected);
            const deadline = clock.now() + 15000;
            while (!received && !failure && clock.now() < deadline) {
                sender.pollGateway();
                if (ready && request === 0n) {
                    const pending = sender.beginRequest(5, 42, 10000);
                    if (pending.result !== 0 || pending.requestId === 0n) throw new Error('Begin request failed');
                    request = pending.requestId;
                    if (sender.postRequest(request, payload) !== 0) throw new Error('Submit failed');
                }
                await delay();
            }
            if (failure || !received) throw failure || new Error('Actual response timed out');
            sender.closeGateway(0);
            while (sender.pollGateway() && clock.now() < deadline) await delay();
            if (failure || closed !== 1 || sender.releaseGateway() !== 0 || runtime.clients !== 0 || runtime.hosts.size !== 0) {
                throw failure || new Error('Lifecycle did not quiesce');
            }
            sender = null;
            report.cases.push({ id: 401, name: 'TCPSocket/TLS/RPC二进制完整往返及释放', state: 'PASS', bytes: payload.length });
            report.state = 'PASS';
        } catch (error) {
            report.state = 'FAIL'; report.error = error.stack || String(error);
            report.cases.push({ id: report.cases.length ? 401 : 400, state: 'FAIL', error: report.error });
        } finally {
            if (sender) {
                sender.closeGateway(0);
                const deadline = clock.now() + 2000;
                while (sender.pollGateway() && clock.now() < deadline) await delay();
                if (sender.releaseGateway() !== 0 || runtime.clients !== 0 || runtime.hosts.size !== 0) {
                    report.state = 'FAIL'; report.cleanupError = 'Owned runtime did not quiesce';
                }
            }
        }
    }
});