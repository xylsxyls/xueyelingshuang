const api = require('../../utils/api')
const config = require('../../utils/config')
const cropper = require('../../utils/avatarCrop')
const media = require('../../utils/avatarMedia')

Page({
  data: { ready: false, loading: false, saving: false, crop: { x: 0, y: 0, size: 0 } },

  onLoad() {
    this.alive = true
    this.owner = getApp().globalData.userId
    this.generation = 0
  },

  onReady() {
    wx.createSelectorQuery().in(this).select('.crop-stage').boundingClientRect()
      .select('#avatar-preview').fields({ node: true, size: true })
      .select('#avatar-output').fields({ node: true, size: true }).exec(result => {
        if (!this.current() || !result || result.length !== 3 || !result[0] || !result[1] || !result[2] || !result[1].node || !result[2].node) return
        this.viewport = result[0]
        this.preview = result[1].node
        this.output = result[2].node
        const device = typeof wx.getWindowInfo === 'function' ? wx.getWindowInfo() : wx.getSystemInfoSync()
        this.ratio = Math.min(2, Number(device.pixelRatio) || 1)
        this.preview.width = Math.round(this.viewport.width * this.ratio)
        this.preview.height = Math.round(this.viewport.height * this.ratio)
        this.output.width = this.output.height = config.AVATAR_OUTPUT_SIDE
        this.choose()
      })
  },

  current(generation) {
    return this.alive && !!this.owner && getApp().globalData.userId === this.owner &&
      (generation === undefined || generation === this.generation)
  },

  async choose() {
    if (!this.current() || this.data.loading || this.data.saving || !this.preview) return
    const generation = ++this.generation
    let prepared = null
    this.setData({ loading: true })
    try {
      // 选择器可停留较久；超时后仍不会接受过期回调。
      const result = await media.nativeCall(wx.chooseMedia, { count: 1, mediaType: ['image'], sourceType: ['album', 'camera'], sizeType: ['original'] }, 300000)
      if (!this.current(generation)) return
      const info = prepared = await media.prepare(result.tempFiles[0].tempFilePath)
      if (!this.current(generation)) return
      const image = await media.loadImage(this.preview, info.path)
      if (!this.current(generation)) return
      const size = cropper.orientedSize(info.width, info.height, info.orientation)
      const placement = cropper.fit(size.width, size.height, this.viewport.width, this.viewport.height)
      if (!placement) throw new Error('图片尺寸无效')
      const context = this.preview.getContext('2d')
      context.clearRect(0, 0, this.preview.width, this.preview.height)
      cropper.draw(context, image, info, placement, null, this.ratio)
      this.image = image
      media.removePrepared(this.info)
      this.info = info
      prepared = null
      this.placement = placement
      this.selection = cropper.initial(placement, config.AVATAR_CROP_MIN_SIDE)
      this.setData({ ready: true, crop: this.selection })
    } catch (error) {
      if (this.current(generation) && !error.cancelled) wx.showToast({ title: error.message || '图片处理失败', icon: 'none' })
    } finally {
      media.removePrepared(prepared)
      if (this.current(generation)) this.setData({ loading: false })
    }
  },

  touchStart(event) {
    if (!this.current() || !this.data.ready || this.data.loading || this.data.saving) return
    this.gesture = cropper.gesture(event.touches, event.currentTarget.dataset.mode || 'move', this.selection)
  },

  touchMove(event) {
    if (!this.current() || !this.gesture || this.data.loading || this.data.saving) return
    const mode = event.touches.length > 1 ? 'pinch' : (event.currentTarget.dataset.mode || 'move')
    if (mode !== this.gesture.mode) this.gesture = cropper.gesture(event.touches, mode, this.selection)
    if (!this.gesture) return
    this.selection = cropper.update(this.gesture, event.touches, this.placement, config.AVATAR_CROP_MIN_SIDE)
    if (!this.frameTimer) this.frameTimer = setTimeout(() => this.flush(), config.AVATAR_GESTURE_INTERVAL_MS)
  },

  flush() {
    clearTimeout(this.frameTimer)
    this.frameTimer = null
    if (this.current() && this.selection) this.setData({ crop: Object.assign({}, this.selection) })
  },

  touchEnd(event) {
    this.flush()
    this.gesture = event.touches && event.touches.length ? cropper.gesture(event.touches, 'move', this.selection) : null
  },

  async save() {
    if (!this.current() || !this.data.ready || this.data.loading || this.data.saving) return
    const generation = ++this.generation
    this.gesture = null
    this.flush()
    this.setData({ saving: true })
    try {
      const context = this.output.getContext('2d')
      context.fillStyle = '#ffffff'
      context.fillRect(0, 0, config.AVATAR_OUTPUT_SIDE, config.AVATAR_OUTPUT_SIDE)
      const ratio = config.AVATAR_OUTPUT_SIDE / this.selection.size
      // 输出Canvas创建自己的图像节点，避免跨Canvas图像节点兼容差异。
      const image = await media.loadImage(this.output, this.info.path)
      if (!this.current(generation)) return
      cropper.draw(context, image, this.info, this.placement, this.selection, ratio)
      const bytes = await media.exportBytes(this.output, this)
      if (!this.current(generation)) return
      const result = await api.uploadAvatar(bytes, { isStale: () => !this.current(generation) })
      if (!this.current(generation)) return
      if (!result || !result.ok || !result.profile) throw new Error(result && result.message || '头像保存失败')
      const state = getApp().globalData
      state.socialStateVersion = Number(state.socialStateVersion || 0) + 1
      state.profileStateVersion = Number(state.profileStateVersion || 0) + 1
      wx.showToast({ title: '头像已更新', icon: 'success' })
      wx.navigateBack({ delta: 1 })
    } catch (error) {
      if (this.current(generation) && !api.isStaleError(error)) wx.showToast({ title: error.message || '头像保存失败，请重试', icon: 'none' })
    } finally {
      if (this.current(generation)) this.setData({ saving: false })
    }
  },

  onHide() {
    this.gesture = null
    this.flush()
  },

  onUnload() {
    this.alive = false
    ++this.generation
    clearTimeout(this.frameTimer)
    this.frameTimer = null
    this.gesture = null
    media.removePrepared(this.info)
    this.image = this.info = this.preview = this.output = null
  }
})