const api = require('../../utils/api')
const time = require('../../utils/time')
const config = require('../../utils/config')
const voice = require('../../utils/voice')
const cookVoiceText = require('../../utils/cookVoiceText')
const debugLog = require('../../utils/debugLog')

Page({
  data: {
    plan: null,
    selectedRecipes: [],
    starting: false,
    timeBlocksLoading: false,
    startButtonText: '开始排程',
    voiceGender: 'female',
    voiceLabel: '女声'
  },

  onLoad() {
    this.pageActive = true
    const app = getApp()
    debugLog.info('PLAN', 'LOAD', { selectedCount: (app.globalData.selectedRecipeIds || []).length })
    this.setData({
      plan: this.decoratePlan(app.globalData.lastPreview || null),
      selectedRecipes: app.globalData.selectedRecipes || [],
      voiceGender: voice.getGender(),
      voiceLabel: voice.genderLabel(voice.getGender())
    }, () => this.ensureTimeBlocksPlan())
  },

  onUnload() {
    this.pageActive = false
    this.timeBlocksRequestId = (this.timeBlocksRequestId || 0) + 1
  },

  onShow() {
    const app = getApp()
    if (app.globalData.exitCookFlowToMenu) {
      app.globalData.exitCookFlowToMenu = false
      debugLog.info('PLAN', 'COOK_EXIT_REDIRECT_MENU')
      wx.switchTab({ url: '/pages/menu/menu' })
      return
    }
    const gender = voice.getGender()
    this.setData({
      voiceGender: gender,
      voiceLabel: voice.genderLabel(gender)
    })
  },

  recipeIdsKey(ids) {
    return (ids || []).slice().sort().join('|')
  },

  hasTimeBlocks(plan) {
    return !!(plan &&
      Array.isArray(plan.timeSegments) &&
      plan.timeSegments.length &&
      Array.isArray(plan.freeBlocks))
  },

  ensureTimeBlocksPlan() {
    const app = getApp()
    const ids = (app.globalData.selectedRecipeIds || []).slice()
    const plan = app.globalData.lastPreview || this.data.plan
    if (!ids.length || !plan || !plan.summary || this.hasTimeBlocks(plan)) return
    const recipeKey = app.globalData.lastPreviewRecipeKey || this.recipeIdsKey(ids)
    this.timeBlocksRequestId = (this.timeBlocksRequestId || 0) + 1
    const requestId = this.timeBlocksRequestId
    const isStale = () => {
      return requestId !== this.timeBlocksRequestId ||
        this.pageActive === false ||
        this.recipeIdsKey((getApp().globalData.selectedRecipeIds || [])) !== recipeKey
    }
    debugLog.info('PLAN', 'TIME_BLOCKS_REQUEST', { requestId, recipeIds: ids, recipeKey })
    this.setData({ timeBlocksLoading: true })
    api.previewPlan(ids, {
      isStale,
      summaryOnly: true,
      includeTimeBlocks: true,
      timeout: config.PLAN_TIME_BLOCKS_REQUEST_TIMEOUT_MS || 30000,
      maxAttempts: config.PLAN_TIME_BLOCKS_MAX_ATTEMPTS || 3,
      retryDelayMs: 500
    })
      .then((res) => {
        if (!res.ok) throw new Error(res.message)
        if (isStale()) return
        app.globalData.lastPreview = res
        app.globalData.lastPreviewNeedsTimeBlocks = false
        this.setData({
          plan: this.decoratePlan(res),
          timeBlocksLoading: false
        })
        debugLog.info('PLAN', 'TIME_BLOCKS_OK', {
          requestId,
          recipeKey,
          freeBlockCount: Array.isArray(res.freeBlocks) ? res.freeBlocks.length : 0,
          timeSegmentCount: Array.isArray(res.timeSegments) ? res.timeSegments.length : 0
        })
      })
      .catch((err) => {
        if (api.isStaleError(err) || isStale()) {
          debugLog.info('PLAN', 'TIME_BLOCKS_STALE', { requestId, recipeKey })
          return
        }
        debugLog.warn('PLAN', 'TIME_BLOCKS_FAIL', { requestId, recipeKey, error: err && err.message ? err.message : String(err) })
        this.setData({ timeBlocksLoading: false })
      })
  },

  decoratePlan(plan) {
    if (!plan) return null
    const summary = Object.assign({}, plan.summary || {})
    const totalSeconds = Math.max(0, Math.floor(summary.totalSeconds || 0))
    summary.totalText = time.formatDurationText(totalSeconds)
    summary.activeText = time.formatDurationText(summary.activeSeconds || 0)
    summary.freeText = time.formatDurationText(summary.freeSeconds || 0)
    summary.watchText = time.formatDurationText(summary.watchSeconds !== undefined ? summary.watchSeconds : (summary.edgeSeconds || 0))
    summary.freeBlockMinimumText = time.formatDurationText(summary.freeBlockMinimumSeconds || 120)
    const decorateBlock = (item, type) => Object.assign({}, item, {
      type: item.type || type,
      startText: time.formatDurationText(item.startSeconds),
      endText: time.formatDurationText(item.endSeconds),
      durationText: time.formatDurationText(item.durationSeconds),
      rangeText: `${time.formatDurationText(item.startSeconds)} 到 ${time.formatDurationText(item.endSeconds)}`
    })
    const freeBlocks = Array.isArray(plan.freeBlocks) ? plan.freeBlocks.map((item) => Object.assign({}, item, {
      type: item.type || 'free',
      startText: time.formatDurationText(item.startSeconds),
      endText: time.formatDurationText(item.endSeconds),
      durationText: time.formatDurationText(item.durationSeconds),
      rangeText: `${time.formatDurationText(item.startSeconds)} 到 ${time.formatDurationText(item.endSeconds)}`
    })) : []
    const watchBlocks = Array.isArray(plan.watchBlocks) ? plan.watchBlocks.map((item) => decorateBlock(item, 'watch')) : []
    const timeSegments = Array.isArray(plan.timeSegments) ? plan.timeSegments.map((item) => {
      const durationSeconds = Math.max(0, Math.floor(item.durationSeconds || 0))
      const widthPercent = totalSeconds > 0 ? (durationSeconds / totalSeconds) * 100 : 0
      const type = item.type || 'watch'
      let typeText = '守候'
      if (type === 'active') typeText = '动手'
      else if (type === 'free') typeText = '休息'
      return Object.assign({}, item, {
        type,
        typeText,
        startText: time.formatDurationText(item.startSeconds),
        endText: time.formatDurationText(item.endSeconds),
        durationText: time.formatDurationText(durationSeconds),
        style: `width: ${widthPercent.toFixed(3)}%;`
      })
    }) : []
    return Object.assign({}, plan, { summary, freeBlocks, watchBlocks, timeSegments })
  },

  voiceInitialPromptGroupCount(plan) {
    const groupValue = Number(plan && plan.clientConfig && plan.clientConfig.voiceInitialPromptGroupCount)
    if (isFinite(groupValue) && groupValue > 0) return Math.floor(groupValue)
    const legacyValue = Number(plan && plan.clientConfig && plan.clientConfig.voiceInitialRequiredCount)
    return isFinite(legacyValue) && legacyValue > 0 ? Math.floor(legacyValue) : 2
  },

  voiceClientConcurrency(plan) {
    const value = Number(plan && plan.clientConfig && plan.clientConfig.voiceClientConcurrency)
    const clientLimit = Number(config.VOICE_INITIAL_REQUIRED_FETCH_CONCURRENCY || config.VOICE_PREFETCH_QUEUE_CONCURRENCY || 2)
    const fallback = isFinite(clientLimit) && clientLimit > 0 ? Math.floor(clientLimit) : 2
    if (isFinite(value) && value > 0) return Math.min(Math.floor(value), fallback)
    return fallback
  },

  startCook() {
    if (this.data.starting) return
    const app = getApp()
    const ids = (app.globalData.selectedRecipeIds || []).slice()
    if (!ids.length) {
      debugLog.warn('PLAN', 'START_BLOCKED_EMPTY')
      wx.showToast({ title: '还没有选菜', icon: 'none' })
      return
    }
    debugLog.info('PLAN', 'START_REQUEST', { recipeIds: ids })
    this.setData({ starting: true, startButtonText: '生成排程...' })
    api.startPlan(ids)
      .then((res) => {
        if (!res.ok) throw new Error(res.message)
        this.setData({ startButtonText: '准备语音...' })
        const cookVoiceGender = voice.getGender()
        const fixedTexts = cookVoiceText.buildFixedPlanVoiceTexts(res)
        const requiredTexts = cookVoiceText.buildInitialPromptVoiceTexts(res, this.voiceInitialPromptGroupCount(res))
        const requiredMap = {}
        requiredTexts.forEach((text) => { requiredMap[text] = true })
        const backgroundTexts = fixedTexts.filter((text) => !requiredMap[text])
        const concurrency = this.voiceClientConcurrency(res)
        app.globalData.currentCookVoiceGender = cookVoiceGender
        app.globalData.pendingCookVoiceTexts = backgroundTexts
        debugLog.info('PLAN', 'VOICE_REQUIRED_START', {
          requiredCount: requiredTexts.length,
          backgroundCount: backgroundTexts.length,
          fixedCount: fixedTexts.length,
          configuredGroupCount: this.voiceInitialPromptGroupCount(res),
          requiredAudioWaitMilliseconds: config.VOICE_INITIAL_REQUIRED_AUDIO_WAIT_MS || 15000,
          concurrency,
          gender: cookVoiceGender
        })
        return voice.preparePlanVoiceTexts(fixedTexts, cookVoiceGender, requiredTexts.length, res.planId || '', {
          includeRequiredAudio: true,
          requiredAudioWaitMilliseconds: config.VOICE_INITIAL_REQUIRED_AUDIO_WAIT_MS || 15000
        })
          .then(() => voice.prefetchRequired(requiredTexts, cookVoiceGender, {
            timeoutMs: config.VOICE_REQUIRED_PREFETCH_TIMEOUT_MS || 15000,
            concurrency
          }))
          .then(() => {
            debugLog.info('PLAN', 'VOICE_REQUIRED_OK', {
              requiredCount: requiredTexts.length,
              backgroundCount: backgroundTexts.length,
              gender: cookVoiceGender
            })
            app.globalData.lastPlan = res
            this.setData({ starting: false, startButtonText: '开始排程' })
            debugLog.info('PLAN', 'NAVIGATE_COOK', {
              recipeIds: ids,
              totalSeconds: res.summary && res.summary.totalSeconds,
              activeSeconds: res.summary && res.summary.activeSeconds
            })
            wx.navigateTo({ url: '/pages/cook/cook' })
          })
      })
      .catch((err) => {
        debugLog.error('PLAN', 'START_FAIL', { recipeIds: ids, error: err.message || '开始失败' })
        this.setData({ starting: false, startButtonText: '开始排程' })
        wx.showToast({ title: err.message || '开始失败', icon: 'none' })
      })
  },

  chooseVoice(e) {
    const gender = voice.setGender(e.currentTarget.dataset.value)
    debugLog.info('PLAN', 'VOICE_CHOOSE', { gender })
    this.setData({
      voiceGender: gender,
      voiceLabel: voice.genderLabel(gender)
    })
  },

  backMenu() {
    wx.switchTab({ url: '/pages/menu/menu' })
  }
})
