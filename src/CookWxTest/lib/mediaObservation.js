// 只插入隔离副本，记录产品真实事件，不修改播放结果或就绪条件。
function instrumentPlayer(source) {
  const draw = 'if (this._renderer.draw(frame)) {'
  const preview = "this.triggerEvent('frame', { backend: 'local-preview' })"
  const seek = "this.triggerEvent('seekcomplete',"
  for (const marker of [draw, preview, seek]) {
    if (!source.includes(marker)) throw new Error('播放器观察入口改变，需更新测试观察桥')
  }
  const prefix = `function __cookObserveFrame(owner, backend) {
    wx.__cookTestFrameCount = (wx.__cookTestFrameCount || 0) + 1
    const frames = wx.__cookTestFrames = wx.__cookTestFrames || []
    frames.push({ backend, videoId: owner.properties.videoId, generation: owner._generation, position: owner.currentPosition(), at: Date.now() })
    if (frames.length > 2000) frames.shift()
  }
`
  return prefix + source
    .replace(draw, draw + " __cookObserveFrame(this, 'decoder');")
    .replace(preview, "__cookObserveFrame(this, 'local-preview'); " + preview)
    .replace(seek, `const observations = wx.__cookTestSeeks = wx.__cookTestSeeks || []
      observations.push({ backend: localPreview ? 'local-preview' : 'decoder', videoId: this.properties.videoId, generation: this._generation, requested: this._requestedPosition, actual: positionSeconds, at: Date.now() })
      if (observations.length > 2000) observations.shift()
      ` + seek)
}

module.exports = { instrumentPlayer }