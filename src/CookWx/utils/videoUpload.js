const api = require('./api')
const config = require('./config')

function readFileChunk(fs, filePath, position, length) {
  return new Promise((resolve, reject) => {
    fs.readFile({
      filePath,
      position,
      length,
      success(result) {
        if (!result || !result.data) {
          reject(new Error('无法读取本地视频分块'))
          return
        }
        resolve(result.data)
      },
      fail(error) {
        reject(new Error(error && error.errMsg ? error.errMsg : '无法读取本地视频'))
      }
    })
  })
}

async function uploadVideoFile(filePath, fileName, fileSizeBytes, onProgress, mediaType) {
  const type = mediaType === 'image' ? 'image' : 'video'
  const size = Math.floor(Number(fileSizeBytes) || 0)
  const maxBytes = type === 'image' ? 4 * 1024 * 1024 : Math.max(1, Number(config.VIDEO_UPLOAD_MAX_BYTES) || 100 * 1024 * 1024)
  const chunkBytes = Math.max(64 * 1024, Number(config.VIDEO_UPLOAD_CHUNK_BYTES) || 512 * 1024)
  if (!filePath || size <= 0 || size > maxBytes) throw new Error('视频文件为空或超过上传大小限制')
  if (!wx.getFileSystemManager) throw new Error('当前基础库不支持分块读取视频文件')
  const fs = wx.getFileSystemManager()
  let uploadId = ''
  try {
    const started = await api.uploadVideoStart({
      mediaType: type,
      fileName: String(fileName || 'upload.mp4'),
      fileSizeBytes: size
    })
    if (!started || !started.ok || !started.uploadId) throw new Error(started && started.message ? started.message : '视频上传初始化失败')
    uploadId = started.uploadId

    for (let offset = 0; offset < size; offset += chunkBytes) {
      const length = Math.min(chunkBytes, size - offset)
      const data = await readFileChunk(fs, filePath, offset, length)
      const actualLength = data.byteLength !== undefined ? data.byteLength : (data.length || 0)
      if (actualLength !== length) throw new Error('本地视频分块长度不完整')
      const result = await api.uploadVideoChunk(uploadId, offset, data)
      if (!result || !result.ok || Number(result.receivedBytes) !== offset + length) {
        throw new Error(result && result.message ? result.message : '视频分块上传失败')
      }
      if (typeof onProgress === 'function') onProgress(Math.min(100, Math.floor((offset + length) * 100 / size)))
    }

    const completed = await api.uploadVideoComplete(uploadId)
    if (!completed || !completed.ok || !completed.video || !completed.video.id) {
      throw new Error(completed && completed.message ? completed.message : '视频上传确认失败')
    }
    uploadId = ''
    return completed.video
  } catch (error) {
    if (uploadId) api.cancelVideoUpload(uploadId).catch(() => {})
    throw error
  }
}

module.exports = { uploadVideoFile }