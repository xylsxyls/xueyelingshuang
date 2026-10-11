const api = require('../../utils/api')
const assets = require('../../utils/recipeAssets')
const time = require('../../utils/time')
const flow = require('../../utils/personalizationFlow')

Page({
  data: { recipeId: '', recipe: null, steps: [], loading: false, saving: false, error: '' },
  onLoad(options) { this.setData({ recipeId: options.id || options.recipeId || '' }) },
  onShow() { this.pageActive = true; this.setData({ saving: false }); if (!this._loadedOnce) this.load() },
  onHide() { this.pageActive = false; this.loadRequestId = (this.loadRequestId || 0) + 1 },
  onUnload() { this.unloaded = true; this.onHide() },
  load() {
    if (!this.data.recipeId) return this.setData({ error: '菜谱不存在' })
    const requestId = this.loadRequestId = (this.loadRequestId || 0) + 1
    const isStale = () => requestId !== this.loadRequestId || !this.pageActive
    this.setData({ loading: true, error: '' })
    api.getPersonalizationDetail(this.data.recipeId, { isStale }).then(result => {
      if (isStale()) return
      if (!result.ok) throw new Error(result.message || '加载失败')
      this.setData({ recipe: assets.normalize(result.recipe), steps: flow.merge(result.recipe || {}, result.personalization, time.formatDurationText), loading: false })
      this._loadedOnce = true
    }).catch(error => { if (!isStale()) this.setData({ loading: false, error: error.message || '个性化加载失败' }) })
  },
  buildSteps(recipe, personalization) { return flow.merge(recipe, personalization, time.formatDurationText) },
  taskIdMatches(task, id) { return !!id && flow.key(task) === String(id).split('.').pop() },
  secondsToFields(value) { return flow.fields(value) },
  fieldsToSeconds(step) { return flow.seconds(step) },
  toggleDisabled(event) {
    if (this.data.saving) return
    const index = Number(event.currentTarget.dataset.index), steps = this.data.steps.slice()
    if (!steps[index] || steps[index].added) return
    steps[index] = Object.assign({}, steps[index], { disabled: !steps[index].disabled }); this.setData({ steps })
  },
  onDurationInput(event) {
    if (this.data.saving) return
    const index = Number(event.currentTarget.dataset.index), field = event.currentTarget.dataset.field, steps = this.data.steps.slice()
    if (!steps[index] || ['hours', 'minutes', 'seconds'].indexOf(field) < 0) return
    const value = Math.max(0, Math.floor(Number(event.detail.value) || 0))
    const step = Object.assign({}, steps[index], { [field]: field === 'hours' ? value : Math.min(59, value) })
    step.durationSeconds = flow.seconds(step); step.durationText = time.formatDurationText(step.durationSeconds)
    steps[index] = step; this.setData({ steps })
  },
  async save() {
    if (this.data.saving || !this.pageActive || !this.data.recipeId) return
    const requestId = this.loadRequestId || 0
    this.setData({ saving: true })
    try {
      const result = await api.savePersonalization(flow.payload(this.data.recipeId, this.data.steps))
      if (!this.pageActive || requestId !== this.loadRequestId) return
      wx.showToast({ title: result.message || (result.ok ? '已保存' : '保存失败'), icon: result.ok ? 'success' : 'none' })
      if (result.ok) { this.setData({ saving: false }); this.load() }
    } catch (error) { if (this.pageActive && requestId === this.loadRequestId) wx.showToast({ title: error.message || '保存失败', icon: 'none' }) }
    finally { if (this.pageActive && requestId === this.loadRequestId) this.setData({ saving: false }) }
  },
  reset() {
    if (this.data.saving) return
    const requestId = this.loadRequestId || 0
    const isStale = () => !this.pageActive || requestId !== this.loadRequestId
    wx.showModal({ title: '重置个性化？', content: '会恢复这道菜的原始步骤和原始时间。', confirmText: '重置', success: async result => {
      if (!result.confirm || isStale()) return
      this.setData({ saving: true })
      try { const response = await api.resetPersonalization(this.data.recipeId); if (isStale()) return; wx.showToast({ title: response.message || '已重置', icon: response.ok ? 'success' : 'none' }); if (response.ok) { this.setData({ saving: false }); this.load() } }
      catch (error) { if (!isStale()) wx.showToast({ title: error.message || '重置失败', icon: 'none' }) }
      finally { if (!isStale()) this.setData({ saving: false }) }
    } })
  },
  addStep() {
    if (this.data.saving) return
    const steps = this.data.steps.map(step => ({ key: flow.key(step), label: step.index + '. ' + step.title }))
    wx.navigateTo({ url: '/pages/personalization/taskEditor', success: result => {
      if (!result || !result.eventChannel) return
      result.eventChannel.on('taskCreated', task => {
        if (this.unloaded || !task || !task.id || !task.title) return
        const items = this.data.steps.slice()
        const anchor = items.findIndex(item => flow.key(item) === task.insertRelativeTo)
        if (items.length && anchor < 0) return wx.showToast({ title: '插入位置已变化，请重新添加', icon: 'none' })
        const index = anchor < 0 ? 0 : anchor + (task.insertPosition === 'before' ? 0 : 1)
        const created = flow.merge({ tasks: [] }, { addedTasks: [task] }, time.formatDurationText)[0]
        items.splice(index, 0, created); this.setData({ steps: flow.renumber(items) })
      })
      result.eventChannel.emit('configureInsertion', { steps })
    } })
  },
  removeAddedTask(event) {
    if (this.data.saving) return
    const index = Number(event.currentTarget.dataset.index)
    if (!this.data.steps[index] || !this.data.steps[index].added) return
    this.setData({ steps: flow.renumber(this.data.steps.filter((_, current) => current !== index)) })
  },
  openVideo() { const recipe = this.data.recipe; if (recipe && recipe.videoId) wx.navigateTo({ url: assets.videoPage(recipe) }); else wx.showToast({ title: '暂未发布原视频', icon: 'none' }) },
  onCoverError(event) { if (this.data.recipe) this.setData({ recipe: assets.thumbnailFallback(this.data.recipe, event.currentTarget.dataset.url) }) }
})