const upload = require('./videoUpload')
function choose(kind) {
  return new Promise(resolve => {
    if (kind === 'video') wx.chooseVideo({ sourceType: ['album', 'camera'], compressed: true, success: result => resolve([{ path: result.tempFilePath, size: result.size, thumbnail: result.thumbTempFilePath || '' }]), fail: () => resolve([]) })
    else wx.chooseImage({ count: 9, sourceType: ['album', 'camera'], sizeType: ['compressed'], success: result => resolve((result.tempFiles || []).map(file => ({ path: file.path, size: file.size }))), fail: () => resolve([]) })
  })
}
async function replace(page, kind) {
  if (!page.canEdit() || page.data.dragging) return
  const generation = page.generation
  const files = await choose(kind)
  if (!files.length || !page.current(generation) || !page.canEdit()) return
  page.setData({ uploading: true, uploadProgress: 0 })
  const ids = []
  try {
    for (let index = 0; index < files.length; ++index) {
      if (!page.owns(generation)) return
      const file = files[index], name = file.path.split('/').pop().split('?')[0]
      const video = await upload.uploadVideoFile(file.path, /\.(mp4|mov|m4v|webm|png|jpe?g)$/i.test(name) ? name : kind === 'video' ? 'upload.mp4' : 'upload.jpg', file.size,
        percent => { if (page.owns(generation)) page.setData({ uploadProgress: Math.floor((index * 100 + percent) / files.length) }) }, kind === 'video' ? 'video' : 'photo')
      ids.push(video.id)
    }
    if (page.owns(generation)) page.setData({ mediaIdsDraft: ids, mediaChanged: true, dirty: true, error: '', uploadProgress: 100,
      'recipe.thumbnailUrl': kind === 'video' ? files[0].thumbnail || '' : files[0].path })
  } catch (error) { if (page.owns(generation)) page.setData({ error: error.message || '媒体上传失败，已提交的作品保持原样' }) }
  finally { if (page.owns(generation)) page.setData({ uploading: false }) }
}
module.exports = { replace }