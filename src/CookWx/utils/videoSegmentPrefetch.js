const { LocalVideoPreview } = require('./videoLocalPreview')
const { canceled } = require('./videoDownloadQueue')

// 最多保留一个待播放的本地片段和原生节点；交接后所有权转给播放器。
class PreparedLocalSegment {
  constructor(owner, cache, position, timeout) {
    this.owner = owner
    this.cache = cache
    this.position = position
    this.videoId = owner.properties.videoId
    this.videoUrl = owner.properties.videoUrl
    this.closed = false
    this.entry = null
    this.preview = null
    this.promise = cache.getAt(this.videoId, this.videoUrl, position, true, () => this.current(), 0,
      () => this.sourceWanted()).then(async entry => {
      if (!this.current()) { cache.unpin(entry); throw canceled() }
      this.entry = entry
      const token = owner._previewSequence = (owner._previewSequence || 0) + 1
      this.preview = new LocalVideoPreview(owner, entry, position, token, timeout,
        { preload: true, operationGeneration: owner._generation, wanted: () => this.current(), onPreloadError: () => this.close() })
      await this.preview.start()
      if (!this.current()) throw canceled()
      return this
    }).catch(error => { const wasCanceled = !this.current(); this.close(); throw wasCanceled ? canceled() : error })
    this.promise.catch(() => {})
  }

  current() {
    return !this.closed && this.sourceWanted()
  }

  sourceWanted() {
    const owner = this.owner
    return owner._alive && owner._pageVisible !== false && owner._desiredPlaying && owner.properties.active &&
      owner.properties.videoId === this.videoId && owner.properties.videoUrl === this.videoUrl
  }

  take() {
    if (!this.current() || !this.preview || !this.preview.current() || !this.preview.started || !this.entry) return null
    const result = { entry: this.entry, preview: this.preview }
    this.entry = this.preview = null
    this.closed = true
    return result
  }

  close() {
    if (this.closed) return
    this.closed = true
    if (this.preview) this.preview.stop(false)
    this.cache.unpin(this.entry)
    this.entry = this.preview = null
    this.cache.cancelUnused()
  }
}

module.exports = PreparedLocalSegment