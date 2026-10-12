const assert = require('node:assert/strict')
const storage = require('../lib/isolatedStorage')

module.exports = {
  52: async context => {
    const { http } = context
    const author = await http.json('/api/auth/login', { body: { account: 'qmzdc', password: 'yangnan' } })
    const original = (await http.json('/api/recipes/creation/detail?recipeId=cook_000001', { token: author.token })).recipe
    const first = original.tasks[0], id = first.id
    await http.json('/api/recipes/personalization/save', { token: author.token,
      body: { recipeId: original.id, durationOverrides: [{ taskId: id, durationSeconds: 47 }] } })
    const reread = (await http.json('/api/recipes/creation/detail?recipeId=' + original.id, { token: author.token })).recipe
    assert.equal(reread.tasks[0].durationSeconds, first.durationSeconds, '原始创作不能混入个性化时长')
    const steps = original.tasks.slice(0, -1).map((step, index) => ({ id: step.id, title: index ? step.title : '切成小块',
      detail: index ? step.detail : '冷重启后仍要保存的实际说明', durationSeconds: index ? step.durationSeconds : 23, active: step.active }))
    assert.ok(steps.length >= 2, '冷重启用例需要至少三个原始步骤')
    steps.splice(1, 0, { id: original.id + '.added_cold', title: '拌匀食材', detail: '作者新增步骤也需要持久保存', durationSeconds: 17, active: false,
      safetyLevel: 'normal', backgroundWaitMode: 'free', canLeaveKitchen: true })
    const response = await http.json('/api/recipes/creation/save', { token: author.token, body: {
      recipeId: original.id, currentRevisionId: original.currentRevisionId, priceType: 'yuanbao', priceAmount: 29,
      editorVersion: 2, steps
    } })
    const updated = response.recipe
    assert.equal(updated.reviewState, 'pending', '收费修改须审核，不能立即覆盖公开版')
    await http.json('/api/recipes/reviews/decide', { token: author.token, body: { reviewId: updated.reviewId, decision: 'approved' } })
    const snapshot = await storage.snapshot(context)
    assert.ok(snapshot.includes(updated.currentRevisionId), '作者编辑必须实际落MySQL')
    await storage.stopOwnedBackend(context)
    await storage.startOwnedBackend(context)
    const restored = (await http.json('/api/recipes/creation/detail?recipeId=' + original.id, { token: author.token })).recipe
    assert.equal(restored.currentRevisionId, updated.currentRevisionId)
    assert.equal(restored.tasks[0].title, '切成小块')
    assert.equal(restored.tasks[0].detail, '冷重启后仍要保存的实际说明')
    assert.equal(restored.tasks[0].durationSeconds, 23)
    assert.equal(restored.authorUserId, 'qmzdc')
    assert.equal(restored.systemRecipe, true)
    assert.equal(restored.priceType, 'yuanbao')
    assert.equal(restored.priceAmount, 29)
    assert.equal(restored.priceCoins, 0)
    assert.equal(restored.coverImageId, original.coverImageId)
    assert.deepEqual(restored.tasks, updated.tasks, '新增删除顺序、性质和排程依赖必须在冷重启后完整恢复')
    assert.deepEqual(restored.tasks.map(step => step.id), steps.map(step => step.id))
    assert.equal(restored.tasks[1].active, false)
    assert.equal(restored.tasks[1].title, '拌匀食材')
    assert.ok(!restored.tasks.some(step => step.id === original.tasks[original.tasks.length - 1].id))
    const newcomer = await http.json('/api/auth/register', { body: { account: 'cw_price_' + context.reporter.runId.replace(/[^a-zA-Z0-9]/g, '').slice(0, 32), password: 'PriceCold123!', nickname: '定价新用户' } })
    assert.ok(newcomer.token)
    const newCatalog = await http.json('/api/recipes?scope=catalog', { token: newcomer.token })
    const priced = newCatalog.recipes.find(recipe => recipe.id === original.id)
    assert.ok(priced)
    assert.equal(priced.owned, false, '冷重启后新用户不能凭旧内置免费价格获得使用权')
    assert.equal(priced.inMenu, false, '冷重启后新的收费菜不能自动进入新用户菜单')
    assert.equal(priced.priceType, 'yuanbao')
    assert.equal(priced.priceAmount, 29)
    const catalog = await http.json('/api/recipes?scope=catalog', { token: author.token })
    assert.equal(catalog.recipes.filter(recipe => recipe.id === original.id).length, 1, '覆盖后的内置菜不能重复列出')
    const personal = await http.json('/api/recipes/personalization/detail?recipeId=' + original.id, { token: author.token })
    assert.ok(personal.personalization.durationOverrides.some(step => step.taskId === id && step.durationSeconds === 47), '原始编辑不得丢失个人设置')
    const stale = await http.request('/api/recipes/creation/save', { token: author.token, body: { recipeId: original.id,
      currentRevisionId: original.currentRevisionId, steps: original.tasks.map(step => ({ id: step.id, title: step.title, detail: step.detail, durationSeconds: step.durationSeconds })) } })
    assert.equal(stale.json.ok, false, '冷重启不能让旧版本重新获得写权限')
    return { recipeId: original.id, originalAndPersonalSeparated: true, titleDescriptionDurationRestored: true,
      authorPreserved: true, structureAndModeRestored: true, priceTypeAndAmountRestored: true, newUserNotAutoGranted: true, systemCatalogUnique: true, staleRevisionRejected: true }
  }
}