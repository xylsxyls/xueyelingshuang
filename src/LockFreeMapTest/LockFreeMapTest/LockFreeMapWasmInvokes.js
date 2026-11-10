'use strict';

// 仅对私有Wasm测试产物增加异常桥接计数，不改容器或共享SDK
const fs = require('fs');
const path = require('path');

/** 给生成的invoke包装加入诊断计数，保留原调用和异常处理
 * 参数依次为输入JS及不同路径的输出JS；未匹配包装时明确失败
 */
function InstrumentInvokes() {
    if (process.argv.length !== 4) {
        throw new Error('需要输入JS和输出JS两个路径');
    }
    const source = path.resolve(process.argv[2]);
    const destination = path.resolve(process.argv[3]);
    if (source === destination) {
        throw new Error('诊断产物不能覆盖原始JS');
    }
    const original = fs.readFileSync(source, 'utf8');
    let wrappers = 0;
    // 回调保留函数签名，仅在C++诊断阶段打开计数时记录真实调用目标
    const result = original.replace(/function (invoke_[A-Za-z0-9_]+)\([^)]*\)\{/g, (declaration, name) => {
        ++wrappers;
        return declaration + 'if(globalThis.lockFreeMapInvokeEnabled){' +
            'var lfmTarget=getWasmTableEntry(Number(index));' +
            'var lfmKey="' + name + ':"+index+":"+lfmTarget.name;' +
            'var lfmCounts=globalThis.lockFreeMapInvokeCounts;' +
            'lfmCounts[lfmKey]=(lfmCounts[lfmKey]||0)+1;}';
    });
    if (wrappers === 0) {
        throw new Error('没有匹配invoke包装，不能把零计数当作测量结果');
    }
    fs.writeFileSync(destination, result, 'utf8');
    fs.writeFileSync(destination + '.instrumentation.json', JSON.stringify({source, destination, wrappers}, null, 2), 'utf8');
}

/** 读取Wasm名称段中的函数索引，给运行期invoke计数匹配真实函数名
 * 输入为带profiling名称段的Wasm文件；输出为函数索引到名称的JSON映射
 */
function ReadFunctionNames(source, destination) {
    const bytes = fs.readFileSync(source);
    if (bytes.length < 8 || bytes.readUInt32LE(0) !== 0x6d736100) {
        throw new Error('输入不是Wasm模块');
    }
    const cursor = {position: 8, limit: bytes.length};
    // 读取至多32位的无符号LEB128，拒绝越界及无终止字节
    function ReadUnsigned() {
        let value = 0;
        for (let shift = 0; shift <= 28; shift += 7) {
            if (cursor.position >= cursor.limit) {
                throw new Error('名称段整数越界');
            }
            const byte = bytes[cursor.position++];
            if (shift === 28 && byte > 15) {
                throw new Error('名称段整数溢出');
            }
            value += (byte & 127) * Math.pow(2, shift);
            if ((byte & 128) === 0) {
                return value;
            }
        }
        throw new Error('名称段整数过长');
    }
    // UTF-8名字按编码字节数读取，不能越过当前子段边界
    function ReadName() {
        const length = ReadUnsigned();
        const end = cursor.position + length;
        if (end > cursor.limit) {
            throw new Error('函数名越界');
        }
        const name = bytes.toString('utf8', cursor.position, end);
        cursor.position = end;
        return name;
    }
    const result = {};
    while (cursor.position < bytes.length) {
        cursor.limit = bytes.length;
        const section = bytes[cursor.position++];
        const length = ReadUnsigned();
        const end = cursor.position + length;
        if (end > bytes.length) {
            throw new Error('Wasm段越界');
        }
        cursor.limit = end;
        if (section === 0 && ReadName() === 'name') {
            while (cursor.position < end) {
                cursor.limit = end;
                const subsection = bytes[cursor.position++];
                const size = ReadUnsigned();
                const subsectionEnd = cursor.position + size;
                if (subsectionEnd > end) {
                    throw new Error('Wasm名称子段越界');
                }
                cursor.limit = subsectionEnd;
                if (subsection === 1) {
                    const count = ReadUnsigned();
                    for (let index = 0; index < count; ++index) {
                        const functionIndex = ReadUnsigned();
                        result[functionIndex] = ReadName();
                    }
                }
                cursor.position = subsectionEnd;
            }
        }
        cursor.position = end;
    }
    if (Object.keys(result).length === 0) {
        throw new Error('模块没有函数名称段');
    }
    fs.writeFileSync(destination, JSON.stringify(result, null, 2), 'utf8');
}
try {
    if (process.argv[2] === '--names' && process.argv.length === 5) {
        ReadFunctionNames(process.argv[3], process.argv[4]);
    } else {
        InstrumentInvokes();
    }
} catch (error) {
    process.stderr.write(String(error) + '\n');
    process.exitCode = 1;
}