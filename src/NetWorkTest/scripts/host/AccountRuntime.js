'use strict';
const { setModuleBits } = require('./WasmMemory');

// Test application loader. The optional network argument is not passed into Account.
async function loadAccountRuntime(factory, networkRuntime, options = {}) {
    const module = setModuleBits(await factory(options), factory);
    if (typeof module._accountClientCreate !== 'function') throw new Error('Missing Account business API');
    return { module };
}

async function loadWxAccountRuntime(factory, wasmPath, networkRuntime) {
    if (typeof WXWebAssembly === 'undefined') throw new Error('WXWebAssembly unavailable');
    let timer;
    let rejectLoad;
    const failed = new Promise((resolve, reject) => {
        rejectLoad = reject;
        timer = setTimeout(() => reject(new Error('Account module load timeout')), 10000);
    });
    try {
        return await Promise.race([failed, loadAccountRuntime(factory, null, {
            wasmApi: WXWebAssembly,
            instantiateWasm(imports, receive) {
                WXWebAssembly.instantiate(wasmPath, imports).then(result => receive(result.instance, result.module)).catch(rejectLoad);
                return {};
            },
            onAbort: reason => rejectLoad(new Error(String(reason)))
        })]);
    } finally { clearTimeout(timer); }
}

module.exports = { loadAccountRuntime, loadWxAccountRuntime };