const api = require('../../utils/api')
const reviews = require('../../utils/mineRecipeReviews')
const privateMedia = require('../../utils/privateMediaCache')
const creationAssets = require('../../utils/creationAssets')
const time = require('../../utils/time')
const debugLog = require('../../utils/debugLog')
const tabBar = require('../../utils/tabBar')
const personalizationSync = require('../../utils/cookPersonalizationSync')
const config = require('../../utils/config')
const mediaAssets = require('../../utils/mineMediaAssets')
const assets = require('../../utils/recipeAssets')
const actions = require('../../utils/menuActions')
const users = require('../../utils/userPresentation')

const ASSET_TABS = [
  { key: 'liked', title: '喜欢' },
  { key: 'favorite', title: '收藏' },
  { key: 'created', title: '创作' },
  { key: 'menu', title: '菜谱库' },
  { key: 'personal', title: '个性化' }
]

Page({
  data: {
    reviewState: 'pending',
    account: {},
    profile: {},
    editingIntroduction: false,
    introductionDraft: '',
    savingIntroduction: false,
    menuActionId: '',
    recipes: [],
    rewarding: false,
    adSecondsLeft: 0,
    assetTabs: ASSET_TABS,
    activeAssetTab: 'favorite',
    assetGroups: {},
    assetItems: [],
    mediaGrid: true,
    assetHasMore: {},
    assetErrors: {},
    loadingMore: false,
    loading: false
  },

  onShow() {
    tabBar.showHome()
    this.pageActive = true
    this.setData({ menuActionId: '', savingIntroduction: false })
    personalizationSync.showPending(getApp(), wx, api, { isActive: () => this.pageActive !== false })
    const app = getApp()
    if (this.mineOwner !== app.globalData.userId) {
      reviews.stop(this)
      this.mineOwner = app.globalData.userId
      this.createdMediaAssets = []
      this.setData({ account: {}, profile: {}, recipes: [], assetGroups: {}, assetItems: [], assetHasMore: {}, assetErrors: {},
        assetTabs: ASSET_TABS, activeAssetTab: 'favorite', reviewState: 'pending', loadingMore: false })
    }
    const initialTab = app.globalData.mineInitialTab || ''
    if (initialTab) {
      app.globalData.mineInitialTab = ''
      this.setData({ activeAssetTab: initialTab })
    }
    this.load()
    this.resumeAdCountdown()
  },

  editAvatar() {
    if (this.pageActive === false || !getApp().globalData.userId) return
    wx.navigateTo({ url: '/pages/avatarCrop/avatarCrop' })
  },
  newCreation() { if (this.pageActive !== false && getApp().globalData.userId) wx.navigateTo({ url: '/pages/creationEditor/creationEditor?new=1' }) },

  onHide() {
    reviews.stop(this)
    this.pageActive = false
    this.loadRequestId = (this.loadRequestId || 0) + 1
    this.introductionGeneration = (this.introductionGeneration || 0) + 1
    this.pauseAdCountdown()
  },

  onUnload() {
    reviews.stop(this)
    this.pageActive = false
    this.loadRequestId = (this.loadRequestId || 0) + 1
    this.pauseAdCountdown()
  },

  load() {
    this.loadRequestId = (this.loadRequestId || 0) + 1
    const requestId = this.loadRequestId
    const isStale = () => requestId !== this.loadRequestId || this.pageActive === false
    const introductionGeneration = this.introductionGeneration || 0
    const count = this.mediaPageSize()
    this.mediaLoading = false
    debugLog.info('MINE', 'LOAD_START', { requestId })
    this.setData({ loading: true, loadingMore: false })
    const feeds = ['liked', 'favorite', 'created'].map(key => api.getFeed({ isStale, mode: mediaAssets.mediaMode(key), count }).catch(() => ({ ok: false, feed: [] })))
    Promise.all([api.getAccount({ isStale }), api.getRecipes({ isStale, catalog: true }), ...feeds])
      .then(async ([accountRes, recipeRes, likedFeedRes, favoriteFeedRes, createdFeedRes]) => {
        if (!accountRes.ok || !recipeRes.ok) throw new Error('load failed')
        if (isStale()) return
        const account = Object.assign({}, accountRes.account || {}, {
          followingUsers: accountRes.followingUsers || (accountRes.account || {}).followingUsers || [],
          friendUsers: accountRes.friendUsers || (accountRes.account || {}).friendUsers || []
        })
        const recipes = recipeRes.recipes || []
        let publicRecipes = recipes
        if (recipes.some(recipe => recipe.authorUserId === account.userId && recipe.status && recipe.status !== 'published')) {
          const publicResult = await api.getRecipes({ publicCatalog: true, isStale })
          if (isStale()) return
          if (!publicResult.ok) throw new Error('公开菜谱加载失败')
          publicRecipes = publicResult.recipes || []
        }
        const assetGroups = this.buildAssetGroups(account, recipes, likedFeedRes.feed || [], favoriteFeedRes.feed || [], createdFeedRes.feed || [], publicRecipes)
        const assetHasMore = {}
        const assetErrors = {}
        ;[likedFeedRes, favoriteFeedRes, createdFeedRes].forEach((res, index) => {
          const key = ['liked', 'favorite', 'created'][index]
          assetHasMore[key] = res.ok && (res.feed || []).length >= count
          assetErrors[key] = !res.ok
        })
        this.setData({
          account,
          profile: introductionGeneration === (this.introductionGeneration || 0) ? users.fromServer(accountRes.profile, account) : this.data.profile,
          recipes,
          assetGroups,
          assetHasMore,
          assetErrors,
          loading: false
        }, () => { if (!isStale()) {
          this.refreshAssetItems()
          if (this.data.activeAssetTab === 'review') { reviews.load(this); reviews.poll(this) }
          this.loadCreationCovers(requestId)
        } })
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

  buildAssetGroups(account, recipes, likedFeed, favoriteFeed, createdFeed, publicRecipes = recipes) {
    const liked = mediaAssets.toMediaAssets(likedFeed, '已点赞')
    const favorite = mediaAssets.toMediaAssets(favoriteFeed, '已收藏')
    this.createdMediaAssets = mediaAssets.toMediaAssets(createdFeed, '我的创作', account.userId || '')
    const created = creationAssets.build(recipes, this.createdMediaAssets, account.userId || '', this.recipeToAsset.bind(this))
    const menu = publicRecipes.filter(item => item.inMenu && !(assets.normalize(item).free && (item.systemRecipe || (account.userId && item.authorUserId === account.userId))))
      .sort((a, b) => Number(a.menuPosition) - Number(b.menuPosition))
      .map(item => this.recipeToAsset(item, item.purchased ? '已拥有' : (assets.normalize(item).free ? '免费菜谱' : '')))
    const personal = publicRecipes.filter((item) => !!item.personalizationApplied).map((item) => this.recipeToAsset(item, '已个性化'))
    return { liked, favorite, created, menu, personal }
  },

  recipeToAsset(recipe, badge) {
    const normalized = assets.normalize(recipe)
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
      thumbnailUrl: normalized.thumbnailUrl,
      posterUrl: normalized.posterUrl,
      canBuy: normalized.canBuy,
      canRemove: normalized.canRemove,
      accessible: normalized.accessible,
      priceAmount: normalized.priceAmount,
      priceType: normalized.priceType,
      badge: badge || '',
      personalizationSummary: recipe.personalizationSummary || ''
    }
  },

  refreshAssetItems() {
    const groups = this.data.assetGroups || {}
    const tabs = reviews.tabs(ASSET_TABS, this.data.account, this.data.reviewState)
    const validKeys = tabs.map((item) => item.key)
    const activeAssetTab = validKeys.indexOf(this.data.activeAssetTab) >= 0 ? this.data.activeAssetTab : tabs[0].key
    this.setData({
      assetTabs: tabs,
      activeAssetTab,
      mediaGrid: activeAssetTab !== 'created' && !!mediaAssets.mediaMode(activeAssetTab),
      assetItems: groups[activeAssetTab] || []
    })
  },

  async loadCreationCovers(requestId) {
    const owner = getApp().globalData.userId
    const stale = () => requestId !== this.loadRequestId || this.pageActive === false || owner !== getApp().globalData.userId
    for (const row of (this.data.assetGroups.created || []).slice(0, this.mediaPageSize())) {
      if (stale()) break
      if (!row.privateMedia || !row.mediaId) continue
      try { const file = await privateMedia.get(row.mediaId); if (!stale()) { const index = (this.data.assetGroups.created || []).findIndex(item => item.id === row.id); if (index >= 0) this.setData({ ['assetGroups.created[' + index + '].thumbnailUrl']: file }, () => this.refreshAssetItems()) } } catch (_) {}
    }
  },

  editIntroduction() {
    if (this.data.savingIntroduction) return
    this.setData({ editingIntroduction: true, introductionDraft: this.data.profile.introduction || '开心做大厨' })
  },
  onIntroductionInput(event) { this.setData({ introductionDraft: Array.from(String(event.detail.value || '')).slice(0, 20).join('') }) },
  cancelIntroduction() { if (!this.data.savingIntroduction) this.setData({ editingIntroduction: false }) },
  async saveIntroduction() {
    if (this.data.savingIntroduction || !this.pageActive) return
    const introduction = String(this.data.introductionDraft || '').trim()
    if (Array.from(introduction).length > 20) return wx.showToast({ title: '自我介绍最多20个字', icon: 'none' })
    const generation = this.introductionGeneration = (this.introductionGeneration || 0) + 1
    const isActive = () => this.pageActive && generation === (this.introductionGeneration || 0)
    this.setData({ savingIntroduction: true })
    try {
      const result = actions.requireOk(await api.updateIntroduction(introduction))
      if (isActive()) this.setData({ profile: users.fromServer(Object.assign({}, this.data.profile, result.profile || {}, { introduction }), this.data.account), editingIntroduction: false })
    } catch (error) { if (isActive()) wx.showToast({ title: error.message || '介绍保存失败', icon: 'none' }) }
    finally { if (isActive()) this.setData({ savingIntroduction: false }) }
  },

  openFans() { wx.navigateTo({ url: '/pages/fans/fans' }) },

  async buyAsset(event) {
    const item = this.data.assetItems[Number(event.currentTarget.dataset.index)]
    if (!item || this.data.menuActionId) return
    this.setData({ menuActionId: item.id })
    try { if (await actions.purchase(item, () => this.pageActive)) { if (this.pageActive) this.load() } }
    catch (err) { if (this.pageActive) wx.showToast({ title: err.message || '购买失败', icon: 'none' }) }
    finally { if (this.pageActive) this.setData({ menuActionId: '' }) }
  },

  async removeMenuAsset(event) {
    const item = this.data.assetItems[Number(event.currentTarget.dataset.index)]
    if (!item || this.data.menuActionId) return
    this.setData({ menuActionId: item.id })
    try {
      if (await actions.remove(item, () => this.pageActive)) {
        const app = getApp()
        app.globalData.selectedRecipeIds = (app.globalData.selectedRecipeIds || []).filter(id => id !== item.recipeId)
        if (this.pageActive) this.load()
      }
    } catch (err) { if (this.pageActive) wx.showToast({ title: err.message || '移出失败', icon: 'none' }) }
    finally { if (this.pageActive) this.setData({ menuActionId: '' }) }
  },

  onRecipeCoverError(event) {
    const { id, url } = event.currentTarget.dataset
    const groups = Object.assign({}, this.data.assetGroups)
    ;['menu', 'personal', 'created'].forEach(key => { groups[key] = (groups[key] || []).map(item => item.id === id ? assets.thumbnailFallback(item, url) : item) })
    this.setData({ assetGroups: groups }, () => this.refreshAssetItems())
  },

  switchAssetTab(e) {
    const key = e.currentTarget.dataset.key || ''
    reviews.stop(this)
    if (key === 'review' && !reviews.allowed(this.data.account)) return
    const reviewState = key === 'review' && this.data.activeAssetTab === 'review' ? (this.data.reviewState === 'pending' ? 'processed' : 'pending') : this.data.reviewState
    this.setData({ activeAssetTab: key, reviewState, loadingMore: false }, () => { this.refreshAssetItems(); if (key === 'review') { reviews.load(this); reviews.poll(this) } })
  },

  mediaPageSize() {
    return Math.max(3, Math.min(60, Math.floor(Number(config.MINE_MEDIA_PAGE_SIZE) || 24)))
  },

  onReachBottom() { if (this.data.activeAssetTab === 'review') reviews.load(this, true); else this.loadMoreAssets(false) },

  retryAssetTab() { if (this.data.activeAssetTab === 'review') reviews.load(this); else this.loadMoreAssets(true) },

  async loadMoreAssets(replace) {
    const key = this.data.activeAssetTab
    const mode = mediaAssets.mediaMode(key)
    if (!mode || this.data.loading || this.mediaLoading || (!replace && !this.data.assetHasMore[key])) return
    const requestId = this.loadRequestId
    const isStale = () => requestId !== this.loadRequestId || this.pageActive === false
    const previous = replace ? [] : (this.data.assetGroups[key] || [])
    this.mediaLoading = true
    this.setData({ loadingMore: true })
    try {
      const count = this.mediaPageSize()
      const res = await api.getFeed({ mode, count, excludeIds: previous.map(item => item.videoId), isStale })
      if (isStale()) return
      if (!res.ok) throw new Error(res.message || '分区加载失败')
      const incoming = mediaAssets.toMediaAssets(res.feed, '', key === 'created' ? this.data.account.userId || '' : undefined)
      const previousMedia = replace ? [] : (this.createdMediaAssets || [])
      const combinedMedia = mediaAssets.appendMediaAssets(key === 'created' ? previousMedia : previous, incoming)
      const items = key === 'created' ? creationAssets.build(this.data.recipes, combinedMedia, this.data.account.userId || '', this.recipeToAsset.bind(this)) : combinedMedia
      if (key === 'created') this.createdMediaAssets = combinedMedia
      const groups = Object.assign({}, this.data.assetGroups, { [key]: items })
      const hasMore = Object.assign({}, this.data.assetHasMore, { [key]: (res.feed || []).length >= count && combinedMedia.length > (key === 'created' ? previousMedia.length : previous.length) })
      this.setData({
        assetGroups: groups,
        assetHasMore: hasMore,
        [`assetErrors.${key}`]: false
      }, () => { if (!isStale()) this.refreshAssetItems() })
    } catch (error) {
      if (!isStale() && !api.isStaleError(error)) this.setData({ [`assetErrors.${key}`]: true })
    } finally {
      if (!isStale()) { this.mediaLoading = false; this.setData({ loadingMore: false }) }
    }
  },

  onAssetCoverError(event) {
    const { index, id, tab, url } = event.currentTarget.dataset
    const item = this.data.assetItems[index]
    if (this.data.activeAssetTab !== tab || !item || item.id !== id || item.posterUrl !== url || item.coverFailed) return
    this.setData({ [`assetItems[${index}].coverFailed`]: true, [`assetGroups.${tab}[${index}].coverFailed`]: true })
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
    api.startAdReward()
      .then((res) => {
        if (!res.ok) throw new Error(res.message)
        this.adToken = res.adToken
        this.adRemainingSeconds = Math.max(1, Number(res.requiredSeconds) || 1)
        this.setData({ rewarding: true, adSecondsLeft: this.adRemainingSeconds })
        this.resumeAdCountdown()
      })
      .catch((err) => {
        wx.showToast({ title: err.message || '奖励失败', icon: 'none' })
      })
  },

  resumeAdCountdown() {
    if (!this.pageActive || !this.adToken || this._adCountdownTimer) return
    this._adCountdownTimer = setTimeout(() => {
      this._adCountdownTimer = null
      this.adRemainingSeconds = Math.max(0, (this.adRemainingSeconds || 0) - 1)
      this.setData({ adSecondsLeft: this.adRemainingSeconds })
      if (this.adRemainingSeconds > 0) {
        this.resumeAdCountdown()
        return
      }
      const adToken = this.adToken
      this.adToken = ''
      api.finishAdReward(adToken)
        .then((res) => {
          wx.showToast({ title: res.message || '奖励到账', icon: res.ok ? 'success' : 'none' })
          this.setData({ rewarding: false, adSecondsLeft: 0 })
          this.load()
        })
        .catch((err) => {
          this.setData({ rewarding: false, adSecondsLeft: 0 })
          wx.showToast({ title: err.message || '奖励失败', icon: 'none' })
        })
    }, 1000)
  },

  pauseAdCountdown() {
    if (!this._adCountdownTimer) return
    clearTimeout(this._adCountdownTimer)
    this._adCountdownTimer = null
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
    if (this.data.activeAssetTab === 'review') {
      const row = this.data.assetItems[Number(e.currentTarget.dataset.index)]
      if (row) wx.navigateTo({ url: '/pages/recipeReview/recipeReview?recipeId=' + encodeURIComponent(row.recipeId) + '&reviewId=' + encodeURIComponent(row.id) })
      return
    }
    const index = Number(e.currentTarget.dataset.index)
    const item = this.data.assetItems[index] || {}
    if (this.data.activeAssetTab === 'created' && item.creation && item.recipeId) {
      wx.navigateTo({ url: '/pages/creationEditor/creationEditor?id=' + encodeURIComponent(item.recipeId) })
      return
    }
    if (item.type === 'feed') {
      if (!item.videoId || !item.videoUrl) {
        wx.showToast({ title: '视频暂不可播放', icon: 'none' })
        return
      }
      wx.navigateTo({
        url: `/pages/videoPlayer/videoPlayer?videoId=${encodeURIComponent(item.videoId)}&src=${encodeURIComponent(item.videoUrl)}&poster=${encodeURIComponent(item.posterUrl || '')}&title=${encodeURIComponent(item.title || '')}`
      })
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