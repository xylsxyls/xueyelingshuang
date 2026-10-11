const assert = require('node:assert/strict')
const fs = require('node:fs')

module.exports = {
  34: async context => {
    const result = await context.environment.runServerTests(false, context.signal)
    const text = fs.readFileSync(result.reportPath, 'utf8')
    assert.ok(text.includes('整体结果：通过') && !text.includes('[FAIL]'))
    const passedCases = (text.match(/\[PASS\]/g) || []).length
    const notRunCases = (text.match(/\[NOT-RUN\]/g) || []).length
    const voiceEnabled = context.config.voiceProvider === 'windows_sapi'
    assert.equal(passedCases, voiceEnabled ? 69 : 68, '真实语音开启时必须全部通过')
    assert.equal(notRunCases, voiceEnabled ? 0 : 1)
    return { ...result, passedCases, notRunCases, incomplete: voiceEnabled ? '' : '实际语音合成未执行' }
  },
  36: async context => {
    const result = await context.environment.runServerTests(true, context.signal)
    const text = fs.readFileSync(result.reportPath, 'utf8')
    assert.ok(text.includes('整体结果：通过') && !text.includes('[FAIL]'))
    assert.ok(text.includes('失败请求数：0'))
    assert.ok(text.includes(`总请求数：${context.config.serverStressThreads * context.config.serverStressLoops}`))
    return { ...result, threads: context.config.serverStressThreads, loops: context.config.serverStressLoops }
  }
}