'use strict';
// 只验证Wasm宿主执行器的失败收尾；不替代Semaphore的C++能力测试
const fs = require('fs');
const path = require('path');
const vm = require('vm');
const events = require('events');
const runner = require('./run_wasm.js');

/** 核对执行器的独立预期，异常交给入口报告
@param [in] condition 必须成立的条件
@param [in] message 失败说明
*/
function check(condition, message)
{
    if (!condition)
    {
        throw new Error(message);
    }
}

/** 模拟系统拒绝清理或日志错误，不能真实制造不可终止的进程
@param [in] mode spawn/write/close/kill，分别注入对应故障
@return Promise，错误必须被转换为125并且日志恰好关闭一次
*/
async function fault(mode)
{
    const worker = new events.EventEmitter();
    worker.pid = 123;
    worker.exitCode = null;
    worker.signalCode = null;
    let unreferenced = false;
    let closed = 0;
    const messages = [];
    worker.unref = () => { unreferenced = true; };
    worker.kill = () => false;
    const fakeFs = {
        openSync: () => 7,
        writeSync: (descriptor, text) =>
        {
            check(descriptor === 7, 'wrong log descriptor');
            if (mode === 'write') throw new Error('injected write failure');
            messages.push(text);
        },
        closeSync: descriptor =>
        {
            check(descriptor === 7, 'wrong close descriptor');
            ++closed;
            if (mode === 'close') throw new Error('injected close failure');
        }
    };
    const fakeChild = {
        spawn: () =>
        {
            if (mode === 'spawn') throw new Error('injected spawn failure');
            if (mode !== 'kill') setImmediate(() =>
            {
                if (mode === 'write') worker.emit('error', new Error('injected async error'));
                worker.emit('close', 0);
            });
            return worker;
        },
        spawnSync: () => ({status: 1})
    };
    const scope = {
        module: {exports: {}}, __dirname: __dirname,
        require: name => name === 'fs' ? fakeFs : name === 'child_process' ? fakeChild : require(name),
        console: {error: text => messages.push(text)},
        // 仅缩短替身的清理等待，真实执行器仍使用原来的5秒预算
        setTimeout: (callback, delay) => setTimeout(callback, mode === 'kill' ? 5 : delay),
        clearTimeout: clearTimeout
    };
    vm.runInNewContext(fs.readFileSync(path.join(__dirname, 'run_wasm.js'), 'utf8'), scope);
    const result = await scope.module.exports.run('fake', [], 'fake.log', {}, 1000);
    check(result === 125 && closed === 1, mode + ': failure hidden or descriptor not closed exactly once');
    if (mode === 'spawn')
    {
        check(messages.length === 1 && messages[0].endsWith('\n'), 'diagnostic line is not terminated');
    }
    if (mode === 'kill')
    {
        check(unreferenced && messages.some(text => text.includes('cleanup was not confirmed')),
            'unconfirmed cleanup did not terminate the executor wait');
        // 迟到的退出不得二次关闭日志或覆盖失败结果
        worker.emit('close', 0);
        check(closed === 1, 'late close released the descriptor twice');
    }
}

/** 验证真实退出、启动失败、超时及受控清理失败，产物只写专属临时目录
@return Promise，所有执行器预期成立时正常完成
*/
async function main()
{
    const single = 'PASS id=87\nNOT_RUN id=49 nonpthread\nNOT_RUN id=67 nonpthread\nPASS id=48 ok';
    const multi = 'PASS id=87\nPASS id=49 ok\nPASS id=67 ok\nPASS id=48 ok';
    check(runner.validResults(single, false) && runner.validResults(multi, true), 'complete Wasm results rejected');
    for (const invalid of [single.replace('PASS id=48', 'BYPASS id=48'), single.replace('id=48', 'id=480'),
        single.replace('NOT_RUN id=67 nonpthread\n', ''), multi.replace('PASS id=67', 'FAIL id=67'),
        single + '\nPASS id=48', single + '\nFAIL id=87', single.replace('NOT_RUN', 'PASS')])
    {
        check(!runner.validResults(invalid, false), 'incorrect Wasm results accepted');
    }
    const allowed = path.resolve(__dirname, '../../temp/Semaphore');
    const output = path.resolve(process.argv[2] || '');
    const relative = path.relative(allowed, output);
    check(relative && !relative.startsWith('..') && !path.isAbsolute(relative), 'Use a new private temp subdirectory');
    check(!fs.existsSync(output), 'Output already exists');
    fs.mkdirSync(output, {recursive: true});
    for (const code of [0, 19])
    {
        const result = await runner.run(process.execPath, ['-e', 'process.exitCode=' + code],
            path.join(output, 'exit-' + code + '.log'), process.env, 5000);
        check(result === code, 'wrong real child exit code');
    }
    check(await runner.run(path.join(output, 'missing.exe'), [], path.join(output, 'missing.log'), process.env, 5000) === 125,
        'spawn failure accepted');
    check(await runner.run(process.execPath, ['-e', 'setTimeout(()=>{},60000)'],
        path.join(output, 'timeout.log'), process.env, 100) === 125, 'timeout accepted');
    for (const mode of ['spawn', 'write', 'close', 'kill'])
    {
        await fault(mode);
    }
    console.log('PASS Wasm runner: exit codes, spawn failure, timeout, log failures, failed cleanup and late close');
}

main().catch(error =>
{
    console.error(error.stack);
    process.exitCode = 1;
});