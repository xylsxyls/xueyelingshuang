const WebSocket = require('ws')
const Connection = require('miniprogram-automator/out/Connection').default
const Transport = require('miniprogram-automator/out/Transport').default
const MiniProgram = require('miniprogram-automator/out/MiniProgram').default

// SDK 0.12.1 的连接/版本检查没有保护超时且失败后不关闭 socket。
// 复用该固定版本的协议和 MiniProgram，只在连接与请求外补有界生命周期。
async function connectBounded(endpoint, timeoutMs, signal, requestTimeoutMs = 30000) {
  let socket
  let mini
  let timer
  let cancel
  let settled = false
  try {
    return await new Promise((resolve, reject) => {
      const fail = error => {
        if (settled) return
        settled = true
        reject(error)
        if (socket) socket.terminate()
        clearTimeout(timer)
        if (signal && cancel) signal.removeEventListener('abort', cancel)
      }
      socket = new WebSocket(endpoint, { handshakeTimeout: timeoutMs })
      timer = setTimeout(() => fail(new Error('微信自动化握手/版本响应超时')), timeoutMs)
      cancel = () => fail(new Error('微信自动化连接已取消'))
      if (signal) signal.addEventListener('abort', cancel, { once: true })
      socket.once('error', fail)
      socket.once('open', async () => {
        const connection = new Connection(new Transport(socket))
        const send = connection.send.bind(connection)
        connection.send = (method, params) => {
          let deadline
          return Promise.race([
            send(method, params),
            new Promise((_, reject) => {
              deadline = setTimeout(() => { socket.terminate(); reject(new Error('自动化请求超时：' + method)) }, requestTimeoutMs)
            })
          ]).finally(() => clearTimeout(deadline))
        }
        mini = new MiniProgram(connection)
        try {
          await mini.checkVersion()
          clearTimeout(timer)
          if (signal && cancel) signal.removeEventListener('abort', cancel)
          settled = true
          resolve(mini)
        } catch (error) { fail(error) }
      })
      if (signal && signal.aborted) cancel()
    })
  } catch (error) {
    if (mini) mini.disconnect()
    throw error
  }
}

module.exports = { connectBounded }