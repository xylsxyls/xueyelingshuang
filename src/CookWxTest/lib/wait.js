function abortError() {
  const error = new Error('本批已取消')
  error.name = 'AbortError'
  return error
}

async function sleep(ms, signal) {
  if (signal && signal.aborted) throw abortError()
  await new Promise((resolve, reject) => {
    const timer = setTimeout(() => { if (signal) signal.removeEventListener('abort', cancel); resolve() }, ms)
    const cancel = () => { clearTimeout(timer); signal.removeEventListener('abort', cancel); reject(abortError()) }
    if (signal) signal.addEventListener('abort', cancel, { once: true })
  })
}

async function until(probe, description, timeoutMs = 15000, signal) {
  const deadline = Date.now() + timeoutMs
  let lastError
  while (Date.now() < deadline) {
    if (signal && signal.aborted) throw abortError()
    try {
      const result = await probe()
      if (result) return result
    } catch (error) {
      if (error.fatal) throw error
      lastError = error
    }
    await sleep(150, signal)
  }
  throw new Error(`等待超时：${description}${lastError ? '；' + lastError.message : ''}`)
}

module.exports = { sleep, until, abortError }