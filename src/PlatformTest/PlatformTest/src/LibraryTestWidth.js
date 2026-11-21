/** 为成对的 wasm32/wasm64 测试选择实际位宽。显式指定 32 或 64 时直接运行指定产物。
 * auto 模式探测与 Emscripten wasm64 输出一致的 64 位表和内存。
 * 微信在小程序包内加载探针，放在分包时可传入对应的绝对包内路径。
 */
async function selectLibraryTestWidth(mode = 'auto', probePath) {
    if (mode === 32 || mode === '32') return 32;
    if (mode === 64 || mode === '64') return 64;
    if (mode !== 'auto') throw new Error('Expected auto, 32 or 64');

    const file = 'LibraryTestMemory64.wasm';
    if (typeof WXWebAssembly !== 'undefined') {
        try {
            await WXWebAssembly.instantiate(probePath || '/' + file, {});
            return 64;
        } catch (error) {
            if (error && (error.name === 'CompileError' || /^CompileError:/.test(String(error)))) {
                return 32;
            }
            throw error;
        }
    }

    let bytes;
    if (typeof process !== 'undefined' && process.versions && process.versions.node) {
        bytes = require('fs').readFileSync(probePath || require('path').join(__dirname, file));
    } else {
        const response = await fetch(probePath || file);
        if (!response.ok) throw new Error('Cannot load Wasm width probe: HTTP ' + response.status);
        bytes = await response.arrayBuffer();
    }
    return WebAssembly.validate(bytes) ? 64 : 32;
}

/** 两个函数分别负责加载并运行一种产物。只加载选中的位宽，返回该位宽和实际返回值。 */
async function runLibraryTestPair(run32, run64, mode = 'auto', probePath) {
    const bits = await selectLibraryTestWidth(mode, probePath);
    const result = bits === 64 ? await run64() : await run32();
    return { bits, result };
}

if (typeof module !== 'undefined' && module.exports) {
    module.exports = selectLibraryTestWidth;
    module.exports.runPair = runLibraryTestPair;
}