// VideoDecoder 的 start/seek 返回 void。完成条件来自原生事件，不能 await 返回值。
function decoderOperation(decoder, eventName, invoke, timeoutMs) {
  let finished = false
  let timer
  let rejectOperation
  let ready
  const cleanup = () => {
    clearTimeout(timer)
    decoder.off(eventName, ready)
  }
  const reject = error => {
    if (finished) return
    finished = true
    cleanup()
    rejectOperation(error)
  }
  const promise = new Promise((resolve, rejectPromise) => {
    rejectOperation = rejectPromise
    ready = metadata => {
      if (finished) return
      finished = true
      cleanup()
      resolve(metadata)
    }
    decoder.on(eventName, ready)
    timer = setTimeout(() => reject(new Error('视频解码操作超时：' + eventName)), timeoutMs)
    try {
      const result = invoke()
      if (result && typeof result.catch === 'function') result.catch(reject)
    } catch (error) { reject(error) }
  })
  return { promise, cancel: () => reject(new Error('视频解码操作已取消')) }
}

module.exports = { decoderOperation }