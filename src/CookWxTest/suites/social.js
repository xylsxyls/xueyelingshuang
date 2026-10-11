const { freshAccount, account, ownedRecipe, feed, secondaryAccount, until, assert } = require('./helpers')

async function home(context) {
  const page = await context.simulator.page('/pages/home/home', true)
  await context.simulator.data(page, data => !data.loading && data.feed.length, '首页真实推荐流')
  return page
}

module.exports = {
  19: async context => {
    const user = await freshAccount(context)
    const page = await home(context)
    const initial = await page.data()
    const ids = initial.feed.map(item => item.videoId)
    assert.equal(new Set(ids).size, ids.length)
    const following = initial.tabs.find(item => item.includes('关注'))
    assert.ok(following)
    await context.simulator.tap(page, `.tab[data-tab="${following}"]`)
    await context.simulator.data(page, data => data.activeTab === following && !data.loading, '关注流实际加载')
    const request = (await context.simulator.network()).find(item => item.url.includes('/api/feed') && item.url.includes('mode=following'))
    assert.ok(request, '切换必须请求关注流')
    const original = initial.activeTab
    await context.simulator.tap(page, `.tab[data-tab="${original}"]`)
    await context.simulator.data(page, data => data.activeTab === original && !data.loading && data.feed.length, '回到推荐流')
    return { initialIds: ids, followingRequested: true, userId: user.account.userId }
  },
  20: async context => {
    const user = await freshAccount(context)
    const page = await home(context)
    const id = (await page.data()).feed[0].videoId
    await context.simulator.tap(page, '.action-item[bindtap="toggleLike"][data-index="0"]')
    await context.simulator.data(page, data => data.feed[0].liked, '点赞实际反馈')
    assert.ok((await account(context, user.token)).likedTargetIds.includes('video:' + id))
    await context.simulator.tap(page, '.action-item[bindtap="favorite"][data-index="0"]')
    await context.simulator.data(page, data => data.feed[0].favorite, '视频收藏反馈')
    assert.ok((await account(context, user.token)).favoriteTargetIds.includes('video:' + id))
    await context.simulator.tap(page, '.action-item[bindtap="toggleLike"][data-index="0"]')
    await context.simulator.data(page, data => !data.feed[0].liked, '取消点赞')
    await context.simulator.tap(page, '.action-item[bindtap="favorite"][data-index="0"]')
    await context.simulator.data(page, data => !data.feed[0].favorite, '取消视频收藏')
    const final = await account(context, user.token)
    assert.ok(!final.likedTargetIds.includes('video:' + id) && !final.favoriteTargetIds.includes('video:' + id))
    return { videoId: id, toggledAndRestored: true }
  },
  21: async context => {
    const user = await freshAccount(context)
    const peer = await secondaryAccount(context)
    // 联系人前置只通过真实接口创建；页面执行取消并核对跨页面关系。
    await context.http.json('/api/follows/toggle', { token: user.token, body: { targetUserId: peer.account.userId } })
    const page = await context.simulator.page('/pages/messages/messages', true)
    await context.simulator.tap(page, '.contact-tab[data-key="following"]')
    await context.simulator.data(page, data => data.contactItems.some(item => item.userId === peer.account.userId), '关注联系人')
    await context.simulator.mock('showModal', { confirm: true, cancel: false })
    await context.simulator.tap(page, '.unfollow-btn[data-index="0"]')
    await until(async () => !(await account(context, user.token)).followingUserIds.includes(peer.account.userId), '取消关注真实落盘', 15000, context.signal)
    const mine = await context.simulator.page('/pages/mine/mine', true)
    await context.simulator.data(mine, data => data.account && !data.loading, '我的关注同步')
    assert.ok(!(await mine.data()).account.followingUserIds.includes(peer.account.userId))
    return { peerId: peer.account.userId, removedAcrossPages: true, incomplete: '首页真实作者作品的关注入口尚未覆盖' }
  },
  22: async context => {
    const user = await freshAccount(context)
    const item = await feed(context, user.token)
    const id = item.videoId || String(item.id).replace(/^feed_/, '')
    const route = '/pages/comments/comments?targetType=video&targetId=' + encodeURIComponent(id)
    const page = await context.simulator.page(route)
    await context.simulator.data(page, data => !data.loading && data.targetId === id, '评论加载')
    const content = '中文评论，边界 "引用" <标签> ' + Date.now()
    await context.simulator.input(page, '.comment-input', content)
    await context.simulator.tap(page, '.send-button')
    await context.simulator.data(page, data => !data.submitting && data.comments.some(value => value.content === content), '发送后显示真实评论')
    const stored = await context.http.json('/api/comments/list?targetType=video&targetId=' + encodeURIComponent(id), { token: user.token })
    assert.ok(stored.comments.some(value => value.content === content && value.userId === user.account.userId))
    const reopened = await context.simulator.page(route)
    await context.simulator.data(reopened, data => !data.loading && data.comments.some(value => value.content === content), '重新打开读回评论')
    return { videoId: id, content, persisted: true }
  }
}