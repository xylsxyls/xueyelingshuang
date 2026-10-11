const assert = require('node:assert/strict')
const { until } = require('../lib/wait')
const storage = require('../lib/isolatedStorage')

module.exports = {
  51: async context => {
    const { http } = context
    const system = await http.json('/api/auth/login', { body: { account: 'qmzdc', password: 'yangnan' } })
    const user = await http.json('/api/auth/register', { body: { account: 'identity_' + Date.now().toString(36), password: 'IdentityTest2026!', nickname: '迁移测试' } })
    const existing = await http.request('/api/auth/register', { body: { account: 'xylsxyls', password: 'IdentityAdmin2026!', nickname: '雪夜' } })
    assert.equal(existing.json.ok, true, '角色迁移必须从新建隔离库开始')
    const image = Buffer.from([137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,1,0,0,0,1,8,2,0,0,0,144,119,83,222,0,0,0,12,73,68,65,84,120,156,99,104,104,104,0,0,3,4,1,129,75,211,210,16,0,0,0,0,73,69,78,68,174,66,96,130])
    const avatar = await http.json('/api/users/avatar/upload', { token: system.token, body: image, headers: { 'Content-Type': 'application/octet-stream' } })
    const avatarUrl = avatar.profile.avatarUrl
    await http.json('/api/follows/toggle', { token: user.token, body: { targetUserId: 'qmzdc' } })
    await http.json('/api/follows/toggle', { token: system.token, body: { targetUserId: user.account.userId } })
    const sent = await http.json('/api/messages/send', { token: system.token, body: { targetUserId: user.account.userId, text: '迁移前消息' } })
    const personal = { recipeId: 'cook_000002', durationOverrides: [{ taskId: 'wash_rice', durationSeconds: 31 }] }
    await http.json('/api/recipes/personalization/save', { token: system.token, body: personal })
    const shared = await http.json('/api/recipes/personalization/share', { token: system.token, body: { recipeId: personal.recipeId } })
    await http.json('/api/likes/toggle', { token: system.token, body: { targetType: 'video', targetId: 'cook_000001' } })
    await http.json('/api/favorites/toggle', { token: system.token, body: { targetType: 'video', targetId: 'cook_000001' } })
    const comment = await http.json('/api/comments/create', { token: system.token, body: { targetType: 'video', targetId: 'cook_000001', content: '迁移前评论' } })
    const ad = await http.json('/api/wallet/ad/start', { token: system.token, body: {} })
    const recipeId = 'identity_recipe_' + Date.now().toString(36)
    await http.json('/api/recipes/custom/publish', { token: system.token, body: { recipe: {
      schemaVersion: '1.0', id: recipeId, title: '迁移测试菜', category: '蔬菜', priceType: 'free',
      tasks: [{ id: 'cut', title: '切菜', durationSeconds: 10, active: true }]
    } } })
    const before = (await http.json('/api/account', { token: system.token })).account
    let stable
    await until(async () => {
      stable = await storage.snapshot(context)
      return stable.includes(recipeId) && stable.includes(shared.shareId) && stable.includes(ad.adToken)
    }, '迁移素材真实落盘', 20000, context.signal)
    await storage.stopOwnedBackend(context)
    let legacy = stable.replace(/\bqmzdc\b/g, 'system').split('\n').map(line => {
      if (!line.startsWith('USER\t')) return line
      const parts = line.split('\t')
      if (parts[1] === 'system') { parts[6] = 'system'; parts[7] = 'system'; parts[8] = ''; parts[9] = '' }
      return parts.slice(0, 21).join('\t')
    }).join('\n')
    legacy += '\nIDEMP\tsystem|purchase|identity|migration_key\n'
    await storage.replaceSnapshot(context, legacy)
    await storage.startOwnedBackend(context)
    const login = await http.json('/api/auth/login', { body: { account: 'qmzdc', password: 'yangnan' } })
    assert.equal(login.account.userId, 'qmzdc')
    assert.equal(login.account.nickname, '全民做大厨')
    assert.equal(login.account.role, 'super_admin')
    assert.equal(login.account.coins, before.coins)
    assert.equal(login.account.yuanbao, before.yuanbao)
    // 旧系统会话也必须指向同一身份，不能创建第二个system账户。
    assert.equal((await http.json('/api/auth/session', { token: system.token })).account.userId, 'qmzdc')
    const admin = await http.json('/api/auth/login', { body: { account: 'xylsxyls', password: 'IdentityAdmin2026!' } })
    assert.equal(admin.account.userId, existing.json.account.userId)
    assert.equal(admin.account.role, 'admin')
    assert.equal((await http.json('/api/auth/session', { token: user.token })).account.role, 'user')
    const profile = (await http.json('/api/users/profile?userId=qmzdc', { token: user.token })).profile
    assert.equal(profile.avatarUrl, avatarUrl)
    assert.equal(profile.friend, true)
    assert.deepEqual((await http.request(avatarUrl)).data, image)
    assert.equal((await http.request(avatarUrl.replace('userId=qmzdc', 'userId=system'))).status, 404)
    const feed = (await http.json('/api/videos/detail?videoId=cook_000001', { token: user.token })).item
    assert.equal(feed.authorUserId, 'qmzdc')
    assert.equal(feed.authorAvatarUrl, avatarUrl)
    const conversation = await http.json('/api/messages/conversation?userId=qmzdc', { token: user.token })
    assert.ok(conversation.messages.some(item => item.id === sent.messageItem.id && item.fromUserId === 'qmzdc'))
    const detail = await http.json('/api/recipes/personalization/detail?recipeId=cook_000002', { token: login.token })
    assert.equal(detail.personalization.durationOverrides.find(item => item.taskId === 'wash_rice').durationSeconds, 31)
    const recipe = (await http.json('/api/recipes/detail?recipeId=' + recipeId, { token: login.token })).recipe
    assert.equal(recipe.authorUserId, 'qmzdc')
    assert.equal(recipe.owned, true)
    const migrated = await storage.snapshot(context)
    for (const line of migrated.split('\n')) {
      const parts = line.split('\t')
      if (['USER', 'MENU', 'VIDEO_SEEN', 'CUSTOM', 'PERS'].includes(parts[0])) assert.notEqual(parts[1], 'system')
      if (['SESSION', 'AD', 'SHARE', 'VIDEO'].includes(parts[0])) assert.notEqual(parts[2], 'system')
      if (parts[0] === 'COMMENT') assert.notEqual(parts[4], 'system')
      if (parts[0] === 'MESSAGE') { assert.notEqual(parts[2], 'system'); assert.notEqual(parts[3], 'system') }
      if (parts[0] === 'IDEMP') assert.ok(!parts[1].startsWith('system|'))
    }
    assert.ok(migrated.includes('qmzdc|purchase|identity|migration_key'))
    // 再次冷启动验证头像、密码、角色和关联均已写入MySQL，且不重置余额。
    await storage.stopOwnedBackend(context)
    await storage.startOwnedBackend(context)
    const again = await http.json('/api/auth/login', { body: { account: 'qmzdc', password: 'yangnan' } })
    assert.equal(again.account.userId, 'qmzdc')
    assert.equal(again.account.coins, before.coins)
    assert.deepEqual((await http.request(avatarUrl)).data, image)
    await storage.stopOwnedBackend(context)
    const safe = await storage.snapshot(context)
    const conflict = safe.split('\n').map(line => {
      if (!line.startsWith('USER\t' + user.account.userId + '\t')) return line
      const parts = line.split('\t'); parts[6] = 'qmzdc'; return parts.join('\t')
    }).join('\n')
    await storage.replaceSnapshot(context, conflict)
    await storage.startOwnedBackend(context, true)
    assert.equal(await storage.snapshot(context), conflict, '冲突启动不能重写数据库')
    return { migratedId: 'qmzdc', administratorIdPreserved: true, oldSessionPreserved: true,
      avatarBytesRestored: image.length, privateMessagesAndFollowRestored: true, personalizationRestored: true,
      idempotencyMigrated: true, repeatedColdStartPassed: true, conflictRefusedWithoutOverwrite: true, commentFixtureCreated: !!comment.ok }
  }
}