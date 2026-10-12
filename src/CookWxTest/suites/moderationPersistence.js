const assert = require('node:assert/strict')
const { once } = require('node:events')
const { waitForPort } = require('../lib/environment')
const storage = require('../lib/isolatedStorage')
async function run(context, admin) {
  const { http, environment: env, config } = context
  const account = await http.json('/api/auth/register', { body: { account: 'fail_' + context.reporter.runId.replace(/[^a-z0-9]/gi, '').slice(-30), password: 'StorageReview123!', nickname: '回滚测试' } })
  const token = account.token
  const original = (await http.json('/api/recipes/custom/publish', { token, body: { recipe: { id: 'failure', schemaVersion: '1.0', title: '回滚菜', category: '小荤', priceType: 'coin', priceAmount: 7,
    tasks: [{ id: 'cut', title: '切小块', active: true, durationSeconds: 10 }] } } })).recipe
  const mysql = [...env.children].reverse().find(item => item.name === 'mysql' && item.child.exitCode == null && item.child.signalCode == null)
  assert.ok(mysql && mysql.child.pid, '故障只允许本批数据库句柄')
  const exited = once(mysql.child, 'exit'); mysql.child.kill(); await exited
  const save = await http.request('/api/recipes/creation/save', { token, timeoutMs: 60000, body: { recipeId: original.id, currentRevisionId: original.currentRevisionId, editorVersion: 2, steps: original.tasks, priceType: 'coin', priceAmount: 8 } })
  assert.equal(save.json.ok, false)
  const decision = await http.request('/api/recipes/reviews/decide', { token: admin.token, timeoutMs: 60000, body: { reviewId: original.reviewId, decision: 'approved' } })
  assert.equal(decision.json.ok, false); assert.equal(decision.json.code, 'storage_failed')
  const restarted = env.spawn('mysql', mysql.child.spawnfile, mysql.child.spawnargs.slice(1))
  await waitForPort(config.mysqlPort, restarted, config.startupTimeoutMs, context.signal)
  const actual = (await http.json('/api/recipes/creation/detail?recipeId=' + original.id, { token })).recipe
  assert.equal(actual.currentRevisionId, original.currentRevisionId); assert.equal(actual.priceAmount, 7)
  assert.equal(actual.changesRemaining, 3); assert.equal(actual.reviewState, 'pending')
  await http.json('/api/recipes/reviews/decide', { token: admin.token, body: { reviewId: original.reviewId, decision: 'approved' } })
  await storage.stopOwnedBackend(context); await storage.startOwnedBackend(context)
  const restored = await http.json('/api/recipes/reviews/detail?recipeId=' + original.id, { token })
  assert.equal(restored.history.length, 1, '故障不能持久化未提交的新版本')
  assert.equal(restored.review.state, 'approved')
  assert.equal(restored.recipe.priceAmount, 7)
  return { mysqlFailureNotReportedAsSuccess: true, quotaAndVersionRolledBack: true, approvalRolledBack: true, reconnectAndColdRestorePassed: true }
}
module.exports = { run }