'use strict';

// 性能报告的独立判定器，只检查真实程序报告，不生成正文、包或预期回包。
const assert = require('node:assert/strict');

function integer(value, label, minimum = 0) {
    assert.ok(Number.isSafeInteger(value) && value >= minimum, label + ' must be a safe integer');
}

function producerCounts(count, producers) {
    integer(count, 'count', 1);
    integer(producers, 'producers', 1);
    assert.ok(producers <= count);
    return Array.from({ length: producers }, (_, index) =>
        Math.floor(count / producers) + (index < count % producers ? 1 : 0));
}

function wire(record) {
    integer(record.tcpSentBytes, 'tcpSentBytes', 1);
    integer(record.tcpReceivedBytes, 'tcpReceivedBytes', 1);
}

function oneWay(tx, rx, expected) {
    integer(expected.bodyBytes, 'bodyBytes');
    assert.equal(tx.submitted, expected.count, 'every call must be accepted');
    assert.equal(rx.received, expected.count, 'every frame must be decrypted and checked');
    assert.equal(rx.bodyBytes, expected.bodyBytes, 'receiver body differs from selected case');
    assert.deepEqual(rx.producerReceived, producerCounts(expected.count, expected.producers));
    integer(tx.startedUs, 'startedUs', 1);
    integer(tx.submitFinishedUs, 'submitFinishedUs', tx.startedUs);
    integer(tx.backpressure, 'backpressure');
    integer(rx.finishedUs, 'finishedUs', tx.startedUs + 1);
    // 结束点必须取接收端校验完成时刻，不拿发送接口返回时刻计算吞吐。
    return rx.finishedUs - tx.startedUs;
}

function rpc(record, expected) {
    integer(expected.bodyBytes, 'bodyBytes');
    assert.equal(record.requests, expected.count);
    assert.equal(record.responses, expected.count);
    assert.equal(record.bodyBytes, expected.bodyBytes);
    assert.equal(record.method, expected.method);
    assert.equal(record.producers, expected.producers);
    assert.equal(record.clientSentBodyBytes, expected.count * expected.bodyBytes);
    assert.equal(record.clientReceivedBodyBytes, expected.count * expected.bodyBytes);
    assert.deepEqual(record.producerReceived, producerCounts(expected.count, expected.producers));
    integer(record.window, 'RPC window', 1);
    integer(record.elapsedUs, 'elapsedUs', 1);
    integer(record.p50Us, 'p50Us');
    integer(record.p95Us, 'p95Us', record.p50Us);
    integer(record.p99Us, 'p99Us', record.p95Us);
    const rate = expected.count * 1000000 / record.elapsedUs;
    assert.ok(Number.isFinite(record.requestsPerSecond) && record.requestsPerSecond > 0 &&
        Math.abs(record.requestsPerSecond - rate) <= 0.001, 'reported rate differs from count/time');
    wire(record);
}

function rpcService(stdout, count, bodyBytes) {
    integer(count, 'RPC service expected replies', 1);
    integer(bodyBytes, 'RPC service body size');
    const records = [...stdout.matchAll(/^SERVICE_PROBE_SUMMARY (.+)\r?$/gm)];
    assert.equal(records.length, 1, 'Exactly one terminal service summary required');
    assert.ok(!/^SERVICE_PROBE_ECHO /m.test(stdout), 'Per-message logging must be disabled during RPC measurements');
    const record = JSON.parse(records[0][1]);
    assert.equal(record.responses, count, 'Service response count including warm-up');
    assert.equal(record.bodyBytes, count * bodyBytes, 'Service response bytes including warm-up');
}

module.exports = { oneWay, rpc, rpcService, wire };