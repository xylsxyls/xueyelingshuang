const path = require('node:path')
const { execFile } = require('node:child_process')
const fs = require('node:fs')
const { once } = require('node:events')
const { until, assert } = require('./helpers')

function command(executable, args, signal) {
  return new Promise((resolve, reject) => execFile(executable, args, { windowsHide: true, timeout: 10000, maxBuffer: 16 * 1024 * 1024, encoding: 'utf8', signal }, (error, stdout) => error ? reject(error) : resolve(stdout)))
}

module.exports = {
  44: async context => {
    const { environment: env, config, http, signal } = context
    const server = env.children.find(item => item.name === 'cook-server' && item.child.exitCode == null && item.child.signalCode == null)
    const redis = env.children.find(item => item.name === 'redis' && item.child.exitCode == null && item.child.signalCode == null)
    assert.ok(server && redis && server.child.pid && redis.child.pid, '只允许重启本批持有句柄的子进程')
    assert.ok(env.root.startsWith(path.join(config.projectRoot, 'temp') + path.sep), '只允许隔离测试目录')
    const name = 'persist_' + Date.now().toString(36)
    const register = suffix => http.json('/api/auth/register', { body: { account: name + suffix, password: 'CookWxPersist2026!', nickname: 'Persistence ' + suffix } })
    const a = await register('_a'), b = await register('_b')
    const introduction = '开心做大厨，认真做好每一道菜'
    await http.json('/api/users/profile/update', { token: a.token, body: { introduction } })
    const order = ['wash_rice', 'persist_insert', 'cook_rice', 'fluff_rice']
    await http.json('/api/recipes/personalization/save', { token: a.token, body: { recipeId: 'cook_000002', taskOrder: order, addedTasks: [{ id: 'persist_insert', title: '检查水量', durationSeconds: 7, active: true }] } })
    const recipeId = name + '_recipe'
    await http.json('/api/recipes/custom/publish', { token: b.token, body: { recipe: { schemaVersion: '1.0', id: recipeId, title: 'Persistence recipe', category: '蔬菜', priceType: 'free', tasks: [{ id: 'cut', title: '切菜', durationSeconds: 10, active: true }] } } })
    await http.json('/api/recipes/join', { token: a.token, body: { recipeId } })
    await http.json('/api/recipes/menu/remove', { token: a.token, body: { recipeId } })
    await http.json('/api/recipes/join', { token: a.token, body: { recipeId: 'cook_000006' } })
    await http.json('/api/recipes/detail?recipeId=cook_000003', { token: a.token })
    await http.json('/api/follows/toggle', { token: a.token, body: { targetUserId: b.account.userId } })
    await http.json('/api/follows/toggle', { token: b.token, body: { targetUserId: a.account.userId } })
    const text = name + '_read_message'
    const sent = await http.json('/api/messages/send', { token: b.token, body: { targetUserId: a.account.userId, text } })
    await http.json('/api/messages/read', { token: a.token, body: { targetUserId: b.account.userId } })
    const before = (await http.json('/api/recipes', { token: a.token })).recipes
    const beforeAccount = (await http.json('/api/account', { token: a.token })).account
    const mysql = path.join(config.mysqlHome, 'bin/mysql.exe')
    const query = ['--no-defaults', '--host=127.0.0.1', '--protocol=TCP', '--port=' + config.mysqlPort, '--user=root', '--batch', '--skip-column-names', 'cook_wx_test', '--execute=SELECT HEX(snapshotValue) FROM cook_state_snapshot WHERE snapshotKey=\'account_store\'']
    await until(async () => {
      const output = (await command(mysql, query, signal)).trim()
      const snapshot = Buffer.from(output, 'hex').toString('utf8')
      const menu = snapshot.split('\n').find(line => line.startsWith('MENU\t' + a.account.userId + '\t'))
      const message = snapshot.split('\n').find(line => line.startsWith('MESSAGE\t' + sent.messageItem.id + '\t'))
      const profile = snapshot.split('\n').find(line => line.startsWith('USER\t' + a.account.userId + '\t'))
      const personal = snapshot.split('\n').find(line => line.startsWith('PERS\t' + a.account.userId + '\t'))
      return menu && menu.split('\t')[3].split(',').includes(recipeId) && message && /\t(?:true|1)$/.test(message) && profile && profile.includes(introduction) && personal && personal.includes('persist_insert')
    }, '异步快照实际落入本批 MySQL', 20000, signal)
    const exited = once(server.child, 'exit')
    server.child.kill()
    await exited
    // 本批Redis明确关闭RDB/AOF；重启本对象持有的进程以清空内存，避免碰任何既有实例。
    const redisExit = once(redis.child, 'exit')
    redis.child.kill()
    await redisExit
    const redisDirectory = path.join(env.root, 'redis')
    assert.deepEqual(fs.readdirSync(redisDirectory), [], '本批Redis目录没有旧持久化数据')
    const freshRedis = env.spawn('redis-restarted', path.join(config.redisHome, 'redis-server.exe'), ['--bind', '127.0.0.1', '--port', String(config.redisPort), '--dir', redisDirectory, '--save', '', '--appendonly', 'no'])
    assert.notEqual(freshRedis.pid, redis.child.pid)
    await require('../lib/environment').waitForPort(config.redisPort, freshRedis, config.startupTimeoutMs, signal)
    const restarted = env.spawn('cook-server-restarted', env.serverExecutablePath, [])
    await until(async () => {
      if (restarted.spawnError || restarted.exitCode != null) throw new Error('重启服务器退出')
      try { return (await http.json('/api/health')).ok } catch (_) { return false }
    }, '重启服务实际可用', config.startupTimeoutMs, signal)
    const logged = await http.json('/api/auth/login', { body: { account: name + '_a', password: 'CookWxPersist2026!' } })
    const after = (await http.json('/api/recipes', { token: logged.token })).recipes
    assert.deepEqual(after.map(item => item.id), before.map(item => item.id), '加入顺序、查看顺序、移除状态应保留')
    assert.ok(!after.some(item => item.id === recipeId), '已移除的免费用户菜不可复活')
    assert.ok(after.some(item => item.id === 'cook_000006' && !item.owned && item.inMenu), '未购买的已加入付费菜保留且不获制作权限')
    const afterAccount = (await http.json('/api/account', { token: logged.token })).account
    assert.equal(afterAccount.coins, beforeAccount.coins)
    assert.equal(afterAccount.yuanbao, beforeAccount.yuanbao)
    const ownProfile = await http.json('/api/users/profile?userId=' + a.account.userId, { token: logged.token })
    assert.equal(ownProfile.profile.introduction, introduction)
    assert.equal((await http.json('/api/users/profile?userId=' + b.account.userId, { token: logged.token })).profile.introduction, '')
    const restored = await http.json('/api/recipes/personalization/detail?recipeId=cook_000002', { token: logged.token })
    assert.deepEqual(restored.personalization.taskOrder, order)
    const timeline = (await http.json('/api/plan/preview', { token: logged.token, body: { recipeIds: ['cook_000002'], summaryOnly: false } })).timeline
    const inserted = timeline.find(item => item.id === 'cook_000002.persist_insert')
    assert.ok(inserted && inserted.startSeconds >= timeline.find(item => item.id === 'cook_000002.wash_rice').endSeconds)
    assert.ok(timeline.find(item => item.id === 'cook_000002.cook_rice').startSeconds >= inserted.endSeconds)
    const profile = await http.json('/api/users/profile?userId=' + b.account.userId, { token: logged.token })
    assert.equal(profile.profile.friend, true)
    const conversation = await http.json('/api/messages/conversation?userId=' + b.account.userId, { token: logged.token })
    assert.ok(conversation.messages.some(item => item.id === sent.messageItem.id && item.read && item.text === text))
    return { actualProcessRestart: true, emptyRedisRecoveryFromMySql: true, menuCount: after.length, preservedReadMessage: true, mutualFollow: true, introductionAndTaskOrderRestored: true }
  }
}