const assert = require('node:assert/strict')
const { until } = require('./wait')
const storage = require('./isolatedStorage')
// 旧管理员只从本批全新测试数据库的合法普通账号快照构造，不能开放正式管理员注册或伪造API角色。
async function ensure(context) {
  const { http } = context
  const login = await http.request('/api/auth/login', { body: { account: 'xylsxyls', password: 'IdentityAdmin2026!' } })
  if (login.json && login.json.ok) { assert.equal(login.json.account.role, 'admin'); return login.json }
  const blocked = await http.request('/api/auth/register', { body: { account: 'xylsxyls', password: 'IdentityAdmin2026!', nickname: '雪夜' } })
  assert.equal(blocked.json.ok, false, '管理员保留名不能被公开抢注后提权')
  assert.ok(!(await storage.snapshot(context)).includes('\txylsxyls\t'), '既有管理员凭据不一致，拒绝改写')
  const name = 'legacy_admin_' + context.reporter.runId.replace(/[^a-z0-9]/gi, '').slice(-25)
  await http.json('/api/auth/register', { body: { account: name, password: 'IdentityAdmin2026!', nickname: '雪夜' } })
  await until(async () => (await storage.snapshot(context)).includes('\t' + name + '\t'), '测试旧管理员账号实际落盘', 10000, context.signal)
  await storage.stopOwnedBackend(context)
  const original = await storage.snapshot(context)
  let changes = 0
  const legacy = original.split('\n').map(line => {
    const parts = line.split('\t')
    if (parts[0] === 'USER' && parts[6] === name) { parts[6] = 'xylsxyls'; ++changes; return parts.join('\t') }
    return line
  }).join('\n')
  assert.equal(changes, 1)
  await storage.replaceSnapshot(context, legacy)
  await storage.startOwnedBackend(context)
  const actual = await http.json('/api/auth/login', { body: { account: 'xylsxyls', password: 'IdentityAdmin2026!' } })
  assert.equal(actual.account.role, 'admin')
  return actual
}
module.exports = { ensure }