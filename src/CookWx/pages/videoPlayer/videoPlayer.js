const api = require('../../utils/api')
const actions = require('../../utils/menuActions')
const assets = require('../../utils/recipeAssets')
const pageActions = require('../../utils/videoPageActions')
const presentation = require('../../utils/videoPresentation')

function formatSeconds(value) {
  const total = Math.max(0, Math.floor(Number(value) || 0))
  const minutes = Math.floor(total / 60)
  const seconds = total % 60
  return minutes + ':' + (seconds < 10 ? '0' : '') + seconds
}

function decodeOption(value) {
  const text = String(value || '')
  try { return decodeURIComponent(text) } catch (error) { return text }
}

Page(Object.assign({}, pageActions, {
  data: {
    item: {},
    metadataError: '',
    videoId: '',
    videoUrl: '',
    posterUrl: '',
    title: '',
    active: false,
    paused: false,
    frameReady: false,
    currentSeconds: 0,
    durationSeconds: 0,
    currentLabel: '0:00',
    durationLabel: '0:00'
  },

  onLoad(options) {
    this.wasPaused = false
    this.setData({
      videoId: decodeOption(options.videoId),
      videoUrl: decodeOption(options.src),
      posterUrl: decodeOption(options.poster),
      title: decodeOption(options.title),
      paused: false
    })
  },

  onShow() {
    this.setData({ active: true, paused: this.wasPaused === true })
    this.loadMetadata()
  },

  onHide() {
    this.metadataRequestId = (this.metadataRequestId || 0) + 1
    this.wasPaused = this.data.paused
    this.setData({ active: false })
  },

  onUnload() {
    this.metadataRequestId = (this.metadataRequestId || 0) + 1
    this.setData({ active: false })
  },

  async loadMetadata() {
    if (!this.data.videoId) return
    const requestId = this.metadataRequestId = (this.metadataRequestId || 0) + 1
    const isStale = () => !this.data.active || requestId !== this.metadataRequestId
    try {
      const result = actions.requireOk(await api.getVideoDetail(this.data.videoId, { isStale }))
      if (isStale()) return
      const item = presentation.fromServer(result.item)
      // Do not replace an active decoder source merely to refresh social state.
      this.setData({ item, title: item.title || this.data.title, videoUrl: this.data.videoUrl || item.videoUrl, posterUrl: this.data.posterUrl || item.posterUrl, metadataError: '' })
    } catch (err) { if (!isStale()) this.setData({ metadataError: err.message || '视频信息加载失败' }) }
  },

  player() {
    try { return this.selectComponent('#standaloneVideoPlayer') } catch (error) { return null }
  },

  togglePlayback() {
    const player = this.player()
    if (!player) return
    const paused = !this.data.paused
    if (paused) player.pause()
    else player.play()
    this.setData({ paused })
  },

  onMetadata(event) {
    const duration = Math.max(0, Number(event.detail && event.detail.duration) || 0)
    this.setData({ durationSeconds: duration, durationLabel: formatSeconds(duration) })
  },

  onTimeUpdate(event) {
    const current = Math.max(0, Number(event.detail && event.detail.currentTime) || 0)
    const duration = Math.max(0, Number(event.detail && event.detail.duration) || this.data.durationSeconds)
    this.setData({
      currentSeconds: current,
      durationSeconds: duration,
      currentLabel: formatSeconds(current),
      durationLabel: formatSeconds(duration)
    })
  },

  onPause() { if (!this.data.paused) this.setData({ paused: true }) },
  onPlay() { if (this.data.paused) this.setData({ paused: false }) },
  onEnded() { this.setData({ paused: true, currentSeconds: this.data.durationSeconds }) },
  onFrame() { this.setData({ frameReady: true }) },

  onError(event) {
    wx.showToast({ title: event.detail && event.detail.message ? event.detail.message : '视频播放失败', icon: 'none' })
  },

  onSeek(event) {
    const seconds = Math.max(0, Number(event.detail && event.detail.value) || 0)
    const player = this.player()
    if (player) player.seek(seconds)
    this.setData({ currentSeconds: seconds, currentLabel: formatSeconds(seconds), paused: false })
  }
}))