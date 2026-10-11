const MEDIA_MODES = { liked: 'liked', favorite: 'collection', created: 'created' }

function mediaMode(tab) { return Object.prototype.hasOwnProperty.call(MEDIA_MODES, tab) ? MEDIA_MODES[tab] : '' }

function videoIdOf(item) {
  return String(item.videoId || item.id || '').replace(/^feed_/, '')
}

// 按媒体身份去重；同一道菜的不同视频仍是不同作品。
function toMediaAssets(items, badge, ownerUserId) {
  const seen = Object.create(null)
  const result = []
  ;(Array.isArray(items) ? items : []).forEach(item => {
    if (!item) return
    if (ownerUserId !== undefined && (!ownerUserId || item.authorUserId !== ownerUserId)) return
    const videoId = videoIdOf(item)
    if (!videoId || seen[videoId]) return
    seen[videoId] = true
    const recipeIds = Array.isArray(item.recipeIds) ? item.recipeIds.filter(Boolean) : []
    result.push({
      type: 'feed', id: 'video_' + videoId, videoId,
      videoUrl: item.videoUrl || '', posterUrl: item.posterUrl || item.imageUrl || '',
      recipeId: recipeIds[0] || '', title: item.title || '做饭动态',
      subtitle: item.description || item.caption || '', badge: badge || '', coverFailed: false,
    })
  })
  return result
}

function appendMediaAssets(existing, incoming) {
  const seen = Object.create(null)
  return existing.concat(incoming).filter(item => {
    if (!item || !item.videoId || seen[item.videoId]) return false
    seen[item.videoId] = true
    return true
  })
}

module.exports = { mediaMode, toMediaAssets, appendMediaAssets }