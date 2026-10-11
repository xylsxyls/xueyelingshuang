const fs = require('node:fs')
const path = require('node:path')
const { loadConfig } = require('./config')
const { catalog, requirements } = require('./catalog')
const { selectCases } = require('./selection')
const { Reporter, redact } = require('./report')
const { TestEnvironment } = require('./environment')
const { TestHttpClient } = require('./http')
const { until, abortError } = require('./wait')

async function executeCase(test, action, context, reporter, batchSignal) {
  const startedAt = Date.now()
  const controller = new AbortController()
  const cancel = () => controller.abort()
  batchSignal.addEventListener('abort', cancel, { once: true })
  if (batchSignal.aborted) controller.abort()
  let timedOut = false
  const limit = [34, 36, 51].includes(test.id) ? 240000 : test.id === 35 ? context.config.caseTimeoutMs * context.config.stressLoops : context.config.caseTimeoutMs
  const timer = setTimeout(() => { timedOut = true; controller.abort() }, limit)
  reporter.result(test.id, { state: 'running', reason: '', startedAt: new Date().toISOString() })
  reporter.event(`执行 ${test.id} ${test.name} (${test.type})`)
  try {
    if (controller.signal.aborted) throw abortError()
    const evidence = await action({ ...context, signal: controller.signal, http: new TestHttpClient(context.config.serverPort, controller.signal) })
    if (controller.signal.aborted) throw abortError()
    let screenshot
    if (context.simulator && context.simulator.mini) screenshot = await context.simulator.screenshot(test.id)
    reporter.result(test.id, { state: 'passed', elapsedMs: Date.now() - startedAt, evidence, screenshot })
  } catch (error) {
    let screenshot
    if (context.simulator && context.simulator.mini) {
      try { screenshot = await context.simulator.screenshot(test.id) } catch (_) {}
    }
    const state = batchSignal.aborted && !timedOut ? 'canceled' : 'failed'
    const reason = timedOut ? '用例保护超时；本批停止，避免在途操作污染下一项' : redact(error.stack || error.message)
    reporter.result(test.id, { state, reason, elapsedMs: Date.now() - startedAt, screenshot })
    reporter.event(`${test.id} ${state} ${reason}`)
  } finally {
    clearTimeout(timer)
    batchSignal.removeEventListener('abort', cancel)
  }
  return timedOut
}

async function main(args) {
  let selected = selectCases(args, catalog)
  if (args.includes('--list')) {
    for (const item of catalog) process.stdout.write(`${item.id}\t${item.type}\t${item.name}\t${item.expected}\n`)
    return 0
  }
  if (args.includes('--backend-only')) {
    selected = selected.filter(item => [34, 36, 44, 51].includes(item.id))
    if (!selected.length) throw new Error('backend-only 筛选为空；指定34、36、44或51')
  }
  const config = loadConfig()
  const reporter = new Reporter(config, catalog, requirements, selected)
  const controller = new AbortController()
  const interrupt = () => { reporter.event('收到取消请求，正在停止本批'); controller.abort() }
  process.once('SIGINT', interrupt)
  process.once('SIGTERM', interrupt)
  const environment = new TestEnvironment(config, reporter, controller.signal)
  let simulator
  let setupError
  let simulatorError
  try {
    environment.prepare()
    if (args.includes('--prepare')) {
      reporter.finish('prepared')
      reporter.event('只准备隔离副本，未执行产品用例')
      return 0
    }
    await environment.startBackend()
    if (selected.some(item => ![34, 36, 44, 51].includes(item.id))) {
      const { Simulator } = require('./simulator')
      simulator = new Simulator(config, reporter, controller.signal)
      try { await simulator.connect(environment.projectPath) } catch (error) {
        simulatorError = redact(error.stack || error.message)
        reporter.report.simulatorError = simulatorError
        reporter.event('模拟器连接失败；继续独立服务端用例：' + simulatorError)
        await simulator.disconnect()
        simulator = null
      }
    }
    const actions = require('../suites')
    for (const item of catalog) if (typeof actions[item.id] !== 'function') throw new Error(`登记与实现不一致：${item.id}`)
    for (const item of selected) {
      if (!simulator && ![34, 36, 44, 51].includes(item.id)) {
        reporter.result(item.id, { reason: '模拟器连接未就绪：' + simulatorError })
        continue
      }
      if (controller.signal.aborted) { reporter.result(item.id, { reason: '本批取消，尚未开始' }); continue }
      const timeout = await executeCase(item, actions[item.id], { config, environment, simulator, reporter }, reporter, controller.signal)
      if (simulator) {
        const network = await simulator.network()
        fs.writeFileSync(path.join(reporter.directory, `network-${item.id}.json`), JSON.stringify(network, null, 2))
        await simulator.restoreMocks()
      }
      if (timeout) { controller.abort(); break }
    }
  } catch (error) {
    setupError = redact(error.stack || error.message)
    reporter.report.setupError = setupError
    reporter.event('测试环境失败：' + setupError)
    for (const item of selected) if (reporter.report.cases.find(result => result.id === item.id).state === 'not-run') reporter.result(item.id, { reason: '测试环境失败：' + setupError })
  } finally {
    if (simulator) {
      try { await simulator.disconnect() } catch (error) { reporter.event('模拟器收尾失败 ' + error.message) }
    }
    await environment.stop()
    process.removeListener('SIGINT', interrupt)
    process.removeListener('SIGTERM', interrupt)
    if (reporter.report.state !== 'prepared') reporter.finish(setupError || simulatorError ? 'blocked' : controller.signal.aborted ? 'canceled' : reporter.report.summary.failed ? 'failed' : 'completed')
    reporter.event('报告：' + path.join(reporter.directory, 'report.html'))
  }
  const results = selected.map(item => reporter.report.cases.find(result => result.id === item.id))
  return results.every(item => item.state === 'passed') ? 0 : 1
}

module.exports = { main, executeCase }