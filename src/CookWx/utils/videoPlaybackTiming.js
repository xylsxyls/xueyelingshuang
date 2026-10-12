// 按时间戳解码时仍要持续取帧；限制绘制频率不能让解码缓冲一直积压。
function latestFrame(decoder, limit) {
  let latest = null
  for (let i = 0; i < limit; i += 1) {
    const frame = decoder.getFrameData()
    if (!frame || !frame.data) break
    latest = frame
  }
  return latest
}

function renderTime(previous, now, interval) {
  if (!previous || now < previous) return now
  if (now - previous < interval) return previous
  return previous + Math.floor((now - previous) / interval) * interval
}

function prefetchLead(configured, elapsedSeconds, adaptive, entryDuration) {
  if (!adaptive || !Number.isFinite(elapsedSeconds) || elapsedSeconds <= 0) return configured
  return Math.max(configured, Math.min(entryDuration, elapsedSeconds * 1.25 + 0.25))
}

module.exports = { latestFrame, renderTime, prefetchLead }