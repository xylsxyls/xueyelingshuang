const api = require('../../utils/api')

Page({
  data: {
    tabs: ['推荐', '精选', '关注', '好友'],
    activeTab: '推荐',
    feed: [],
    currentIndex: 0,
    error: '',
    loading: false,
    buyingRecipeId: ''
  },

  onShow() {
    this.loadData()
  },

  onTabItemTap() {
    this.loadData()
  },

  loadData() {
    this.loadRequestId = (this.loadRequestId || 0) + 1
    const requestId = this.loadRequestId
    this.setData({ error: '', loading: true })
    api.getFeed()
      .then((res) => {
        if (!res.ok) throw new Error(res.message || '加载失败')
        if (requestId !== this.loadRequestId) return
        this.setData({
          tabs: res.tabs || this.data.tabs,
          feed: this.normalizeFeed(res.feed || []),
          loading: false
        })
      })
      .catch(() => {
        if (requestId !== this.loadRequestId) return
        this.setData({
          error: '连接服务器失败，请确认 CookServer 已在 5207 端口运行。',
          loading: false
        })
      })
  },

  normalizeFeed(feed) {
    return feed.map((item) => {
      const recipeIds = Array.isArray(item.recipeIds) ? item.recipeIds.filter((id) => !!id) : []
      const primaryRecipeId = recipeIds.length ? recipeIds[0] : ''
      const priceCoins = Number(item.priceCoins || 0)
      const owned = !!item.owned
      return Object.assign({}, item, {
        recipeIds,
        primaryRecipeId,
        priceCoins,
        owned,
        favorite: !!item.favorite,
        actionText: owned ? '查看菜谱' : `购买菜谱 ${priceCoins}金币`,
        menuText: recipeIds.length > 1 ? '加入套餐' : '加入菜单',
        canAddToMenu: owned && recipeIds.length > 0
      })
    })
  },

  switchTab(e) {
    this.setData({
      activeTab: e.currentTarget.dataset.tab,
      currentIndex: 0
    })
  },

  onSwiperChange(e) {
    this.setData({ currentIndex: e.detail.current })
  },

  openRecipe(e) {
    const index = Number(e.currentTarget.dataset.index)
    const item = this.data.feed[index] || {}
    const recipeId = item.primaryRecipeId
    if (!recipeId) {
      wx.showToast({ title: '菜谱不存在', icon: 'none' })
      return
    }
    wx.navigateTo({ url: `/pages/recipe/recipe?id=${recipeId}` })
  },

  addToMenu(e) {
    const index = Number(e.currentTarget.dataset.index)
    const item = this.data.feed[index] || {}
    const recipeIds = item.recipeIds || []
    if (!item.owned) {
      wx.showToast({ title: '请先购买菜谱', icon: 'none' })
      return
    }
    const app = getApp()
    const ids = app.globalData.selectedRecipeIds || []
    let added = 0
    recipeIds.forEach((recipeId) => {
      if (recipeId && ids.indexOf(recipeId) < 0) {
        ids.push(recipeId)
        added += 1
      }
    })
    app.globalData.selectedRecipeIds = ids
    wx.showToast({ title: added ? '已加入菜单' : '已在菜单中', icon: added ? 'success' : 'none' })
  },

  handleRecipeAction(e) {
    const index = Number(e.currentTarget.dataset.index)
    const item = this.data.feed[index] || {}
    if (!item.primaryRecipeId) {
      wx.showToast({ title: '菜谱不存在', icon: 'none' })
      return
    }
    if (item.owned) {
      this.openRecipe(e)
      return
    }
    if (this.data.buyingRecipeId) return
    wx.showModal({
      title: '购买菜谱',
      content: `确认花费 ${item.priceCoins} 金币购买《${item.title}》吗？`,
      confirmText: '购买',
      success: (modalRes) => {
        if (!modalRes.confirm) return
        this.setData({ buyingRecipeId: item.primaryRecipeId })
        api.purchaseRecipe(item.primaryRecipeId)
          .then((res) => {
            wx.showToast({ title: res.message || (res.ok ? '购买成功' : '购买失败'), icon: res.ok ? 'success' : 'none' })
            this.setData({ buyingRecipeId: '' })
            this.loadData()
          })
          .catch((err) => {
            wx.showToast({ title: err.message || '购买失败', icon: 'none' })
            this.setData({ buyingRecipeId: '' })
          })
      }
    })
  },

  favorite(e) {
    const index = Number(e.currentTarget.dataset.index)
    const item = this.data.feed[index] || {}
    if (!item.primaryRecipeId) {
      wx.showToast({ title: '菜谱不存在', icon: 'none' })
      return
    }
    api.toggleFavorite(item.primaryRecipeId)
      .then((res) => {
        if (!res.ok) throw new Error(res.message)
        wx.showToast({ title: res.favorite ? '已收藏' : '已取消', icon: 'none' })
        this.loadData()
      })
      .catch(() => wx.showToast({ title: '收藏失败', icon: 'none' }))
  },

  toggleLike() {
    wx.showToast({ title: '已记录喜欢', icon: 'none' })
  },

  openComments() {
    wx.showToast({ title: '评论区稍后接入', icon: 'none' })
  }
})