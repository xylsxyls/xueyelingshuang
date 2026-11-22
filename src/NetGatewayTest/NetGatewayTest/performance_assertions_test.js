'use strict';

// 固定正反例证明性能判定器会拒绝漏收、错正文、错误并发度和伪造计时。
const assert = require('node:assert/strict');
const checks = require('./NetPerformanceAssertions');
const expected = { count: 13, bodyBytes: 6, producers: 4, method: 'post' };
const tx = { submitted: 13, startedUs: 100, submitFinishedUs: 130, backpressure: 0 };
const rx = { received: 13, bodyBytes: 6, producerReceived: [4, 3, 3, 3], finishedUs: 200 };
const rpc = { requests: 13, responses: 13, bodyBytes: 6, method: 'post', producers: 4,
    clientSentBodyBytes: 78, clientReceivedBodyBytes: 78, producerReceived: [4, 3, 3, 3],
    window: 128, elapsedUs: 100, p50Us: 1, p95Us: 2, p99Us: 3,
    requestsPerSecond: 130000, tcpSentBytes: 256, tcpReceivedBytes: 256 };
let count = 0;
assert.equal(checks.oneWay(tx, rx, expected), 100); ++count;
checks.rpc(rpc, expected); ++count;
// 最后一次提交可晚于接收完成，例如发送线程被抢占；不能误判合法执行。
assert.equal(checks.oneWay({ ...tx, submitFinishedUs: 250 }, rx, expected), 100); ++count;
for (const [field, value] of [
    ['submitted', 12], ['submitted', 14], ['startedUs', 0], ['startedUs', NaN],
    ['startedUs', 100.5], ['startedUs', Number.MAX_SAFE_INTEGER + 1], ['submitFinishedUs', 99],
    ['backpressure', -1], ['backpressure', undefined]
]) {
    assert.throws(() => checks.oneWay({ ...tx, [field]: value }, rx, expected)); ++count;
}
for (const [field, value] of [
    ['received', 12], ['received', 14], ['bodyBytes', 0], ['producerReceived', [13]],
    ['producerReceived', [3, 4, 3, 3]], ['finishedUs', 100], ['finishedUs', Infinity]
]) {
    assert.throws(() => checks.oneWay(tx, { ...rx, [field]: value }, expected)); ++count;
}
for (const [field, value] of [
    ['requests', 12], ['responses', 12], ['bodyBytes', 0], ['method', 'send'], ['producers', 1],
    ['clientSentBodyBytes', 79], ['clientReceivedBodyBytes', 77], ['producerReceived', [13]],
    ['producerReceived', [3, 4, 3, 3]], ['window', undefined], ['window', 0], ['elapsedUs', 0],
    ['p50Us', -1], ['p95Us', 0], ['p99Us', 1], ['requestsPerSecond', 15000000],
    ['requestsPerSecond', NaN], ['tcpSentBytes', 0], ['tcpReceivedBytes', -1]
]) {
    assert.throws(() => checks.rpc({ ...rpc, [field]: value }, expected)); ++count;
}
const summary = 'SERVICE_PROBE_SUMMARY {"responses":77,"bodyBytes":462}\n';
checks.rpcService(summary, 77, 6); ++count;
checks.rpcService('SERVICE_PROBE_SUMMARY {"responses":77,"bodyBytes":0}\r\n', 77, 0); ++count;
for (const stdout of ['', summary + summary, summary.replace('77', '76'), summary.replace('462', '461'),
    summary.replace('77', '"77"'), summary.replace('462', 'null'), summary.replace('{', '!'),
    'SERVICE_PROBE_ECHO pid=1 size=6\n' + summary]) {
    assert.throws(() => checks.rpcService(stdout, 77, 6)); ++count;
}
console.log(`PERFORMANCE_ASSERTIONS checks=${count} PASS`);