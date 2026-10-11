const api = require('../../utils/api')
const time = require('../../utils/time')
const assets = require('../../utils/recipeAssets')
const actions = require('../../utils/menuActions')
const selection = require('../../utils/mealSelection')

Page({
  data: { recipeId: '', recipe: null, mealSelection: false, selectedForMeal: false, loading: false, buying: false, joining: false, error: '' },
  onLoad(options) { this.setData({ recipeId: options.id || '', mealSelection: options.selection === 'meal' }) },
  onShow() { this.pageActive = true; this.setData({ buying: false, joining: false }); this.refreshMealSelection(); this.loadRecipe() },
  onHide() { this.invalidate() },
  onUnload() { this.invalidate() },
  invalidate() { this.pageActive = false; this.loadRequestId = (this.loadRequestId || 0) + 1 },
  refreshMealSelection() { this.setData({ selectedForMeal: this.data.mealSelection && selection.has(getApp(), this.data.recipeId) }) },
  loadRecipe() {
    if (!this.data.recipeId) return this.setData({ recipe: null, error: '菜谱不存在' })
    const requestId = this.loadRequestId = (this.loadRequestId || 0) + 1
    const isStale = () => !this.pageActive || requestId !== this.loadRequestId
    this.setData({ loading: true, error: '' })
    api.getRecipeDetail(this.data.recipeId, { isStale }).then(res => {
      actions.requireOk(res)
      if (!isStale()) {
        this.setData({ recipe: this.normalizeRecipe(res.recipe), loading: false })
        this.refreshMealSelection()
      }
    }).catch(err => { if (!isStale()) this.setData({ loading: false, error: err.message || '菜谱加载失败' }) })
  },
  normalizeRecipe(recipe) {
    const item = assets.normalize(recipe)
    return Object.assign(item, {
      ingredients: item.ingredients || [], tools: item.tools || [], tags: item.tags || [],
      priceLabel: item.free ? '免费' : (item.priceType === 'yuanbao' ? '元宝' : '金币'),
      purchaseText: `购买 ${item.priceText}`,
      tasks: (item.tasks || []).map(task => Object.assign({}, task, { durationText: time.formatDurationText(task.durationSeconds), detailText: task.detail || '' }))
    })
  },
  async purchase() {
    if (this.data.buying) return
    this.setData({ buying: true })
    try {
      const result = await actions.purchase(this.data.recipe, () => this.pageActive)
      if (result && this.pageActive) this.loadRecipe()
    } catch (err) { if (this.pageActive) wx.showToast({ title: err.message || '购买失败', icon: 'none' }) }
    finally { if (this.pageActive) this.setData({ buying: false }) }
  },
  async addToMenu() {
    const recipe = this.data.recipe
    if (!recipe || !this.pageActive || this.data.joining) return
    if (this.data.mealSelection) {
      if (!recipe.accessible) return wx.showToast({ title: '请先购买菜谱', icon: 'none' })
      const added = selection.add(getApp(), recipe)
      this.refreshMealSelection()
      if (added) wx.showToast({ title: '已加入本次做菜列表', icon: 'success' })
      return
    }
    if (recipe.inMenu) return
    const requestId = this.loadRequestId = (this.loadRequestId || 0) + 1
    const isActive = () => this.pageActive && requestId === this.loadRequestId && this.data.recipe && this.data.recipe.id === recipe.id
    this.setData({ joining: true, loading: false })
    try {
      await actions.join(recipe.id)
      if (isActive()) { this.setData({ 'recipe.inMenu': true }); wx.showToast({ title: '已加入菜单', icon: 'success' }) }
    } catch (err) { if (isActive()) wx.showToast({ title: err.message || '加入失败', icon: 'none' }) }
    finally { if (isActive()) this.setData({ joining: false }) }
  },
  openPersonalization() {
    const recipe = this.data.recipe
    if (recipe && recipe.accessible) wx.navigateTo({ url: `/pages/personalization/personalization?id=${encodeURIComponent(recipe.id)}` })
  },
  openVideo() {
    const recipe = this.data.recipe
    if (recipe && recipe.videoId) wx.navigateTo({ url: assets.videoPage(recipe) })
    else wx.showToast({ title: '暂未发布原视频', icon: 'none' })
  },
  onCoverError(event) {
    const recipe = this.data.recipe
    if (recipe) this.setData({ recipe: assets.thumbnailFallback(recipe, event.currentTarget.dataset.url) })
  }
})