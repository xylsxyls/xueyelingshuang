const cache = require('../../utils/videoSegmentCache')
const Renderer = require('../../utils/videoFrameRenderer')
const config = require('../../utils/config')
const { decoderOperation } = require('../../utils/videoDecoderLifecycle')
const { LocalVideoPreview, isDeveloperTools } = require('../../utils/videoLocalPreview')

function numberOr(value, fallback) {
  const number = Number(value)
  return isFinite(number) ? number : fallback
}

Component({
  properties: {
    videoId: { type: String, value: '', observer: 'onSourceChanged' },
    videoUrl: { type: String, value: '', observer: 'onSourceChanged' },
    active: { type: Boolean, value: false, observer: 'onActiveChanged' },
    autoplay: { type: Boolean, value: false, observer: 'onAutoplayChanged' },
    loop: { type: Boolean, value: true },
    index: { type: Number, value: 0 }
  },

  data: {
    status: '',
    localPreview: false,
    previewSegments: []
  },

  lifetimes: {
    ready() {
      this._alive = true
      this._pageVisible = this._pageVisible !== false
      this._generation = 0
      this._positionSeconds = 0
      this._durationSeconds = 0
      this._desiredPlaying = false
      this._canvas = null
      this._renderer = null
      this._renderIntervalMs = 1000 / Math.max(1, numberOr(config.VIDEO_RENDER_MAX_FPS, 30))
      this._lastRenderAt = 0
      this._currentEntry = null
      this._identity = String(this.properties.videoId || '') + '|' + String(this.properties.videoUrl || '')
      this._timeTimer = null
      this._frameRequestId = null
      this._renderErrorReported = false
      this._previewMode = isDeveloperTools()
      this._readyPromise = this._previewMode
        ? new Promise(resolve => this.setData({ localPreview: true }, resolve))
        : this.createCanvas()
      this._readyPromise.then(() => {
        if (this.properties.active && this.properties.autoplay) this.play()
      }).catch(() => this.setData({ status: '当前基础库不支持 Canvas 视频渲染' }))
    },
    detached() {
      this._alive = false
      clearTimeout(this._sourceChangeTimer)
      this._sourceChangeTimer = null
      this._generation += 1
      this._desiredPlaying = false
      cache.cancelUnused()
      this.stopDecoder()
      cache.unpin(this._currentEntry)
      this._currentEntry = null
    }
  },

  pageLifetimes: {
    hide() { this._pageVisible = false; this.pause() },
    show() {
      this._pageVisible = true
      if (this.properties.active && this.properties.autoplay) this.play()
    }
  },

  methods: {
    createCanvas() {
      return new Promise((resolve, reject) => {
        this.createSelectorQuery().select('#videoCanvas').fields({ node: true, size: true }).exec((results) => {
          const entry = results && results[0]
          if (!entry || !entry.node) { reject(new Error('canvas unavailable')); return }
          this._canvas = entry.node
          this._renderer = new Renderer(entry.node)
          if (!this._renderer.supported) { reject(new Error('canvas renderer unavailable')); return }
          resolve()
        })
      })
    },

    onActiveChanged(active) {
      if (!this._alive) return
      if (!active) {
        this.pause()
        return
      }
      if (this.properties.autoplay) this.play()
    },

    onAutoplayChanged(autoplay) {
      if (!this._alive || !this.properties.active) return
      if (autoplay) this.play()
      else this.pause()
    },

    onSourceChanged() {
      if (!this._alive) return
      if (this._sourceChangeTimer) clearTimeout(this._sourceChangeTimer)
      this._sourceChangeTimer = setTimeout(() => {
        this._sourceChangeTimer = null
        if (!this._alive) return
        const identity = String(this.properties.videoId || '') + '|' + String(this.properties.videoUrl || '')
        if (identity === this._identity) return
        this._identity = identity
        this._generation += 1
        this._desiredPlaying = false
        cache.cancelUnused()
        this._loadInFlight = false
        this._positionSeconds = 0
        this._durationSeconds = 0
        this._frameReady = false
        this._renderErrorReported = false
        this.stopDecoder()
        cache.unpin(this._currentEntry)
        this._currentEntry = null
        if (this._renderer) this._renderer.clear()
        if (this.properties.active && this.properties.autoplay) this.play()
      }, 0)
    },

    onTap() {
      this.triggerEvent('playertap', {})
    },

    currentPosition() {
      if (this._preview) return this._preview.currentPosition()
      if (this._desiredPlaying && this._decoder && this._clockStartAt) {
        return Math.min(this._durationSeconds || Number.MAX_SAFE_INTEGER,
          this._clockStartPosition + (Date.now() - this._clockStartAt) / 1000)
      }
      return this._positionSeconds || 0
    },

    play() {
      if (!this._alive || this._pageVisible === false || !this.properties.active || !this.properties.videoUrl) return
      if (this._desiredPlaying && (this._decoder || this._preview || this._loadInFlight)) return
      this._desiredPlaying = true
      if (this._decoder || this._preview) return
      this.loadAndPlay(this.currentPosition())
    },

    pause() {
      if (!this._alive) return
      const wasPlaying = this._desiredPlaying || !!this._decoder
      const wasLoading = this._loadInFlight
      this._positionSeconds = this.currentPosition()
      this._desiredPlaying = false
      this._generation += 1
      this._loadInFlight = false
      cache.cancelUnused()
      this.stopDecoder(true)
      if (wasLoading) this.setData({ status: '' })
      if (wasPlaying) this.triggerEvent('pause', { currentTime: this._positionSeconds, duration: this._durationSeconds })
    },

    seek(seconds) {
      if (!this._alive || this._pageVisible === false || !this.properties.active) return
      this._requestedPosition = Math.max(0, Math.min(this._durationSeconds || Number.MAX_SAFE_INTEGER, numberOr(seconds, 0)))
      const endMargin = Math.max(0.05, numberOr(config.VIDEO_SEEK_END_MARGIN_SECONDS, 0.2))
      this._positionSeconds = this._durationSeconds
        ? Math.min(this._requestedPosition, Math.max(0, this._durationSeconds - endMargin)) : this._requestedPosition
      this._desiredPlaying = true
      this.triggerEvent('waiting', { currentTime: this._positionSeconds, duration: this._durationSeconds })
      this.loadAndPlay(this._positionSeconds)
    },

    async loadAndPlay(positionSeconds) {
      const generation = ++this._generation
      this._loadInFlight = true
      const videoId = this.properties.videoId
      const videoUrl = this.properties.videoUrl
      const wanted = () => this._alive && this._pageVisible !== false && generation === this._generation && this._desiredPlaying && this.properties.active
      // 先停旧时钟与旧画面，避免网络等待期间仍推进旧位置。
      const stopped = this.stopDecoder(true)
      if (!this._previewMode && typeof wx.createVideoDecoder !== 'function') {
        this._loadInFlight = false
        this.setData({ status: '当前基础库不支持逐帧视频解码' })
        this.triggerEvent('error', { message: '当前基础库不支持逐帧视频解码' })
        return
      }
      this.setData({ status: '正在加载缓存片段…' })
      let borrowedEntry = null
      try {
        await this._readyPromise
        if (!wanted()) return
        const request = cache.getAt(videoId, videoUrl, positionSeconds, true, wanted)
        cache.cancelUnused()
        const entry = await request
        borrowedEntry = entry
        if (!this._alive || generation !== this._generation || !this._desiredPlaying || !this.properties.active) return
        await stopped
        if (generation !== this._generation || !this._desiredPlaying) return
        this._currentEntry = entry
        borrowedEntry = null
        this._durationSeconds = entry.mediaDurationSeconds || this._durationSeconds
        const endMargin = Math.max(0.05, numberOr(config.VIDEO_SEEK_END_MARGIN_SECONDS, 0.2))
        this._positionSeconds = Math.max(0, Math.min(numberOr(positionSeconds, 0),
          this._durationSeconds ? Math.max(0, this._durationSeconds - endMargin) : Number.MAX_SAFE_INTEGER))
        await this.startDecoder(entry, this._positionSeconds, generation)
      } catch (error) {
        await this.handlePlaybackError(error, generation)
      } finally {
        if (borrowedEntry) cache.unpin(borrowedEntry)
        if (generation === this._generation) this._loadInFlight = false
      }
    },

    async startDecoder(entry, positionSeconds, generation) {
      if (this._previewMode) {
        const timeout = Math.max(1000, numberOr(config.VIDEO_DECODER_OPERATION_TIMEOUT_MS, 15000))
        const preview = new LocalVideoPreview(this, entry, positionSeconds, generation, timeout)
        this._preview = preview
        const metadata = await preview.start()
        if (!this._alive || generation !== this._generation || !this._desiredPlaying || this._preview !== preview) return
        this.finishPlayback(preview.currentPosition(), metadata, true)
        return
      }
      const decoder = wx.createVideoDecoder()
      this._decoder = decoder
      decoder.on('ended', () => this.onSegmentEnded(generation))
      decoder.on('bufferchange', (event) => {
        if (event && event.buffered != null) this._buffered = event.buffered
      })
      const timeout = Math.max(1000, numberOr(config.VIDEO_DECODER_OPERATION_TIMEOUT_MS, 15000))
      const start = decoderOperation(decoder, 'start', () => decoder.start({ source: entry.path, mode: 0 }), timeout)
      this._decoderOperation = start
      const metadata = await start.promise
      if (this._decoderOperation === start) this._decoderOperation = null
      if (!this._alive || generation !== this._generation || !this._desiredPlaying) {
        try { await decoder.stop() } catch (error) {}
        try { await decoder.remove() } catch (error) {}
        if (this._decoder === decoder) this._decoder = null
        return
      }
      if (metadata && metadata.width && metadata.height) this._renderer.resize(metadata.width, metadata.height)
      this._decoderBaseSeconds = entry.actualStartMilliseconds / 1000
      const relativeMilliseconds = Math.max(0, Math.round((positionSeconds - this._decoderBaseSeconds) * 1000))
      if (relativeMilliseconds > 0) {
        const seek = decoderOperation(decoder, 'seek', () => decoder.seek(relativeMilliseconds), timeout)
        this._decoderOperation = seek
        await seek.promise
        if (this._decoderOperation === seek) this._decoderOperation = null
      }
      if (!this._alive || generation !== this._generation || !this._desiredPlaying || this._decoder !== decoder) return
      if (typeof wx.createMediaAudioPlayer === 'function') {
        let audio
        try {
          audio = wx.createMediaAudioPlayer()
          this._audio = audio
          await audio.start()
          await audio.addAudioSource(decoder)
        } catch (error) {
          if (this._audio === audio) this._audio = null
          if (audio) {
            try { await audio.stop() } catch (error) {}
            try { await audio.destroy() } catch (error) {}
          }
        }
      }
      if (!this._alive || generation !== this._generation || !this._desiredPlaying || this._decoder !== decoder) return
      this.finishPlayback(positionSeconds, metadata, false)
    },

    finishPlayback(positionSeconds, metadata, localPreview) {
      this._clockStartPosition = positionSeconds
      this._clockStartAt = Date.now()
      this._frameReady = false
      this.setData({ status: '' })
      this.triggerEvent('loadedmetadata', { duration: this._durationSeconds, width: metadata && metadata.width, height: metadata && metadata.height })
      this.triggerEvent('canplay', { currentTime: positionSeconds, duration: this._durationSeconds })
      this.triggerEvent('seekcomplete', { generation: this._generation, requestedPosition: this._requestedPosition,
        position: positionSeconds, currentTime: positionSeconds, duration: this._durationSeconds })
      this.triggerEvent('play', { currentTime: positionSeconds, duration: this._durationSeconds })
      if (localPreview) {
        this._frameReady = true
        this.triggerEvent('frame', { backend: 'local-preview' })
      } else this.startFrameLoop()
      this.startTimeUpdates()
      this.schedulePrefetch()
    },

    onPreviewEvent(event) {
      if (this._preview) this._preview.handle(event)
    },

    async handlePlaybackError(error, generation) {
      if (!this._alive || this._pageVisible === false || !this.properties.active || generation !== this._generation) return
      this._desiredPlaying = false
      cache.cancelUnused()
      await this.stopDecoder()
      if (!this._alive || generation !== this._generation) return
      const message = error && error.message ? error.message : '视频加载失败'
      this.setData({ status: message })
      this.triggerEvent('error', { message, errMsg: message })
    },

    startFrameLoop() {
      if (!this._canvas || !this._desiredPlaying || this._frameRequestId != null) return
      this._frameRequestId = this._canvas.requestAnimationFrame(() => {
        this._frameRequestId = null
        const decoder = this._decoder
        const now = Date.now()
        if (decoder && this._renderer && this._desiredPlaying && now - this._lastRenderAt >= this._renderIntervalMs) {
          try {
            const frame = decoder.getFrameData()
            if (frame && frame.data) {
              this._lastRenderAt = now
              if (this._renderer.draw(frame)) {
                if (!this._frameReady) {
                  this._frameReady = true
                  this.triggerEvent('frame', {})
                }
              } else if (!this._renderErrorReported) {
                this._renderErrorReported = true
                this.setData({ status: '当前视频帧格式不受支持' })
                this.triggerEvent('error', { message: '当前视频帧格式不受支持' })
              }
            }
          } catch (error) {
            this.setData({ status: '视频帧读取失败' })
          }
        }
        this.startFrameLoop()
      })
    },

    stopFrameLoop() {
      if (this._frameRequestId != null && this._canvas) this._canvas.cancelAnimationFrame(this._frameRequestId)
      this._frameRequestId = null
    },

    startTimeUpdates() {
      this.stopTimeUpdates()
      this._timeTimer = setInterval(() => {
        if (!this._desiredPlaying || !this._currentEntry) return
        this._positionSeconds = this.currentPosition()
        this.triggerEvent('timeupdate', { currentTime: this._positionSeconds, duration: this._durationSeconds })
        this.schedulePrefetch()
      }, Math.max(200, numberOr(config.VIDEO_PROGRESS_UPDATE_INTERVAL_MS, 500)))
    },

    stopTimeUpdates() {
      if (this._timeTimer) clearInterval(this._timeTimer)
      this._timeTimer = null
    },

    schedulePrefetch() {
      const entry = this._currentEntry
      if (!entry || !this._desiredPlaying || !this._durationSeconds) return
      const end = entry.startSeconds + entry.durationSeconds
      const lead = Math.max(0, numberOr(config.VIDEO_CACHE_PREFETCH_LEAD_SECONDS, 2))
      const triggerAt = entry.startSeconds === 0
        ? Math.max(0, numberOr(config.VIDEO_CACHE_INITIAL_PREFETCH_AT_SECONDS, 1))
        : Math.max(0, end - lead)
      if (this.currentPosition() < triggerAt || end >= this._durationSeconds) return
      const duration = Math.max(1, numberOr(config.VIDEO_CACHE_NEXT_SECONDS, 5))
      const key = this._generation + ':' + end + ':' + duration
      if (this._prefetchKey === key) return
      this._prefetchKey = key
      const generation = this._generation
      cache.getSegment(this.properties.videoId, this.properties.videoUrl, end, duration,
        () => this._alive && this._pageVisible !== false && generation === this._generation && this._desiredPlaying && this.properties.active)
        .catch((error) => { if (!error || error.code !== 'VIDEO_REQUEST_CANCELED') console.warn('video segment prefetch failed', error && error.message) })
    },

    async onSegmentEnded(generation) {
      if (this._pageVisible === false || generation !== this._generation || !this._desiredPlaying || !this._currentEntry) return
      const nextPosition = this._currentEntry.startSeconds + this._currentEntry.durationSeconds
      if (this._durationSeconds && nextPosition >= this._durationSeconds) {
        this._positionSeconds = this._durationSeconds
        await this.stopDecoder(true)
        this.triggerEvent('ended', { currentTime: this._positionSeconds, duration: this._durationSeconds })
        if (this.properties.loop && this.properties.active) {
          this._positionSeconds = 0
          this._desiredPlaying = true
          this.loadAndPlay(0)
        } else {
          this._desiredPlaying = false
          this.triggerEvent('pause', { currentTime: this._positionSeconds, duration: this._durationSeconds })
        }
        return
      }
      this._positionSeconds = nextPosition
      this.loadAndPlay(nextPosition)
    },

    async stopDecoder(keepPreviewFrame = false) {
      this.stopFrameLoop()
      this.stopTimeUpdates()
      const decoder = this._decoder
      const audio = this._audio
      const operation = this._decoderOperation
      const entry = this._currentEntry
      const preview = this._preview
      this._preview = null
      this._decoder = null
      this._clockStartAt = 0
      this._audio = null
      this._decoderOperation = null
      this._currentEntry = null
      if (operation) operation.cancel()
      const keepFrame = keepPreviewFrame && this._alive && this._pageVisible && this.properties.active
      if (preview) preview.stop(keepFrame)
      else if (!keepFrame && this._alive && this.data.previewSegments && this.data.previewSegments.length) {
        this.setData({ previewSegments: [] })
      }
      if (audio && decoder) {
        try { await audio.removeAudioSource(decoder) } catch (error) {}
      }
      if (audio) {
        try { await audio.stop() } catch (error) {}
        try { await audio.destroy() } catch (error) {}
      }
      if (decoder) {
        try { await decoder.stop() } catch (error) {}
        try { await decoder.remove() } catch (error) {}
      }
      cache.unpin(entry)
    }
  }
})