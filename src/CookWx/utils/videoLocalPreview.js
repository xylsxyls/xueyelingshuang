// 原生播放器只读取我们已下载的本地片段，网络与缓存完全由缓存层控制。
function isDeveloperTools() {
  try {
    const info = typeof wx.getDeviceInfo === 'function' ? wx.getDeviceInfo() : wx.getSystemInfoSync()
    return info && info.platform === 'devtools'
  } catch (error) { return false }
}

class LocalVideoPreview {
  constructor(owner, entry, positionSeconds, generation, timeoutMs, options = {}) {
    this.owner = owner
    this.entry = entry
    this.generation = generation
    this.operationGeneration = options.operationGeneration === undefined ? generation : options.operationGeneration
    this.preload = !!options.preload
    this.wanted = options.wanted || null
    this.onPreloadError = options.onPreloadError || null
    this.positionSeconds = positionSeconds
    this.baseSeconds = entry.actualStartMilliseconds / 1000
    this.startSeconds = Math.max(0, positionSeconds - this.baseSeconds)
    this.timeoutMs = timeoutMs
    this.context = null
    this.stopped = false
    this.failed = false
    this.started = false
    this.metadata = null
    this.playing = false
    this.hasTime = false
    this.seekIssued = false
  }

  start() {
    const userPath = wx.env && wx.env.USER_DATA_PATH
    if (!userPath || typeof this.entry.path !== 'string' || this.entry.path.indexOf(userPath + '/cook-video-cache/') !== 0) {
      return Promise.reject(new Error('预览只能读取本地视频缓存'))
    }
    return new Promise((resolve, reject) => {
      this.resolve = resolve
      this.reject = reject
      this.timer = setTimeout(() => this.fail(new Error('本地视频片段加载超时')), this.timeoutMs)
      const previous = (this.owner.data.previewSegments || []).filter(item => item.ready).slice(-1)
      this.owner.setData({ previewSegments: previous.concat([{
        generation: this.generation, path: this.entry.path, startSeconds: this.startSeconds,
        id: 'localPreview' + this.generation, ready: false
      }]) }, () => {
        if (!this.current()) return
        try {
          this.context = wx.createVideoContext('localPreview' + this.generation, this.owner)
          // 工具部分节点直到play才加载元数据；定位前保持透明且静音。
          this.context.play()
          this.positionAfterMetadata()
        } catch (error) { this.fail(error) }
      })
    })
  }

  currentPosition() { return this.positionSeconds }

  current() {
    return !this.stopped && !this.failed && (this.wanted ? this.wanted() : this.owner._generation === this.operationGeneration)
  }

  activate(generation) {
    this.operationGeneration = generation
    this.wanted = null
    this.preload = false
    const node = (this.owner.data.previewSegments || []).find(item => item.generation === this.generation)
    if (!this.started || !node || !this.current()) return Promise.reject(new Error('预加载片段已失效'))
    this.owner.setData({ previewSegments: [Object.assign({}, node, { ready: true })] }, () => { if (this.current() && this.context) this.context.play() })
    return Promise.resolve(this.metadata)
  }

  seek(positionSeconds, generation) {
    if (this.stopped || this.failed || !this.context || !this.metadata) return Promise.reject(new Error('本地播放器不可定位'))
    if (this.reject) this.reject(new Error('本地视频定位已取消'))
    clearTimeout(this.timer)
    this.operationGeneration = generation
    this.startSeconds = Math.max(0, positionSeconds - this.baseSeconds)
    this.hasTime = false
    this.seekIssued = false
    this.started = false
    return new Promise((resolve, reject) => {
      this.resolve = resolve
      this.reject = reject
      this.timer = setTimeout(() => this.fail(new Error('本地视频定位超时')), this.timeoutMs)
      this.positionAfterMetadata()
    })
  }

  positionAfterMetadata() {
    if (!this.context || !this.metadata || this.seekIssued || this.stopped) return
    this.seekIssued = true
    try {
      this.context.seek(this.startSeconds)
      this.context.play()
    } catch (error) { this.fail(error) }
  }

  handle(event) {
    if (!this.current() || Number(event.currentTarget && event.currentTarget.dataset.generation) !== this.generation) return
    const detail = event.detail || {}
    if (event.type === 'error') { this.fail(new Error(detail.errMsg || detail.message || '本地视频片段解码失败')); return }
    if (event.type === 'loadedmetadata') { this.metadata = detail; this.positionAfterMetadata() }
    if (event.type === 'play') this.playing = true
    if (event.type === 'timeupdate') {
      if (this.preload && this.started) return
      const localSeconds = Number(detail.currentTime)
      const atTarget = localSeconds >= this.startSeconds - 0.05 && localSeconds <= this.startSeconds + 0.5
      const advances = this.baseSeconds + localSeconds >= this.positionSeconds - 0.05
      if (this.seekIssued && isFinite(localSeconds) && (this.started ? advances : atTarget)) {
        this.positionSeconds = Math.max(this.entry.startSeconds, this.baseSeconds + localSeconds)
        this.hasTime = true
      }
    }
    if (!this.started && this.metadata && this.playing && this.hasTime) {
      this.started = true
      const node = (this.owner.data.previewSegments || []).find(item => item.generation === this.generation)
      if (this.preload) {
        this.context.pause()
        // timeupdate确认可能已前进数帧；待命前回到真实片段起点。
        this.context.seek(this.startSeconds)
        this.positionSeconds = this.baseSeconds + this.startSeconds
        this.playing = false
      } else if (node) this.owner.setData({ previewSegments: [Object.assign({}, node, { ready: true })] })
      clearTimeout(this.timer)
      const resolve = this.resolve
      this.resolve = null
      this.reject = null
      resolve(this.metadata)
    }
    if (event.type === 'waiting' && !this.preload) this.owner.triggerEvent('waiting', { currentTime: this.positionSeconds })
    if (event.type === 'ended') {
      if (!this.started) { this.fail(new Error('视频片段没有可播放帧')); return }
      if (!this.preload) this.owner.onSegmentEnded(this.operationGeneration)
    }
  }

  fail(error) {
    if (this.stopped || this.failed) return
    this.failed = true
    clearTimeout(this.timer)
    if (this.reject) {
      const reject = this.reject
      this.reject = null
      this.resolve = null
      reject(error)
    } else if (this.started) {
      if (this.preload) { if (this.onPreloadError) this.onPreloadError(error) }
      else this.owner.handlePlaybackError(error, this.operationGeneration)
    }
  }

  stop(keepFrame = false) {
    if (this.stopped) return
    this.stopped = true
    clearTimeout(this.timer)
    if (this.reject) {
      this.reject(new Error('本地视频片段加载已取消'))
      this.reject = null
      this.resolve = null
    }
    if (this.context) {
      try { this.context.pause() } catch (error) {}
      this.context = null
    }
    const current = this.owner.data.previewSegments || []
    if (this.owner._alive && current.some(item => item.generation === this.generation)) {
      // 加载中取消保留之前已确认的帧，不保留透明的未就绪节点。
      const others = current.filter(item => item.generation !== this.generation)
      const own = current.find(item => item.generation === this.generation && item.ready)
      this.owner.setData({ previewSegments: keepFrame && own ? [own].concat(others.filter(item => !item.ready).slice(-1)) : others })
    }
  }
}

module.exports = { LocalVideoPreview, isDeveloperTools }