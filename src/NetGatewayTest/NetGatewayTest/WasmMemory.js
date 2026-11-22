'use strict';

// Pointer cells follow the module ABI. Protocol words and lengths remain uint32.
function pointerBytes(module) { return module.wasmBits === 64 ? 8 : 4; }

function heapPointer(module, value) {
    const pointer = typeof value === 'bigint' ? Number(value) : value;
    if (!Number.isSafeInteger(pointer) || pointer < 0 || pointer > module.HEAPU8.byteLength)
        throw new RangeError('Wasm pointer exceeds the host memory range');
    return pointer;
}

function readPointer(module, address) {
    const size = pointerBytes(module);
    address = heapPointer(module, address);
    if (address % size || address + size > module.HEAPU8.byteLength) throw new RangeError('Invalid Wasm pointer cell');
    if (size === 4) return heapPointer(module, module.HEAPU32[address / 4]);
    const view = new DataView(module.HEAPU8.buffer);
    const low = view.getUint32(address, true);
    const high = size === 8 ? view.getUint32(address + 4, true) : 0;
    return heapPointer(module, low + high * 4294967296);
}

function writePointer(view, address, value, size) {
    if (!Number.isSafeInteger(value) || value < 0 || (size !== 4 && size !== 8) ||
        (size === 4 && value > 0xffffffff) || address % size || address < 0 || address + size > view.byteLength)
        throw new RangeError('Invalid Wasm pointer assignment');
    view.setUint32(address, value % 4294967296, true);
    if (size === 8) view.setUint32(address + 4, Math.floor(value / 4294967296), true);
}

function setModuleBits(module, factory) {
    const bits = factory.sdk ? factory.sdk.bits : 32;
    if (bits !== 32 && bits !== 64) throw new Error('Invalid Wasm SDK ABI');
    module.wasmBits = bits;
    return module;
}

function u64Arguments(module, low, high) {
    if (![low, high].every(value => Number.isInteger(value) && value >= 0 && value <= 0xffffffff))
        throw new RangeError('Invalid uint64 words');
    // Emscripten wasm32 legalizes i64 into two words; Memory64 uses native BigInt.
    return module.wasmBits === 64 ? [BigInt(low) + BigInt(high) * BigInt(4294967296)] : [low, high];
}

module.exports = { pointerBytes, heapPointer, readPointer, writePointer, setModuleBits, u64Arguments };