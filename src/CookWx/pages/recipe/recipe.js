const api = require('../../utils/api')

Page({
  data: {
    recipeId: '',
    recipe: null
  },

  onLoad(options) {
    this.setData({ recipeId: options.id || '' })
  },

  onShow() {
    this.loadRecipe()
  },

  loadRecipe() {
    api.getRecipes()
      .then((res) => {
        if (!res.ok) throw new Error(res.message)
        const recipe = (res.recipes || []).find((item) => item.id === this.data.recipeId)
        this.setData({ recipe })
      })
      .catch(() => wx.showToast({ title: '菜谱加载失败', icon: 'none' }))
  },

  purchase() {
    const recipe = this.data.recipe
    if (!recipe) return
    api.purchaseRecipe(recipe.id)
      .then((res) => {
        wx.showToast({ title: res.message || (res.ok ? '购买成功' : '购买失败'), icon: res.ok ? 'success' : 'none' })
        this.loadRecipe()
      })
      .catch(() => wx.showToast({ title: '购买失败', icon: 'none' }))
  },

  favorite() {
    const recipe = this.data.recipe
    if (!recipe) return
    api.toggleFavorite(recipe.id)
      .then(() => this.loadRecipe())
      .catch(() => wx.showToast({ title: '收藏失败', icon: 'none' }))
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
    }
    wx.showToast({ title: '已加入菜单', icon: 'success' })
  }
})