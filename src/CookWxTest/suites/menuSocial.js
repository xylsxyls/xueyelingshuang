const { freshAccount, secondaryAccount, account, recipes, ownedRecipe, recipePage, menu, pickRecipe, feed, until, assert } = require('./helpers')

async function mine(context, tab) {
  const page = await context.simulator.page('/pages/mine/mine', true)
  await context.simulator.data(page, data => !data.loading && data.account && data.account.userId, '个人页真实账户')
  if (tab) await context.simulator.tap(page, `.asset-tab[data-key="${tab}"]`)
  return page
}

async function createFree(context, owner) {
  const id = 'menu_' + Date.now().toString(36) + '_' + Math.random().toString(36).slice(2, 5)
  await context.http.json('/api/recipes/custom/publish', { token: owner.token, body: { recipe: { schemaVersion: '1.0', id, title: '测试免费菜', category: '蔬菜', priceType: 'free', ingredients: [{ name: '白菜', amount: '150克' }], tasks: [{ id: 'cut', title: '切白菜', durationSeconds: 10, active: true }] } } })
  return id
}

module.exports = {
  8: async context => {
    const user = await freshAccount(context), peer = await secondaryAccount(context)
    const id = await createFree(context, peer)
    await context.http.json('/api/recipes/join', { token: user.token, body: { recipeId: id } })
    const page = await mine(context, 'menu'), sim = context.simulator
    await sim.data(page, data => data.assetItems.some(item => item.recipeId === id && item.canRemove), '免费用户菜显示移出按钮')
    const index = (await page.data()).assetItems.findIndex(item => item.recipeId === id)
    await sim.mock('showModal', { confirm: false, cancel: true })
    await sim.tap(page, `.remove-menu[data-index="${index}"]`)
    await sim.data(page, data => !data.menuActionId, '取消移出完成')
    assert.ok((await recipes(context, user.token)).some(item => item.id === id))
    await sim.mock('showModal', { confirm: true, cancel: false })
    await sim.tap(page, `.remove-menu[data-index="${index}"]`)
    await sim.data(page, data => !data.menuActionId && !data.assetItems.some(item => item.recipeId === id), '确认后我的菜单同步消失')
    const bottom = await menu(context)
    assert.ok(!(await bottom.data()).recipes.some(item => item.id === id))
    return { recipeId: id, cancelKept: true, bothMenusRemoved: true }
  },
  38: async context => {
    const user = await freshAccount(context), sim = context.simulator, page = await menu(context)
    const data = await page.data()
    assert.deepEqual(data.categories, ['荤菜', '小荤', '蔬菜', '汤饮', '主食'])
    assert.equal(data.menuSections.length, 5)
    const ids = data.filteredRecipes.map(item => item.id)
    await sim.tap(page, '.cat[data-name="汤饮"]')
    assert.deepEqual((await page.data()).filteredRecipes.map(item => item.id), ids)
    const scroll = await sim.element(page, '#recipeScroll')
    await scroll.scrollTo(0, await scroll.scrollHeight())
    await sim.data(page, data => data.activeCategory === '主食', '真实滚动到底联动左侧主食')
    const recipe = await ownedRecipe(context, user.token)
    await pickRecipe(context, page, recipe)
    const button = await sim.element(page, '.cart .primary-btn')
    assert.equal((await button.text()).trim(), '开始做菜')
    const style = Object.fromEntries(await Promise.all(['display', 'align-items', 'justify-content'].map(async key => [key, await button.style(key)])))
    assert.equal(style.display, 'flex'); assert.equal(style['align-items'], 'center'); assert.equal(style['justify-content'], 'center')
    const personal = await mine(context)
    const state = await personal.data()
    assert.deepEqual(state.assetTabs.map(item => item.key), ['liked', 'favorite', 'created', 'menu', 'personal'])
    assert.ok(state.assetTabs.every(item => item.count == null))
    assert.equal(await personal.$('scroll-view.asset-tabs'), null)
    assert.equal(await personal.$('.stats'), null)
    assert.ok(await personal.$('.logout-inline'))
    return { categoryOrder: data.categories, continuous: true, centered: style, fixedFiveTabs: true }
  },
  39: async context => {
    const user = await freshAccount(context), sim = context.simulator
    const paid = await ownedRecipe(context, user.token, true)
    assert.ok(!(await recipes(context, user.token)).some(item => item.id === paid.id))
    const before = await account(context, user.token)
    const page = await recipePage(context, paid.id)
    await sim.tap(page, '.action-bar .primary-btn')
    await sim.data(page, data => data.recipe.inMenu && !data.joining, '加入付费菜不购买')
    const after = await account(context, user.token)
    assert.equal(after.coins, before.coins); assert.equal(after.yuanbao, before.yuanbao)
    assert.ok(!after.purchasedRecipeIds.includes(paid.id))
    const bottom = await menu(context)
    assert.ok((await bottom.data()).recipes.some(item => item.id === paid.id && item.canBuy))
    assert.ok(await bottom.$(`.buy[data-id="${paid.id}"]`))
    const personal = await mine(context, 'menu')
    const state = await personal.data()
    assert.equal(state.assetItems[0].recipeId, paid.id)
    assert.ok(await personal.$('.buy-button[data-index="0"]'))
    const freePeer = await secondaryAccount(context), freeId = await createFree(context, freePeer)
    await context.http.json('/api/recipes/join', { token: user.token, body: { recipeId: freeId } })
    await recipePage(context, paid.id)
    const reordered = await mine(context, 'menu')
    assert.equal((await reordered.data()).assetItems[0].recipeId, paid.id)
    return { paidId: paid.id, noCharge: true, buyInBothMenus: true, viewedMovesFirst: true }
  },
  40: async context => {
    const user = await freshAccount(context), peer = await secondaryAccount(context), sim = context.simulator
    const page = await sim.page('/pages/profile/profile?userId=' + peer.account.userId)
    await sim.data(page, data => !data.loading && data.profile.userId === peer.account.userId, '公开资料加载')
    assert.equal((await page.$$('.profile-tabs > view')).length, 2)
    assert.equal(await page.$('.wallet'), null)
    const state = await page.data()
    for (const key of ['coins', 'yuanbao', 'menuRecipeIds', 'purchasedRecipeIds', 'favoriteRecipeIds']) assert.equal(state.profile[key], undefined)
    await sim.tap(page, '.follow-button')
    await sim.data(page, data => data.profile.following && !data.followingBusy, '关注成功')
    await sim.mock('showModal', { confirm: false, cancel: true }); await sim.tap(page, '.follow-button')
    await sim.data(page, data => !data.followingBusy, '取消弹框结束')
    assert.equal((await page.data()).profile.following, true)
    await sim.mock('showModal', { confirm: true, cancel: false }); await sim.tap(page, '.follow-button')
    await sim.data(page, data => !data.profile.following && !data.followingBusy, '确认取消关注')
    await context.http.json('/api/follows/toggle', { token: peer.token, body: { targetUserId: user.account.userId } })
    const personal = await mine(context)
    await sim.tap(personal, '.fans-link')
    const fans = await until(async () => { const current = await sim.mini.currentPage(); return current.path === 'pages/fans/fans' && current }, '进入粉丝页', 15000, context.signal)
    await sim.data(fans, data => data.users.some(item => item.userId === peer.account.userId), '真实粉丝列表')
    assert.ok(await fans.$(`.fan[data-id="${peer.account.userId}"]`))
    return { publicOnlyTwoTabs: true, privateDataAbsent: true, confirmUnfollow: true, fans: 1 }
  },
  41: async context => {
    const user = await freshAccount(context), peer = await secondaryAccount(context), sim = context.simulator
    await context.http.json('/api/follows/toggle', { token: peer.token, body: { targetUserId: user.account.userId } })
    const first = 'muted_' + Date.now()
    await context.http.json('/api/messages/send', { token: peer.token, body: { targetUserId: user.account.userId, text: first } })
    const page = await sim.page('/pages/messages/messages', true)
    const muted = await sim.data(page, data => data.messages.some(item => item.text === first && item.muted && item.unreadCount === 1), '真实免打扰未读')
    assert.ok(await page.$('.muted-unread'))
    await context.http.json('/api/follows/toggle', { token: user.token, body: { targetUserId: peer.account.userId } })
    const refreshed = await sim.page('/pages/messages/messages', true)
    await sim.data(refreshed, data => data.messages.some(item => item.text === first && !item.muted && item.unreadCount === 1), '互关红色未读')
    assert.ok(await refreshed.$('.friend-unread'))
    const index = (await refreshed.data()).messages.findIndex(item => item.fromUserId === peer.account.userId && item.type === 'chat')
    await sim.tap(refreshed, `.msg[data-index="${index}"]`)
    const conversation = await until(async () => { const current = await sim.mini.currentPage(); return current.path === 'pages/conversation/conversation' && current }, '打开实际会话', 15000, context.signal)
    await sim.data(conversation, data => data.profile.friend && data.messages.some(item => item.text === first), '双向会话载入')
    await until(async () => (await context.http.json('/api/messages/list', { token: user.token })).messages.some(item => item.text === first && item.read), '进入会话实际已读', 15000, context.signal)
    const reply = 'reply_' + Date.now()
    await sim.input(conversation, 'input[bindinput="onInput"]', reply); await sim.tap(conversation, 'button[bindtap="send"]')
    await until(async () => (await context.http.json('/api/messages/list', { token: peer.token })).messages.some(item => item.text === reply), '回复实际送达', 15000, context.signal)
    return { grayOneWay: muted.messages.some(item => item.muted), redMutual: true, readOnOpen: true, realReply: true }
  },
  42: async context => {
    const user = await freshAccount(context), sim = context.simulator, media = await feed(context, user.token)
    const source = await recipePage(context, media.recipeIds[0])
    await sim.tap(source, '.recipe-hero')
    const page = await until(async () => { const current = await sim.mini.currentPage(); return current.path === 'pages/videoPlayer/videoPlayer' && current }, '详情原视频入口', 15000, context.signal)
    await sim.data(page, data => data.item && data.item.videoId === media.videoId && !data.metadataLoading, '完整原视频资料')
    for (const selector of ['.author', '.title', '.desc', '.creator-avatar', '.action-icon.heart', '.action-icon.comment', '.action-icon.star', '.recipe-btn', '.ghost-recipe-btn']) assert.ok(await page.$(selector), selector)
    assert.equal(await page.$('.feed-tabs'), null)
    await sim.tap(page, '.action-item[bindtap="toggleLike"]')
    await sim.data(page, data => data.item.liked, '原视频点赞反馈')
    assert.ok((await account(context, user.token)).likedTargetIds.includes('video:' + media.videoId))
    await sim.mini.navigateBack({ delta: 1 })
    await until(async () => (await sim.mini.currentPage()).path === 'pages/recipe/recipe', '返回原菜谱详情', 15000, context.signal)
    return { videoId: media.videoId, fullControls: true, liked: true, returned: true }
  },
  43: async context => {
    const user = await freshAccount(context), sim = context.simulator
    const catalog = (await context.http.json('/api/recipes?scope=catalog', { token: user.token })).recipes.filter(item => item.systemRecipe)
    for (const recipe of catalog) {
      // 免费及已拥有菜谱返回完整内容；锁定菜谱的明细已由 C++ 用例60验证。
      if (!recipe.owned) continue
      const detail = (await context.http.json('/api/recipes/detail?recipeId=' + recipe.id, { token: user.token })).recipe
      assert.ok(detail.ingredients.every(item => /^\d+(?:\.\d+)?克$/.test(item.amount)))
      assert.ok(detail.tasks.every(item => Array.from(item.title).length <= 8 && item.voiceText === item.title))
    }
    const media = await feed(context, user.token), page = await recipePage(context, media.recipeIds[0])
    const data = await page.data()
    assert.ok(data.recipe.thumbnailUrl && data.recipe.posterUrl)
    assert.ok(await page.$('.recipe-hero-image')); assert.ok(await page.$('.recipe-hero-gradient')); assert.ok(await page.$('.recipe-hero-triangle'))
    assert.equal((await page.$$('.action-bar button')).length, 2)
    assert.equal(await page.$('.action-bar button[bindtap="favorite"]'), null)
    return { validatedBuiltins: catalog.length, realPoster: true, gradientAndPlay: true, twoActions: true, squareImageUpload: '服务端用例62验证真实PNG，发布页选图须人工补验' }
  }
}