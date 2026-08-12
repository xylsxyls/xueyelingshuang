const api = require('../../utils/api')

Page({
  data: {
    tabs: ['推荐', '精选', '关注', '好友'],
    activeTab: '推荐',
    feed: [],
    currentIndex: 0,
    error: ''
  },

  onShow() {
    this.loadData()
  },

  onTabItemTap() {
    this.loadData()
  },

  loadData() {
    this.setData({ error: '' })
    api.getFeed()
      .then((res) => {
        if (!res.ok) throw new Error(res.message || '加载失败')
        this.setData({
          tabs: res.tabs || this.data.tabs,
          feed: res.feed || []
        })
      })
      .catch(() => {
        this.setData({ error: '连接服务器失败，请确认 CookServer 已在 5207 端口运行。' })
      })
  },

  switchTab(e) {
    this.setData({ activeTab: e.currentTarget.dataset.tab })
  },

  onSwiperChange(e) {
    this.setData({ currentIndex: e.detail.current })
  },

  openRecipe(e) {
    const recipeId = e.currentTarget.dataset.id
    wx.navigateTo({ url: `/pages/recipe/recipe?id=${recipeId}` })
  },

  addToMenu(e) {
    const recipeId = e.currentTarget.dataset.id
    const app = getApp()
    const ids = app.globalData.selectedRecipeIds || []
    if (ids.indexOf(recipeId) < 0) {
      ids.push(recipeId)
      app.globalData.selectedRecipeIds = ids
    }
    wx.showToast({ title: '已加入菜单', icon: 'success' })
  },

  favorite(e) {
    api.toggleFavorite(e.currentTarget.dataset.id)
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