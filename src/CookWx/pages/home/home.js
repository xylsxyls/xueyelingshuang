const api = require('../../utils/api')
const config = require('../../utils/config')
const debugLog = require('../../utils/debugLog')
const tabBar = require('../../utils/tabBar')

function normalizeVideoId(videoId) {
  const text = videoId === undefined || videoId === null ? '' : String(videoId)
  if (text.indexOf('feed_') === 0) return text.slice(5)
  return text
}

function numberOrDefault(value, defaultValue) {
  const numberValue = Number(value)
  return isFinite(numberValue) ? numberValue : defaultValue
}

function clampProgressValue(value) {
  return Math.max(0, Math.min(1000, numberOrDefault(value, 0)))
}

function secondsFromProgress(durationSeconds, progressValue) {
  const duration = numberOrDefault(durationSeconds, 0)
  if (duration <= 0) return 0
  return Math.max(0, Math.min(duration, duration * clampProgressValue(progressValue) / 1000))
}

function progressPercentFromValue(value) {
  return clampProgressValue(value) / 10
}

function formatVideoSeconds(seconds, clockStyle) {
  const safeSeconds = Math.max(0, Math.floor(numberOrDefault(seconds, 0)))
  if (clockStyle || safeSeconds >= 60) {
    const minutes = Math.floor(safeSeconds / 60)
    const remainSeconds = safeSeconds % 60
    return `${minutes}:${remainSeconds < 10 ? '0' : ''}${remainSeconds}`
  }
  return `${safeSeconds}秒`
}

function formatProgressHint(currentSeconds, durationSeconds) {
  const duration = Math.max(0, Math.floor(numberOrDefault(durationSeconds, 0)))
  const current = Math.max(0, Math.min(duration || Number.MAX_SAFE_INTEGER, Math.floor(numberOrDefault(currentSeconds, 0))))
  const clockStyle = duration >= 60 || current >= 60
  return `${formatVideoSeconds(current, clockStyle)}/${formatVideoSeconds(duration, clockStyle)}`
}

Page({
  data: {
    tabs: ['推荐', '精选', '关注', '好友'],
    activeTab: '推荐',
    feed: [],
    currentIndex: 0,
    error: '',
    loading: false,
    loadingMore: false,
    buyingRecipeId: '',
    videoPaused: false,
    videoDurationSeconds: 0,
    videoCurrentSeconds: 0,
    videoProgressValue: 0,
    videoProgressPercent: 0,
    showProgressHint: false,
    progressHintText: ''
  },

  onShow() {
    tabBar.showRefresh()
    const restoredFromOtherTab = this.homeHidden === true && (this.data.feed || []).length > 0
    if (restoredFromOtherTab) {
      this.suppressNextHomeTabRefreshUntil = Date.now() + 1200
    }
    this.homeHidden = false
    this.pageActive = true
    this.transitionSourceIndex = -1
    this.logUserAction('PAGE_SHOW', {
      currentIndex: this.data.currentIndex,
      feedCount: (this.data.feed || []).length,
      restoredFromOtherTab
    })
    if (!this.data.feed.length) this.loadData()
    else {
      this.activateCurrentVideoSource(this.data.currentIndex, () => {
        this.syncVideoPlayback({ restorePosition: true })
        this.prefetchPosterImages(this.data.currentIndex)
      })
    }
  },

  onTabItemTap() {
    const now = Date.now()
    const suppressRefresh = this.homeHidden === true ||
      (this.suppressNextHomeTabRefreshUntil && now < this.suppressNextHomeTabRefreshUntil)
    this.logUserAction('TAB_ITEM_TAP', {
      activeTab: this.data.activeTab,
      currentIndex: this.data.currentIndex,
      suppressRefresh: !!suppressRefresh
    })
    if (suppressRefresh) {
      this.homeHidden = false
      this.suppressNextHomeTabRefreshUntil = 0
      return
    }
    this.loadData()
  },

  onHide() {
    this.pageActive = false
    this.homeHidden = true
    this.hiddenAtMilliseconds = Date.now()
    this.loadRequestId = (this.loadRequestId || 0) + 1
    this.transitionSourceIndex = -1
    this.clearSeekFallbackTimer()
    this.pendingSeek = null
    this.progressDragging = false
    this.clearPosterPrefetchRequests()
    this.setData({ loading: false, loadingMore: false, showProgressHint: false, progressHintText: '' })
    this.logUserAction('PAGE_HIDE', {
      currentIndex: this.data.currentIndex,
      feedCount: (this.data.feed || []).length,
      videoCurrentSeconds: this.data.videoCurrentSeconds,
      videoPaused: !!this.data.videoPaused
    })
    this.clearWatchTimer()
    this.pauseAllVideos()
  },

  onUnload() {
    this.pageActive = false
    this.homeHidden = false
    this.suppressNextHomeTabRefreshUntil = 0
    this.loadRequestId = (this.loadRequestId || 0) + 1
    this.transitionSourceIndex = -1
    this.clearSeekFallbackTimer()
    this.pendingSeek = null
    this.progressDragging = false
    this.clearPosterPrefetchRequests()
    this.setData({ loading: false, loadingMore: false, showProgressHint: false, progressHintText: '' })
    this.logUserAction('PAGE_UNLOAD', {
      currentIndex: this.data.currentIndex,
      feedCount: (this.data.feed || []).length
    })
    this.clearWatchTimer()
    this.pauseAllVideos()
  },

  logUserAction(action, detail) {
    debugLog.info('HOME_ACTION', action, Object.assign({
      currentIndex: this.data.currentIndex,
      currentVideoId: this.currentVideoId()
    }, detail || {}))
  },

  currentVideoId() {
    const item = (this.data.feed || [])[this.data.currentIndex] || {}
    return item.videoId || item.id || ''
  },

  clearSeekFallbackTimer() {
    if (this.seekFallbackTimer) {
      clearTimeout(this.seekFallbackTimer)
      this.seekFallbackTimer = null
    }
  },

  playVideoAfterSeek(index, reason) {
    if (index !== this.data.currentIndex || !this.pageActive) return
    const item = (this.data.feed || [])[index] || {}
    const ctx = wx.createVideoContext(`feedVideo${index}`, this)
    if (!ctx) return
    const delayMs = Math.max(0, numberOrDefault(config.VIDEO_SEEK_PLAY_DELAY_MS, 120))
    setTimeout(() => {
      if (index !== this.data.currentIndex || !this.pageActive) return
      ctx.play()
      this.setData({ videoPaused: false })
      debugLog.info('HOME', 'VIDEO_SEEK_PLAY', {
        index,
        videoId: item.videoId || '',
        reason
      })
    }, delayMs)
  },

  feedBatchSize() {
    return Math.max(1, this.metadataAheadCount() + 1)
  },

  loadMoreBatchSize() {
    return Math.max(1, this.metadataAheadCount())
  },

  metadataAheadCount() {
    return Math.max(0, Number(config.VIDEO_FEED_METADATA_AHEAD_COUNT || 0))
  },

  firstFrameSideCount() {
    const serverCount = Number(this.posterPrefetchCount)
    if (isFinite(serverCount) && serverCount >= 0) return Math.max(0, serverCount)
    return Math.max(0, Number(config.VIDEO_FEED_FIRST_FRAME_SIDE_COUNT || 0))
  },

  loadedVideoIds() {
    return (this.data.feed || []).map((item) => normalizeVideoId(item.videoId || item.id || '')).filter((id) => !!id)
  },

  loadData() {
    if (!api.token()) {
      this.setData({ error: '', loading: false, feed: [] })
      wx.reLaunch({ url: '/pages/auth/login/login' })
      return
    }
    if (this.data.loading) {
      debugLog.info('HOME', 'LOAD_SKIP_LOADING', {
        currentIndex: this.data.currentIndex,
        feedCount: (this.data.feed || []).length
      })
      return
    }
    this.loadRequestId = (this.loadRequestId || 0) + 1
    this.transitionSourceIndex = -1
    this.clearPosterPrefetchRequests()
    const requestId = this.loadRequestId
    const isStale = () => requestId !== this.loadRequestId || this.pageActive === false
    this.setData({ error: '', loading: true })
    debugLog.info('HOME', 'FEED_LOAD_START', {
      requestId,
      count: this.feedBatchSize(),
      activeTab: this.data.activeTab
    })
    api.getFeed({ isStale, count: this.feedBatchSize(), purpose: 'feed_initial' })
      .then((res) => {
        if (!res.ok) throw new Error(res.message || '加载失败')
        if (isStale()) return
        if (res.posterPrefetchCount !== undefined) this.posterPrefetchCount = Number(res.posterPrefetchCount)
        this.setData({
          tabs: res.tabs || this.data.tabs,
          feed: this.normalizeFeed(res.feed || [], 'init'),
          currentIndex: 0,
          loading: false,
          videoPaused: false,
          videoDurationSeconds: 0,
          videoCurrentSeconds: 0,
          videoProgressValue: 0,
          videoProgressPercent: 0,
          showProgressHint: false,
          progressHintText: ''
        }, () => {
          this.activateCurrentVideoSource(0, () => {
            this.syncVideoPlayback()
            this.scheduleMarkCurrentWatched()
            this.prefetchPosterImages(0)
            this.ensureFeedAhead(0)
          })
        })
        debugLog.info('HOME', 'FEED_LOAD_OK', {
          requestId,
          feedCount: (res.feed || []).length,
          resetSeen: !!res.resetSeen
        })
      })
      .catch((err) => {
        if (api.isStaleError(err) || isStale()) return
        this.setData({
          error: '连接服务器失败，请确认 CookServer 已在 5207 端口运行。',
          loading: false
        })
        debugLog.warn('HOME', 'FEED_LOAD_FAIL', {
          requestId,
          error: err && err.message ? err.message : String(err)
        })
      })
  },

  loadMoreFeed(options) {
    if (this.data.loadingMore || this.data.loading || !this.pageActive) return
    const safeOptions = options || {}
    const foreground = safeOptions.foreground === true
    this.setData({ loadingMore: true })
    const requestId = this.loadRequestId
    const isStale = () => requestId !== this.loadRequestId || this.pageActive === false
    const excludeIds = this.loadedVideoIds()
    const current = (this.data.feed || [])[this.data.currentIndex] || {}
    debugLog.info('HOME', 'FEED_MORE_START', {
      foreground,
      count: this.loadMoreBatchSize(),
      excludeCount: excludeIds.length,
      currentIndex: this.data.currentIndex,
      currentVideoId: normalizeVideoId(current.videoId || current.id || '')
    })
    api.getFeed({
      isStale,
      count: this.loadMoreBatchSize(),
      excludeIds,
      currentVideoId: normalizeVideoId(current.videoId || current.id || ''),
      purpose: foreground ? 'feed_next' : 'background_prefetch',
      maxAttempts: foreground ? 3 : 1,
      silentFailure: !foreground
    })
      .then((res) => {
        if (!res.ok) throw new Error(res.message || '加载失败')
        if (isStale()) return
        if (res.posterPrefetchCount !== undefined) this.posterPrefetchCount = Number(res.posterPrefetchCount)
        const nextFeed = this.normalizeFeed(res.feed || [], `more_${Date.now()}`)
        this.setData({
          feed: (this.data.feed || []).concat(nextFeed),
          loadingMore: false
        }, () => {
          this.prefetchPosterImages(this.data.currentIndex)
          this.activateCurrentVideoSource(this.data.currentIndex, () => this.syncVideoPlayback())
        })
        debugLog.info('HOME', 'FEED_MORE_OK', {
          currentCount: this.data.feed.length,
          appendCount: nextFeed.length,
          resetSeen: !!res.resetSeen
        })
      })
      .catch((err) => {
        if (api.isStaleError(err) || isStale()) return
        this.setData({ loadingMore: false })
        const detail = {
          foreground,
          error: err && err.message ? err.message : String(err)
        }
        if (foreground) debugLog.warn('HOME', 'FEED_MORE_FAIL', detail)
        else debugLog.info('HOME', 'FEED_MORE_BACKGROUND_FAIL', detail)
      })
  },

  normalizeFeed(feed, batchKey) {
    return feed.map((item, index) => {
      const recipeIds = Array.isArray(item.recipeIds) ? item.recipeIds.filter((id) => !!id) : []
      const primaryRecipeId = recipeIds.length ? recipeIds[0] : ''
      const priceCoins = Number(item.priceCoins || 0)
      const priceAmount = Number(item.priceAmount || priceCoins || 0)
      const priceType = item.priceType || (priceCoins > 0 ? 'coin' : 'free')
      const owned = !!item.owned
      let priceText = '免费'
      if (priceType === 'yuanbao' && priceAmount > 0) priceText = `${priceAmount}元宝`
      else if (priceType === 'coin' && priceAmount > 0) priceText = `${priceAmount}金币`
      const videoId = normalizeVideoId(item.videoId || item.id || '')
      const rawVideoUrl = item.videoUrl || ''
      const posterUrl = item.posterUrl || ''
      return Object.assign({}, item, {
        feedKey: `${videoId || primaryRecipeId || 'feed'}_${batchKey || 'batch'}_${index}_${Math.floor(Math.random() * 100000)}`,
        id: videoId || item.id || '',
        videoId,
        rawVideoUrl,
        posterUrl,
        posterSrc: '',
        posterLoaded: false,
        posterFailed: false,
        videoUrl: '',
        recipeIds,
        primaryRecipeId,
        priceCoins,
        priceAmount,
        priceType,
        priceText,
        owned,
        favorite: !!item.favorite,
        liked: !!item.liked,
        actionText: !primaryRecipeId ? '暂无菜谱' : (owned ? '查看菜谱' : (priceAmount > 0 ? `加入菜谱 ${priceText}` : '免费加入菜谱')),
        menuText: recipeIds.length > 1 ? '加入套餐' : '加入菜单',
        canAddToMenu: owned && recipeIds.length > 0,
        watchMarked: false
      })
    })
  },

  switchTab(e) {
    const nextTab = e.currentTarget.dataset.tab
    this.logUserAction('SWITCH_TAB', {
      fromTab: this.data.activeTab,
      toTab: nextTab
    })
    this.transitionSourceIndex = -1
    this.setData({
      activeTab: nextTab,
      currentIndex: 0
    }, () => this.loadData())
  },

  onSwiperChange(e) {
    const currentIndex = e.detail.current
    const fromIndex = this.data.currentIndex
    const feed = this.data.feed || []
    const item = feed[currentIndex] || {}
    this.logUserAction('SWIPER_CHANGE', {
      fromIndex,
      toIndex: currentIndex,
      toVideoId: item.videoId || item.id || '',
      feedCount: feed.length
    })
    this.clearSeekFallbackTimer()
    this.pendingSeek = null
    this.progressDragging = false
    this.transitionSourceIndex = -1
    this.setData({
      currentIndex,
      videoPaused: false,
      videoDurationSeconds: 0,
      videoCurrentSeconds: 0,
      videoProgressValue: 0,
      videoProgressPercent: 0,
      showProgressHint: false,
      progressHintText: ''
    }, () => {
      this.activateCurrentVideoSource(currentIndex, () => {
        this.syncVideoPlayback()
        this.scheduleMarkCurrentWatched()
        this.prefetchPosterImages(currentIndex)
        this.ensureFeedAhead(currentIndex)
      })
    })
  },

  onSwiperTransition(e) {
    const detail = e.detail || {}
    const dy = Number(detail.dy || 0)
    const dx = Number(detail.dx || 0)
    if (Math.max(Math.abs(dy), Math.abs(dx)) < 16) return
    const currentIndex = this.data.currentIndex
    if (this.transitionSourceIndex === currentIndex) return
    const feed = this.data.feed || []
    const extraIndexes = []
    if (Math.abs(dy) >= Math.abs(dx) && dy !== 0) {
      const targetIndex = dy < 0 ? currentIndex + 1 : currentIndex - 1
      if (targetIndex >= 0 && targetIndex < feed.length) extraIndexes.push(targetIndex)
    } else {
      if (currentIndex > 0) extraIndexes.push(currentIndex - 1)
      if (currentIndex + 1 < feed.length) extraIndexes.push(currentIndex + 1)
    }
    if (!extraIndexes.length) return
    this.transitionSourceIndex = currentIndex
    this.logUserAction('SWIPER_TRANSITION', {
      currentIndex,
      dy,
      dx,
      targetIndexes: extraIndexes
    })
    this.prefetchPosterImages(currentIndex)
    this.ensureFeedAhead(currentIndex)
  },

  onSwiperAnimationFinish(e) {
    const detail = e.detail || {}
    const eventIndex = Number(detail.current)
    const feed = this.data.feed || []
    const currentIndex = !isNaN(eventIndex) && eventIndex >= 0 && eventIndex < feed.length ? eventIndex : this.data.currentIndex
    this.logUserAction('SWIPER_ANIMATION_FINISH', {
      eventIndex,
      finalIndex: currentIndex,
      feedCount: feed.length
    })
    this.transitionSourceIndex = -1
    if (currentIndex !== this.data.currentIndex) {
      this.setData({
        currentIndex,
        videoPaused: false,
        videoDurationSeconds: 0,
        videoCurrentSeconds: 0,
        videoProgressValue: 0,
        videoProgressPercent: 0,
        showProgressHint: false,
        progressHintText: ''
      }, () => {
        this.activateCurrentVideoSource(currentIndex, () => {
          this.syncVideoPlayback()
          this.scheduleMarkCurrentWatched()
          this.prefetchPosterImages(currentIndex)
          this.ensureFeedAhead(currentIndex)
        })
      })
      return
    }
    this.activateCurrentVideoSource(currentIndex, () => {
      this.syncVideoPlayback()
      this.prefetchPosterImages(currentIndex)
    })
  },

  onVideoPlay(e) {
    const index = Number(e.currentTarget.dataset.index)
    if (index === this.data.currentIndex) {
      const item = this.data.feed[index] || {}
      if (this.data.videoPaused) this.setData({ videoPaused: false })
      debugLog.info('HOME', 'VIDEO_PLAY', {
        index,
        videoId: item.videoId || ''
      })
      this.scheduleMarkCurrentWatched()
    }
  },

  onVideoPause(e) {
    const index = Number(e.currentTarget.dataset.index)
    if (index === this.data.currentIndex && this.pageActive) {
      const item = this.data.feed[index] || {}
      this.setData({ videoPaused: true })
      debugLog.info('HOME', 'VIDEO_PAUSE', {
        index,
        videoId: item.videoId || ''
      })
    }
  },

  onVideoTap(e) {
    const index = Number(e.currentTarget.dataset.index)
    if (index !== this.data.currentIndex) return
    const now = Date.now()
    if (this.ignoreVideoTapUntil && now < this.ignoreVideoTapUntil) {
      this.logUserAction('VIDEO_TAP_IGNORED_AFTER_PROGRESS', {
        index,
        ignoreLeftMilliseconds: this.ignoreVideoTapUntil - now
      })
      return
    }
    const ctx = wx.createVideoContext(`feedVideo${index}`, this)
    if (!ctx) return
    const item = this.data.feed[index] || {}
    this.logUserAction('VIDEO_TAP', {
      index,
      videoId: item.videoId || '',
      nextPaused: !this.data.videoPaused
    })
    if (this.data.videoPaused) {
      ctx.play()
      this.setData({ videoPaused: false })
    } else {
      ctx.pause()
      this.setData({ videoPaused: true })
    }
  },

  onVideoTimeUpdate(e) {
    const index = Number(e.currentTarget.dataset.index)
    if (index !== this.data.currentIndex || this.progressDragging) return
    const detail = e.detail || {}
    const duration = Number(detail.duration || this.data.videoDurationSeconds || 0)
    const currentTime = Number(detail.currentTime || 0)
    const progressValue = duration > 0 ? Math.max(0, Math.min(1000, Math.round(currentTime * 1000 / duration))) : 0
    const now = Date.now()
    const intervalMs = Math.max(100, numberOrDefault(config.VIDEO_PROGRESS_UPDATE_INTERVAL_MS, 500))
    if (this.lastProgressUiUpdateMs && now - this.lastProgressUiUpdateMs < intervalMs && progressValue !== 0) return
    this.lastProgressUiUpdateMs = now
    this.setData({
      videoDurationSeconds: duration,
      videoCurrentSeconds: currentTime,
      videoProgressValue: progressValue,
      videoProgressPercent: progressPercentFromValue(progressValue)
    })
  },

  readProgressTouchPosition(e, callback) {
    const touches = e.touches && e.touches.length ? e.touches : (e.changedTouches || [])
    const touch = touches && touches.length ? touches[0] : null
    if (!touch) return
    const clientX = Number(touch.clientX || 0)
    const handleRect = (rect) => {
      if (!rect || !rect.width) return
      callback(clientX, rect)
    }
    if (this.progressTouchRect && this.progressTouchRect.width) {
      handleRect(this.progressTouchRect)
      return
    }
    wx.createSelectorQuery()
      .in(this)
      .select('.progress-track')
      .boundingClientRect((rect) => {
        this.progressTouchRect = rect
        handleRect(rect)
      })
      .exec()
  },

  progressValueFromTouchDrag(clientX, rect) {
    const startValue = this.progressDragStartValue === undefined ? this.data.videoProgressValue : this.progressDragStartValue
    const startX = this.progressDragStartX === undefined ? clientX : this.progressDragStartX
    const width = rect && rect.width ? rect.width : 1
    return clampProgressValue(startValue + (clientX - startX) * 1000 / width)
  },

  updateProgressDragValue(index, value, source) {
    const seekSeconds = secondsFromProgress(this.data.videoDurationSeconds, value)
    this.setData({
      videoProgressValue: value,
      videoProgressPercent: progressPercentFromValue(value),
      videoCurrentSeconds: seekSeconds,
      showProgressHint: true,
      progressHintText: formatProgressHint(seekSeconds, this.data.videoDurationSeconds)
    })
    debugLog.info('HOME', 'VIDEO_PROGRESS_DRAG_VALUE', {
      index,
      value,
      seekSeconds,
      source
    })
  },

  finishProgressSeek(index, value, source) {
    const duration = Number(this.data.videoDurationSeconds || 0)
    const now = Date.now()
    this.progressDragging = false
    this.progressDragStartX = undefined
    this.progressDragStartValue = undefined
    this.progressDragLastValue = undefined
    this.progressDragMoved = false
    this.ignoreVideoTapUntil = now + Math.max(0, numberOrDefault(config.VIDEO_PROGRESS_TAP_IGNORE_MS, 500))
    this.lastProgressUiUpdateMs = now
    this.clearSeekFallbackTimer()
    this.setData({
      videoProgressValue: value,
      videoProgressPercent: progressPercentFromValue(value),
      showProgressHint: false,
      progressHintText: ''
    })
    if (index !== this.data.currentIndex || duration <= 0) {
      this.logUserAction('PROGRESS_DRAG_END_IGNORED', {
        index,
        value,
        durationSeconds: duration,
        currentIndex: this.data.currentIndex,
        source
      })
      return
    }
    const seekSeconds = secondsFromProgress(duration, value)
    const ctx = wx.createVideoContext(`feedVideo${index}`, this)
    const item = this.data.feed[index] || {}
    const token = `${now}_${Math.floor(Math.random() * 100000)}`
    this.pendingSeek = {
      token,
      index,
      videoId: item.videoId || '',
      seekSeconds,
      startMilliseconds: now
    }
    this.logUserAction('PROGRESS_DRAG_END', {
      index,
      videoId: item.videoId || '',
      value,
      seekSeconds,
      durationSeconds: duration,
      dragMilliseconds: this.progressDragStartMs ? now - this.progressDragStartMs : 0,
      source
    })
    if (ctx) {
      ctx.seek(seekSeconds)
      const fallbackMs = Math.max(300, numberOrDefault(config.VIDEO_SEEK_FALLBACK_PLAY_MS, 1200))
      this.seekFallbackTimer = setTimeout(() => {
        const pendingSeek = this.pendingSeek
        if (!pendingSeek || pendingSeek.token !== token) return
        debugLog.info('HOME', 'VIDEO_SEEK_FALLBACK_PLAY', {
          index,
          videoId: item.videoId || '',
          seekSeconds,
          waitMilliseconds: Date.now() - now
        })
        this.pendingSeek = null
        this.clearSeekFallbackTimer()
        this.playVideoAfterSeek(index, 'fallback')
      }, fallbackMs)
    } else {
      this.pendingSeek = null
      debugLog.warn('HOME', 'VIDEO_SEEK_CONTEXT_MISSING', {
        index,
        videoId: item.videoId || '',
        seekSeconds
      })
    }
    this.setData({
      videoPaused: false,
      videoCurrentSeconds: seekSeconds
    })
  },

  onProgressTouchStart(e) {
    const index = Number(e.currentTarget.dataset.index)
    this.progressTouchRect = null
    this.readProgressTouchPosition(e, (clientX, rect) => {
      const now = Date.now()
      const value = clampProgressValue(this.data.videoProgressValue)
      const seekSeconds = secondsFromProgress(this.data.videoDurationSeconds, value)
      this.progressDragging = true
      this.progressDragStartMs = now
      this.lastProgressDragUpdateMs = now
      this.progressDragStartX = clientX
      this.progressDragStartValue = value
      this.progressDragLastValue = value
      this.progressDragMoved = false
      this.progressTouchRect = rect
      this.logUserAction('PROGRESS_DRAG_START', {
        index,
        value,
        seekSeconds,
        durationSeconds: this.data.videoDurationSeconds,
        source: 'touch'
      })
      this.setData({
        showProgressHint: true,
        progressHintText: formatProgressHint(seekSeconds, this.data.videoDurationSeconds)
      })
    })
  },

  onProgressTouchMove(e) {
    const index = Number(e.currentTarget.dataset.index)
    if (!this.progressDragging) return
    const now = Date.now()
    const intervalMs = Math.max(50, numberOrDefault(config.VIDEO_PROGRESS_DRAG_UPDATE_INTERVAL_MS, 120))
    if (this.lastProgressDragUpdateMs && now - this.lastProgressDragUpdateMs < intervalMs) return
    this.readProgressTouchPosition(e, (clientX, rect) => {
      const value = this.progressValueFromTouchDrag(clientX, rect)
      if (Math.abs(clientX - (this.progressDragStartX === undefined ? clientX : this.progressDragStartX)) >= 2) {
        this.progressDragMoved = true
      }
      this.progressDragLastValue = value
      this.lastProgressDragUpdateMs = now
      this.updateProgressDragValue(index, value, 'touch_move')
    })
  },

  onProgressTouchEnd(e) {
    const index = Number(e.currentTarget.dataset.index)
    const finish = (value, moved) => {
      this.progressTouchRect = null
      if (!moved) {
        this.progressDragging = false
        this.progressDragStartX = undefined
        this.progressDragStartValue = undefined
        this.progressDragLastValue = undefined
        this.progressDragMoved = false
        this.setData({ showProgressHint: false, progressHintText: '' })
        this.logUserAction('PROGRESS_DRAG_END_NO_MOVE', { index, value })
        return
      }
      this.finishProgressSeek(index, value, 'touch_end')
    }
    if (!this.progressDragging) {
      finish(clampProgressValue(this.data.videoProgressValue), false)
      return
    }
    this.readProgressTouchPosition(e, (clientX, rect) => {
      const value = this.progressValueFromTouchDrag(clientX, rect)
      const moved = !!this.progressDragMoved ||
        Math.abs(clientX - (this.progressDragStartX === undefined ? clientX : this.progressDragStartX)) >= 2
      this.progressDragLastValue = value
      finish(value, moved)
    })
  },

  onProgressTouchCancel(e) {
    const index = Number(e.currentTarget.dataset.index)
    this.progressDragging = false
    this.progressTouchRect = null
    this.progressDragStartX = undefined
    this.progressDragStartValue = undefined
    this.progressDragLastValue = undefined
    this.progressDragMoved = false
    this.setData({ showProgressHint: false, progressHintText: '' })
    this.logUserAction('PROGRESS_DRAG_CANCEL', { index })
  },

  onProgressChanging(e) {
    const now = Date.now()
    const value = clampProgressValue(e.detail && e.detail.value !== undefined ? e.detail.value : 0)
    const seekSeconds = secondsFromProgress(this.data.videoDurationSeconds, value)
    if (!this.progressDragging) {
      this.progressDragging = true
      this.progressDragStartMs = now
      this.logUserAction('PROGRESS_DRAG_START', {
        index: Number(e.currentTarget.dataset.index),
        value,
        seekSeconds,
        durationSeconds: this.data.videoDurationSeconds
      })
    }
    const intervalMs = Math.max(50, numberOrDefault(config.VIDEO_PROGRESS_DRAG_UPDATE_INTERVAL_MS, 120))
    if (!this.lastProgressDragUpdateMs || now - this.lastProgressDragUpdateMs >= intervalMs) {
      this.lastProgressDragUpdateMs = now
      this.setData({
        videoProgressValue: value,
        videoProgressPercent: progressPercentFromValue(value),
        videoCurrentSeconds: seekSeconds,
        showProgressHint: true,
        progressHintText: formatProgressHint(seekSeconds, this.data.videoDurationSeconds)
      })
    }
  },

  onProgressAreaTap(e) {
    const index = Number(e.currentTarget.dataset.index)
    this.ignoreVideoTapUntil = Date.now() + Math.max(0, numberOrDefault(config.VIDEO_PROGRESS_TAP_IGNORE_MS, 500))
    this.logUserAction('PROGRESS_AREA_TAP', {
      index,
      ignoreMilliseconds: Math.max(0, numberOrDefault(config.VIDEO_PROGRESS_TAP_IGNORE_MS, 500))
    })
  },

  onProgressChange(e) {
    const index = Number(e.currentTarget.dataset.index)
    const value = clampProgressValue(e.detail && e.detail.value !== undefined ? e.detail.value : 0)
    this.finishProgressSeek(index, value, 'slider')
  },

  onVideoLoadedMetadata(e) {
    const index = Number(e.currentTarget.dataset.index)
    const item = this.data.feed[index] || {}
    if (index === this.data.currentIndex) {
      const duration = e.detail && e.detail.duration ? Number(e.detail.duration) : 0
      if (duration > 0) this.setData({ videoDurationSeconds: duration })
      this.prefetchPosterImages(index)
    }
    debugLog.info('HOME', 'VIDEO_METADATA', {
      index,
      videoId: item.videoId || '',
      width: e.detail && e.detail.width ? e.detail.width : 0,
      height: e.detail && e.detail.height ? e.detail.height : 0,
      duration: e.detail && e.detail.duration ? e.detail.duration : 0
    })
  },

  onVideoCanPlay(e) {
    const index = Number(e.currentTarget.dataset.index)
    const item = this.data.feed[index] || {}
    debugLog.info('HOME', 'VIDEO_CAN_PLAY', {
      index,
      videoId: item.videoId || ''
    })
    if (index === this.data.currentIndex) {
      this.prefetchPosterImages(index)
    }
  },

  onVideoWaiting(e) {
    const index = Number(e.currentTarget.dataset.index)
    const item = this.data.feed[index] || {}
    const key = `${index}:${item.videoId || ''}`
    const now = Date.now()
    const intervalMs = Math.max(1000, numberOrDefault(config.VIDEO_WAITING_LOG_INTERVAL_MS, 5000))
    if (this.lastWaitingLogByKey && this.lastWaitingLogByKey[key] && now - this.lastWaitingLogByKey[key] < intervalMs) return
    this.lastWaitingLogByKey = this.lastWaitingLogByKey || {}
    this.lastWaitingLogByKey[key] = now
    debugLog.info('HOME', 'VIDEO_WAITING', {
      index,
      videoId: item.videoId || '',
      pendingSeek: !!this.pendingSeek
    })
  },

  onVideoSeekComplete(e) {
    const index = Number(e.currentTarget.dataset.index)
    const item = this.data.feed[index] || {}
    const position = e.detail && e.detail.position !== undefined ? Number(e.detail.position) : 0
    const pendingSeek = this.pendingSeek
    const matchedPendingSeek = !!(pendingSeek && pendingSeek.index === index)
    debugLog.info('HOME', 'VIDEO_SEEK_COMPLETE', {
      index,
      videoId: item.videoId || '',
      position,
      matchedPendingSeek,
      pendingSeekSeconds: pendingSeek ? pendingSeek.seekSeconds : 0,
      waitMilliseconds: pendingSeek ? Date.now() - pendingSeek.startMilliseconds : 0
    })
    if (matchedPendingSeek) {
      this.clearSeekFallbackTimer()
      this.pendingSeek = null
      this.playVideoAfterSeek(index, 'seekcomplete')
    }
  },

  onPosterLoad(e) {
    const index = Number(e.currentTarget.dataset.index)
    const item = this.data.feed[index] || {}
    debugLog.info('HOME', 'VIDEO_POSTER_LOAD_OK', {
      index,
      videoId: item.videoId || ''
    })
  },

  onPosterError(e) {
    const index = Number(e.currentTarget.dataset.index)
    const item = this.data.feed[index] || {}
    if (!item) return
    const updates = {}
    updates[`feed[${index}].posterSrc`] = ''
    updates[`feed[${index}].posterLoaded`] = false
    updates[`feed[${index}].posterFailed`] = true
    this.setData(updates)
    this.posterFailCount = (this.posterFailCount || 0) + 1
    debugLog.warn('HOME', 'VIDEO_POSTER_LOAD_FAIL', {
      index,
      videoId: item.videoId || '',
      posterUrl: item.posterUrl || '',
      errMsg: e && e.detail && e.detail.errMsg ? e.detail.errMsg : '',
      posterFailCount: this.posterFailCount
    })
    const disableAfter = Math.max(1, numberOrDefault(config.VIDEO_POSTER_DISABLE_AFTER_FAIL_COUNT, 1))
    if (this.posterFailCount >= disableAfter) {
      this.posterPrefetchDisabledByFail = true
      debugLog.warn('HOME', 'VIDEO_POSTER_PREFETCH_DISABLED', {
        posterFailCount: this.posterFailCount,
        disableAfter
      })
    }
  },

  onVideoError(e) {
    const index = Number(e.currentTarget.dataset.index)
    const item = this.data.feed[index] || {}
    debugLog.warn('HOME', 'VIDEO_ERROR', {
      index,
      videoId: item.videoId || '',
      errMsg: e && e.detail && e.detail.errMsg ? e.detail.errMsg : ''
    })
    wx.showToast({ title: '视频加载失败', icon: 'none' })
  },

  ensureFeedAhead(currentIndex) {
    const prefetchCount = Math.max(0, Number(config.VIDEO_FEED_PREFETCH_COUNT || 0))
    const metadataAheadCount = this.metadataAheadCount()
    const feed = this.data.feed || []
    const aheadCount = Math.max(0, feed.length - currentIndex - 1)
    const threshold = prefetchCount > 0 ? Math.max(prefetchCount, metadataAheadCount) : metadataAheadCount
    if (aheadCount >= threshold) return
    if (threshold > 0 || currentIndex >= feed.length - 1) {
      this.loadMoreFeed({ foreground: currentIndex >= feed.length - 1 })
    }
  },

  activateCurrentVideoSource(currentIndex, callback) {
    return this.activateVideoSources(currentIndex, [], callback)
  },

  firstFrameIndexes(currentIndex) {
    const feed = this.data.feed || []
    const sideCount = this.firstFrameSideCount()
    const indexes = []
    for (let offset = 1; offset <= sideCount; offset += 1) {
      const nextIndex = currentIndex + offset
      if (nextIndex < feed.length) indexes.push(nextIndex)
    }
    return indexes
  },

  clearPosterPrefetchRequests() {
    this.posterPrefetchQueue = []
    ;(this.posterPrefetchTasks || []).forEach((task) => {
      if (task && typeof task.abort === 'function') {
        try {
          task.abort()
        } catch (err) {
          debugLog.warn('HOME', 'VIDEO_POSTER_PREFETCH_ABORT_FAIL', {
            error: err && err.message ? err.message : String(err)
          })
        }
      }
    })
    this.posterPrefetchTasks = []
    this.posterPrefetchRunning = 0
    this.posterPrefetchRequestedByVideoId = {}
  },

  enqueuePosterPrefetchRequests(indexes) {
    if (config.VIDEO_POSTER_PREFETCH_REQUEST_ENABLED !== true) return
    const feed = this.data.feed || []
    this.posterPrefetchRequestedByVideoId = this.posterPrefetchRequestedByVideoId || {}
    this.posterPrefetchQueue = this.posterPrefetchQueue || []
    ;(indexes || []).forEach((index) => {
      const item = feed[index] || {}
      const videoId = item.videoId || item.id || ''
      if (!videoId || !item.posterUrl || item.posterFailed || this.posterPrefetchRequestedByVideoId[videoId]) return
      this.posterPrefetchRequestedByVideoId[videoId] = true
      this.posterPrefetchQueue.push({
        index,
        videoId,
        feedKey: item.feedKey || '',
        posterUrl: item.posterUrl,
        loadRequestId: this.loadRequestId || 0
      })
    })
    this.pumpPosterPrefetchQueue()
  },

  pumpPosterPrefetchQueue() {
    const maxConcurrency = Math.max(1, Number(config.VIDEO_POSTER_PREFETCH_CONCURRENCY || 2))
    this.posterPrefetchQueue = this.posterPrefetchQueue || []
    this.posterPrefetchTasks = this.posterPrefetchTasks || []
    this.posterPrefetchRunning = this.posterPrefetchRunning || 0
    while (this.posterPrefetchRunning < maxConcurrency && this.posterPrefetchQueue.length) {
      const taskInfo = this.posterPrefetchQueue.shift()
      if (!taskInfo || taskInfo.loadRequestId !== (this.loadRequestId || 0)) continue
      this.posterPrefetchRunning += 1
      debugLog.info('HOME', 'VIDEO_POSTER_PREFETCH_START', {
        index: taskInfo.index,
        videoId: taskInfo.videoId,
        running: this.posterPrefetchRunning,
        queued: this.posterPrefetchQueue.length
      })
      const requestTask = wx.request({
        url: taskInfo.posterUrl,
        method: 'GET',
        responseType: 'arraybuffer',
        timeout: Math.max(1000, Number(config.VIDEO_POSTER_PREFETCH_TIMEOUT_MS || 10000)),
        success: (res) => {
          const body = res && res.data ? res.data : null
          debugLog.info('HOME', 'VIDEO_POSTER_PREFETCH_OK', {
            index: taskInfo.index,
            videoId: taskInfo.videoId,
            statusCode: res ? res.statusCode : 0,
            bytes: body && body.byteLength ? body.byteLength : 0
          })
        },
        fail: (err) => {
          debugLog.warn('HOME', 'VIDEO_POSTER_PREFETCH_FAIL', {
            index: taskInfo.index,
            videoId: taskInfo.videoId,
            errMsg: err && err.errMsg ? err.errMsg : String(err)
          })
        },
        complete: () => {
          this.posterPrefetchRunning = Math.max(0, (this.posterPrefetchRunning || 1) - 1)
          this.posterPrefetchTasks = (this.posterPrefetchTasks || []).filter((item) => item !== requestTask)
          this.pumpPosterPrefetchQueue()
        }
      })
      this.posterPrefetchTasks.push(requestTask)
    }
  },

  prefetchPosterImages(currentIndex) {
    if (config.VIDEO_POSTER_PREFETCH_ENABLED !== true) return
    if (this.posterPrefetchDisabledByFail) return
    const feed = this.data.feed || []
    const indexes = this.firstFrameIndexes(currentIndex)
    const updates = {}
    let changed = false
    indexes.forEach((index) => {
      const item = feed[index] || {}
      if (!item.posterUrl || item.posterFailed || item.posterLoaded || item.posterSrc === item.posterUrl) return
      updates[`feed[${index}].posterSrc`] = item.posterUrl
      updates[`feed[${index}].posterLoaded`] = true
      changed = true
    })
    if (changed) {
      this.setData(updates)
      debugLog.info('HOME', 'VIDEO_POSTER_URL_READY', {
        currentIndex,
        preparedIndexes: indexes
      })
    }
    this.enqueuePosterPrefetchRequests(indexes)
  },

  activateVideoSources(currentIndex, extraIndexes, callback) {
    const feed = this.data.feed || []
    const activeIndexes = {}
    activeIndexes[currentIndex] = true
    ;(extraIndexes || []).forEach((index) => {
      if (index >= 0 && index < feed.length) activeIndexes[index] = true
    })
    const updates = {}
    let changed = false
    for (let i = 0; i < feed.length; i += 1) {
      const targetUrl = activeIndexes[i] ? (feed[i].rawVideoUrl || '') : ''
      if ((feed[i].videoUrl || '') !== targetUrl) {
        updates[`feed[${i}].videoUrl`] = targetUrl
        changed = true
      }
    }
    if (!changed) {
      if (callback) callback()
      return
    }
    this.setData(updates, () => {
      const item = (this.data.feed || [])[currentIndex] || {}
      debugLog.info('HOME', 'VIDEO_SOURCE_ACTIVE', {
        currentIndex,
        videoId: item.videoId || '',
        hasVideoUrl: !!item.videoUrl,
        extraIndexes: extraIndexes || [],
        disabledVideoSources: Math.max(0, feed.length - Object.keys(activeIndexes).length)
      })
      if (callback) callback()
    })
  },

  syncVideoPlayback(options) {
    const safeOptions = options || {}
    const feed = this.data.feed || []
    for (let i = 0; i < feed.length; i += 1) {
      const ctx = wx.createVideoContext(`feedVideo${i}`, this)
      if (!ctx) continue
      if (i === this.data.currentIndex && this.pageActive) {
        if (safeOptions.restorePosition && this.data.videoCurrentSeconds > 0) {
          ctx.seek(this.data.videoCurrentSeconds)
          debugLog.info('HOME', 'VIDEO_RESTORE_POSITION', {
            index: i,
            videoId: (feed[i] || {}).videoId || '',
            seconds: this.data.videoCurrentSeconds,
            paused: !!this.data.videoPaused
          })
          if (!this.data.videoPaused) {
            this.playVideoAfterSeek(i, 'restore')
            continue
          }
        }
        if (this.data.videoPaused) ctx.pause()
        else ctx.play()
      } else {
        ctx.pause()
      }
    }
  },

  pauseAllVideos() {
    const feed = this.data.feed || []
    for (let i = 0; i < feed.length; i += 1) {
      const ctx = wx.createVideoContext(`feedVideo${i}`, this)
      if (ctx) ctx.pause()
    }
  },

  pauseVideoIndexes(indexes) {
    ;(indexes || []).forEach((index) => {
      const ctx = wx.createVideoContext(`feedVideo${index}`, this)
      const item = (this.data.feed || [])[index] || {}
      if (ctx) {
        ctx.seek(0)
        ctx.pause()
        debugLog.info('HOME', 'VIDEO_ADJACENT_FIRST_FRAME_PREPARE', {
          index,
          videoId: item.videoId || ''
        })
      }
    })
  },

  clearWatchTimer() {
    if (this.watchTimer) {
      clearTimeout(this.watchTimer)
      this.watchTimer = null
    }
  },

  scheduleMarkCurrentWatched() {
    this.clearWatchTimer()
    const index = this.data.currentIndex
    const item = (this.data.feed || [])[index] || {}
    if (!item.videoId || item.watchMarked) return
    const feedKey = item.feedKey
    this.watchTimer = setTimeout(() => {
      const current = (this.data.feed || [])[this.data.currentIndex] || {}
      if (!this.pageActive || current.feedKey !== feedKey || current.watchMarked) return
      api.markVideoWatched(current.videoId)
        .then((res) => {
          if (!res.ok) throw new Error(res.message || '标记失败')
          const feed = this.data.feed.slice()
          const latest = feed[this.data.currentIndex] || {}
          if (latest.feedKey === feedKey) {
            feed[this.data.currentIndex] = Object.assign({}, latest, { watchMarked: true })
            this.setData({ feed }, () => this.ensureFeedAhead(this.data.currentIndex))
          } else {
            this.ensureFeedAhead(this.data.currentIndex)
          }
          debugLog.info('HOME', 'VIDEO_WATCH_MARK_OK', {
            videoId: current.videoId,
            watchedAtSeconds: res.watchedAtSeconds || 0
          })
        })
        .catch((err) => {
          debugLog.warn('HOME', 'VIDEO_WATCH_MARK_FAIL', {
            videoId: current.videoId,
            error: err && err.message ? err.message : String(err)
          })
        })
    }, Math.max(0, Number(config.VIDEO_FEED_WATCH_MARK_DELAY_MS || 0)))
  },

  openRecipe(e) {
    const index = Number(e.currentTarget.dataset.index)
    const item = this.data.feed[index] || {}
    const recipeId = item.primaryRecipeId
    this.logUserAction('OPEN_RECIPE', {
      index,
      videoId: item.videoId || '',
      recipeId,
      owned: !!item.owned
    })
    if (!recipeId) {
      wx.showToast({ title: '菜谱不存在', icon: 'none' })
      return
    }
    wx.navigateTo({ url: `/pages/recipe/recipe?id=${recipeId}` })
  },

  addToMenu(e) {
    const index = Number(e.currentTarget.dataset.index)
    const item = this.data.feed[index] || {}
    const recipeIds = item.recipeIds || []
    this.logUserAction('ADD_TO_MENU_TAP', {
      index,
      videoId: item.videoId || '',
      recipeIds,
      owned: !!item.owned
    })
    if (!item.owned) {
      wx.showToast({ title: '请先购买菜谱', icon: 'none' })
      return
    }
    const app = getApp()
    const ids = app.globalData.selectedRecipeIds || []
    let added = 0
    recipeIds.forEach((recipeId) => {
      if (recipeId && ids.indexOf(recipeId) < 0) {
        ids.push(recipeId)
        added += 1
      }
    })
    app.globalData.selectedRecipeIds = ids
    debugLog.info('HOME', 'ADD_TO_MENU_OK', {
      videoId: item.videoId || '',
      recipeIds,
      added,
      selectedCount: ids.length
    })
    wx.showToast({ title: added ? '已加入菜单' : '已在菜单中', icon: added ? 'success' : 'none' })
  },

  handleRecipeAction(e) {
    const index = Number(e.currentTarget.dataset.index)
    const item = this.data.feed[index] || {}
    this.logUserAction('RECIPE_ACTION_TAP', {
      index,
      videoId: item.videoId || '',
      recipeId: item.primaryRecipeId || '',
      owned: !!item.owned,
      priceAmount: item.priceAmount || 0,
      priceType: item.priceType || ''
    })
    if (!item.primaryRecipeId) {
      wx.showToast({ title: '菜谱不存在', icon: 'none' })
      return
    }
    if (item.owned) {
      this.openRecipe(e)
      return
    }
    if (this.data.buyingRecipeId) return
    wx.showModal({
      title: '购买菜谱',
      content: item.priceAmount > 0 ? `确认花费 ${item.priceText} 购买《${item.title}》吗？` : `确认加入免费菜谱《${item.title}》吗？`,
      confirmText: item.priceAmount > 0 ? '购买' : '加入',
      success: (modalRes) => {
        this.logUserAction('RECIPE_ACTION_MODAL', {
          index,
          recipeId: item.primaryRecipeId,
          confirm: !!modalRes.confirm
        })
        if (!modalRes.confirm) return
        this.setData({ buyingRecipeId: item.primaryRecipeId })
        api.joinRecipe(item.primaryRecipeId)
          .then((res) => {
            debugLog.info('HOME', 'RECIPE_JOIN_OK', {
              recipeId: item.primaryRecipeId,
              ok: !!res.ok,
              message: res.message || ''
            })
            wx.showToast({ title: res.message || (res.ok ? '购买成功' : '购买失败'), icon: res.ok ? 'success' : 'none' })
            this.setData({ buyingRecipeId: '' })
            this.loadData()
          })
          .catch((err) => {
            debugLog.warn('HOME', 'RECIPE_JOIN_FAIL', {
              recipeId: item.primaryRecipeId,
              error: err && err.message ? err.message : String(err)
            })
            wx.showToast({ title: err.message || '购买失败', icon: 'none' })
            this.setData({ buyingRecipeId: '' })
          })
      }
    })
  },

  favorite(e) {
    const index = Number(e.currentTarget.dataset.index)
    const item = this.data.feed[index] || {}
    const videoId = item.videoId || item.id || ''
    this.logUserAction('FAVORITE_TAP', {
      index,
      videoId,
      currentFavorite: !!item.favorite
    })
    if (!videoId) {
      wx.showToast({ title: '视频不存在', icon: 'none' })
      return
    }
    api.toggleFavorite(videoId, 'video')
      .then((res) => {
        if (!res.ok) throw new Error(res.message)
        wx.showToast({ title: res.favorite ? '已收藏' : '已取消', icon: 'none' })
        const feed = this.data.feed.slice()
        feed[index] = Object.assign({}, item, { favorite: !!res.favorite })
        this.setData({ feed })
        debugLog.info('HOME', 'FAVORITE_OK', {
          videoId,
          favorite: !!res.favorite
        })
      })
      .catch((err) => {
        debugLog.warn('HOME', 'FAVORITE_FAIL', {
          videoId,
          error: err && err.message ? err.message : String(err)
        })
        wx.showToast({ title: '收藏失败', icon: 'none' })
      })
  },

  toggleLike(e) {
    const index = Number(e.currentTarget.dataset.index)
    const item = this.data.feed[index] || {}
    const videoId = item.videoId || item.id || ''
    this.logUserAction('LIKE_TAP', {
      index,
      videoId,
      currentLiked: !!item.liked,
      currentLikes: Number(item.likes || 0)
    })
    if (!videoId) {
      wx.showToast({ title: '视频不存在', icon: 'none' })
      return
    }
    api.toggleLike('video', videoId)
      .then((res) => {
        if (!res.ok) throw new Error(res.message)
        const feed = this.data.feed.slice()
        const liked = !!res.liked
        feed[index] = Object.assign({}, item, {
          liked,
          likes: Math.max(0, Number(item.likes || 0) + (liked ? 1 : -1))
        })
        this.setData({ feed })
        wx.showToast({ title: liked ? '已点赞' : '已取消', icon: 'none' })
        debugLog.info('HOME', 'LIKE_OK', {
          videoId,
          liked,
          likes: feed[index].likes
        })
      })
      .catch((err) => {
        debugLog.warn('HOME', 'LIKE_FAIL', {
          videoId,
          error: err && err.message ? err.message : String(err)
        })
        wx.showToast({ title: err.message || '点赞失败', icon: 'none' })
      })
  },

  openComments() {
    this.logUserAction('OPEN_COMMENTS', {})
    wx.showToast({ title: '评论区稍后接入', icon: 'none' })
  }
})
