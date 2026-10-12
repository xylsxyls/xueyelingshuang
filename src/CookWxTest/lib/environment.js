const fs = require('node:fs')
const path = require('node:path')
const net = require('node:net')
const { spawn } = require('node:child_process')
const { until } = require('./wait')
const { TestHttpClient } = require('./http')
const { hashFile } = require('./report')
const { instrumentWxml } = require('./selectors')

async function portFree(port) {
  await new Promise((resolve, reject) => {
    const probe = net.createServer()
    probe.once('error', () => reject(new Error(`测试端口 ${port} 已占用，拒绝连接或停止已有实例`)))
    probe.listen(port, '127.0.0.1', () => probe.close(resolve))
  })
}

async function portReady(port) {
  return new Promise(resolve => {
    const socket = net.connect({ host: '127.0.0.1', port })
    const finish = ready => { socket.destroy(); resolve(ready) }
    socket.once('connect', () => finish(true))
    socket.once('error', () => finish(false))
    socket.setTimeout(500, () => finish(false))
  })
}

function copyDirectory(source, target) {
  if (fs.existsSync(target) && fs.lstatSync(target).isSymbolicLink()) throw new Error(`拒绝写入链接 ${target}`)
  fs.mkdirSync(target, { recursive: true })
  for (const entry of fs.readdirSync(source, { withFileTypes: true })) {
    if (entry.isSymbolicLink()) throw new Error(`拒绝复制链接 ${entry.name}`)
    const from = path.join(source, entry.name)
    const to = path.join(target, entry.name)
    if (fs.existsSync(to) && fs.lstatSync(to).isSymbolicLink()) throw new Error(`拒绝覆盖链接 ${to}`)
    if (entry.isDirectory()) copyDirectory(from, to)
    else fs.copyFileSync(from, to)
  }
}

class TestEnvironment {
  constructor(config, reporter, signal) {
    this.config = config
    this.reporter = reporter
    this.signal = signal
    this.root = path.join(config.projectRoot, 'temp', reporter.runId)
    this.projectPath = path.join(config.projectRoot, 'temp/project')
    this.serverPath = path.join(this.root, 'server')
    this.resourcePath = path.join(this.root, 'CookServerRes')
    this.serverExecutablePath = path.join(this.serverPath, 'CookServer1.0.exe')
    this.children = []
    this.stopped = false
    this.lockPath = path.join(config.projectRoot, 'temp/runner.lock')
  }

  prepare() {
    const { config } = this
    if (fs.existsSync(this.root)) throw new Error('测试批次目录已存在')
    fs.mkdirSync(this.root, { recursive: true })
    this.lockHandle = fs.openSync(this.lockPath, 'wx')
    fs.writeFileSync(this.lockHandle, JSON.stringify({ runId: this.reporter.runId, pid: process.pid }))
    const ownershipFile = path.join(config.projectRoot, 'temp/project.owner.json')
    if (fs.existsSync(this.projectPath)) {
      if (!fs.existsSync(ownershipFile)) throw new Error('固定测试副本目录已存在但无归属标记，拒绝覆盖')
      const ownership = JSON.parse(fs.readFileSync(ownershipFile, 'utf8'))
      if (ownership.owner !== 'CookWxTest' || ownership.path !== this.projectPath) throw new Error('测试副本归属不匹配')
    }
    fs.writeFileSync(ownershipFile, JSON.stringify({ owner: 'CookWxTest', path: this.projectPath }))
    copyDirectory(config.sourceRoot, this.projectPath)
    const markWxml = directory => {
      for (const entry of fs.readdirSync(directory, { withFileTypes: true })) {
        const file = path.join(directory, entry.name)
        if (entry.isDirectory()) markWxml(file)
        else if (entry.name.endsWith('.wxml')) fs.writeFileSync(file, instrumentWxml(fs.readFileSync(file, 'utf8')))
      }
    }
    markWxml(this.projectPath)
    const configFile = path.join(this.projectPath, 'utils/config.js')
    let source = fs.readFileSync(configFile, 'utf8')
    if (!/const USE_LOCAL_SERVER = (?:false|true)/.test(source) || !/const LOCAL_API_PORT = \d+/.test(source)) throw new Error('CookWx 配置结构改变，拒绝自动替换')
    source = source.replace(/const USE_LOCAL_SERVER = (?:false|true)/, 'const USE_LOCAL_SERVER = true').replace(/const LOCAL_API_PORT = \d+/, `const LOCAL_API_PORT = ${config.serverPort}`)
    fs.writeFileSync(configFile, source)
    const projectFile = path.join(this.projectPath, 'project.config.json')
    const project = JSON.parse(fs.readFileSync(projectFile, 'utf8'))
    project.projectname = 'CookWxTest'
    project.setting = { ...project.setting, urlCheck: false }
    fs.writeFileSync(projectFile, JSON.stringify(project, null, 2))
    const privateFile = path.join(this.projectPath, 'project.private.config.json')
    fs.writeFileSync(privateFile, JSON.stringify({ projectname: project.projectname, setting: { urlCheck: false }, libVersion: project.libVersion }, null, 2))
    // 只在本批副本加入观察桥；真实 wx API 照常执行，成功状态不由测试赋值。
    const appFile = path.join(this.projectPath, 'app.js')
    const observation = `const __cookTestConfig = require('./utils/config')\nconst __cookTestCache = require('./utils/videoSegmentCache')\nwx.__cookTestNetwork = []\n;['request', 'downloadFile'].forEach(function(method) {\n  const original = wx[method]\n  wx[method] = function(options) {\n    const url = String(options.url || '')\n    if (url.indexOf(__cookTestConfig.BASE_URL + '/') !== 0) throw new Error('测试副本拒绝非隔离服务器请求：' + url.split('?')[0])\n    const record = { method: method, url: url, startedAt: Date.now(), status: 0 }\n    wx.__cookTestNetwork.push(record)\n    if (wx.__cookTestNetwork.length > 2000) wx.__cookTestNetwork.shift()\n    const success = options.success\n    const fail = options.fail\n    return original.call(wx, Object.assign({}, options, { success: function(result) { record.status = result.statusCode; record.finishedAt = Date.now(); if (success) success(result) }, fail: function(error) { record.error = error.errMsg || 'request failed'; record.finishedAt = Date.now(); if (fail) fail(error) } }))\n  }\n})\n`
    let app = fs.readFileSync(appFile, 'utf8')
    app = app.replace('  globalData: {', `  __cookTest: { config: __cookTestConfig, cache: __cookTestCache },\n  globalData: {`)
    fs.writeFileSync(appFile, observation + app)
    const playerFile = path.join(this.projectPath, 'components/cachedVideoPlayer/cachedVideoPlayer.js')
    const playerSource = fs.readFileSync(playerFile, 'utf8')
    const player = require('./mediaObservation').instrumentPlayer(playerSource)
    fs.writeFileSync(playerFile, player)
    fs.mkdirSync(this.serverPath)
    for (const entry of fs.readdirSync(config.serverRoot, { withFileTypes: true })) {
      if (entry.isFile() && /\.(exe|dll)$/i.test(entry.name)) fs.copyFileSync(path.join(config.serverRoot, entry.name), path.join(this.serverPath, entry.name))
    }
    if (config.serverExecutable) fs.copyFileSync(config.serverExecutable, this.serverExecutablePath)
    copyDirectory(path.join(config.serverResourceRoot || config.serverRoot, config.serverRecipesDirectory || 'res/recipes'), path.join(this.serverPath, 'res/recipes'))
    fs.mkdirSync(path.join(this.serverPath, 'res/video'), { recursive: true })
    // 真实素材只读；只复制一条，减少初始化耗时、流量与磁盘使用。
    this.fixture = path.join(config.serverResourceRoot || config.serverRoot, config.serverVideoDirectory || 'res/video', 'cook_000001.mp4')
    fs.copyFileSync(this.fixture, path.join(this.serverPath, 'res/video/cook_000001.mp4'))
    this.fixtureLastWriteTime = fs.statSync(path.join(this.serverPath, 'res/video/cook_000001.mp4')).mtimeMs
    fs.mkdirSync(path.join(this.root, 'mysql/data'), { recursive: true })
    fs.mkdirSync(path.join(this.root, 'mysql/tmp'), { recursive: true })
    fs.mkdirSync(path.join(this.root, 'redis'), { recursive: true })
    const ini = `[Server]\r\nPublicIp=127.0.0.1\r\nListenHost=127.0.0.1\r\nPort=${config.serverPort}\r\nWorkerThreads=8\r\nMaxRequestBytes=1048576\r\n[Storage]\r\nBackend=redis_mysql\r\nRecipeResourceDirectoryName=res/recipes\r\nVideoStorageDirectoryName=data/videos/files\r\n[Dependencies]\r\nAutoStart=false\r\n[MySQL]\r\nHost=127.0.0.1\r\nPort=${config.mysqlPort}\r\nUser=root\r\nPassword=\r\nDatabase=cook_wx_test\r\nAutoStart=false\r\n[Redis]\r\nHost=127.0.0.1\r\nPort=${config.redisPort}\r\nDbIndex=0\r\nKeyPrefix=cook_wx_test\r\nAutoStart=false\r\n[Account]\r\nDefaultCoins=100\r\nDefaultYuanbao=100\r\nAdRequiredSeconds=3\r\nCheckinRewardCoins=5\r\nAdRewardCoins=30\r\n[Voice]\r\nTtsProvider=${config.voiceProvider}\r\n[Log]\r\nPath=${this.reporter.directory.replace(/\\/g, '/')}\r\nOutputConsole=false\r\nArchiveOldLog=false\r\n`
    fs.writeFileSync(path.join(this.serverPath, 'CookServer.ini'), ini.replace('[Storage]\r\n', '[Storage]\r\nResourceRoot=' + this.resourcePath.replace(/\\/g, '/') + '\r\n'))
    this.reporter.report.environment = {
      ...this.reporter.report.environment, projectPath: this.projectPath, sourcePath: config.sourceRoot,
      serverPath: this.serverExecutablePath, serverSha256: hashFile(this.serverExecutablePath),
      configSha256: hashFile(path.join(this.serverPath, 'CookServer.ini')), sourceConfigSha256: hashFile(path.join(config.sourceRoot, 'utils/config.js')),
      cliPath: config.cliPath, isolation: { host: '127.0.0.1', serverPort: config.serverPort, mysqlPort: config.mysqlPort, redisPort: config.redisPort },
      overrides: { voiceProvider: config.voiceProvider, adRequiredSeconds: 3, defaultCoins: 100 },
      fixture: { path: this.fixture, sha256: hashFile(this.fixture), bytes: fs.statSync(this.fixture).size }
    }
    this.reporter.save()
    this.reporter.event(`已创建隔离副本 ${this.projectPath}`)
  }

  spawn(name, executable, args, input) {
    if (!fs.existsSync(executable)) throw new Error(`缺少 ${name}：${executable}`)
    const log = fs.openSync(path.join(this.reporter.directory, name + '.log'), 'a')
    const child = spawn(executable, args, { cwd: this.root, windowsHide: true, stdio: ['pipe', log, log] })
    fs.closeSync(log)
    child.spawnError = null
    child.on('error', error => { child.spawnError = error })
    if (input != null) child.stdin.end(input)
    else child.stdin.end()
    this.children.push({ name, child })
    this.reporter.event(`启动 ${name} pid=${child.pid}`)
    return child
  }

  async completed(child, timeoutMs) {
    await until(() => child.spawnError || child.exitCode != null || child.signalCode != null, '子进程实际退出', timeoutMs, this.signal)
    if (child.spawnError) throw child.spawnError
    if (child.exitCode !== 0) throw new Error(`子进程退出 ${child.exitCode}，查看本批日志`)
  }

  async startBackend() {
    const { config } = this
    for (const port of [config.serverPort, config.mysqlPort, config.redisPort]) await portFree(port)
    const mysqld = path.join(config.mysqlHome, 'bin/mysqld.exe')
    const dataPath = path.join(this.root, 'mysql/data')
    const mysqlArgs = ['--no-defaults', `--basedir=${config.mysqlHome}`, `--datadir=${dataPath}`, '--innodb-buffer-pool-size=32M', `--tmpdir=${path.join(this.root, 'mysql/tmp')}`, '--console']
    // 从官方系统建表脚本生成全新库，不读取或复制安装目录 data 中的业务数据。
    const tables = fs.readFileSync(path.join(config.mysqlHome, 'share/mysql_system_tables.sql'), 'utf8')
    const users = fs.readFileSync(path.join(config.mysqlHome, 'share/mysql_system_tables_data.sql'), 'utf8')
    const bootstrap = 'CREATE DATABASE mysql;\nUSE mysql;\n' + tables + '\n' + users + '\n'
    await this.completed(this.spawn('mysql-bootstrap', mysqld, [...mysqlArgs, '--bootstrap'], bootstrap), config.startupTimeoutMs)
    const mysql = this.spawn('mysql', mysqld, [...mysqlArgs, '--bind-address=127.0.0.1', `--port=${config.mysqlPort}`, `--pid-file=${path.join(this.root, 'mysql/mysql.pid')}`])
    await until(async () => { if (mysql.spawnError || mysql.exitCode != null) throw new Error('MySQL 启动退出'); return portReady(config.mysqlPort) }, '隔离 MySQL 端口', config.startupTimeoutMs, this.signal)
    const redis = this.spawn('redis', path.join(config.redisHome, 'redis-server.exe'), ['--bind', '127.0.0.1', '--port', String(config.redisPort), '--dir', path.join(this.root, 'redis'), '--save', '', '--appendonly', 'no'])
    await until(async () => { if (redis.spawnError || redis.exitCode != null) throw new Error('Redis 启动退出'); return portReady(config.redisPort) }, '隔离 Redis 端口', config.startupTimeoutMs, this.signal)
    const server = this.spawn('cook-server', this.serverExecutablePath, [])
    const client = new TestHttpClient(config.serverPort, this.signal)
    await until(async () => {
      if (server.spawnError || server.exitCode != null) throw new Error('CookServer 启动退出')
      try { const result = await client.request('/api/health', { timeoutMs: 1000 }); return result.status === 200 && result.json && result.json.ok === true } catch (_) { return false }
    }, '隔离 CookServer 健康检查', config.startupTimeoutMs, this.signal)
    this.reporter.event('隔离 CookServer / MySQL / Redis 已就绪')
  }

  async runServerTests(stress, signal) {
    const directory = path.join(this.root, stress ? 'server-test-stress' : 'server-test-instant')
    fs.mkdirSync(directory, { recursive: true })
    const executable = path.join(directory, 'CookServerTest1.0.exe')
    fs.copyFileSync(this.config.serverTestExecutable || path.join(this.config.serverTestRoot, 'CookServerTest1.0.exe'), executable)
    // C++ Test直接只读本批外部资源，不再为旧目录关系复制整份视频。
    const args = [`--host=127.0.0.1`, `--port=${this.config.serverPort}`, '--timeoutMs=15000', `--resourceRoot=${this.resourcePath}`, `--userPrefix=cw_${stress ? 's' : 'i'}_${this.reporter.runId.replace(/[^a-zA-Z0-9]/g, '').slice(-24)}`]
    if (this.config.voiceProvider === 'disabled') args.push('--allowDisabledVoice')
    if (stress) args.push(`--stressThreads=${this.config.serverStressThreads}`, `--stressLoops=${this.config.serverStressLoops}`)
    else args.push('--noStress')
    const child = this.spawn(stress ? 'server-test-stress' : 'server-test', executable, args)
    const cancel = () => child.kill()
    signal.addEventListener('abort', cancel, { once: true })
    const reportPath = path.join(directory, 'CookServerTestReport.txt')
    const target = path.join(this.reporter.directory, stress ? 'CookServerTest-stress.txt' : 'CookServerTest.txt')
    try { await this.completed(child, 240000) } finally {
      signal.removeEventListener('abort', cancel)
      if (fs.existsSync(reportPath)) fs.copyFileSync(reportPath, target)
    }
    if (!fs.existsSync(reportPath)) throw new Error('CookServerTest 未生成报告')
    this.reporter.report.environment.serverTestSha256 = hashFile(executable)
    this.reporter.save()
    return { reportPath: target, executable, args }
  }

  async stop() {
    if (this.stopped) return
    this.stopped = true
    // 只处置通过本对象 spawn 创建、仍持有句柄的子进程，不按进程名查杀。
    for (const { name, child } of [...this.children].reverse()) {
      if (child.exitCode == null && child.signalCode == null && child.pid) {
        this.reporter.event(`停止本批 ${name} pid=${child.pid}（Windows 子进程终止，不作为优雅退出验证）`)
        child.kill()
        await until(() => child.exitCode != null || child.signalCode != null, name + ' 收尾', 10000).catch(error => this.reporter.event(error.message))
      }
    }
    if (this.lockHandle != null) {
      fs.closeSync(this.lockHandle)
      this.lockHandle = null
      const ownership = JSON.parse(fs.readFileSync(this.lockPath, 'utf8'))
      if (ownership.runId === this.reporter.runId && ownership.pid === process.pid) fs.unlinkSync(this.lockPath)
    }
  }
}

async function waitForPort(port, child, timeoutMs, signal) {
  await until(async () => {
    if (child.spawnError || child.exitCode != null) throw new Error('本批子进程启动退出')
    return portReady(port)
  }, '本批子进程端口', timeoutMs, signal)
}

module.exports = { TestEnvironment, portFree, copyDirectory, waitForPort }