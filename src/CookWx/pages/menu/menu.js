const api = require('../../utils/api')

Page({
  data: {
    categories: [],
    activeCategory: '最喜欢',
    recipes: [],
    filteredRecipes: [],
    keyword: '',
    selectedMap: {},
    selectedRecipes: [],
    plan: {},
    cartOpen: false
  },

  onShow() {
    this.loadRecipes()
  },

  loadRecipes() {
    api.getRecipes()
      .then((res) => {
        if (!res.ok) throw new Error(res.message)
        const app = getApp()
        const selectedIds = app.globalData.selectedRecipeIds || []
        const selectedMap = {}
        selectedIds.forEach((id) => { selectedMap[id] = true })
        this.setData({
          categories: res.categories || [],
          recipes: res.recipes || [],
          selectedMap
        })
        this.refreshSelected()
        this.applyFilter()
      })
      .catch(() => wx.showToast({ title: '服务器连接失败', icon: 'none' }))
  },

  applyFilter() {
    const { recipes, activeCategory, keyword } = this.data
    const key = (keyword || '').trim()
    const list = recipes.filter((item) => {
      const matchKeyword = !key || item.title.indexOf(key) >= 0 || item.subtitle.indexOf(key) >= 0 || item.category.indexOf(key) >= 0
      let matchCategory = true
      if (activeCategory === '最喜欢') matchCategory = item.favorite
      else if (activeCategory === '购买区') matchCategory = item.owned && !item.systemRecipe
      else if (activeCategory === '创作区') matchCategory = !item.systemRecipe
      else matchCategory = item.category === activeCategory
      return matchKeyword && matchCategory
    })
    this.setData({ filteredRecipes: list })
  },

  selectCategory(e) {
    this.setData({ activeCategory: e.currentTarget.dataset.name }, () => this.applyFilter())
  },

  onSearch(e) {
    this.setData({ keyword: e.detail.value }, () => this.applyFilter())
  },

  addRecipe(e) {
    const id = e.currentTarget.dataset.id
    const selectedMap = Object.assign({}, this.data.selectedMap)
    selectedMap[id] = true
    this.setData({ selectedMap }, () => this.syncSelectedIds())
  },

  removeRecipe(e) {
    const id = e.currentTarget.dataset.id
    const selectedMap = Object.assign({}, this.data.selectedMap)
    delete selectedMap[id]
    this.setData({ selectedMap }, () => this.syncSelectedIds())
  },

  syncSelectedIds() {
    const ids = Object.keys(this.data.selectedMap)
    getApp().globalData.selectedRecipeIds = ids
    this.refreshSelected(false)
    if (ids.length) {
      api.previewPlan(ids)
        .then((res) => {
          if (!res.ok) throw new Error(res.message)
          this.setData({ plan: res })
        })
        .catch((err) => wx.showToast({ title: err.message || '计划生成失败', icon: 'none' }))
    } else {
      this.setData({ plan: {}, cartOpen: false })
    }
  },

  refreshSelected(needPreview) {
    const ids = Object.keys(this.data.selectedMap)
    const selectedRecipes = this.data.recipes.filter((item) => ids.indexOf(item.id) >= 0)
    getApp().globalData.selectedRecipes = selectedRecipes
    this.setData({ selectedRecipes })
    if (needPreview !== false && ids.length) {
      api.previewPlan(ids)
        .then((res) => {
          if (res.ok) this.setData({ plan: res })
        })
        .catch(() => {})
    }
  },

  toggleCart() {
    this.setData({ cartOpen: !this.data.cartOpen })
  },

  openRecipe(e) {
    wx.navigateTo({ url: `/pages/recipe/recipe?id=${e.currentTarget.dataset.id}` })
  },

  openCook() {
    const ids = Object.keys(this.data.selectedMap)
    if (!ids.length) {
      wx.showToast({ title: '先选几道菜', icon: 'none' })
      return
    }
    api.startPlan(ids)
      .then((res) => {
        if (!res.ok) throw new Error(res.message)
        getApp().globalData.lastPlan = res
        wx.navigateTo({ url: '/pages/cook/cook' })
      })
      .catch((err) => wx.showToast({ title: err.message || '开始失败', icon: 'none' }))
  }
})