Page({
  data: {
    videoPath: '',
    durationSeconds: 0,
    currentSeconds: 0,
    running: false,
    status: '等待选择视频',
    detail: ''
  },

  onReady() {
    wx.createSelectorQuery()
      .select('#decoderCanvas')
      .fields({ node: true, size: true })
      .exec((result) => {
        const entry = result && result[0]
        if (!entry || !entry.node) {
          this.setData({ status: '无法创建 2D Canvas，请在真机基础库中验证' })
          return
        }
        this._canvas = entry.node
        this._context = this._canvas.getContext('2d')
      })
  },

  chooseVideo() {
    wx.chooseMedia({
      count: 1,
      mediaType: ['video'],
      sourceType: ['album', 'camera'],
      success: (result) => {
        const file = result.tempFiles && result.tempFiles[0]
        if (!file) return
        this.stopDecoder().then(() => {
          this.setData({
            videoPath: file.tempFilePath,
            durationSeconds: Math.max(0, Math.ceil(file.duration || 0)),
            currentSeconds: 0,
            status: '已选择视频，点击“开始解码”',
            detail: `${file.width || '?'} × ${file.height || '?'}，${file.size || 0} bytes`
          })
          if (this._context) this._context.clearRect(0, 0, this._canvas.width, this._canvas.height)
        })
      },
      fail: (error) => {
        this.setData({ status: `选择失败：${error && error.errMsg ? error.errMsg : '未知错误'}` })
      }
    })
  },

  async startDecoder() {
    if (!this.data.videoPath || !this._canvas || !this._context) {
      this.setData({ status: '视频或 Canvas 尚未准备好' })
      return
    }
    if (typeof wx.createVideoDecoder !== 'function') {
      this.setData({ status: '当前微信基础库没有 wx.createVideoDecoder' })
      return
    }

    await this.stopDecoder()
    this.setData({ status: '正在启动逐帧解码…', detail: '' })

    try {
      const decoder = wx.createVideoDecoder()
      this._decoder = decoder
      decoder.on('bufferchange', (event) => {
        if (event && event.buffered != null) {
          this._buffered = event.buffered
        }
      })
      decoder.on('ended', () => {
        this._cancelFrameLoop()
        this.setData({ running: false, status: '解码结束' })
      })
      decoder.on('stop', () => {
        if (this._stopping) return
        this._cancelFrameLoop()
      })

      const mediaInfo = await decoder.start({ source: this.data.videoPath, mode: 0 })
      this._canvas.width = mediaInfo.width
      this._canvas.height = mediaInfo.height
      this._imageData = this._context.createImageData(mediaInfo.width, mediaInfo.height)
      this._frameCount = 0
      this._lastFrameStatusAt = 0
      this._fps = mediaInfo.fps || 30
      this._seekBaseSeconds = 0
      this._playStartAt = Date.now()
      this.setData({
        running: true,
        status: '视频解码已启动，正在取帧',
        detail: `${mediaInfo.width} × ${mediaInfo.height}${mediaInfo.fps ? `，${mediaInfo.fps} fps` : ''}`
      })

      await this._startAudio(decoder)
      this._scheduleFrame()
    } catch (error) {
      this.setData({ running: false, status: `启动失败：${error && error.errMsg ? error.errMsg : error}` })
      await this.stopDecoder()
    }
  },

  async _startAudio(decoder) {
    if (typeof wx.createMediaAudioPlayer !== 'function') {
      this.setData({ detail: `${this.data.detail}；当前基础库无 MediaAudioPlayer` })
      return
    }
    try {
      const audio = wx.createMediaAudioPlayer()
      this._audio = audio
      await audio.start()
      await audio.addAudioSource(decoder)
    } catch (error) {
      this.setData({ detail: `${this.data.detail}；音频输出未启动：${error && error.errMsg ? error.errMsg : error}` })
    }
  },

  _scheduleFrame() {
    if (!this._canvas || !this.data.running || this._frameRequestId != null) return
    this._frameRequestId = this._canvas.requestAnimationFrame(() => {
      this._frameRequestId = null
      this._renderNextFrame()
      this._scheduleFrame()
    })
  },

  _renderNextFrame() {
    if (!this._decoder || !this._context) return
    let frame
    try {
      frame = this._decoder.getFrameData()
    } catch (error) {
      this.setData({ status: `取帧失败：${error && error.errMsg ? error.errMsg : error}` })
      return
    }
    if (!frame || !frame.data) return

    const pixels = frame.data instanceof ArrayBuffer
      ? new Uint8ClampedArray(frame.data)
      : ArrayBuffer.isView(frame.data)
        ? new Uint8ClampedArray(frame.data.buffer, frame.data.byteOffset, frame.data.byteLength)
        : null
    if (!pixels) return

    const pixelCount = frame.width * frame.height
    const expectedRgbaBytes = pixelCount * 4
    const expectedI420Bytes = pixelCount * 3 / 2
    if (pixels.byteLength !== expectedRgbaBytes && pixels.byteLength !== expectedI420Bytes) {
      const now = Date.now()
      if (now - this._lastFrameStatusAt > 1000) {
        this._lastFrameStatusAt = now
        this.setData({ status: '已收到解码帧，但像素格式需要适配', detail: `${frame.width} × ${frame.height}；${pixels.byteLength} bytes/frame` })
      }
      return
    }

    try {
      if (pixels.byteLength === expectedRgbaBytes) {
        this._imageData.data.set(pixels)
      } else if (!this._copyI420ToRgba(pixels, frame.width, frame.height)) {
        return
      }
      this._context.putImageData(this._imageData, 0, 0)
      this._frameCount += 1
      if (this._frameCount % 30 === 0) {
        const elapsedSeconds = Math.max(0, (Date.now() - this._playStartAt) / 1000)
        const position = this._seekBaseSeconds + elapsedSeconds
        this.setData({
          currentSeconds: Math.min(this.data.durationSeconds || position, Math.floor(position)),
          status: `逐帧渲染中：已显示 ${this._frameCount} 帧`
        })
      }
    } catch (error) {
      this.setData({ status: `渲染失败：${error && error.errMsg ? error.errMsg : error}` })
    }
  },

  _copyI420ToRgba(pixels, width, height) {
    if (width % 2 !== 0 || height % 2 !== 0) return false
    const ySize = width * height
    const chromaSize = ySize >> 2
    const uOffset = ySize
    const vOffset = ySize + chromaSize
    const rgba = this._imageData.data
    let output = 0

    for (let y = 0; y < height; y += 1) {
      const chromaRow = (y >> 1) * (width >> 1)
      const row = y * width
      for (let x = 0; x < width; x += 1) {
        const luma = Math.max(0, pixels[row + x] - 16)
        const chromaIndex = chromaRow + (x >> 1)
        const u = pixels[uOffset + chromaIndex] - 128
        const v = pixels[vOffset + chromaIndex] - 128
        rgba[output++] = Math.max(0, Math.min(255, (298 * luma + 409 * v + 128) >> 8))
        rgba[output++] = Math.max(0, Math.min(255, (298 * luma - 100 * u - 208 * v + 128) >> 8))
        rgba[output++] = Math.max(0, Math.min(255, (298 * luma + 516 * u + 128) >> 8))
        rgba[output++] = 255
      }
    }
    return true
  },

  async onSeek(event) {
    if (!this._decoder || !this.data.running) return
    const seconds = Number(event.detail.value) || 0
    this.setData({ currentSeconds: seconds, status: `跳转到 ${seconds}s…` })
    try {
      await this._decoder.seek(seconds * 1000)
      this._seekBaseSeconds = seconds
      this._playStartAt = Date.now()
    } catch (error) {
      this.setData({ status: `跳转失败：${error && error.errMsg ? error.errMsg : error}` })
    }
  },

  _cancelFrameLoop() {
    if (this._frameRequestId == null || !this._canvas) return
    this._canvas.cancelAnimationFrame(this._frameRequestId)
    this._frameRequestId = null
  },

  async stopDecoder() {
    this._stopping = true
    this._cancelFrameLoop()
    const decoder = this._decoder
    const audio = this._audio
    this._decoder = null
    this._audio = null

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
    this._stopping = false
    if (this.data.running) this.setData({ running: false })
  },

  onUnload() {
    this.stopDecoder()
  }
})
