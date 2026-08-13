const api = require('../../utils/api')

Page({
  data: {
    recipeId: '',
    recipe: null,
    loading: false,
    buying: false,
    favoriting: false,
    error: ''
  },

  onLoad(options) {
    this.setData({ recipeId: options.id || '' })
  },

  onShow() {
    this.loadRecipe()
  },

  loadRecipe() {
    if (!this.data.recipeId) {
      this.setData({ recipe: null, error: '菜谱不存在' })
      return
    }
    this.loadRequestId = (this.loadRequestId || 0) + 1
    const requestId = this.loadRequestId
    this.setData({ loading: true, error: '' })
    api.getRecipes()
      .then((res) => {
        if (!res.ok) throw new Error(res.message)
        if (requestId !== this.loadRequestId) return
        const recipe = (res.recipes || []).find((item) => item.id === this.data.recipeId)
        this.setData({
          recipe: recipe ? this.normalizeRecipe(recipe) : null,
          loading: false,
          error: recipe ? '' : '菜谱不存在'
        })
      })
      .catch(() => {
        if (requestId !== this.loadRequestId) return
        this.setData({ loading: false, error: '菜谱加载失败' })
        wx.showToast({ title: '菜谱加载失败', icon: 'none' })
      })
  },

  normalizeRecipe(recipe) {
    return Object.assign({}, recipe, {
      ingredients: Array.isArray(recipe.ingredients) ? recipe.ingredients : [],
      tools: Array.isArray(recipe.tools) ? recipe.tools : [],
      tags: Array.isArray(recipe.tags) ? recipe.tags : [],
      priceCoins: Number(recipe.priceCoins || 0),
      owned: !!recipe.owned,
      favorite: !!recipe.favorite
    })
  },

  purchase() {
    const recipe = this.data.recipe
    if (!recipe || this.data.buying) return
    if (recipe.owned) {
      wx.showToast({ title: '已经拥有菜谱', icon: 'none' })
      return
    }
    this.setData({ buying: true })
    api.purchaseRecipe(recipe.id)
      .then((res) => {
        wx.showToast({ title: res.message || (res.ok ? '购买成功' : '购买失败'), icon: res.ok ? 'success' : 'none' })
        this.setData({
          buying: false,
          recipe: res.recipe ? this.normalizeRecipe(res.recipe) : recipe
        })
        if (!res.recipe) this.loadRecipe()
      })
      .catch((err) => {
        this.setData({ buying: false })
        wx.showToast({ title: err.message || '购买失败', icon: 'none' })
      })
  },

  favorite() {
    const recipe = this.data.recipe
    if (!recipe || this.data.favoriting) return
    this.setData({ favoriting: true })
    api.toggleFavorite(recipe.id)
      .then((res) => {
        if (!res.ok) throw new Error(res.message)
        this.setData({
          favoriting: false,
          recipe: res.recipe ? this.normalizeRecipe(res.recipe) : Object.assign({}, recipe, { favorite: !!res.favorite })
        })
      })
      .catch((err) => {
        this.setData({ favoriting: false })
        wx.showToast({ title: err.message || '收藏失败', icon: 'none' })
      })
  },

  addToMenu() {
    const recipe = this.data.recipe
    if (!recipe || !recipe.owned) {
      wx.showToast({ title: '请先购买菜谱', icon: 'none' })
      return
    }
    const app = getApp()
    const ids = app.globalData.selectedRecipeIds || []
    if (ids.indexOf(recipe.id) < 0) {
      ids.push(recipe.id)
      app.globalData.selectedRecipeIds = ids
      wx.showToast({ title: '已加入菜单', icon: 'success' })
      return
    }
    wx.showToast({ title: '已在菜单中', icon: 'none' })
  }
})