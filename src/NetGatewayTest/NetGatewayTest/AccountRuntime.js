'use strict';
const { setModuleBits } = require('./WasmMemory');

// Test application loader. The optional network argument is not passed into Account.
async function loadAccountRuntime(factory, networkRuntime, options = {}) {
    const module = setModuleBits(await factory(options), factory);
    if (typeof module._accountClientCreate !== 'function') throw new Error('Missing Account business API');
    return { module };
}

async function loadWxAccountRuntime(factory, wasmPath, networkRuntime, wxApi) {
    if (typeof WXWebAssembly === 'undefined') throw new Error('WXWebAssembly unavailable');
    wxApi = wxApi || (typeof wx !== 'undefined' ? wx : null);
    if (!wxApi || typeof wxApi.getPerformance !== 'function') throw new Error('Platform monotonic clock unavailable');
    const performance = wxApi.getPerformance();
    if (!performance || typeof performance.now !== 'function') throw new Error('Platform monotonic clock unavailable');
    let timer;
    let rejectLoad;
    const failed = new Promise((resolve, reject) => {
        rejectLoad = reject;
        timer = setTimeout(() => reject(new Error('Account module load timeout')), 10000);
    });
    try {
        return await Promise.race([failed, loadAccountRuntime(factory, null, {
            wasmApi: WXWebAssembly,
            performance,
            instantiateWasm(imports, receive) {
                WXWebAssembly.instantiate(wasmPath, imports).then(result => receive(result.instance, result.module)).catch(rejectLoad);
                return {};
            },
            onAbort: reason => rejectLoad(new Error(String(reason)))
        })]);
    } finally { clearTimeout(timer); }
}

module.exports = { loadAccountRuntime, loadWxAccountRuntime };