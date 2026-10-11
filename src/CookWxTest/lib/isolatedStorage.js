const fs = require('node:fs')
const path = require('node:path')
const { once } = require('node:events')
const { until } = require('./wait')
const { waitForPort } = require('./environment')
let serial = 0

function guard(context) {
  const { environment: env, config } = context
  if (!env.root.startsWith(path.join(config.projectRoot, 'temp') + path.sep) ||
    !env.children.some(item => item.name === 'mysql' && item.child.exitCode == null && item.child.signalCode == null)) {
    throw new Error('数据库操作仅允许本批创建的隔离MySQL')
  }
}

async function sql(context, query) {
  guard(context)
  const { environment: env, config, signal } = context
  if (signal.aborted) throw new Error('测试已取消')
  const name = 'identity-sql-' + (++serial)
  const child = env.spawn(name, path.join(config.mysqlHome, 'bin/mysql.exe'), [
    '--no-defaults', '--host=127.0.0.1', '--protocol=TCP', '--port=' + config.mysqlPort,
    '--user=root', '--batch', '--skip-column-names', 'cook_wx_test'
  ], query + ';\n')
  await env.completed(child, 10000)
  return fs.readFileSync(path.join(context.reporter.directory, name + '.log'), 'utf8').trim()
}

async function snapshot(context) {
  const hex = await sql(context, "SELECT HEX(snapshotValue) FROM cook_state_snapshot WHERE snapshotKey='account_store'")
  if (!/^[0-9a-f]+$/i.test(hex)) throw new Error('没有有效隔离快照')
  return Buffer.from(hex, 'hex').toString('utf8')
}

async function replaceSnapshot(context, value) {
  const hex = Buffer.from(value, 'utf8').toString('hex')
  await sql(context, "UPDATE cook_state_snapshot SET snapshotValue=UNHEX('" + hex + "'),updateVersion=updateVersion+1 WHERE snapshotKey='account_store'")
}

async function stopOwnedBackend(context) {
  guard(context)
  const env = context.environment
  for (const prefix of ['cook-server', 'redis']) {
    const owned = [...env.children].reverse().find(item => item.name.startsWith(prefix) && item.child.exitCode == null && item.child.signalCode == null)
    if (!owned || !owned.child.pid) throw new Error('没有本批服务器/Redis进程句柄')
    const exited = once(owned.child, 'exit')
    owned.child.kill()
    await exited
  }
}

async function startOwnedBackend(context, expectedFailure = false) {
  guard(context)
  const { environment: env, config, signal, http } = context
  const directory = path.join(env.root, 'redis')
  if (fs.readdirSync(directory).length) throw new Error('测试Redis存在持久化文件，拒绝冒充冷启动')
  const redis = env.spawn('redis-identity-' + (++serial), path.join(config.redisHome, 'redis-server.exe'), [
    '--bind', '127.0.0.1', '--port', String(config.redisPort), '--dir', directory, '--save', '', '--appendonly', 'no'
  ])
  await waitForPort(config.redisPort, redis, config.startupTimeoutMs, signal)
  const server = env.spawn('cook-server-identity-' + (++serial), env.serverExecutablePath, [])
  if (expectedFailure) {
    await until(() => server.exitCode != null, '冲突账号使服务拒绝启动', 15000, signal)
    if (server.exitCode === 0) throw new Error('冲突账号未阻止启动')
    let accessible = false
    try { accessible = (await http.request('/api/health', { timeoutMs: 1000 })).status === 200 } catch (_) {}
    if (accessible) throw new Error('冲突后HTTP仍开放')
  } else {
    await until(async () => {
      if (server.spawnError || server.exitCode != null) throw new Error('迁移服务器启动失败')
      try { return (await http.json('/api/health')).ok } catch (_) { return false }
    }, '迁移服务实际健康', config.startupTimeoutMs, signal)
  }
}

module.exports = { snapshot, replaceSnapshot, stopOwnedBackend, startOwnedBackend }