const path = require('node:path')
const fs = require('node:fs')
const { spawn } = require('node:child_process')
const assert = require('node:assert/strict')
const automator = require('miniprogram-automator')
const { until } = require('./wait')
const { connectBounded } = require('./automationConnection')
const { runtimeSelector } = require('./selectors')

class Simulator {
  constructor(config, reporter, signal) { this.config = config; this.reporter = reporter; this.signal = signal; this.mocks = new Set() }

  async connect(projectPath) {
    // Node 18 的 Windows spawn 不能直接执行 .bat；以参数数组调用同目录官方 cli.js。
    // SDK 的 connect、页面及断言 API 不变，不使用 shell 拼接项目路径。
    if (process.platform === 'win32') {
      const { portFree } = require('./environment')
      await portFree(this.config.automationPort)
      const cliScript = path.join(path.dirname(this.config.cliPath), 'cli.js')
      if (!fs.existsSync(cliScript)) throw new Error('缺少官方 CLI 脚本 ' + cliScript)
      const log = fs.openSync(path.join(this.reporter.directory, 'devtools-cli.log'), 'a')
      this.cliProcess = spawn(this.config.nodePath, [cliScript, 'auto', '--project', projectPath, '--auto-port', String(this.config.automationPort), '--trust-project'], { windowsHide: true, stdio: ['ignore', log, log] })
      fs.closeSync(log)
      let spawnError
      this.cliProcess.on('error', error => { spawnError = error })
      await until(() => {
        if (spawnError || (this.cliProcess.exitCode != null && this.cliProcess.exitCode !== 0)) {
          const error = spawnError || new Error('微信官方CLI auto退出失败')
          error.fatal = true
          throw error
        }
        return this.cliProcess.exitCode === 0
      }, '官方CLI auto完成后再连接，避免编译重启断开旧socket', this.config.startupTimeoutMs, this.signal)
      this.mini = await until(async () => {
        const logFile = path.join(this.reporter.directory, 'devtools-cli.log')
        const logText = fs.readFileSync(logFile, 'utf8')
        if (logText.includes('[error]') && logText.includes('TypeError: d.on is not a function')) {
          const error = new Error('微信开发者工具创建窗口失败：d.on is not a function；请手动打开 temp/project 再运行')
          error.fatal = true
          throw error
        }
        if (spawnError || (this.cliProcess.exitCode != null && this.cliProcess.exitCode !== 0)) {
          const error = spawnError || new Error('微信官方 CLI 退出失败，查看 devtools-cli.log')
          error.fatal = true
          throw error
        }
        try { return await connectBounded(`ws://127.0.0.1:${this.config.automationPort}`, 4000, this.signal) } catch (_) { return false }
      }, '官方 CLI 自动化连接（日志 devtools-cli.log）', this.config.startupTimeoutMs, this.signal)
    } else {
      this.mini = await automator.launch({ cliPath: this.config.cliPath, projectPath, port: this.config.automationPort, timeout: this.config.startupTimeoutMs, trustProject: true })
    }
    if (!this.mini) throw new Error('微信官方 SDK 未返回连接；检查设置→安全→服务端口')
    this.mini.on('exception', error => this.reporter.event('模拟器异常 ' + JSON.stringify(error)))
    // SDK 的 on('console') 会丢弃 enableLog Promise，失败时造成未处理拒绝。
    await this.mini.send('App.enableLog')
    require('node:events').EventEmitter.prototype.on.call(this.mini, 'console', event => {
      if (event.level === 'error' || event.type === 'error') this.reporter.event('模拟器错误 ' + JSON.stringify(event))
    })
    const target = await this.mini.evaluate(() => getApp().__cookTest && getApp().__cookTest.config.BASE_URL)
    assert.equal(target, `http://127.0.0.1:${this.config.serverPort}`, '自动化连接必须是本批隔离副本')
    this.reporter.report.environment.systemInfo = await this.mini.systemInfo()
    this.reporter.save()
  }

  async page(route, tab = false) {
    await (tab ? this.mini.switchTab(route) : this.mini.reLaunch(route))
    return until(async () => { const page = await this.mini.currentPage(); return page && page.path === route.split('?')[0].replace(/^\//, '') && page }, '打开 ' + route, 20000, this.signal)
  }

  async element(page, selector) { return until(() => page.$(runtimeSelector(selector)), '元素 ' + selector, 15000, this.signal) }
  async tap(page, selector) { await (await this.element(page, selector)).tap() }
  async input(page, selector, value) { await (await this.element(page, selector)).input(String(value)) }
  async data(page, predicate, description, timeout = 15000) { return until(async () => { const data = await page.data(); return predicate(data) && data }, description, timeout, this.signal) }
  async mock(method, result) { await this.mini.mockWxMethod(method, result); this.mocks.add(method); this.reporter.event('官方替身边界：wx.' + method) }
  async restoreMocks() {
    for (const method of this.mocks) await this.mini.restoreWxMethod(method)
    this.mocks.clear()
  }

  async screenshot(id) {
    const file = `case-${id}.png`
    await this.mini.screenshot({ path: path.join(this.reporter.directory, file) })
    return file
  }

  async network() { return this.mini.evaluate(() => (wx.__cookTestNetwork || []).slice()) }
  async mediaNetwork() { return (await this.network()).filter(item => item.url.includes('/api/videos/segment?')) }
  async token() { return this.mini.evaluate(() => getApp().globalData.token) }

  async reset() {
    await this.restoreMocks()
    await this.page('/pages/auth/login/login')
    await this.mini.evaluate(() => {
      wx.removeStorageSync('cook_token')
      wx.removeStorageSync('cook_account')
      const data = getApp().globalData
      data.token = ''; data.account = null; data.userId = ''; data.selectedRecipeIds = []; data.selectedRecipes = []
      data.lastPreview = null; data.lastPlan = null; data.lastPreviewRecipeKey = ''
      wx.__cookTestNetwork = []
      wx.__cookTestFrames = []
      wx.__cookTestFrameCount = 0
      wx.__cookTestSeeks = []
    })
  }

  async disconnect() {
    if (this.mini) {
      try { await this.restoreMocks() } finally { this.mini.disconnect(); this.mini = null }
    }
    if (this.cliProcess && this.cliProcess.exitCode == null && this.cliProcess.signalCode == null) this.cliProcess.kill()
  }
}

module.exports = { Simulator }