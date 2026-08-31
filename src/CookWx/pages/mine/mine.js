const api = require('../../utils/api')
const time = require('../../utils/time')
const debugLog = require('../../utils/debugLog')
const tabBar = require('../../utils/tabBar')

const ASSET_TABS = [
  { key: 'liked', title: '喜欢' },
  { key: 'favorite', title: '收藏' },
  { key: 'created', title: '创作' },
  { key: 'menu', title: '菜单' },
  { key: 'personal', title: '个性化' },
  { key: 'following', title: '关注' },
  { key: 'friends', title: '好友' }
]

function firstDisplayChar(text, fallback) {
  const value = text === undefined || text === null ? '' : String(text).trim()
  if (!value) return fallback || ''
  return value.charAt(0)
}

Page({
  data: {
    account: {},
    recipes: [],
    purchased: [],
    rewarding: false,
    assetTabs: ASSET_TABS,
    activeAssetTab: 'favorite',
    assetGroups: {},
    assetItems: [],
    stats: [],
    loading: false
  },

  onShow() {
    tabBar.showHome()
    this.pageActive = true
    const app = getApp()
    const initialTab = app.globalData.mineInitialTab || ''
    if (initialTab) {
      app.globalData.mineInitialTab = ''
      this.setData({ activeAssetTab: initialTab })
    }
    this.load()
  },

  onHide() {
    this.pageActive = false
    this.loadRequestId = (this.loadRequestId || 0) + 1
  },

  onUnload() {
    this.pageActive = false
    this.loadRequestId = (this.loadRequestId || 0) + 1
  },

  load() {
    this.loadRequestId = (this.loadRequestId || 0) + 1
    const requestId = this.loadRequestId
    const isStale = () => requestId !== this.loadRequestId || this.pageActive === false
    debugLog.info('MINE', 'LOAD_START', { requestId })
    this.setData({ loading: true })
    const likedFeedPromise = api.getFeed({ isStale, mode: 'liked', count: 100 }).catch(() => ({ ok: false, feed: [] }))
    const favoriteFeedPromise = api.getFeed({ isStale, mode: 'favorite', count: 100 }).catch(() => ({ ok: false, feed: [] }))
    Promise.all([api.getAccount({ isStale }), api.getRecipes({ isStale }), likedFeedPromise, favoriteFeedPromise])
      .then(([accountRes, recipeRes, likedFeedRes, favoriteFeedRes]) => {
        if (!accountRes.ok || !recipeRes.ok) throw new Error('load failed')
        if (isStale()) return
        const account = Object.assign({}, accountRes.account || {}, {
          followingUsers: accountRes.followingUsers || (accountRes.account || {}).followingUsers || [],
          friendUsers: accountRes.friendUsers || (accountRes.account || {}).friendUsers || []
        })
        const recipes = recipeRes.recipes || []
        const purchasedIds = Array.isArray(account.purchasedRecipeIds) ? account.purchasedRecipeIds : []
        const purchased = recipes.filter((item) => purchasedIds.indexOf(item.id) >= 0)
        const assetGroups = this.buildAssetGroups(account, recipes, likedFeedRes.feed || [], favoriteFeedRes.feed || [])
        this.setData({
          account,
          recipes,
          purchased,
          stats: this.buildStats(account, assetGroups),
          assetGroups,
          loading: false
        }, () => this.refreshAssetItems())
        debugLog.info('MINE', 'LOAD_OK', {
          requestId,
          recipeCount: recipes.length,
          favoriteCount: (assetGroups.favorite || []).length,
          likedCount: (assetGroups.liked || []).length
        })
      })
      .catch((err) => {
        if (api.isStaleError(err) || isStale()) {
          debugLog.info('MINE', 'LOAD_STALE', {
            requestId,
            error: err && err.message ? err.message : String(err)
          })
          return
        }
        debugLog.warn('MINE', 'LOAD_FAIL', {
          requestId,
          error: err && err.message ? err.message : String(err)
        })
        this.setData({ loading: false })
        wx.showToast({ title: '账户加载失败', icon: 'none' })
      })
  },

  buildStats(account, groups) {
    return [
      { label: '收藏', value: (groups.favorite || []).length },
      { label: '关注', value: (groups.following || []).length },
      { label: '好友', value: (groups.friends || []).length },
      { label: '已购', value: Array.isArray(account.purchasedRecipeIds) ? account.purchasedRecipeIds.length : 0 }
    ]
  },

  buildAssetGroups(account, recipes, likedFeed, favoriteFeed) {
    const favoriteIds = Array.isArray(account.favoriteRecipeIds) ? account.favoriteRecipeIds : []
    const customIds = Array.isArray(account.customRecipeIds) ? account.customRecipeIds : []
    const liked = (likedFeed || []).map((item) => this.feedToAsset(item, '已点赞'))
    const favoriteVideos = (favoriteFeed || []).map((item) => this.feedToAsset(item, '已收藏'))
    const favoriteRecipes = recipes.filter((item) => favoriteIds.indexOf(item.id) >= 0 || !!item.favorite).map((item) => this.recipeToAsset(item, '已收藏'))
    const favorite = favoriteVideos.concat(favoriteRecipes)
    const created = recipes.filter((item) => customIds.indexOf(item.id) >= 0 || item.authorUserId === account.userId).map((item) => this.recipeToAsset(item, '我的创作'))
    const menu = recipes.filter((item) => !!item.owned).map((item) => this.recipeToAsset(item, item.defaultOwned ? '免费菜谱' : '已拥有'))
    const personal = recipes.filter((item) => !!item.personalizationApplied).map((item) => this.recipeToAsset(item, '已个性化'))
    const following = this.normalizeContactAssets(account.followingUsers || [], account.followingUserIds || [], false)
    const friends = this.normalizeContactAssets(account.friendUsers || [], [], true)
    return { liked, favorite, created, menu, personal, following, friends }
  },

  normalizeContactAssets(users, fallbackIds, friendOnly) {
    const result = []
    const used = {}
    const list = Array.isArray(users) ? users : []
    for (let i = 0; i < list.length; i += 1) {
      const item = list[i] || {}
      const userId = item.userId || ''
      if (!userId || used[userId]) continue
      if (friendOnly && !item.friend) continue
      used[userId] = true
      result.push(this.contactToAsset(item, item.friend ? '好友' : '已关注'))
    }
    const ids = Array.isArray(fallbackIds) ? fallbackIds : []
    for (let i = 0; i < ids.length; i += 1) {
      const userId = ids[i] || ''
      if (!userId || used[userId]) continue
      used[userId] = true
      result.push(this.contactToAsset({ userId, nickname: userId, following: true, friend: false }, '已关注'))
    }
    return result
  },

  contactToAsset(contact, badge) {
    const name = contact.nickname || contact.account || contact.userId || '厨友'
    const fans = Number(contact.followerCount || 0)
    const following = Number(contact.followingCount || 0)
    return {
      type: 'user',
      id: `user_${contact.userId || name}`,
      userId: contact.userId || '',
      title: name,
      subtitle: `${contact.friend ? '互相关注' : '已关注'} · 粉丝 ${fans} · 关注 ${following}`,
      coverText: firstDisplayChar(name, '厨'),
      coverColor: contact.friend ? '#214c3a' : '#18231f',
      badge: badge || ''
    }
  },

  feedToAsset(item, badge) {
    const recipeIds = Array.isArray(item.recipeIds) ? item.recipeIds.filter((id) => !!id) : []
    const recipeId = recipeIds.length ? recipeIds[0] : ''
    return {
      type: 'feed',
      id: `feed_${item.id || recipeId}`,
      recipeId,
      title: item.title || '做饭动态',
      subtitle: `${item.author || '厨友'} · ${Number(item.likes || 0)}赞 · ${Number(item.comments || 0)}评论`,
      coverColor: item.coverColor || '#ebe5d9',
      badge: badge || '已点赞'
    }
  },

  recipeToAsset(recipe, badge) {
    const estimatedSeconds = Number(recipe.estimatedSeconds || 0)
    const activeSeconds = Number(recipe.activeSeconds || 0)
    const totalText = estimatedSeconds > 0 ? time.formatDurationText(estimatedSeconds) : `${recipe.estimatedMinutes || 0}分钟`
    const activeText = activeSeconds > 0 ? time.formatDurationText(activeSeconds) : `${recipe.activeMinutes || 0}分钟`
    return {
      type: 'recipe',
      id: `recipe_${recipe.id || ''}`,
      recipeId: recipe.id || '',
      title: recipe.title || '未命名菜谱',
      subtitle: `${recipe.category || '未分类'} · ${totalText} · 动手 ${activeText}`,
      coverColor: recipe.coverColor || '#ebe5d9',
      badge: badge || '',
      personalizationSummary: recipe.personalizationSummary || ''
    }
  },

  refreshAssetItems() {
    const groups = this.data.assetGroups || {}
    const tabs = ASSET_TABS.map((tab) => Object.assign({}, tab, {
      count: (groups[tab.key] || []).length
    }))
    const validKeys = tabs.map((item) => item.key)
    const activeAssetTab = validKeys.indexOf(this.data.activeAssetTab) >= 0 ? this.data.activeAssetTab : tabs[0].key
    this.setData({
      assetTabs: tabs,
      activeAssetTab,
      assetItems: groups[activeAssetTab] || []
    })
  },

  switchAssetTab(e) {
    const key = e.currentTarget.dataset.key || ''
    this.setData({ activeAssetTab: key }, () => this.refreshAssetItems())
  },

  recharge(e) {
    const coins = Number(e.currentTarget.dataset.coins)
    api.recharge(coins)
      .then((res) => {
        wx.showToast({ title: res.message || '充值成功', icon: res.ok ? 'success' : 'none' })
        this.load()
      })
      .catch(() => wx.showToast({ title: '充值失败', icon: 'none' }))
  },

  checkin() {
    api.checkin()
      .then((res) => {
        wx.showToast({ title: res.message || '签到完成', icon: res.ok ? 'success' : 'none' })
        this.load()
      })
      .catch((err) => wx.showToast({ title: err.message || '签到失败', icon: 'none' }))
  },

  watchAd() {
    if (this.data.rewarding) return
    this.setData({ rewarding: true })
    api.startAdReward()
      .then((res) => {
        if (!res.ok) throw new Error(res.message)
        return api.finishAdReward(res.adToken, res.requiredSeconds || 31)
      })
      .then((res) => {
        wx.showToast({ title: res.message || '奖励到账', icon: res.ok ? 'success' : 'none' })
        this.setData({ rewarding: false })
        this.load()
      })
      .catch((err) => {
        this.setData({ rewarding: false })
        wx.showToast({ title: err.message || '奖励失败', icon: 'none' })
      })
  },

  logout() {
    api.logout()
      .then(() => wx.reLaunch({ url: '/pages/auth/login/login' }))
      .catch(() => wx.reLaunch({ url: '/pages/auth/login/login' }))
  },

  openRecipe(e) {
    const id = e.currentTarget.dataset.id
    if (!id) {
      wx.showToast({ title: '菜谱不存在', icon: 'none' })
      return
    }
    wx.navigateTo({ url: `/pages/recipe/recipe?id=${id}` })
  },

  openAsset(e) {
    const index = Number(e.currentTarget.dataset.index)
    const item = this.data.assetItems[index] || {}
    if (item.type === 'user') {
      const app = getApp()
      app.globalData.messageInitialTab = this.data.activeAssetTab === 'friends' ? 'friends' : 'following'
      app.globalData.messageTargetUserId = item.userId || ''
      wx.switchTab({ url: '/pages/messages/messages' })
      return
    }
    if (!item.recipeId) {
      wx.showToast({ title: '菜谱不存在', icon: 'none' })
      return
    }
    if (this.data.activeAssetTab === 'personal') {
      wx.navigateTo({ url: `/pages/personalization/personalization?id=${item.recipeId}` })
      return
    }
    wx.navigateTo({ url: `/pages/recipe/recipe?id=${item.recipeId}` })
  }
})