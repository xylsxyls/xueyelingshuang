Page({
  data: {
    insertionSteps: [],
    insertionIndex: 0,
    insertionPositions: ['之前', '之后'],
    insertionPositionIndex: 1,
    title: '',
    detail: '',
    hours: 0,
    minutes: 0,
    seconds: 0,
    taskModes: ['前台操作', '后台等待'],
    taskModeIndex: 0,
    safetyLevels: ['普通', '注意', '危险'],
    safetyValues: ['normal', 'attention', 'danger'],
    safetyIndex: 0,
    waitModes: ['留在厨房守候', '可以离开厨房'],
    waitValues: ['watch', 'free'],
    waitModeIndex: 0
  },

  onLoad() {
    const channel = this.getOpenerEventChannel()
    channel.on('configureInsertion', config => { const steps = config.steps || []; this.setData({ insertionSteps: steps, insertionIndex: Math.max(0, steps.length - 1) }) })
  },
  onInsertionStepChange(event) { this.setData({ insertionIndex: Math.max(0, Math.min(this.data.insertionSteps.length - 1, Number(event.detail.value) || 0)) }) },
  onInsertionPositionChange(event) { this.setData({ insertionPositionIndex: Number(event.detail.value) === 0 ? 0 : 1 }) },

  onInput(e) {
    const field = e.currentTarget.dataset.field
    if (field !== 'title' && field !== 'detail') return
    const update = {}
    update[field] = e.detail.value || ''
    this.setData(update)
  },

  onDurationInput(e) {
    const field = e.currentTarget.dataset.field
    if (['hours', 'minutes', 'seconds'].indexOf(field) < 0) return
    let value = Math.max(0, Math.floor(Number(e.detail.value || 0)))
    if (field !== 'hours') value = Math.min(59, value)
    const update = {}
    update[field] = value
    this.setData(update)
  },

  onTaskModeChange(e) {
    this.setData({ taskModeIndex: Number(e.detail.value) === 1 ? 1 : 0 })
  },

  onSafetyChange(e) {
    this.setData({ safetyIndex: Math.max(0, Math.min(2, Number(e.detail.value) || 0)) })
  },

  onWaitModeChange(e) {
    this.setData({ waitModeIndex: Number(e.detail.value) === 1 ? 1 : 0 })
  },

  save() {
    const title = (this.data.title || '').trim()
    const durationSeconds = Math.max(0, Number(this.data.hours) || 0) * 3600 +
      Math.max(0, Number(this.data.minutes) || 0) * 60 + Math.max(0, Number(this.data.seconds) || 0)
    if (Array.from(title).length > 8) { wx.showToast({ title: '步骤名称最多8个字', icon: 'none' }); return }
    if (!title) {
      wx.showToast({ title: '请填写步骤名称', icon: 'none' })
      return
    }
    if (durationSeconds <= 0) {
      wx.showToast({ title: '预计用时需要大于零', icon: 'none' })
      return
    }
    const background = this.data.taskModeIndex === 1
    const safetyLevel = this.data.safetyValues[this.data.safetyIndex] || 'normal'
    const canLeaveKitchen = background && this.data.waitModeIndex === 1 && safetyLevel === 'normal'
    const task = {
      id: 'added_' + Date.now().toString(36) + '_' + Math.floor(Math.random() * 10000).toString(36),
      title,
      detail: (this.data.detail || '').trim(),
      durationSeconds,
      taskMode: background ? 'background' : 'foreground',
      active: !background,
      manualSkippable: !background,
      canPause: !background,
      continuesDuringPause: background,
      canLeaveKitchen,
      backgroundWaitMode: background && canLeaveKitchen ? 'free' : 'watch',
      safetyLevel,
      dependencies: [],
      insertRelativeTo: (this.data.insertionSteps[this.data.insertionIndex] || {}).key || '',
      insertPosition: this.data.insertionPositionIndex === 0 ? 'before' : 'after'
    }
    try {
      const channel = this.getOpenerEventChannel()
      channel.emit('taskCreated', task)
      wx.navigateBack()
    } catch (error) {
      wx.showToast({ title: '无法返回个性化页面', icon: 'none' })
    }
  }
})