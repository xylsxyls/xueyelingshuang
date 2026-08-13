const api = require('../../utils/api')

Page({
  data: {
    categories: [],
    activeCategory: '',
    recipes: [],
    filteredRecipes: [],
    keyword: '',
    selectedMap: {},
    selectedRecipes: [],
    plan: {},
    cartOpen: false,
    previewing: false,
    starting: false
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
        const recipes = this.decorateRecipes(res.recipes || [], selectedMap)
        const activeCategory = this.chooseActiveCategory(res.categories || [], recipes)
        this.setData({
          categories: res.categories || [],
          recipes,
          activeCategory,
          selectedMap
        }, () => {
          this.refreshSelected()
          this.applyFilter()
        })
      })
      .catch(() => wx.showToast({ title: '服务器连接失败', icon: 'none' }))
  },

  decorateRecipes(recipes, selectedMap) {
    return recipes.map((item) => {
      const id = item.id || ''
      const priceCoins = Number(item.priceCoins || 0)
      const owned = !!item.owned
      return Object.assign({}, item, {
        id,
        title: item.title || '',
        subtitle: item.subtitle || '',
        category: item.category || '',
        tags: Array.isArray(item.tags) ? item.tags : [],
        selected: !!selectedMap[id],
        owned,
        priceCoins,
        priceText: owned ? '已拥有' : `${priceCoins}金币`
      })
    })
  },

  chooseActiveCategory(categories, recipes) {
    if (this.data.activeCategory && categories.indexOf(this.data.activeCategory) >= 0) {
      return this.data.activeCategory
    }
    const hasFavorite = recipes.some((item) => !!item.favorite)
    if (hasFavorite && categories.indexOf('最喜欢') >= 0) {
      return '最喜欢'
    }
    const category = categories.find((name) => {
      if (name === '最喜欢' || name === '购买区' || name === '创作区') return false
      return recipes.some((item) => item.category === name)
    })
    return category || categories[0] || ''
  },

  applyFilter() {
    const { recipes, activeCategory, keyword } = this.data
    const key = (keyword || '').trim()
    const list = recipes.filter((item) => {
      const title = item.title || ''
      const subtitle = item.subtitle || ''
      const category = item.category || ''
      const matchKeyword = !key || title.indexOf(key) >= 0 || subtitle.indexOf(key) >= 0 || category.indexOf(key) >= 0
      let matchCategory = true
      if (activeCategory === '最喜欢') matchCategory = item.favorite
      else if (activeCategory === '购买区') matchCategory = item.owned && !item.systemRecipe
      else if (activeCategory === '创作区') matchCategory = !item.systemRecipe
      else matchCategory = category === activeCategory
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
    const recipe = this.data.recipes.find((item) => item.id === id)
    if (!recipe || !recipe.owned) {
      wx.navigateTo({ url: `/pages/recipe/recipe?id=${id}` })
      return
    }
    const selectedMap = Object.assign({}, this.data.selectedMap)
    selectedMap[id] = true
    this.setData({ selectedMap }, () => {
      this.refreshRecipeSelectionState()
      this.syncSelectedIds()
    })
  },

  removeRecipe(e) {
    const id = e.currentTarget.dataset.id
    const selectedMap = Object.assign({}, this.data.selectedMap)
    delete selectedMap[id]
    this.setData({ selectedMap }, () => {
      this.refreshRecipeSelectionState()
      this.syncSelectedIds()
    })
  },

  refreshRecipeSelectionState() {
    const selectedMap = this.data.selectedMap
    const recipes = this.data.recipes.map((item) => Object.assign({}, item, {
      selected: !!selectedMap[item.id]
    }))
    this.setData({ recipes }, () => this.applyFilter())
  },

  syncSelectedIds() {
    const ids = Object.keys(this.data.selectedMap)
    getApp().globalData.selectedRecipeIds = ids
    this.refreshSelected(false)
    if (ids.length) {
      this.previewRequestId = (this.previewRequestId || 0) + 1
      const requestId = this.previewRequestId
      this.setData({ previewing: true })
      api.previewPlan(ids)
        .then((res) => {
          if (!res.ok) throw new Error(res.message)
          if (requestId !== this.previewRequestId) return
          this.setData({ plan: res, previewing: false })
        })
        .catch((err) => {
          if (requestId !== this.previewRequestId) return
          this.setData({ previewing: false })
          wx.showToast({ title: err.message || '计划生成失败', icon: 'none' })
        })
    } else {
      this.previewRequestId = (this.previewRequestId || 0) + 1
      this.setData({ plan: {}, cartOpen: false, previewing: false })
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
    if (this.data.starting || this.data.previewing) return
    const ids = Object.keys(this.data.selectedMap)
    if (!ids.length) {
      wx.showToast({ title: '先选几道菜', icon: 'none' })
      return
    }
    this.setData({ starting: true })
    api.startPlan(ids)
      .then((res) => {
        if (!res.ok) throw new Error(res.message)
        getApp().globalData.lastPlan = res
        this.setData({ starting: false })
        wx.navigateTo({ url: '/pages/cook/cook' })
      })
      .catch((err) => {
        this.setData({ starting: false })
        wx.showToast({ title: err.message || '开始失败', icon: 'none' })
      })
  }
})