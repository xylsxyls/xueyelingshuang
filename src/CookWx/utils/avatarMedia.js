const config = require('./config')

// 原生图片操作集中处理大小、超时和临时结果；页面只负责交互与账号隔离。
function nativeCall(method, options, timeout = 20000) {
  return new Promise((resolve, reject) => {
    let settled = false
    const finish = (callback, value) => {
      if (settled) return
      settled = true
      clearTimeout(timer)
      callback(value)
    }
    const timer = setTimeout(() => finish(reject, new Error('图片处理超时，请重试')), timeout)
    try {
      method(Object.assign({}, options, {
        success: value => finish(resolve, value),
        fail: error => finish(reject, Object.assign(new Error(error.errMsg || '图片处理失败'), { cancelled: /cancel/.test(error.errMsg || '') }))
      }))
    } catch (error) { finish(reject, error) }
  })
}

async function prepare(path) {
  const fs = wx.getFileSystemManager()
  const stat = await nativeCall(options => fs.stat(options), { path })
  if (stat.stats.size > config.AVATAR_SOURCE_MAX_BYTES) throw new Error('原图超过10MB，请选择较小的图片')
  const info = await nativeCall(wx.getImageInfo, { src: path })
  if (!info.width || !info.height || info.width * info.height > config.AVATAR_SOURCE_MAX_PIXELS) throw new Error('图片分辨率过大，请选择较小的图片')
  const scale = Math.min(1, 2048 / Math.max(info.width, info.height))
  if (scale === 1) return info
  const compressed = await nativeCall(wx.compressImage, { src: path, quality: 90,
    compressedWidth: Math.max(1, Math.round(info.width * scale)), compressedHeight: Math.max(1, Math.round(info.height * scale)) })
  try {
    const result = await nativeCall(wx.getImageInfo, { src: compressed.tempFilePath })
    return Object.assign({}, result, { ownedPath: compressed.tempFilePath })
  } catch (error) {
    removePrepared({ ownedPath: compressed.tempFilePath })
    throw error
  }
}

function removePrepared(info) {
  if (info && info.ownedPath) wx.getFileSystemManager().unlink({ filePath: info.ownedPath, fail() {} })
}

function loadImage(canvas, path) {
  return new Promise((resolve, reject) => {
    const image = canvas.createImage()
    const timer = setTimeout(() => finish(new Error('图片加载超时')), 20000)
    let settled = false
    function finish(error) {
      if (settled) return
      settled = true
      clearTimeout(timer)
      image.onload = null
      image.onerror = null
      if (error) reject(error)
      else resolve(image)
    }
    image.onload = () => finish()
    image.onerror = () => finish(new Error('图片无法加载，请重新选择'))
    image.src = path
  })
}

async function exportBytes(canvas, page) {
  let file = ''
  try {
    const result = await nativeCall(options => wx.canvasToTempFilePath(options, page), {
      canvas, fileType: 'jpg', quality: config.AVATAR_JPEG_QUALITY,
      width: config.AVATAR_OUTPUT_SIDE, height: config.AVATAR_OUTPUT_SIDE,
      destWidth: config.AVATAR_OUTPUT_SIDE, destHeight: config.AVATAR_OUTPUT_SIDE
    })
    file = result.tempFilePath
    const fs = wx.getFileSystemManager()
    const stat = await nativeCall(options => fs.stat(options), { path: file })
    if (stat.stats.size <= 0 || stat.stats.size > config.AVATAR_MAX_BYTES) throw new Error('裁剪图片过大，请重新选择')
    const read = await nativeCall(options => fs.readFile(options), { filePath: file })
    return read.data
  } finally {
    // 只清理本次生成的导出文件，不删除相册/拍照原图。
    if (file) wx.getFileSystemManager().unlink({ filePath: file, fail() {} })
  }
}

module.exports = { nativeCall, prepare, removePrepared, loadImage, exportBytes }