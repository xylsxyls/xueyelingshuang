const api = require('../../utils/api')
const time = require('../../utils/time')

Page({
  data: {
    recipeId: '',
    recipe: null,
    steps: [],
    addedTasks: [],
    loading: false,
    saving: false,
    error: ''
  },

  onLoad(options) {
    this.setData({ recipeId: options.id || options.recipeId || '' })
  },

  onShow() {
    this.load()
  },

  load() {
    const recipeId = this.data.recipeId
    if (!recipeId) {
      this.setData({ error: '菜谱不存在' })
      return
    }
    this.loadRequestId = (this.loadRequestId || 0) + 1
    const requestId = this.loadRequestId
    this.setData({ loading: true, error: '' })
    api.getPersonalizationDetail(recipeId)
      .then((res) => {
        if (requestId !== this.loadRequestId) return
        if (!res.ok) throw new Error(res.message || '加载失败')
        const personalization = res.personalization || {}
        this.setData({
          recipe: res.recipe || null,
          steps: this.buildSteps(res.recipe || {}, personalization),
          addedTasks: Array.isArray(personalization.addedTasks) ? personalization.addedTasks : [],
          loading: false
        })
      })
      .catch((err) => {
        if (requestId !== this.loadRequestId) return
        this.setData({
          loading: false,
          error: err && err.message ? err.message : '个性化加载失败'
        })
        wx.showToast({ title: '个性化加载失败', icon: 'none' })
      })
  },

  buildSteps(recipe, personalization) {
    const disabledIds = Array.isArray(personalization.disabledTaskIds) ? personalization.disabledTaskIds : []
    const overrides = Array.isArray(personalization.durationOverrides) ? personalization.durationOverrides : []
    const tasks = Array.isArray(recipe.tasks) ? recipe.tasks : []
    return tasks.map((task, index) => {
      const override = overrides.find((item) => this.taskIdMatches(task, item.taskId))
      const originalSeconds = Math.max(0, Math.floor(Number(task.durationSeconds || 0)))
      const durationSeconds = override ? Math.max(0, Math.floor(Number(override.durationSeconds || 0))) : originalSeconds
      return Object.assign({}, task, this.secondsToFields(durationSeconds), {
        index: index + 1,
        originalDurationSeconds: originalSeconds,
        durationSeconds,
        disabled: disabledIds.some((id) => this.taskIdMatches(task, id)),
        durationText: time.formatDurationText(durationSeconds),
        originalDurationText: time.formatDurationText(originalSeconds),
        modeText: task.active || task.taskMode === 'foreground' ? '前台操作' : (task.canLeaveKitchen ? '可离开等待' : '厨房守候')
      })
    })
  },

  taskIdMatches(task, id) {
    if (!task || !id) return false
    const text = String(id)
    if (task.id === text || task.shortId === text) return true
    return !!(task.id && task.id.indexOf(`.${text}`) === task.id.length - text.length - 1)
  },

  secondsToFields(seconds) {
    const safeSeconds = Math.max(0, Math.floor(Number(seconds || 0)))
    return {
      hours: Math.floor(safeSeconds / 3600),
      minutes: Math.floor((safeSeconds % 3600) / 60),
      seconds: safeSeconds % 60
    }
  },

  fieldsToSeconds(step) {
    const hours = Math.max(0, Math.floor(Number(step.hours || 0)))
    const minutes = Math.max(0, Math.min(59, Math.floor(Number(step.minutes || 0))))
    const seconds = Math.max(0, Math.min(59, Math.floor(Number(step.seconds || 0))))
    return hours * 3600 + minutes * 60 + seconds
  },

  toggleDisabled(e) {
    const index = Number(e.currentTarget.dataset.index)
    const steps = this.data.steps.slice()
    if (!steps[index]) return
    steps[index] = Object.assign({}, steps[index], { disabled: !steps[index].disabled })
    this.setData({ steps })
  },

  onDurationInput(e) {
    const index = Number(e.currentTarget.dataset.index)
    const field = e.currentTarget.dataset.field
    const steps = this.data.steps.slice()
    const step = steps[index]
    if (!step || ['hours', 'minutes', 'seconds'].indexOf(field) < 0) return
    let value = Math.max(0, Math.floor(Number(e.detail.value || 0)))
    if (field !== 'hours') value = Math.min(59, value)
    const nextStep = Object.assign({}, step, { [field]: value })
    nextStep.durationSeconds = this.fieldsToSeconds(nextStep)
    nextStep.durationText = time.formatDurationText(nextStep.durationSeconds)
    steps[index] = nextStep
    this.setData({ steps })
  },

  save() {
    if (this.data.saving || !this.data.recipeId) return
    const disabledTaskIds = []
    const durationOverrides = []
    this.data.steps.forEach((step) => {
      const taskId = step.shortId || step.id
      if (!taskId) return
      if (step.disabled) {
        disabledTaskIds.push(taskId)
        return
      }
      const durationSeconds = this.fieldsToSeconds(step)
      if (durationSeconds !== step.originalDurationSeconds) {
        durationOverrides.push({ taskId, durationSeconds })
      }
    })
    this.setData({ saving: true })
    api.savePersonalization({
      recipeId: this.data.recipeId,
      disabledTaskIds,
      durationOverrides,
      addedTasks: this.data.addedTasks || []
    })
      .then((res) => {
        this.setData({ saving: false })
        wx.showToast({ title: res.message || '已保存', icon: res.ok ? 'success' : 'none' })
        if (res.ok) this.load()
      })
      .catch((err) => {
        this.setData({ saving: false })
        wx.showToast({ title: err && err.message ? err.message : '保存失败', icon: 'none' })
      })
  },

  reset() {
    if (!this.data.recipeId) return
    wx.showModal({
      title: '重置个性化？',
      content: '会恢复这道菜的原始步骤和原始时间。',
      confirmText: '重置',
      success: (res) => {
        if (!res.confirm) return
        api.resetPersonalization(this.data.recipeId)
          .then((resetRes) => {
            wx.showToast({ title: resetRes.message || '已重置', icon: resetRes.ok ? 'success' : 'none' })
            this.load()
          })
          .catch(() => wx.showToast({ title: '重置失败', icon: 'none' }))
      }
    })
  },

  addStep() {
    wx.showToast({ title: '新增步骤下一版接入', icon: 'none' })
  }
})
