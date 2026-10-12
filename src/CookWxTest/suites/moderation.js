const assert = require('node:assert/strict')
const fs = require('node:fs')
const path = require('node:path')
const storage = require('../lib/isolatedStorage')
function source(id, type = 'coin', amount = 7) {
  return { schemaVersion: '1.0', id, title: '审核测试菜', category: '小荤', subtitle: '真实审核测试', priceType: type, priceAmount: type === 'free' ? 0 : amount,
    ingredients: [{ name: '食材', amount: '100克' }], tasks: [{ id: 'cut', title: '切成小块', detail: '切成适合入口的小块', durationSeconds: 10, active: true }] }
}
async function publish(http, token, recipe) { return (await http.json('/api/recipes/custom/publish', { token, body: { recipe } })).recipe }
async function decide(http, token, recipe, decision = 'approved', reason = '') { return http.request('/api/recipes/reviews/decide', { token, body: { reviewId: recipe.reviewId || recipe.currentRevisionId, decision, reason } }) }
async function change(http, token, recipe, amount) { return (await http.json('/api/recipes/creation/save', { token, body: { recipeId: recipe.id, currentRevisionId: recipe.currentRevisionId, editorVersion: 2, priceType: recipe.priceType, priceAmount: amount, steps: recipe.tasks } })).recipe }
async function stopFailedRedis(env) {
  const redis = [...env.children].reverse().find(item => item.name.startsWith('redis') && item.child.exitCode == null && item.child.signalCode == null)
  assert.ok(redis)
  const exited = new Promise(resolve => redis.child.once('exit', resolve))
  redis.child.kill(); await exited
}
module.exports = {
  53: async context => {
    const { environment: env, http } = context
    assert.ok(fs.existsSync(path.join(env.resourcePath, 'recipes/cook_000001.json')))
    assert.ok(!fs.existsSync(path.join(env.resourcePath, 'res')), '资源根不能新建冗余res层')
    assert.ok(!fs.existsSync(path.join(env.serverPath, 'res')), '启动后程序目录不能继续保存已迁资源')
    const fixture = path.join(env.resourcePath, 'video/cook_000001.mp4')
    assert.equal(fs.statSync(fixture).mtimeMs, env.fixtureLastWriteTime, '迁移必须保留媒体修改时间，避免缓存版本无故变化')
    const login = await http.json('/api/auth/login', { body: { account: 'qmzdc', password: 'yangnan' } })
    const feed = await http.json('/api/feed?count=1', { token: login.token })
    const id = String(feed.feed[0].videoId || feed.feed[0].id).replace(/^feed_/, '')
    const before = await http.request('/api/videos/file?videoId=' + encodeURIComponent(id), { token: login.token })
    assert.equal(before.status, 200)
    // 资源表在首次刷视频时建立；随后经过真实登录持久化，不伪造缺失的视频记录。
    await http.json('/api/auth/login', { body: { account: 'qmzdc', password: 'yangnan' } })
    await storage.stopOwnedBackend(context)
    // 模拟前一版外部根中已有的res，不能只覆盖首次从程序目录迁移的情况。
    fs.mkdirSync(path.join(env.resourcePath, 'res'))
    for (const directory of ['recipes', 'video']) fs.renameSync(path.join(env.resourcePath, directory), path.join(env.resourcePath, 'res', directory))
    const legacyReferences = ['res/video/cook_000001.mp4', path.join(env.resourcePath, 'res/video/cook_000001.mp4'), path.join(env.serverPath, 'res/video/cook_000001.mp4')]
    for (let index = 0; index < legacyReferences.length; index += 1) {
      const snapshot = await storage.snapshot(context)
      let found = false
      const updated = snapshot.split('\n').map(line => {
        const fields = line.split('\t')
        if (fields[0] === 'VIDEO' && fields[1] === id) { fields[4] = legacyReferences[index]; found = true; return fields.join('\t') }
        return line
      }).join('\n')
      assert.ok(found, '必须更新实际视频快照中的历史路径')
      await storage.replaceSnapshot(context, updated)
      await storage.startOwnedBackend(context)
      const restored = await http.request('/api/videos/file?videoId=' + encodeURIComponent(id), { token: login.token })
      assert.equal(restored.status, 200); assert.deepEqual(restored.data, before.data)
      const restoredFeed = await http.json('/api/feed?count=1', { token: login.token })
      assert.equal(restoredFeed.feed[0].videoUrl, feed.feed[0].videoUrl, '仅迁移目录不能改变视频缓存URL版本')
      assert.equal(fs.statSync(fixture).mtimeMs, env.fixtureLastWriteTime)
      assert.ok(!fs.existsSync(path.join(env.resourcePath, 'res')))
      await storage.stopOwnedBackend(context)
    }
    // 同名不同内容必须拒绝启动并保留源和目标；恢复后同一批可继续。
    const conflict = path.join(env.resourcePath, 'res/recipes/cook_000001.json')
    const published = fs.readFileSync(path.join(env.resourcePath, 'recipes/cook_000001.json'))
    fs.mkdirSync(path.dirname(conflict), { recursive: true })
    fs.writeFileSync(conflict, 'legacy conflicting recipe')
    const snapshotBeforeConflict = await storage.snapshot(context)
    await storage.startOwnedBackend(context, true)
    assert.equal(fs.readFileSync(conflict, 'utf8'), 'legacy conflicting recipe')
    assert.deepEqual(fs.readFileSync(path.join(env.resourcePath, 'recipes/cook_000001.json')), published)
    assert.equal(await storage.snapshot(context), snapshotBeforeConflict, '失败迁移不能写数据库')
    await stopFailedRedis(env)
    fs.unlinkSync(conflict); fs.rmdirSync(path.dirname(conflict)); fs.rmdirSync(path.join(env.resourcePath, 'res'))
    const linked = path.join(env.root, 'linked-legacy')
    fs.mkdirSync(path.join(linked, 'recipes'), { recursive: true })
    const sentinel = path.join(linked, 'recipes/keep.json')
    fs.writeFileSync(sentinel, 'linked data must remain untouched')
    fs.symlinkSync(linked, path.join(env.resourcePath, 'res'), process.platform === 'win32' ? 'junction' : 'dir')
    await storage.startOwnedBackend(context, true)
    assert.equal(fs.readFileSync(sentinel, 'utf8'), 'linked data must remain untouched')
    assert.ok(fs.lstatSync(path.join(env.resourcePath, 'res')).isSymbolicLink())
    assert.equal(await storage.snapshot(context), snapshotBeforeConflict)
    await stopFailedRedis(env)
    fs.unlinkSync(path.join(env.resourcePath, 'res'))
    await storage.startOwnedBackend(context)
    await storage.stopOwnedBackend(context)
    const oldProgram = env.serverPath
    const newProgram = path.join(env.root, 'server-replaced')
    fs.mkdirSync(newProgram)
    for (const entry of fs.readdirSync(oldProgram, { withFileTypes: true })) {
      if (entry.isFile() && /\.(exe|dll|ini)$/i.test(entry.name)) fs.copyFileSync(path.join(oldProgram, entry.name), path.join(newProgram, entry.name))
    }
    env.serverPath = newProgram; env.serverExecutablePath = path.join(newProgram, 'CookServer1.0.exe')
    await storage.startOwnedBackend(context)
    const after = await http.request('/api/videos/file?videoId=' + encodeURIComponent(id), { token: login.token })
    assert.equal(after.status, 200); assert.deepEqual(after.data, before.data)
    assert.ok(!fs.existsSync(path.join(newProgram, 'res')))
    return { externalResources: true, flatResources: true, legacyReferences: legacyReferences.length, conflictPreservesFilesAndDatabase: true, linkedResourcesRejected: true, programDirectoryReplaced: true, mysqlColdMediaReadable: true, bytes: after.data.length }
  },
  54: async context => {
    const { http } = context
    const admin = await http.json('/api/auth/login', { body: { account: 'qmzdc', password: 'yangnan' } })
    const second = await require('../lib/administratorFixture').ensure(context)
    assert.equal(second.account.role, 'admin')
    const account = await http.json('/api/auth/register', { body: { account: 'review_' + context.reporter.runId.replace(/[^a-z0-9]/gi, '').slice(-30), password: 'ReviewUser123!', nickname: '投稿测试' } })
    const token = account.token
    const free = await publish(http, token, source('free', 'free'))
    assert.equal(free.status, 'published'); assert.equal(free.reviewState, 'published')
    let recipe = await publish(http, token, source('coin'))
    assert.match(recipe.id, /^recipe_[a-f0-9]{32}$/i)
    assert.equal(recipe.reviewState, 'pending'); assert.equal(recipe.reviewNumber, 1)
    const privateList = await http.request('/api/recipes/reviews/list?state=pending', { token })
    assert.equal(privateList.json.ok, false)
    const catalog = await http.json('/api/recipes?scope=catalog', { token: second.token })
    assert.ok(!catalog.recipes.some(row => row.id === recipe.id), '付费首次投稿不能提前公开')
    const forbidden = await decide(http, token, recipe); assert.equal(forbidden.json.ok, false)
    const race = await Promise.all(Array.from({ length: 24 }, (_, index) => decide(http, index % 2 ? second.token : admin.token, recipe, index % 2 ? 'rejected' : 'approved', '并发审核')))
    assert.equal(race.filter(result => result.json.ok).length, 1, '多个审核员相反决定必须只有一次成功')
    assert.ok(race.filter(result => !result.json.ok).every(result => result.json.code === 'already_processed'))
    const winning = race.find(result => result.json.ok).json.review.state
    assert.ok(['approved', 'rejected'].includes(winning))
    const original = recipe
    recipe = await change(http, token, recipe, 8)
    assert.equal(recipe.reviewNumber, 2)
    const approved = await decide(http, admin.token, recipe); assert.equal(approved.json.ok, true)
    recipe = await change(http, token, recipe, 9)
    const superseded = recipe
    recipe = await change(http, token, recipe, 10)
    const outdated = await decide(http, second.token, superseded); assert.equal(outdated.json.ok, false); assert.ok(['version_replaced', 'already_processed'].includes(outdated.json.code))
    const quota = await http.request('/api/recipes/creation/save', { token, body: { recipeId: recipe.id, currentRevisionId: recipe.currentRevisionId, priceType: 'coin', priceAmount: 11, editorVersion: 2, steps: recipe.tasks } })
    assert.equal(quota.json.ok, false)
    assert.ok(quota.json.message.includes('修改'), '真实API错误文案须保持可读中文')
    const noChange = await change(http, token, recipe, 10); assert.equal(noChange.currentRevisionId, recipe.currentRevisionId)
    const detail = await http.json('/api/recipes/reviews/detail?recipeId=' + recipe.id, { token: second.token })
    assert.equal(detail.previousApprovedRecipe.priceAmount, 8)
    assert.ok(detail.changes.fields.some(row => row.field === 'priceAmount' && row.value === 10))
    assert.equal(detail.history.length, 4)
    const old = await http.json('/api/recipes/reviews/detail?recipeId=' + recipe.id + '&reviewId=' + original.reviewId, { token })
    assert.equal(old.recipe.priceAmount, 7)
    const rejected = await decide(http, second.token, recipe, 'rejected', '请补充说明'); assert.equal(rejected.json.ok, true)
    const author = await http.json('/api/recipes/creation/detail?recipeId=' + recipe.id, { token })
    assert.equal(author.recipe.reviewReason, '请补充说明'); assert.equal(author.recipe.reviewState, 'rejected')
    const publicVersion = await http.json('/api/recipes?scope=catalog', { token: second.token })
    assert.equal(publicVersion.recipes.find(row => row.id === recipe.id).priceAmount, 8)
    const ownPublic = await http.json('/api/recipes?scope=public_catalog', { token })
    assert.equal(ownPublic.recipes.find(row => row.id === recipe.id).priceAmount, 8, '本人菜单与个性化也必须采用公开通过版')
    for (let index = 0; index < 3; ++index) await publish(http, token, source('limit' + index, 'free'))
    const sixth = await http.request('/api/recipes/custom/publish', { token, body: { recipe: source('sixth', 'free') } }); assert.equal(sixth.json.ok, false)
    const persisted = await storage.snapshot(context)
    assert.match(persisted, /REVIEWBASE\t/); assert.match(persisted, /SUBMISSIONUSAGE\t/)
    await storage.stopOwnedBackend(context); await storage.startOwnedBackend(context)
    const restored = await http.json('/api/recipes/reviews/detail?recipeId=' + recipe.id, { token })
    assert.equal(restored.recipe.priceAmount, 10); assert.equal(restored.review.reason, '请补充说明')
    assert.equal(restored.history.length, 4)
    const unchanged = await change(http, token, author.recipe, 10); assert.equal(unchanged.currentRevisionId, recipe.currentRevisionId)
    const media = await require('./moderationMedia').run(context, admin)
    const persistence = await require('./moderationPersistence').run(context, admin)
    await storage.stopOwnedBackend(context)
    const nextDay = (await storage.snapshot(context)).split('\n').map(line => {
      const parts = line.split('\t')
      if (parts[0] === 'SUBMISSIONUSAGE' && parts[1].startsWith(account.account.userId + '|')) {
        const day = Number(parts[1].split('|')[1]); parts[1] = account.account.userId + '|' + (day - 1)
        return parts.join('\t')
      }
      return line
    }).join('\n')
    await storage.replaceSnapshot(context, nextDay); await storage.startOwnedBackend(context)
    const later = await change(http, token, author.recipe, 11)
    assert.equal(later.dailyRecipesRemaining, 4, '修改旧菜也应占用新一天的一道菜')
    assert.equal(later.changesRemaining, 2, '新一天可重新修改，但本次应算一次修改')
    const newSixth = await publish(http, token, source('sixth', 'free'))
    assert.equal(newSixth.dailyRecipesRemaining, 3)
    return { twoAdministratorRaceRequests: race.length, successfulDecisions: 1, historyDeltasRestored: true, rejectedReasonRestored: true, latestPublicPreserved: true, dailyDistinctBoundary: 5, dailyChangeBoundary: 3, duplicateNoCharge: true, media, persistence, previousDayQuotaFixtureResetsWithoutChangingWindowsClock: true }
  }
}