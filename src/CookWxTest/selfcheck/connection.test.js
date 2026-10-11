const test = require('node:test')
const assert = require('node:assert/strict')
const WebSocket = require('ws')
const { connectBounded } = require('../lib/automationConnection')

test('官方协议无响应和取消都在期限内关闭测试socket', async () => {
  const server = new WebSocket.Server({ host: '127.0.0.1', port: 0 })
  await new Promise(resolve => server.once('listening', resolve))
  const endpoint = 'ws://127.0.0.1:' + server.address().port
  try {
    await assert.rejects(connectBounded(endpoint, 40), /超时|closed/)
    const cancel = new AbortController()
    const waiting = connectBounded(endpoint, 10000, cancel.signal)
    cancel.abort()
    await assert.rejects(waiting, /取消/)
  } finally {
    for (const client of server.clients) client.terminate()
    await new Promise(resolve => server.close(resolve))
  }
})

test('真实官方协议连接的业务请求有单独超时，失败可以收尾', async () => {
  const server = new WebSocket.Server({ host: '127.0.0.1', port: 0 })
  await new Promise(resolve => server.once('listening', resolve))
  server.on('connection', socket => socket.on('message', raw => {
    const request = JSON.parse(raw)
    if (request.method === 'Tool.getInfo') socket.send(JSON.stringify({ id: request.id, result: { SDKVersion: '3.17.0' } }))
  }))
  let mini
  try {
    mini = await connectBounded('ws://127.0.0.1:' + server.address().port, 1000, undefined, 40)
    await assert.rejects(mini.evaluate(() => true), /超时|closed/)
  } finally {
    if (mini) mini.disconnect()
    for (const client of server.clients) client.terminate()
    await new Promise(resolve => server.close(resolve))
  }
})