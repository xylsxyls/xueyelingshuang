const { freshAccount, account, menu, feed, secondaryAccount, until, sleep, assert } = require('./helpers')

module.exports = {
  1: async context => {
    const user = await freshAccount(context)
    const sim = context.simulator
    const mine = await sim.page('/pages/mine/mine', true)
    const state = await sim.data(mine, data => data.account && data.account.userId === user.account.userId && !data.loading, '我的账号实际加载')
    assert.equal(state.account.nickname, '测试厨友')
    await sim.tap(mine, 'button[bindtap="logout"]')
    await until(async () => (await sim.mini.currentPage()).path === 'pages/auth/login/login', '登出页', 10000, context.signal)
    const login = await sim.mini.currentPage()
    await sim.input(login, 'input[placeholder="账号"]', user.loginName)
    await sim.input(login, 'input[placeholder="密码"]', 'CookWxTest2026!')
    await sim.tap(login, '.primary-btn')
    await until(async () => (await sim.mini.currentPage()).path === 'pages/home/home', '登录后首页', 15000, context.signal)
    assert.equal((await context.http.json('/api/auth/session', { token: await sim.token() })).account.userId, user.account.userId)
    return { userId: user.account.userId, registeredAndLoggedIn: true }
  },
  2: async context => {
    const { simulator: sim, signal } = context
    sim.signal = signal
    await sim.reset()
    const page = await sim.page('/pages/auth/login/login')
    await sim.tap(page, '.primary-btn')
    assert.equal(await sim.token(), '')
    await sim.tap(page, '.ghost-btn')
    await sim.input(page, 'input[placeholder="账号"]', 'short_' + Date.now().toString(36))
    await sim.input(page, 'input[placeholder="密码"]', 'short')
    await sim.tap(page, '.primary-btn')
    assert.equal(await sim.token(), '')
    assert.equal((await sim.network()).filter(item => item.url.includes('/api/auth/register')).length, 0)
    await sim.tap(page, '.ghost-btn')
    await sim.input(page, 'input[placeholder="密码"]', 'wrong-password')
    await sim.tap(page, '.primary-btn')
    await until(async () => (await sim.network()).some(item => item.url.includes('/api/auth/login') && item.finishedAt), '真实错误登录响应', 15000, signal)
    assert.equal(await sim.token(), '')
    assert.equal((await page.data()).submitting, false)
    return { noRegistrationForShortPassword: true, failedLoginFinished: true }
  },
  3: async context => {
    const user = await freshAccount(context)
    const page = await context.simulator.page('/pages/mine/mine', true)
    await context.simulator.tap(page, 'button[bindtap="logout"]')
    await until(async () => (await context.simulator.mini.currentPage()).path === 'pages/auth/login/login', '退出到登录页', 15000, context.signal)
    assert.equal(await context.simulator.token(), '')
    assert.equal((await context.http.request('/api/auth/session', { token: user.token })).status, 401)
    return { oldSessionRevoked: true }
  },
  4: async context => {
    const user = await freshAccount(context)
    await context.http.json('/api/auth/logout', { token: user.token, method: 'POST' })
    await context.simulator.page('/pages/mine/mine', true).catch(() => {})
    await until(async () => (await context.simulator.mini.currentPage()).path === 'pages/auth/login/login', '失效后重新登录', 15000, context.signal)
    assert.equal(await context.simulator.token(), '')
    return { unauthorizedRedirect: true }
  },
  17: async context => {
    const user = await freshAccount(context)
    const sim = context.simulator
    const page = await sim.page('/pages/mine/mine', true)
    await sim.data(page, data => data.account && !data.loading, '钱包实际加载')
    const before = await account(context, user.token)
    await sim.tap(page, 'button[bindtap="checkin"]')
    await until(async () => (await account(context, user.token)).coins === before.coins + 5, '签到真实到账', 15000, context.signal)
    await sim.data(page, data => data.account.coins === before.coins + 5 && !data.loading, '钱包显示到账')
    await sim.tap(page, 'button[bindtap="checkin"]')
    await until(async () => (await sim.network()).filter(item => item.url.includes('/api/wallet/checkin') && item.finishedAt).length === 2, '第二次签到响应', 15000, context.signal)
    assert.equal((await account(context, user.token)).coins, before.coins + 5)
    return { before: before.coins, after: before.coins + 5, reward: 5 }
  },
  18: async context => {
    const user = await freshAccount(context)
    const sim = context.simulator
    const page = await sim.page('/pages/mine/mine', true)
    const before = await account(context, user.token)
    const premature = await context.http.json('/api/wallet/ad/start', { token: user.token, method: 'POST' })
    const reject = await context.http.request('/api/wallet/ad/finish', { token: user.token, body: { adToken: premature.adToken, waitedSeconds: 999 } })
    assert.equal(reject.json.ok, false)
    assert.equal((await account(context, user.token)).coins, before.coins)
    await sim.tap(page, 'button[bindtap="watchAd"]')
    await sim.data(page, data => data.rewarding, '广告计时开始')
    const issuedAdToken = await sim.mini.evaluate(() => getCurrentPages()[getCurrentPages().length - 1].adToken)
    assert.ok(issuedAdToken)
    const start = Date.now()
    await until(async () => (await account(context, user.token)).coins === before.coins + 30, '真实等待后到账', 15000, context.signal)
    assert.ok(Date.now() - start >= 2000, '不能立即发奖励')
    const adToken = await sim.mini.evaluate(() => getCurrentPages()[getCurrentPages().length - 1].adToken)
    assert.equal(adToken, '')
    const duplicate = await context.http.request('/api/wallet/ad/finish', { token: user.token, body: { adToken: issuedAdToken } })
    assert.equal(duplicate.status, 200)
    assert.equal(duplicate.json.ok, false)
    assert.equal((await account(context, user.token)).coins, before.coins + 30)
    return { actualWaitMs: Date.now() - start, reward: 30, prematureRejected: true, configuredSeconds: 3 }
  },
  23: async context => {
    const user = await freshAccount(context)
    const peer = await secondaryAccount(context)
    await context.http.json('/api/follows/toggle', { token: user.token, body: { targetUserId: peer.account.userId } })
    const sim = context.simulator
    const page = await sim.page('/pages/messages/messages', true)
    await sim.tap(page, '.contact-tab[data-key="following"]')
    await sim.data(page, data => data.contactItems.some(item => item.userId === peer.account.userId), '真实联系人')
    await sim.tap(page, '.contact-card[data-index="0"]')
    const text = '自动测试消息 ' + Date.now()
    const conversation = await until(async () => { const current = await sim.mini.currentPage(); return current.path === 'pages/conversation/conversation' && current }, '联系人进入真实会话', 15000, context.signal)
    await sim.data(conversation, data => data.profile.following && !data.loading, '会话身份加载')
    await sim.input(conversation, 'input[bindinput="onInput"]', text)
    await sim.tap(conversation, 'button[bindtap="send"]')
    await until(async () => {
      const response = await context.http.json('/api/messages/list', { token: peer.token })
      return response.messages && response.messages.some(item => item.text === text && item.fromUserId === user.account.userId)
    }, '接收账号真实收到消息', 15000, context.signal)
    return { peerId: peer.account.userId, receivedText: text }
  },
  24: async context => {
    const user = await freshAccount(context)
    const media = await feed(context, user.token)
    assert.ok(media.recipeIds && media.recipeIds.length, '隔离素材必须关联菜谱')
    await context.http.json('/api/favorites/toggle', { token: user.token, body: { recipeId: media.recipeIds[0] } })
    await context.http.json('/api/favorites/toggle', { token: user.token, body: { targetType: 'video', targetId: media.videoId } })
    await context.http.json('/api/likes/toggle', { token: user.token, body: { targetType: 'video', targetId: media.videoId } })
    const sim = context.simulator
    const page = await sim.page('/pages/mine/mine', true)
    const mine = await sim.data(page, data => data.account && !data.loading, '我的分区')
    assert.ok(mine.assetTabs && mine.assetTabs.length)
    assert.equal(mine.assetErrors.favorite, false)
    assert.deepEqual(mine.assetGroups.favorite.map(item => item.videoId), [media.videoId], '菜谱和视频双收藏只显示一次')
    assert.deepEqual(mine.assetGroups.liked.map(item => item.videoId), [media.videoId])
    assert.equal(mine.assetGroups.created.length, 0, '未发布视频的新用户不能出现推荐或菜谱创作')
    const tile = await sim.element(page, '.media-tile[data-index="0"]')
    const tileSize = await tile.size()
    const viewport = await sim.mini.systemInfo()
    assert.ok(Math.abs(tileSize.width - (viewport.windowWidth - 4 * viewport.windowWidth / 750 * 2) / 3) < 2, '每行三列，每列占可用宽度三分之一')
    assert.ok(Math.abs(tileSize.height / tileSize.width - 4 / 3) < 0.03, '竖版封面宽高比3:4')
    const cover = await sim.element(page, '.media-cover[data-index="0"]')
    assert.equal(await cover.attribute('src'), mine.assetItems[0].posterUrl)
    assert.equal((await sim.mediaNetwork()).length, 0, '我的列表只加载封面，不下载视频片段')
    for (const tab of mine.assetTabs) {
      await sim.tap(page, `.asset-tab[data-key="${tab.key}"]`)
      assert.equal((await page.data()).activeAssetTab, tab.key)
      assert.equal((await page.data()).mediaGrid, ['liked', 'favorite', 'created'].includes(tab.key))
    }
    const voice = await sim.page('/pages/voice/voice')
    await sim.tap(voice, '.option[data-value="male"]')
    assert.equal((await voice.data()).voiceGender, 'male')
    const reopened = await sim.page('/pages/voice/voice')
    assert.equal((await reopened.data()).voiceGender, 'male')
    await sim.tap(reopened, '.option[data-value="female"]')
    assert.equal((await reopened.data()).voiceGender, 'female')
    return { userId: user.account.userId, tabs: mine.assetTabs.map(item => item.key), persistedVoice: true, deduplicatedCollection: true, tileSize, incomplete: '三张以上封面排列、本人发布后作品、图片带声作品、分页与失败提示及实际语音听感未由本用例覆盖' }
  }
}