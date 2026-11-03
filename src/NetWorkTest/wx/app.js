const config = require('./test-config');

App({
    globalData: { netInterfaceReport: { state: 'RUNNING', cases: [], capabilities: {} } },
    onLaunch() { this.runNetWorkTest(); },
    async runNetWorkTest() {
        const report = this.globalData.netInterfaceReport;
        let connection;
        let responseTimer;
        try {
            const createModule = require('./net/NetInterfaceAPI');
            const { loadWxRuntime, createWxClient } = require('./net/NetWxClient');
            report.capabilities = {
                wasm: typeof WXWebAssembly,
                tcp: typeof wx.createTCPSocket,
                random: typeof wx.getRandomValues,
                system: wx.getSystemInfoSync()
            };
            const runtime = await loadWxRuntime(createModule, '/net/' + (createModule.sdk ? createModule.sdk.wasm : 'NetInterface.wasm'), wx);
            report.cases.push({ id: 400, name: '微信真实Wasm加载及强随机源', state: 'PASS' });
            const expected = new Uint8Array(65536);
            for (let index = 0; index < expected.length; ++index) expected[index] = (index * 31 + 127) & 255;
            let complete;
            let reject;
            const response = new Promise((resolve, fail) => { complete = resolve; reject = fail; });
            // Observe a rejection immediately, even when connect itself fails first.
            response.catch(() => {});
            connection = createWxClient(runtime, {
                hostname: 'localhost', address: config.address, port: config.port,
                authorityPem: config.authorityPem,
                onFrame: frame => {
                    if (frame.product !== 5 || frame.requestLow !== 0x76543210 ||
                        frame.requestHigh !== 0xfedcba98 || frame.type !== 42 ||
                        frame.payload.length !== expected.length) return reject(new Error('响应字段不一致'));
                    for (let index = 0; index < expected.length; ++index) {
                        if (frame.payload[index] !== expected[index]) return reject(new Error('响应二进制不一致'));
                    }
                    complete();
                },
                onClose: error => { if (error) reject(error); }
            }, wx);
            await connection.connect();
            responseTimer = setTimeout(() => reject(new Error('微信真实TCP回包超时')), 10000);
            connection.sendFrame([3, 5, 0x76543210, 0xfedcba98, 42], expected);
            await response;
            report.cases.push({ id: 401, name: '微信TCPSocket与OpenSSL真实往返', state: 'PASS', bytes: expected.length });
            report.state = 'PASS';
        } catch (error) {
            report.state = 'FAIL';
            report.error = error.stack || String(error);
            report.cases.push({ id: report.cases.length ? 401 : 400, state: 'FAIL', error: report.error });
        } finally {
            clearTimeout(responseTimer);
            if (connection) connection.dispose();
        }
    }
});