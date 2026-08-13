const api = require('../../utils/api')

Page({
  data: {
    account: {},
    recipes: [],
    purchased: []
  },

  onShow() {
    this.load()
  },

  load() {
    Promise.all([api.getAccount(), api.getRecipes()])
      .then(([accountRes, recipeRes]) => {
        if (!accountRes.ok || !recipeRes.ok) throw new Error('load failed')
        const account = accountRes.account || {}
        const purchasedIds = Array.isArray(account.purchasedRecipeIds) ? account.purchasedRecipeIds : []
        const purchased = (recipeRes.recipes || []).filter((item) => purchasedIds.indexOf(item.id) >= 0)
        this.setData({
          account,
          recipes: recipeRes.recipes || [],
          purchased
        })
      })
      .catch(() => wx.showToast({ title: '账户加载失败', icon: 'none' }))
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

  openRecipe(e) {
    wx.navigateTo({ url: `/pages/recipe/recipe?id=${e.currentTarget.dataset.id}` })
  }
})
