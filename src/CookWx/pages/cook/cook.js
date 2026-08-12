const time = require('../../utils/time')

Page({
  data: {
    plan: {},
    running: false,
    started: false,
    pausedContinuing: false,
    startAt: 0,
    pausedAt: 0,
    pausedSeconds: 0,
    elapsedSeconds: 0,
    progressPercent: 0,
    currentTask: {},
    visibleTasks: [],
    timelineBars: [],
    timelineWidth: 1200,
    elapsedText: '0:00',
    totalText: '0:00',
    voiceLabel: '女声',
    doneMap: {}
  },

  onLoad() {
    const app = getApp()
    const plan = app.globalData.lastPlan || {}
    this.setData({
      plan,
      voiceLabel: app.globalData.voiceGender === 'male' ? '男声' : '女声'
    }, () => this.rebuildView())
  },

  onUnload() {
    this.clearTicker()
  },

  rebuildView() {
    const plan = this.data.plan
    if (!plan.summary) return
    const totalSeconds = plan.summary.totalSeconds || 1
    const timelineWidth = Math.max(1200, Math.ceil(totalSeconds / 60) * 150)
    const bars = (plan.timeline || []).map((task) => {
      const left = Math.round((task.startSeconds / totalSeconds) * timelineWidth)
      const width = Math.max(64, Math.round((task.durationSeconds / totalSeconds) * timelineWidth))
      return Object.assign({}, task, {
        left,
        width,
        done: !!this.data.doneMap[task.id]
      })
    })
    this.setData({
      timelineWidth,
      timelineBars: bars,
      totalText: time.formatSeconds(totalSeconds)
    })
    this.updateCurrent()
  },

  toggleRunning() {
    if (!this.data.plan.summary) return
    if (!this.data.started) {
      this.setData({
        started: true,
        running: true,
        pausedContinuing: false,
        startAt: Date.now(),
        pausedAt: 0,
        pausedSeconds: 0
      })
      this.startTicker()
      this.remindCurrent(true)
      return
    }

    if (this.data.running) {
      const task = this.data.currentTask || {}
      if (task.id && !task.canPause && !task.continuesDuringPause) {
        wx.showToast({ title: '当前步骤不能暂停', icon: 'none' })
        return
      }
      const pausedContinuing = !!(task.id && task.continuesDuringPause)
      this.setData({ running: false, pausedAt: Date.now(), pausedContinuing })
      if (pausedContinuing) {
        this.startTicker()
        wx.showToast({ title: '计时继续，关键点会提醒', icon: 'none' })
      } else {
        this.clearTicker()
        wx.showToast({ title: '已暂停指导', icon: 'none' })
      }
    } else {
      const extraPause = this.data.pausedContinuing ? 0 : Math.floor((Date.now() - this.data.pausedAt) / 1000)
      this.setData({
        running: true,
        pausedContinuing: false,
        pausedSeconds: this.data.pausedSeconds + Math.max(0, extraPause),
        pausedAt: 0
      })
      this.startTicker()
      this.tick()
    }
  },

  startTicker() {
    this.clearTicker()
    this.ticker = setInterval(() => this.tick(), 1000)
    this.tick()
  },

  clearTicker() {
    if (this.ticker) {
      clearInterval(this.ticker)
      this.ticker = null
    }
  },

  tick() {
    if (!this.data.running && !this.data.pausedContinuing) return
    const plan = this.data.plan
    const totalSeconds = plan.summary.totalSeconds || 0
    let elapsed = Math.floor((Date.now() - this.data.startAt) / 1000) - this.data.pausedSeconds
    elapsed = Math.max(0, Math.min(elapsed, totalSeconds))
    const progressPercent = totalSeconds ? Math.round((elapsed / totalSeconds) * 100) : 0
    this.setData({
      elapsedSeconds: elapsed,
      elapsedText: time.formatSeconds(elapsed),
      progressPercent
    }, () => this.updateCurrent())

    if (elapsed >= totalSeconds) {
      this.clearTicker()
      this.setData({ running: false, pausedContinuing: false })
      wx.showModal({
        title: '完成',
        content: '这顿饭的流程已经走完，可以拍一下成果去发布。',
        confirmText: '秀一秀',
        cancelText: '留在本页',
        success(res) {
          if (res.confirm) wx.switchTab({ url: '/pages/publish/publish' })
        }
      })
    }
  },

  updateCurrent() {
    const timeline = this.data.plan.timeline || []
    const elapsed = this.data.elapsedSeconds || 0
    let current = timeline.find((task) => elapsed >= task.startSeconds && elapsed < task.endSeconds)
    if (!current) {
      current = timeline.find((task) => elapsed < task.startSeconds) || timeline[timeline.length - 1] || {}
    }

    const visibleTasks = timeline
      .filter((task) => task.endSeconds >= Math.max(0, elapsed - 60))
      .slice(0, 8)
      .map((task) => Object.assign({}, task, {
        startText: time.formatSeconds(task.startSeconds),
        durationText: time.formatSeconds(task.durationSeconds),
        done: !!this.data.doneMap[task.id]
      }))

    const changed = current && current.id && this.data.currentTask.id !== current.id
    this.setData({ currentTask: current || {}, visibleTasks }, () => {
      if (this.data.pausedContinuing && current && current.id && !current.continuesDuringPause) {
        this.remindCurrent(true)
        this.clearTicker()
        this.setData({ pausedContinuing: false, pausedAt: Date.now() })
        wx.showModal({
          title: '需要回到厨房',
          content: current.title || '下一步需要你操作。',
          showCancel: false
        })
        this.rebuildBarsOnly()
        return
      }
      if (changed && (this.data.running || this.data.pausedContinuing)) this.remindCurrent(false)
      this.rebuildBarsOnly()
    })
  },

  rebuildBarsOnly() {
    const doneMap = this.data.doneMap
    const elapsed = this.data.elapsedSeconds
    const bars = (this.data.timelineBars || []).map((bar) => Object.assign({}, bar, {
      done: !!doneMap[bar.id] || elapsed >= bar.endSeconds
    }))
    this.setData({ timelineBars: bars })
  },

  remindCurrent(force) {
    const task = this.data.currentTask || {}
    if (!task.id) return
    if (!force && this.lastRemindedTaskId === task.id) return
    this.lastRemindedTaskId = task.id
    wx.vibrateShort({ type: task.safetyLevel === 'danger' ? 'heavy' : 'medium' })
    wx.showToast({
      title: task.title,
      icon: 'none',
      duration: 1800
    })
  },

  markDone() {
    const task = this.data.currentTask
    if (!task || !task.id) return
    const doneMap = Object.assign({}, this.data.doneMap)
    doneMap[task.id] = true
    this.setData({ doneMap }, () => this.updateCurrent())
  },

  skipTask() {
    wx.showModal({
      title: '跳过当前步骤？',
      content: '新手不建议跳过火候和安全相关步骤。',
      success: (res) => {
        if (!res.confirm) return
        this.markDone()
      }
    })
  },

  openVoice() {
    wx.navigateTo({ url: '/pages/voice/voice' })
  },

  backMenu() {
    wx.switchTab({ url: '/pages/menu/menu' })
  }
})