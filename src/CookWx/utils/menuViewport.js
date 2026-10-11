function ensureVisible(rect, height, scrollTop, contentHeight) {
  const top = Math.max(0, Number(scrollTop) || 0), viewport = Math.max(0, Number(height) || 0)
  if (!rect || !viewport || contentHeight <= viewport) return 0
  const maximum = Math.max(0, contentHeight - viewport)
  if (rect.top < top) return Math.max(0, Math.min(maximum, rect.top))
  if (rect.bottom > top + viewport) return Math.max(0, Math.min(maximum, rect.bottom - viewport))
  return Math.min(top, maximum)
}
module.exports = { ensureVisible }