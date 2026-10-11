const time = require('../../utils/time')
const api = require('../../utils/api')
const planRuntime = require('../../utils/planRuntime')
const voice = require('../../utils/voice')
const cookVoiceText = require('../../utils/cookVoiceText')
const debugLog = require('../../utils/debugLog')
const { createCookExitAlert } = require('../../utils/cookExitAlert')
const personalizationSync = require('../../utils/cookPersonalizationSync')

const BACKGROUND_ALARM_FILE_NAME = 'cook_background_end_alarm.wav'
const BACKGROUND_ALARM_SAMPLE_RATE = 8000

function writeAscii(view, offset, text) {
  for (let i = 0; i < text.length; i += 1) {
    view.setUint8(offset + i, text.charCodeAt(i))
  }
}

function buildBackgroundAlarmWavBuffer() {
  const tones = [
    { seconds: 0.18, hz: 1046 },
    { seconds: 0.07, hz: 0 },
    { seconds: 0.18, hz: 1046 },
    { seconds: 0.07, hz: 0 },
    { seconds: 0.22, hz: 1318 }
  ]
  const totalSamples = tones.reduce((sum, item) => sum + Math.max(1, Math.floor(item.seconds * BACKGROUND_ALARM_SAMPLE_RATE)), 0)
  const dataBytes = totalSamples * 2
  const buffer = new ArrayBuffer(44 + dataBytes)
  const view = new DataView(buffer)
  writeAscii(view, 0, 'RIFF')
  view.setUint32(4, 36 + dataBytes, true)
  writeAscii(view, 8, 'WAVE')
  writeAscii(view, 12, 'fmt ')
  view.setUint32(16, 16, true)
  view.setUint16(20, 1, true)
  view.setUint16(22, 1, true)
  view.setUint32(24, BACKGROUND_ALARM_SAMPLE_RATE, true)
  view.setUint32(28, BACKGROUND_ALARM_SAMPLE_RATE * 2, true)
  view.setUint16(32, 2, true)
  view.setUint16(34, 16, true)
  writeAscii(view, 36, 'data')
  view.setUint32(40, dataBytes, true)

  let sampleIndex = 0
  tones.forEach((tone) => {
    const sampleCount = Math.max(1, Math.floor(tone.seconds * BACKGROUND_ALARM_SAMPLE_RATE))
    const fadeSamples = Math.min(80, Math.floor(sampleCount / 5))
    for (let i = 0; i < sampleCount; i += 1) {
      let gain = 1
      if (fadeSamples > 0 && i < fadeSamples) gain = i / fadeSamples
      if (fadeSamples > 0 && i > sampleCount - fadeSamples) gain = Math.max(0, (sampleCount - i) / fadeSamples)
      const sample = tone.hz
        ? Math.round(Math.sin((2 * Math.PI * tone.hz * i) / BACKGROUND_ALARM_SAMPLE_RATE) * 18000 * gain)
        : 0
      view.setInt16(44 + sampleIndex * 2, sample, true)
      sampleIndex += 1
    }
  })
  return buffer
}

Page({
  data: {
    plan: {},
    running: false,
    started: false,
    pausedContinuing: false,
    complete: false,
    finishing: false,
    elapsedSeconds: 0,
    progressPercent: 0,
    currentProgressPercent: 0,
    currentTask: {},
    promptTasks: [],
    visibleTasks: [],
    backgroundTasks: [],
    timelineBars: [],
    timelineWidth: 1200,
    timelinePointerLeft: 0,
    elapsedText: '0:00',
    totalText: '0:00',
    remainingText: '0:00',
    freeText: '0秒',
    currentCountdownText: '',
    completedText: '0/0',
    voiceLabel: '女声'
  },

  onLoad() {
    this.pageActive = true
    const app = getApp()
    const plan = app.globalData.lastPlan || {}
    this.voiceGender = app.globalData.currentCookVoiceGender || voice.getGender()
    this.runtime = plan.summary ? planRuntime.create(plan) : null
    this.syncPersonalization = !!(plan.cookOptions && plan.cookOptions.syncPersonalization)
    this.syncReportQueued = false
    this.cookOwnerId = app.globalData.userId || ''
    this.lastTickAt = 0
    this.lastRemindedTaskId = ''
    this.lastCurrentTaskId = ''
    this.lastRunningBackgroundTaskMap = {}
    this.lastRuntimeAlertElapsedSeconds = 0
    this.backgroundEndAlertedMap = {}
    this.backgroundEndReminderPromise = Promise.resolve()
    this.backgroundAlarmFilePathPromise = null
    this.backgroundAlarmContext = null
    this.pendingBackgroundCurrentReminderToken = ''
    this.finishModalShown = false
    this.completionSharePromptShown = false
    this.unloaded = false
    this.exiting = false
    this.exitAlert = createCookExitAlert(wx, (error) => {
      debugLog.warn('COOK', 'EXIT_ALERT_FAIL', { error: error.message || String(error) })
    })
    this.suppressUnloadRedirect = false
    this.cleanupDone = false
    debugLog.info('COOK', 'LOAD', {
      hasPlan: !!plan.summary,
      totalSeconds: plan.summary && plan.summary.totalSeconds,
      taskCount: Array.isArray(plan.timeline) ? plan.timeline.length : 0
    })
    this.setData({
      plan,
      voiceLabel: voice.genderLabel(this.getCookVoiceGender())
    }, () => this.refreshView(() => {
      this.startBackgroundFixedVoicePrefetch()
    }, { prefetch: false }))
  },

  onShow() {
    if (this.cleanupDone || this.unloaded) return
    this.pageActive = true
    this.setData({
      voiceLabel: voice.genderLabel(this.getCookVoiceGender())
    })
    if (this.data.started && (this.data.running || this.data.pausedContinuing)) {
      this.startTicker()
    }
    if (this.data.complete && !this.completionSharePromptShown) this.presentCompletionPrompts()
  },

  onHide() {
    this.pageActive = false
    this.clearTicker()
  },

  onUnload() {
    this.pageActive = false
    this.captureElapsedTime()
    this.rememberCookSync()
    this.unloaded = true
    if (this.shouldExitCookFlowOnUnload()) {
      debugLog.warn('COOK', 'SYSTEM_BACK_EXIT_CONFIRMED', {
        elapsedSeconds: this.data.elapsedSeconds,
        currentTaskId: this.data.currentTask && this.data.currentTask.id
      })
      this.cleanupCookFlow()
      return
    }
    this.clearTicker()
    this.stopBackgroundAlarm()
    voice.stop()
    this.disableExitAlertBeforeUnload().catch(() => {})
  },

  shouldExitCookFlowOnUnload() {
    return !!(this.data.started && !this.data.complete && !this.suppressUnloadRedirect && !this.cleanupDone)
  },

  enableExitAlertBeforeUnload() {
    if (!this.exitAlert || this.cleanupDone || this.unloaded) return
    this.exitAlert.enable('是否退出做菜流程？退出后当前步骤计时会清空。')
  },

  disableExitAlertBeforeUnload() {
    if (!this.exitAlert) return Promise.resolve()
    return this.exitAlert.disable().catch((error) => {
      debugLog.warn('COOK', 'EXIT_ALERT_DISABLE_FAIL', { error: error.message || String(error) })
      throw error
    })
  },

  refreshView(callback, options) {
    if (this.cleanupDone || this.unloaded || this.exiting) return
    if (!this.runtime) {
      if (callback) callback()
      return
    }
    const view = planRuntime.buildView(this.runtime)
    const currentTask = view.currentTask || {}
    const summary = view.plan.summary || {}
    const freeSeconds = summary.freeSeconds !== undefined ? summary.freeSeconds : (summary.freeMinutes || 0) * 60
    const data = {
      plan: view.plan,
      complete: !!view.isComplete,
      elapsedSeconds: view.elapsedSeconds,
      progressPercent: view.progressPercent,
      currentProgressPercent: currentTask.progressPercent || 0,
      currentTask: this.decorateTask(currentTask),
      promptTasks: (view.promptTasks || []).map((task) => this.decorateTask(task)),
      visibleTasks: view.visibleTasks.map((task) => this.decorateTask(task)),
      backgroundTasks: view.backgroundTasks.map((task) => this.decorateTask(task)),
      timelineBars: view.timelineBars.map((task) => this.decorateTask(task)),
      timelineWidth: view.timelineWidth,
      timelinePointerLeft: view.timelinePointerLeft,
      elapsedText: time.formatSeconds(view.elapsedSeconds),
      totalText: time.formatSeconds(view.totalSeconds),
      remainingText: time.formatSeconds(Math.max(0, view.totalSeconds - view.elapsedSeconds)),
      freeText: time.formatDurationText(freeSeconds),
      currentCountdownText: this.buildCurrentCountdownText(currentTask, view.elapsedSeconds),
      completedText: `${view.completedCount}/${view.totalCount}`
    }
    this.setData(data, () => {
      if (this.cleanupDone || this.unloaded || this.exiting) return
      if (!options || options.alerts !== false) {
        this.handleRuntimeAlerts(view)
      }
      if (callback) callback()
    })
  },

  decorateTask(task) {
    if (!task || !task.id) return task || {}
    const background = !!(task.background || task.taskMode === 'background' || (!task.rest && task.taskMode !== 'foreground' && task.active === false))
    return Object.assign({}, task, {
      background,
      foreground: !background && !task.rest,
      startText: time.formatSeconds(task.startSeconds),
      endText: time.formatSeconds(task.endSeconds),
      durationText: time.formatSeconds(task.durationSeconds),
      remainingText: time.formatSeconds(task.remainingSeconds),
      detailText: task.detailText || this.buildDetailText(task),
      canOperate: !!task.current && !task.done && !task.skipped && !task.rest && !background
    })
  },

  buildDetailText(task) {
    return cookVoiceText.detailWithExpectedTime(task)
  },

  buildCurrentCountdownText(task, elapsedSeconds) {
    if (!task || !task.id) return this.data.started ? '流程已完成' : '点击开始后开始计时'
    if (task.rest) return task.countdownText || `休息等待 ${time.formatDurationText(task.remainingSeconds || 0)}`
    if (task.startSeconds > elapsedSeconds) return `还有 ${time.formatDurationText(task.startSeconds - elapsedSeconds)} 开始`
    return `剩余 ${time.formatDurationText(Math.max(0, task.endSeconds - elapsedSeconds))}`
  },

  handleRuntimeAlerts(view) {
    const current = view.currentTask || {}
    const currentId = current.id || ''
    const changed = currentId && currentId !== this.lastCurrentTaskId
    if (changed) {
      this.lastCurrentTaskId = currentId
    }
    const endedBackgroundTasks = this.collectEndedBackgroundTasks(view)
    const hasBackgroundEndAlert = endedBackgroundTasks.length > 0 && (this.data.running || this.data.pausedContinuing)
    const shouldRemindCurrent = changed && (this.data.running || this.data.pausedContinuing)

    if (this.data.pausedContinuing && currentId && changed && current.active && !current.continuesDuringPause) {
      this.clearTicker()
      this.setData({ running: false, pausedContinuing: false })
      wx.showModal({
        title: '需要回到厨房',
        content: current.title || '下一步需要你操作。',
        showCancel: false
      })
    }

    if (hasBackgroundEndAlert) {
      const reminderPromise = this.remindBackgroundEnded(endedBackgroundTasks)
      if (shouldRemindCurrent) {
        this.queueCurrentReminderAfterBackgroundEnd(currentId, reminderPromise)
      }
    }

    if (shouldRemindCurrent && !hasBackgroundEndAlert) {
      this.remindCurrent(false)
    }

    if (view.isComplete) this.markRuntimeComplete()
  },

  collectEndedBackgroundTasks(view) {
    const runningMap = {}
    const endedTaskMap = {}
    const backgroundTasks = view && view.backgroundTasks ? view.backgroundTasks : []
    const currentElapsedSeconds = view && view.elapsedSeconds !== undefined ? Math.max(0, Math.floor(view.elapsedSeconds || 0)) : 0
    const previousElapsedSeconds = Math.max(0, Math.floor(this.lastRuntimeAlertElapsedSeconds || 0))
    const alertedMap = this.backgroundEndAlertedMap || {}
    const addEndedTask = (task) => {
      if (!task || !task.id || alertedMap[task.id]) return
      endedTaskMap[task.id] = task
      alertedMap[task.id] = true
    }

    backgroundTasks.forEach((task) => {
      if (task && task.id) {
        runningMap[task.id] = task
      }
    })

    const previousMap = this.lastRunningBackgroundTaskMap || {}
    Object.keys(previousMap).forEach((taskId) => {
      const task = previousMap[taskId]
      if (runningMap[taskId] || alertedMap[taskId]) return
      if (view && view.elapsedSeconds < task.endSeconds) return
      addEndedTask(task)
    })

    const timelineTasks = view && view.timelineBars ? view.timelineBars : []
    timelineTasks.forEach((task) => {
      if (!task || !task.id || !task.background || task.skipped || alertedMap[task.id]) return
      const endSeconds = Math.max(0, Math.floor(task.endSeconds || 0))
      if (previousElapsedSeconds < endSeconds && currentElapsedSeconds >= endSeconds) {
        addEndedTask(task)
      }
    })

    this.lastRunningBackgroundTaskMap = runningMap
    this.lastRuntimeAlertElapsedSeconds = currentElapsedSeconds
    this.backgroundEndAlertedMap = alertedMap
    return Object.keys(endedTaskMap).map((taskId) => endedTaskMap[taskId])
  },

  buildBackgroundEndVoiceText(tasks) {
    return cookVoiceText.buildBackgroundEndVoiceText(tasks) || '后台步骤结束。'
  },

  buildBackgroundEndVoiceTexts(tasks) {
    const texts = cookVoiceText.buildBackgroundEndVoiceTextParts(tasks)
    return texts.length ? texts : ['后台步骤结束。']
  },

  remindBackgroundEnded(tasks) {
    const voiceTexts = this.buildBackgroundEndVoiceTexts(tasks)
    const voiceText = this.buildBackgroundEndVoiceText(tasks)
    debugLog.info('COOK_STEP', 'BACKGROUND_ENDED', {
      taskIds: (tasks || []).map((task) => task.id),
      titles: (tasks || []).map((task) => task.title),
      voiceText,
      voiceTexts
    })
    wx.showToast({
      title: voiceText.replace(/。$/, ''),
      icon: 'none',
      duration: 2500
    })
    this.backgroundEndReminderPromise = this.playBackgroundEndAlarm()
      .then(() => {
        if (!this.runtime || this.cleanupDone) return false
        debugLog.info('COOK_STEP', 'BACKGROUND_END_VOICE_START', {
          taskIds: (tasks || []).map((task) => task.id),
          voiceText,
          voiceTexts
        })
        if (voice.speakSequence) {
          return voice.speakSequence(voiceTexts.length ? voiceTexts : [voiceText], this.getCookVoiceGender())
        }
        return voice.speak(voiceText, this.getCookVoiceGender())
      })
      .catch((err) => {
        debugLog.warn('COOK_STEP', 'BACKGROUND_END_VOICE_FAIL', {
          error: err && err.message ? err.message : String(err)
        })
        return false
      })
    return this.backgroundEndReminderPromise
  },

  queueCurrentReminderAfterBackgroundEnd(currentTaskId, reminderPromise) {
    if (!currentTaskId) return
    const token = `${currentTaskId}_${Date.now()}`
    this.pendingBackgroundCurrentReminderToken = token
    debugLog.info('COOK_STEP', 'BACKGROUND_CURRENT_REMIND_QUEUED', {
      taskId: currentTaskId
    })
    Promise.resolve(reminderPromise)
      .then(() => this.remindQueuedCurrentAfterBackgroundEnd(currentTaskId, token))
      .catch((err) => {
        debugLog.warn('COOK_STEP', 'BACKGROUND_CURRENT_REMIND_WAIT_FAIL', {
          taskId: currentTaskId,
          error: err && err.message ? err.message : String(err)
        })
        this.remindQueuedCurrentAfterBackgroundEnd(currentTaskId, token)
      })
  },

  remindQueuedCurrentAfterBackgroundEnd(currentTaskId, token) {
    if (this.pendingBackgroundCurrentReminderToken !== token) return
    this.pendingBackgroundCurrentReminderToken = ''
    if (!this.runtime || this.data.complete || (!this.data.running && !this.data.pausedContinuing)) {
      debugLog.info('COOK_STEP', 'BACKGROUND_CURRENT_REMIND_CANCEL', {
        taskId: currentTaskId,
        reason: 'not_running'
      })
      return
    }
    const current = this.data.currentTask || {}
    if (current.id !== currentTaskId) {
      debugLog.info('COOK_STEP', 'BACKGROUND_CURRENT_REMIND_SKIP', {
        expectedTaskId: currentTaskId,
        currentTaskId: current.id || ''
      })
      return
    }
    debugLog.info('COOK_STEP', 'BACKGROUND_CURRENT_REMIND_PLAY', {
      taskId: currentTaskId,
      title: current.title || ''
    })
    this.remindCurrent(true)
  },

  playBackgroundEndAlarm() {
    this.vibrateBackgroundEndAlarm()
    if (!wx.createInnerAudioContext) {
      debugLog.warn('COOK_STEP', 'BACKGROUND_ALARM_AUDIO_UNAVAILABLE')
      return Promise.resolve(false)
    }
    debugLog.info('COOK_STEP', 'BACKGROUND_ALARM_START')
    return this.ensureBackgroundAlarmFilePath()
      .then((filePath) => {
        if (!this.runtime || this.cleanupDone) return false
        if (!filePath) return false
        return new Promise((resolve) => {
          this.stopBackgroundAlarm()
          const audio = wx.createInnerAudioContext()
          this.backgroundAlarmContext = audio
          let settled = false
          let timeoutTimer = null
          const settle = (ok, err) => {
            if (settled) return
            settled = true
            if (timeoutTimer) clearTimeout(timeoutTimer)
            if (audio.offEnded) audio.offEnded(onEnded)
            if (audio.offError) audio.offError(onError)
            if (this.backgroundAlarmContext === audio) this.backgroundAlarmContext = null
            try {
              audio.stop()
            } catch (stopErr) {
              // Ignore cleanup errors from already-ended audio.
            }
            try {
              audio.destroy()
            } catch (destroyErr) {
              // Ignore cleanup errors from older base libraries.
            }
            debugLog[ok ? 'info' : 'warn']('COOK_STEP', ok ? 'BACKGROUND_ALARM_DONE' : 'BACKGROUND_ALARM_FAIL', {
              filePath,
              error: err && err.errMsg ? err.errMsg : (err && err.message ? err.message : String(err || ''))
            })
            resolve(ok)
          }
          const onEnded = () => settle(true)
          const onError = (err) => settle(false, err)
          timeoutTimer = setTimeout(() => settle(false, 'alarm_timeout'), 1800)
          audio.obeyMuteSwitch = false
          audio.src = filePath
          audio.onEnded(onEnded)
          audio.onError(onError)
          debugLog.info('COOK_STEP', 'BACKGROUND_ALARM_PLAY', { filePath })
          try {
            audio.play()
          } catch (err) {
            settle(false, err)
          }
        })
      })
      .catch((err) => {
        debugLog.warn('COOK_STEP', 'BACKGROUND_ALARM_FAIL', {
          error: err && err.message ? err.message : String(err)
        })
        return false
      })
  },

  vibrateBackgroundEndAlarm() {
    const vibrate = () => {
      if (wx.vibrateLong) {
        wx.vibrateLong({})
        return
      }
      wx.vibrateShort({ type: 'heavy' })
    }
    vibrate()
    setTimeout(vibrate, 360)
    setTimeout(vibrate, 720)
  },

  ensureBackgroundAlarmFilePath() {
    if (this.backgroundAlarmFilePathPromise) return this.backgroundAlarmFilePathPromise
    if (!wx.getFileSystemManager || !wx.env || !wx.env.USER_DATA_PATH) {
      debugLog.warn('COOK_STEP', 'BACKGROUND_ALARM_FILE_UNAVAILABLE')
      this.backgroundAlarmFilePathPromise = Promise.resolve('')
      return this.backgroundAlarmFilePathPromise
    }
    const filePath = `${wx.env.USER_DATA_PATH}/${BACKGROUND_ALARM_FILE_NAME}`
    const fs = wx.getFileSystemManager()
    this.backgroundAlarmFilePathPromise = new Promise((resolve) => {
      try {
        fs.access({
          path: filePath,
          success: () => {
            debugLog.info('COOK_STEP', 'BACKGROUND_ALARM_FILE_READY', { filePath, cached: true })
            resolve(filePath)
          },
          fail: () => {
            fs.writeFile({
              filePath,
              data: buildBackgroundAlarmWavBuffer(),
              success: () => {
                debugLog.info('COOK_STEP', 'BACKGROUND_ALARM_FILE_READY', { filePath, cached: false })
                resolve(filePath)
              },
              fail: (err) => {
                debugLog.warn('COOK_STEP', 'BACKGROUND_ALARM_FILE_WRITE_FAIL', {
                  filePath,
                  error: err && err.errMsg ? err.errMsg : String(err || '')
                })
                resolve('')
              }
            })
          }
        })
      } catch (err) {
        debugLog.warn('COOK_STEP', 'BACKGROUND_ALARM_FILE_FAIL', {
          filePath,
          error: err && err.message ? err.message : String(err)
        })
        resolve('')
      }
    })
    return this.backgroundAlarmFilePathPromise
  },

  stopBackgroundAlarm() {
    const audio = this.backgroundAlarmContext
    if (!audio) return
    this.backgroundAlarmContext = null
    try {
      audio.stop()
    } catch (err) {
      // Ignore cleanup errors from already-ended audio.
    }
    try {
      audio.destroy()
    } catch (err) {
      // Ignore cleanup errors from older base libraries.
    }
  },

  toggleRunning() {
    if (!this.runtime || this.cleanupDone || this.unloaded || this.exiting) return
    if (this.data.complete) {
      this.finishCookFlow()
      return
    }
    if (!this.data.started) {
      this.lastTickAt = Date.now()
      debugLog.info('COOK', 'START')
      this.enableExitAlertBeforeUnload()
      this.setData({
        started: true,
        running: true,
        pausedContinuing: false
      }, () => {
        if (this.cleanupDone || this.unloaded || this.exiting) return
        this.refreshView(() => {
          this.remindCurrent(true)
        }, { prefetch: false, alerts: false })
        this.startTicker()
      })
      return
    }

    if (this.data.running) {
      debugLog.info('COOK', 'PAUSE_REQUEST', { currentTaskId: this.data.currentTask && this.data.currentTask.id })
      this.pauseRuntime()
      return
    }

    this.lastTickAt = Date.now()
    debugLog.info('COOK', 'RESUME')
    this.setData({
      running: true,
      pausedContinuing: false
    }, () => {
      if (this.cleanupDone || this.unloaded || this.exiting) return
      this.startTicker()
      this.tick()
    })
  },

  pauseRuntime() {
    const task = this.data.currentTask || {}
    const mustContinue = !!(task.id && (!task.canPause || task.continuesDuringPause))
    debugLog.info('COOK', 'PAUSE_DECIDE', {
      taskId: task.id,
      title: task.title,
      mustContinue
    })
    const enterPause = () => {
      this.setData({
        running: false,
        pausedContinuing: mustContinue
      }, () => {
        if (mustContinue) {
          this.startTicker()
          wx.showToast({ title: '计时继续，关键点会提醒', icon: 'none' })
        } else {
          this.clearTicker()
          wx.showToast({ title: '已暂停指导', icon: 'none' })
        }
      })
    }

    if (task.id && !task.canPause) {
      wx.showModal({
        title: '当前步骤不能真正暂停',
        content: '火候、浸泡或焖煮会继续走时间，暂停后我仍会在关键点提醒你。',
        confirmText: '继续计时',
        success: (res) => {
          if (res.confirm) enterPause()
        }
      })
      return
    }
    enterPause()
  },

  startTicker() {
    this.clearTicker()
    if (!this.runtime || this.cleanupDone || this.unloaded || this.exiting) return
    if (!this.lastTickAt) {
      this.lastTickAt = Date.now()
    }
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
    if (!this.runtime || this.unloaded || this.exiting || (!this.data.running && !this.data.pausedContinuing)) return
    const now = Date.now()
    const deltaSeconds = Math.floor((now - this.lastTickAt) / 1000)
    if (deltaSeconds <= 0) return
    this.lastTickAt += deltaSeconds * 1000
    planRuntime.tick(this.runtime, deltaSeconds)
    this.refreshView()
  },

  remindCurrent(force) {
    if (!this.runtime || this.cleanupDone || this.unloaded || this.exiting) return
    const task = this.data.currentTask || {}
    if (!task.id) return
    if (!force && this.lastRemindedTaskId === task.id) return
    this.lastRemindedTaskId = task.id
    const promptTasks = this.buildReminderTasks(task, this.data.promptTasks)
    const title = this.buildPromptTitle(promptTasks)
    wx.vibrateShort({ type: task.safetyLevel === 'danger' ? 'heavy' : 'medium' })
    wx.showToast({
      title,
      icon: 'none',
      duration: 1800
    })
    const voiceTexts = task.rest ? this.buildRestPreviewVoiceTexts(task) : this.buildPromptVoiceTexts(promptTasks)
    const fallbackVoiceText = voiceTexts.join('')
    debugLog.info('COOK_STEP', 'REMIND_CURRENT', {
      taskId: task.id,
      title: task.title,
      rest: !!task.rest,
      background: !!task.background,
      promptTasks: promptTasks.map((item) => item.title),
      fallbackVoiceText,
      voiceTexts
    })
    if (task.rest) {
      this.speakRestPrompt(task, fallbackVoiceText)
      return
    }
    if (voice.speakSequence && voiceTexts.length > 1) {
      voice.speakSequence(voiceTexts, this.getCookVoiceGender())
      return
    }
    voice.speak(fallbackVoiceText, this.getCookVoiceGender())
  },

  buildVoiceText(task) {
    return cookVoiceText.buildTaskVoiceText(task)
  },

  buildPromptTitle(tasks) {
    return cookVoiceText.buildPromptTitle(tasks)
  },

  buildPromptVoiceText(tasks) {
    return cookVoiceText.buildPromptVoiceText(tasks)
  },

  buildPromptVoiceTexts(tasks) {
    return cookVoiceText.buildPromptVoiceTexts(tasks)
  },

  buildRestFixedVoiceItems(task, restParts) {
    const safeTask = task || {}
    const parts = restParts || cookVoiceText.buildRestVoicePartsFromTask(task)
    const instantText = parts.instantText || safeTask.instantVoiceText || (safeTask.title ? `${safeTask.title}。` : '')
    return [
      { label: 'instant', text: instantText },
      { label: 'detail', text: parts.detailText || '' },
      { label: 'waitPrefix', text: parts.waitPrefixText || '' }
    ].filter((item) => !!item.text)
  },

  getRestWaitDurationText(task, fixedTexts, fallbackText) {
    if (!task || !task.id) return fallbackText || ''
    return this.computeRestWaitDurationText(task, fixedTexts, fallbackText)
  },

  computeRestWaitDurationText(task, fixedTexts, fallbackText) {
    const remainingSeconds = Math.max(0, Math.floor(task && task.remainingSeconds ? task.remainingSeconds : 0))
    const waitText = cookVoiceText.buildRestVoiceWaitDurationText(remainingSeconds) || fallbackText || ''
    debugLog.info('COOK_STEP', 'REST_WAIT_DURATION_COMPUTE', {
      taskId: task && task.id,
      remainingSeconds,
      waitText
    })
    return waitText
  },

  buildRestVoicePayload(task, fallbackFullText) {
    const restParts = cookVoiceText.buildRestVoicePartsFromTask(task)
    const fixedSequenceItems = this.buildRestFixedVoiceItems(task, restParts)
    const fixedSequence = fixedSequenceItems.map((item) => item.text)
    const fallbackWaitDurationText = restParts.waitDurationText || restParts.waitText || ''
    const waitDurationText = this.getRestWaitDurationText(task, fixedSequence, fallbackWaitDurationText)
    const actualFullText = restParts.fullText || fallbackFullText || ''
    return {
      restParts,
      fixedSequenceItems,
      fixedSequence,
      waitDurationText,
      sequence: fixedSequence.concat(waitDurationText ? [waitDurationText] : []),
      actualFullText
    }
  },

  buildRestPreviewVoiceTexts(task) {
    return this.buildRestVoicePayload(task).sequence
  },

  buildReminderTasks(task, promptTasks) {
    if (task && task.rest) return [task]
    if (promptTasks && promptTasks.length) {
      return promptTasks.slice()
    }
    return task && task.id ? [task] : []
  },

  getCookVoiceGender() {
    return this.voiceGender || voice.getGender()
  },

  startBackgroundFixedVoicePrefetch() {
    const app = getApp()
    const texts = Array.isArray(app.globalData.pendingCookVoiceTexts) ? app.globalData.pendingCookVoiceTexts.slice() : []
    if (!texts.length) return
    app.globalData.pendingCookVoiceTexts = []
    debugLog.info('COOK', 'VOICE_BACKGROUND_PREFETCH_START', {
      count: texts.length,
      gender: this.getCookVoiceGender()
    })
    voice.prefetchQueued(texts, this.getCookVoiceGender())
  },

  speakRestPrompt(task, fullText) {
    const gender = this.getCookVoiceGender()
    const payload = this.buildRestVoicePayload(task, fullText)
    if (voice.speakRestSequence) {
      debugLog.info('COOK_STEP', 'REST_VOICE_SEQUENCE', {
        taskId: task.id,
        title: task.title,
        remainingSeconds: task.remainingSeconds,
        durationFetchMode: 'foreground_once',
        order: payload.fixedSequenceItems.map((item) => item.label).concat(['waitDuration']),
        waitText: payload.waitDurationText
      })
      voice.speakRestSequence(payload.fixedSequence, payload.waitDurationText, gender)
      return
    }
    debugLog.warn('COOK_STEP', 'REST_VOICE_SEQUENCE_UNAVAILABLE', {
      taskId: task.id,
      title: task.title,
      remainingSeconds: task.remainingSeconds,
      waitText: payload.waitDurationText
    })
    voice.speak(payload.actualFullText, gender)
  },

  markDone(e) {
    if (!this.runtime || this.unloaded || this.exiting || this.data.finishing) return
    this.captureElapsedTime()
    const id = e && e.currentTarget && e.currentTarget.dataset.id
    const task = id ? (planRuntime.buildView(this.runtime).timelineBars.find(item => item.id === id) || {}) : (this.data.currentTask || {})
    if (!this.runtime || !task.id) return
    if (!task.canFinish) {
      debugLog.warn('COOK_ACTION', 'MARK_DONE_BLOCKED', { taskId: task.id, title: task.title, rest: !!task.rest, background: !!task.background })
      wx.showToast({ title: task.rest ? '现在可以休息等待' : (task.background ? '后台计时不能提前完成' : '当前步骤不能完成'), icon: 'none' })
      return
    }
    if (!this.data.started) {
      debugLog.warn('COOK_ACTION', 'MARK_DONE_BLOCKED_NOT_STARTED', { taskId: task.id, title: task.title })
      wx.showToast({ title: '先点击开始', icon: 'none' })
      return
    }
    const finish = () => {
      if (!this.runtime || this.unloaded || this.exiting) return
      this.captureElapsedTime()
      debugLog.info('COOK_ACTION', 'MARK_DONE', { taskId: task.id, title: task.title, elapsedSeconds: this.runtime.elapsedSeconds })
      const completed = planRuntime.finishTask(this.runtime, task.id, 'early')
      this.refreshView(() => {
        if (!this.data.running && !this.data.pausedContinuing) {
          this.remindCurrent(true)
        }
        if (completed) this.markRuntimeComplete()
      })
    }
    if (this.needSafetyConfirm(task)) {
      wx.showModal({
        title: '确认完成当前步骤？',
        content: '这个步骤和火候或等待有关，提前完成可能影响成品或安全。',
        confirmText: '确认完成',
        success: (res) => {
          if (res.confirm) finish()
        }
      })
      return
    }
    finish()
  },

  skipTask(e) {
    const id = e && e.currentTarget && e.currentTarget.dataset.id
    this.skipTaskById(id || (this.data.currentTask ? this.data.currentTask.id : ''))
  },

  skipTaskById(taskId) {
    if (!this.runtime || !taskId || !this.data.started || this.data.finishing || this.exiting || this.unloaded) return
    this.captureElapsedTime()
    const task = this.getTaskById(taskId)
    if (!task || this.runtime.skippedTaskIds[taskId]) return
    const viewTask = planRuntime.buildView(this.runtime).timelineBars.find((item) => item.id === taskId) || task
    if (!viewTask.canSkip) {
      debugLog.warn('COOK_ACTION', 'SKIP_BLOCKED', { taskId, title: viewTask.title, background: !!viewTask.background })
      wx.showToast({ title: '当前步骤不能跳过', icon: 'none' })
      return
    }
    const skip = () => {
      if (!this.runtime || this.exiting || this.unloaded) return
      this.captureElapsedTime()
      const completed = planRuntime.skipTask(this.runtime, taskId)
      this.refreshView(() => {
        if (!this.data.running && !this.data.pausedContinuing) this.remindCurrent(true)
        if (completed) this.markRuntimeComplete()
      })
    }
    if (!this.needSafetyConfirm(task)) { skip(); return }
    wx.showModal({
      title: '跳过当前步骤？',
      content: '这个步骤可能影响安全或成品稳定，确认跳过吗？',
      confirmText: '确认跳过',
      success: (res) => {
        if (!res.confirm) return
        skip()
      }
    })
  },

  addTime(e) {
    const id = e && e.currentTarget && e.currentTarget.dataset.id
    this.addTimeForTask(id || (this.data.currentTask ? this.data.currentTask.id : ''))
  },

  addTimeForTask(taskId) {
    if (!this.runtime || !taskId || !this.data.started || this.data.finishing || this.exiting || this.unloaded) return
    this.captureElapsedTime()
    const task = this.getTaskById(taskId)
    if (!task || this.runtime.skippedTaskIds[taskId]) return
    const viewTask = planRuntime.buildView(this.runtime).timelineBars.find((item) => item.id === taskId) || task
    if (!viewTask.canAddTime) {
      debugLog.warn('COOK_ACTION', 'ADD_TIME_BLOCKED', { taskId, title: viewTask.title })
      wx.showToast({ title: '只能给正在执行的步骤加时', icon: 'none' })
      return
    }
    this.applyAddTime(taskId, 10)
  },

  applyAddTime(taskId, seconds) {
    if (!planRuntime.addTime(this.runtime, taskId, seconds)) {
      debugLog.warn('COOK_ACTION', 'ADD_TIME_FAILED', { taskId, seconds })
      wx.showToast({ title: '当前步骤不能加时', icon: 'none' })
      return
    }
    debugLog.info('COOK_ACTION', 'ADD_TIME', { taskId, seconds })
    this.refreshView(() => {
      wx.showToast({ title: `已加时 ${time.formatSeconds(seconds)}`, icon: 'none' })
    })
  },

  getTaskById(taskId) {
    if (!this.runtime || !taskId) return null
    return this.runtime.timeline.find((task) => task.id === taskId) || null
  },

  needSafetyConfirm(task) {
    return task.safetyLevel === 'danger' || task.safetyLevel === 'attention' || !task.canPause
  },

  markRuntimeComplete() {
    if (this.finishModalShown) return
    this.finishModalShown = true
    this.clearTicker()
    this.stopBackgroundAlarm()
    voice.stop()
    this.setData({
      running: false,
      pausedContinuing: false,
      complete: true,
      finishing: true
    }, () => {
      if (this.unloaded || this.cleanupDone) return
      this.rememberCookSync()
      this.presentCompletionPrompts()
    })
    debugLog.info('COOK', 'COMPLETE', {
      elapsedSeconds: this.data.elapsedSeconds,
      totalSeconds: this.runtime && this.runtime.totalSeconds
    })
  },

  presentCompletionPrompts() {
    const isActive = () => this.pageActive && !this.unloaded && !this.cleanupDone
    if (!isActive() || this.completionSharePromptShown || this.completionPromptPending) return
    this.completionPromptPending = true
    const settled = () => {
      if (!isActive()) return
      this.setData({ finishing: false })
      this.showCompletionSharePrompt()
    }
    this.disableExitAlertBeforeUnload().then(() => {
      if (!isActive()) return
      const app = getApp()
      if (!personalizationSync.showPending(app, wx, api, { isActive, onSettled: settled }) &&
        !(app.globalData.pendingCookSyncReports || []).length) settled()
    }).catch(() => {
      if (!isActive()) return
      this.setData({ finishing: false })
      wx.showToast({ title: '退出提醒未关闭，请点击完成重试', icon: 'none' })
    }).then(() => { this.completionPromptPending = false })
  },

  showCompletionSharePrompt() {
    if (this.completionSharePromptShown || this.cleanupDone || this.unloaded || !this.pageActive) return
    this.completionSharePromptShown = true
    wx.showModal({
      title: '流程完成',
      content: '这顿饭的流程已经走完，可以拍一下成果去发布。',
      confirmText: '秀一秀',
      cancelText: '先不去',
      success: (res) => {
        debugLog.info('COOK', 'COMPLETE_SHARE_PROMPT', {
          confirm: !!res.confirm
        })
        if (res.confirm) {
          this.finishCookFlow({ preferredExit: 'publish' })
        }
      }
    })
  },

  finishCookFlow(options) {
    if (this.data.finishing) return
    const safeOptions = options || {}
    const preferredExit = safeOptions.preferredExit || ''
    this.exitAfterFinish(preferredExit || 'home')
  },

  buildPersonalizationSyncItems() {
    return personalizationSync.buildItems(this.runtime)
  },

  captureElapsedTime() {
    if (!this.runtime || !this.data.started || (!this.data.running && !this.data.pausedContinuing) || !this.lastTickAt) return
    const seconds = Math.max(0, Math.floor((Date.now() - this.lastTickAt) / 1000))
    if (seconds) {
      this.lastTickAt += seconds * 1000
      planRuntime.tick(this.runtime, seconds)
    }
  },

  rememberCookSync() {
    if (!this.syncPersonalization || !this.data.started || this.syncReportQueued || !this.runtime) return
    this.syncReportQueued = true
    personalizationSync.enqueue(getApp(), this.runtime, this.cookOwnerId)
  },

  cleanupCookFlow() {
    if (this.cleanupDone) return
    this.cleanupDone = true
    this.clearTicker()
    this.stopBackgroundAlarm()
    voice.stop()
    this.disableExitAlertBeforeUnload().catch(() => {})
    const app = getApp()
    app.globalData.lastPlan = null
    app.globalData.lastPreview = null
    app.globalData.lastPreviewRecipeKey = ''
    app.globalData.lastPreviewNeedsTimeBlocks = false
    app.globalData.pendingCookVoiceTexts = []
    app.globalData.currentCookVoiceGender = ''
    app.globalData.selectedRecipeIds = []
    app.globalData.selectedRecipes = []
    this.pendingBackgroundCurrentReminderToken = ''
    this.completionSharePromptShown = true
    this.runtime = null
  },

  exitAfterFinish(target) {
    if (target === 'publish') {
      this.exitCookToPublish()
      return
    }
    if (target === 'mine') {
      this.exitCookToMinePersonalization()
      return
    }
    this.exitCookToHome()
  },

  buildCookedRecipeIds() {
    if (!this.runtime || !Array.isArray(this.runtime.timeline)) return []
    const map = {}
    const result = []
    this.runtime.timeline.forEach((task) => {
      const recipeId = task && task.recipeId ? String(task.recipeId) : ''
      if (!recipeId || map[recipeId]) return
      map[recipeId] = true
      result.push(recipeId)
    })
    return result
  },

  exitCookToPublish() {
    const recipeIds = this.buildCookedRecipeIds()
    this.exitCookToTab('/pages/publish/publish', () => {
      const app = getApp()
      app.globalData.selectedRecipeIds = recipeIds
      app.globalData.selectedRecipes = []
      debugLog.info('COOK', 'EXIT_PUBLISH', { recipeIds })
    })
  },

  exitCookToMinePersonalization() {
    this.exitCookToTab('/pages/mine/mine', () => { getApp().globalData.mineInitialTab = 'personal' })
  },

  exitCookToHome() {
    this.exitCookToTab('/pages/home/home')
  },

  backMenu() {
    this.exitCookToTab('/pages/menu/menu')
  },

  exitCookToTab(url, afterCleanup) {
    if (this.exiting || this.unloaded) return
    this.exiting = true
    this.disableExitAlertBeforeUnload().then(() => {
      if (this.unloaded) return
      this.captureElapsedTime()
      this.rememberCookSync()
      this.suppressUnloadRedirect = true
      this.cleanupCookFlow()
      if (afterCleanup) afterCleanup()
      wx.switchTab({
        url,
        fail: (error) => {
          if (this.unloaded) return
          this.exiting = false
          debugLog.warn('COOK', 'EXIT_NAVIGATION_FAIL', { url, error: error.errMsg })
          wx.showToast({ title: '返回失败，请重试', icon: 'none' })
        }
      })
    }).catch(() => {
      if (this.unloaded) return
      this.exiting = false
      wx.showToast({ title: '退出提醒未关闭，请再试一次', icon: 'none' })
    })
  }
})