const api = require('../../utils/api')
const review = require('../../utils/recipeReview')
const assets = require('../../utils/recipeAssets')
const media = require('../../utils/privateMediaCache')

Page({
  data: { recipe: null, previous: null, record: {}, steps: [], changes: [], removedSteps: [], history: [], historyHasMore: false, canDecide: false, reason: '', loading: false, deciding: false, error: '' },
  onLoad(options) { this.recipeId = options.recipeId || ''; this.reviewId = options.reviewId || ''; this.owner = getApp().globalData.userId; this.generation = 0 },
  onShow() { this.visible = true; this.load() },
  onHide() { this.visible = false; ++this.generation },
  onUnload() { this.visible = false; ++this.generation },
  stale(generation) { return !this.visible || generation !== this.generation || this.owner !== getApp().globalData.userId },
  async load() {
    if (this.data.deciding) return
    const generation = ++this.generation
    this.setData({ loading: true, error: '' })
    try {
      const result = await api.getRecipeReview(this.recipeId, this.reviewId, { isStale: () => this.stale(generation) })
      if (this.stale(generation)) return
      if (!result.ok || !result.recipe || !result.review) { this.setData({ error: result.message || '审核加载失败' }); return }
      this.recipeId = result.recipe.id; this.reviewId = result.review.id; this.historyCursor = result.historyCursor
      this.setData(Object.assign({ recipe: assets.normalize(result.recipe), previous: result.previousApprovedRecipe ? assets.normalize(result.previousApprovedRecipe) : null, record: result.review, canDecide: !!result.canDecide, historyHasMore: !!result.historyHasMore, reason: '' }, review.presentation(result)))
      const id = result.recipe.coverImageId || result.recipe.videoId || (result.recipe.mediaIds || [])[0]
      if (id) {
        try { const file = await media.get(id); if (!this.stale(generation)) this.setData({ 'recipe.thumbnailUrl': file }) } catch (_) { if (!this.stale(generation)) this.setData({ 'recipe.thumbnailUrl': '' }) }
      }
    } catch (error) { if (!this.stale(generation)) this.setData({ error: error.message || '审核加载失败' }) }
    finally { if (!this.stale(generation)) this.setData({ loading: false }) }
  },
  selectHistory(event) { if (this.data.deciding) return; const item = this.data.history[Number(event.detail.value)]; if (item) { this.reviewId = item.id; this.load() } },
  async moreHistory() {
    if (this.historyBusy || !this.data.historyHasMore) return
    this.historyBusy = true
    const generation = this.generation
    try {
      const result = await api.getRecipeReviewHistory(this.recipeId, this.historyCursor, { isStale: () => this.stale(generation) })
      if (!this.stale(generation) && result.ok) {
        const next = review.presentation({ history: result.items }).history, known = new Set(this.data.history.map(item => item.id))
        this.historyCursor = result.nextCursor
        this.setData({ history: this.data.history.concat(next.filter(item => !known.has(item.id))), historyHasMore: !!result.hasMore })
      }
    } catch (error) { if (!this.stale(generation)) wx.showToast({ title: error.message || '历史加载失败', icon: 'none' }) }
    finally { this.historyBusy = false }
  },
  onReason(event) { if (!this.data.deciding) this.setData({ reason: Array.from(String(event.detail.value || '')).slice(0, 500).join('') }) },
  decide(event) {
    if (!this.data.canDecide || this.data.deciding || !this.visible) return
    const decision = event.currentTarget.dataset.decision, generation = this.generation
    if (!['approved', 'rejected'].includes(decision)) return
    wx.showModal({ title: decision === 'approved' ? '确认通过审核？' : '确认不通过？', content: decision === 'approved' ? '通过后该版本会公开给用户。' : '原因将反馈给作者。', success: async answer => {
      if (!answer.confirm || this.stale(generation) || this.data.deciding) return
      this.setData({ deciding: true })
      try {
        const result = await api.decideRecipeReview(this.reviewId, decision, this.data.reason)
        if (this.stale(generation)) return
        if (result.ok) {
          this.setData({ canDecide: false })
          getApp().globalData.recipeStateVersion = Number(getApp().globalData.recipeStateVersion || 0) + 1
          wx.showModal({ title: '审核结果已保存', content: result.message || '服务器已保存审核结果', showCancel: false, success: () => { if (this.visible) wx.navigateBack() } })
        } else {
          wx.showModal({ title: ['already_processed', 'version_replaced'].includes(result.code) ? '这条投稿已由他人处理或被新版本替代' : '审核未成功', content: result.message || '结果未保存，请重试', showCancel: false })
        }
      } catch (error) { if (!this.stale(generation)) wx.showModal({ title: '审核结果尚未确认', content: '请刷新查看服务器实际结果后再操作。' + (error.message || ''), showCancel: false }) }
      finally { if (generation === this.generation) this.setData({ deciding: false }) }
    } })
  },
  openMedia(event) {
    const recipe = event.currentTarget.dataset.previous ? this.data.previous : this.data.recipe
    if (recipe) wx.navigateTo({ url: '/pages/reviewMedia/reviewMedia?recipeId=' + encodeURIComponent(this.recipeId) + '&reviewId=' + encodeURIComponent(this.reviewId) + '&previous=' + (event.currentTarget.dataset.previous ? '1' : '0') })
  }
})