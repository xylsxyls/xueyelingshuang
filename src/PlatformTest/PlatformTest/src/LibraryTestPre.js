// 微信使用包内 Wasm 实例化接口。浏览器和 Node 使用各自的 WebAssembly。
var WebAssembly = Module['wasmApi'] || globalThis.WebAssembly;