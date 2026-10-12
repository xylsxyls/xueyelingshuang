const api = require('../../utils/api')
const assets = require('../../utils/recipeAssets')
const editing = require('../../utils/creationEditing')
const order = require('../../utils/recipeStepOrder')
const RecipeStepDrag = require('../../utils/recipeStepDrag')
const config = require('../../utils/config')
const review = require('../../utils/recipeReview')
const privateMedia = require('../../utils/privateMediaCache')
const creationMedia = require('../../utils/creationMediaEditor')
const newCreation = require('../../utils/newCreation')

Page({
  data: { recipe: null, steps: [], priceType: 'free', priceAmount: '', loading: false, saving: false, error: '', dirty: false,
    taskModes: ['前台操作', '后台等待'], dragging: false, dragId: '', dragLabel: '', dragY: 0, dropBeforeId: '', dropAfterId: '',
    reviewStatus: '', uploading: false, uploadProgress: 0, mediaChanged: false, mediaIdsDraft: [],
    newMode: false, titleDraft: '', subtitleDraft: '', ingredientsDraft: '', categories: assets.CATEGORIES, categoryIndex: 0 },
  onLoad(options) {
    this.recipeId = String(options.id || options.recipeId || '')
    this.owner = getApp().globalData.userId
    this.generation = 0
    this.alive = true
    this.stepDrag = new RecipeStepDrag(this, wx)
    if (options.new === '1') {
      const recipe = newCreation.draft()
      this.recipeId = recipe.id
      this.loaded = true
      this.setData({ newMode: true, recipe, steps: editing.steps(recipe), reviewStatus: '尚未提交', dirty: true })
    }
  },
  onShow() { this.visible = true; if (!this.loaded || this.reloadOnShow) { this.reloadOnShow = false; this.load() } },
  onHide() { this.visible = false; this.stepDrag.cancel() },
  onUnload() { this.alive = false; ++this.generation; this.stepDrag.cancel() },
  onPageScroll(event) { this.stepDrag.onScroll(event.scrollTop) },
  onResize() { if (this.stepDrag.ready) this.stepDrag.cancel() },
  owns(generation) { return this.alive && this.owner && this.owner === getApp().globalData.userId && generation === this.generation },
  current(generation) { return this.visible && this.owns(generation) },
  canEdit() { return this.current(this.generation) && !!this.data.recipe && !this.data.saving && !this.data.loading && !this.data.uploading },
  replaceVideo() { creationMedia.replace(this, 'video') },
  replaceImages() { creationMedia.replace(this, 'photo') },
  onNewInput(event) {
    if (!this.canEdit()) return
    const field = event.currentTarget.dataset.field
    if (!['titleDraft', 'subtitleDraft', 'ingredientsDraft'].includes(field)) return
    this.setData({ [field]: String(event.detail.value || ''), dirty: true })
  },
  onCategory(event) { if (this.canEdit()) this.setData({ categoryIndex: Number(event.detail.value), dirty: true }) },
  async showPrivateCover(recipe, generation) {
    if (!['pending', 'rejected'].includes(recipe.reviewState)) return
    const id = recipe.coverImageId || recipe.videoId || (recipe.mediaIds || [])[0]
    if (!id) return
    try { const file = await privateMedia.get(id); if (this.current(generation)) this.setData({ 'recipe.thumbnailUrl': file }) }
    catch (_) { if (this.current(generation)) this.setData({ 'recipe.thumbnailUrl': '' }) }
  },
  openHistory() { if (this.data.recipe && this.data.recipe.reviewId) wx.navigateTo({ url: '/pages/recipeReview/recipeReview?recipeId=' + encodeURIComponent(this.recipeId) + '&reviewId=' + encodeURIComponent(this.data.recipe.reviewId) }) },
  onDragTouchStart(event) { this.stepDrag.arm(event) },
  onDragStart(event) { this.stepDrag.start(event) },
  onDragMove(event) { this.stepDrag.update(event) },
  onDragEnd(event) { this.stepDrag.finish(event) },
  onDragCancel() { this.stepDrag.cancel() },
  async load() {
    if (this.data.saving) return
    if (!this.recipeId) { this.setData({ loading: false, error: '菜谱不存在，请返回创作列表重新打开' }); return }
    if (!this.owner || this.owner !== getApp().globalData.userId) { this.setData({ loading: false, error: '登录状态已变化，请返回我的页面后重试' }); return }
    const generation = ++this.generation
    this.setData({ loading: true, error: '' })
    try {
      const result = await api.getCreationRecipe(this.recipeId, { isStale: () => !this.current(generation) })
      if (!this.current(generation)) return
      if (!result.ok || !result.recipe || result.recipe.authorUserId !== this.owner) throw new Error(result.message || '只能编辑自己的菜谱')
      this.loaded = true
      this.setData(Object.assign({ recipe: assets.normalize(result.recipe), steps: editing.steps(result.recipe), dirty: false, loading: false, mediaChanged: false, mediaIdsDraft: [], reviewStatus: review.status(result.recipe) }, editing.pricing(result.recipe)))
      this.showPrivateCover(result.recipe, generation)
    } catch (error) { if (this.current(generation)) this.setData({ loading: false, error: editing.loadError(error) }); else if (this.alive && this.visible && generation === this.generation) this.setData({ loading: false, error: '登录状态已变化，请返回我的页面后重试' }) }
  },
  onPriceTypeChange(event) {
    if (!this.canEdit() || this.data.dragging || !['free', 'coin', 'yuanbao'].includes(event.detail.value)) return
    this.setData({ priceType: event.detail.value, dirty: true })
  },
  onPriceInput(event) {
    if (!this.canEdit() || this.data.dragging) return
    this.setData({ priceAmount: String(event.detail.value == null ? '' : event.detail.value), dirty: true })
  },
  onStepInput(event) {
    if (!this.canEdit() || this.data.dragging) return
    const index = Number(event.currentTarget.dataset.index), field = event.currentTarget.dataset.field
    if (!this.data.steps[index] || !['title', 'detail', 'hours', 'minutes', 'seconds'].includes(field)) return
    let value = event.detail.value
    if (['hours', 'minutes', 'seconds'].includes(field)) value = Math.max(0, Math.min(field === 'hours' ? 24 : 59, Math.floor(Number(value) || 0)))
    else value = Array.from(String(value || '')).slice(0, field === 'title' ? 8 : 2000).join('')
    this.setData({ ['steps[' + index + '].' + field]: value, dirty: true })
  },
  onModeChange(event) {
    if (!this.canEdit() || this.data.dragging) return
    const index = Number(event.currentTarget.dataset.index), value = Number(event.detail.value)
    if (!this.data.steps[index] || (value !== 0 && value !== 1)) return
    this.setData({ ['steps[' + index + '].active']: value === 0, dirty: true })
  },
  addStep() {
    if (!this.canEdit() || this.data.dragging) return
    if (this.data.steps.length >= config.CREATION_MAX_STEPS) return wx.showToast({ title: '步骤数量已达上限', icon: 'none' })
    const generation = this.generation
    const steps = this.data.steps.map(step => ({ key: step.id, label: step.index + '. ' + step.title }))
    wx.navigateTo({ url: '/pages/personalization/taskEditor', success: result => {
      if (!result || !result.eventChannel || !this.owns(generation)) return
      result.eventChannel.on('taskCreated', task => {
        if (!this.owns(generation) || this.data.saving || !task || !task.id || this.data.steps.length >= config.CREATION_MAX_STEPS) return
        const items = order.insert(this.data.steps, editing.newStep(this.recipeId, task), task.insertRelativeTo, task.insertPosition)
        if (!items) return wx.showToast({ title: '插入位置已变化，请重新添加', icon: 'none' })
        this.setData({ steps: items, dirty: true })
      })
      result.eventChannel.emit('configureInsertion', { steps })
    } })
  },
  deleteStep(event) {
    if (!this.canEdit() || this.data.dragging) return
    const id = event.currentTarget.dataset.id
    const step = this.data.steps.find(item => item.id === id)
    if (!step) return
    if (this.data.steps.length <= 1) return wx.showToast({ title: '请至少保留一个步骤', icon: 'none' })
    const generation = this.generation
    wx.showModal({ title: '删除这个步骤？', content: '第' + step.index + '步：' + step.title, confirmText: '删除', success: result => {
      if (!result.confirm || !this.current(generation) || this.data.saving || this.data.dragging) return
      const steps = order.remove(this.data.steps, id)
      if (steps) this.setData({ steps, dirty: true })
    } })
  },
  async save() {
    if (!this.canEdit() || this.data.dragging) return
    let body
    try { body = editing.payload(this.data.recipe, this.data.steps, this.data) }
    catch (error) { wx.showToast({ title: error.message, icon: 'none' }); return }
    if (this.data.mediaChanged) body.mediaIds = this.data.mediaIdsDraft.slice()
    const generation = this.generation
    this.setData({ saving: true })
    try {
      const creating = this.data.newMode
      let publication
      if (creating) publication = newCreation.publication(this.data.recipe, body, this.data)
      const result = creating ? await api.publishCustomRecipe(JSON.stringify(publication), { isStale: () => !this.owns(generation) }) : await api.saveCreationSteps(body, { isStale: () => !this.owns(generation) })
      if (!this.owns(generation)) return
      if (!result.ok || !result.recipe) throw new Error(result.message || '保存失败')
      if (creating) {
        body = newCreation.canonicalBody(body, this.recipeId, result.recipe.id)
        this.recipeId = result.recipe.id
        this.setData({ newMode: false })
      }
      const app = getApp()
      app.globalData.recipeStateVersion = Number(app.globalData.recipeStateVersion || 0) + 1
      const savedPrice = editing.pricePayload(editing.pricing(result.recipe))
      if (savedPrice.priceType !== body.priceType || savedPrice.priceAmount !== body.priceAmount) {
        // 旧服务可能保存步骤却忽略定价；保留草稿并采用实际新版本，不能报保存成功。
        this.setData({ recipe: assets.normalize(result.recipe) })
        throw new Error('服务器未保存菜谱定价，请更新 CookServer 并重启后重试；修改仍保留')
      }
      if (!editing.matchesSaved(body, result.recipe)) {
        this.setData({ recipe: assets.normalize(result.recipe) })
        throw new Error('服务器未保存步骤顺序或性质，请更新 CookServer 后重试；修改仍保留')
      }
      if (body.priceType !== 'free' && (!result.recipe.reviewId || result.recipe.reviewId !== result.recipe.currentRevisionId)) {
        this.setData({ recipe: assets.normalize(result.recipe) })
        throw new Error('服务端尚未启用付费审核，请更新 CookServer 后重新查看；提交结果以服务器为准')
      }
      if (body.mediaIds && JSON.stringify(body.mediaIds) !== JSON.stringify(result.recipe.mediaIds || [])) {
        this.setData({ recipe: assets.normalize(result.recipe) })
        this.setData({ error: '服务器未保存新媒体，修改仍保留' })
        return
      }
      if (!this.visible) { this.reloadOnShow = true; return }
      this.setData(Object.assign({ recipe: assets.normalize(result.recipe), steps: editing.steps(result.recipe), dirty: false, error: '', mediaChanged: false, mediaIdsDraft: [], reviewStatus: review.status(result.recipe) }, editing.pricing(result.recipe)))
      this.showPrivateCover(result.recipe, generation)
      wx.showToast({ title: result.message || '创作已保存', icon: result.recipe.reviewState === 'pending' ? 'none' : 'success' })
    } catch (error) {
      if (this.owns(generation) && !api.isStaleError(error)) {
        const message = error.message || '保存失败，修改仍保留'
        this.setData({ error: message })
        if (this.visible) wx.showToast({ title: message, icon: 'none' })
      }
    }
    finally { if (this.alive && generation === this.generation) this.setData({ saving: false }) }
  },
  resetDraft() {
    if (!this.canEdit() || this.data.dragging) return
    const generation = this.generation
    wx.showModal({ title: '放弃未保存的修改？', success: result => { if (result.confirm && this.current(generation)) this.setData(Object.assign({ steps: editing.steps(this.data.recipe), dirty: false, error: '', mediaChanged: false, mediaIdsDraft: [] }, editing.pricing(this.data.recipe))) } })
  },
  openVideo() { if (this.data.dragging) return; const recipe = this.data.recipe; if (this.data.mediaChanged && this.data.mediaIdsDraft.length) wx.navigateTo({ url: '/pages/reviewMedia/reviewMedia?mediaIds=' + encodeURIComponent(JSON.stringify(this.data.mediaIdsDraft)) }); else if (recipe && recipe.reviewId) wx.navigateTo({ url: '/pages/reviewMedia/reviewMedia?recipeId=' + encodeURIComponent(this.recipeId) + '&reviewId=' + encodeURIComponent(recipe.reviewId) }); else if (recipe && recipe.videoId) wx.navigateTo({ url: assets.videoPage(recipe) }); else wx.showToast({ title: '暂未发布原视频', icon: 'none' }) },
  onCoverError(event) { if (this.data.recipe) this.setData({ recipe: assets.thumbnailFallback(this.data.recipe, event.currentTarget.dataset.url) }) }
})