// 审核媒体必须带登录凭据。只管理本模块自己的文件，不使用 wx.saveFile 的共享配额。
const config = require('./config')
const entries = new Map(), jobs = new Map()
let sequence = 0, epoch = 0, initialized = false, tail = Promise.resolve()
function remove(file) { try { wx.getFileSystemManager().unlinkSync(file) } catch (_) {} }
function clear() { ++epoch; entries.forEach(item => remove(item.path)); entries.clear(); jobs.clear() }
function get(mediaId, original = false) {
  const api = require('./api'), user = getApp().globalData.userId
  if (!user || !/^[a-zA-Z0-9_-]{1,128}$/.test(mediaId || '')) return Promise.reject(new Error('媒体标识或登录状态无效'))
  const key = user + ':' + mediaId + ':' + original, generation = epoch
  const cached = entries.get(key)
  if (cached) { try { wx.getFileSystemManager().accessSync(cached.path); cached.used = Date.now(); return Promise.resolve(cached.path) } catch (_) { entries.delete(key) } }
  if (jobs.has(key)) return jobs.get(key)
  const promise = tail.catch(() => {}).then(() => new Promise((resolve, reject) => {
    if (generation !== epoch || getApp().globalData.userId !== user) return reject(new Error('登录状态已变化'))
    const fs = wx.getFileSystemManager(), folder = wx.env.USER_DATA_PATH + '/cook-review-media'
    try { fs.mkdirSync(folder, true) } catch (_) {}
    if (!initialized) {
      initialized = true
      try { fs.readdirSync(folder).filter(name => /^media-\d+-\d+\.(img|jpg)$/.test(name)).forEach(name => remove(folder + '/' + name)) } catch (_) {}
    }
    const maximum = config.REVIEW_MEDIA_CACHE_MAX_BYTES || 16 * 1024 * 1024
    const reserve = original ? 4 * 1024 * 1024 : 2 * 1024 * 1024
    let bytes = Array.from(entries.values()).reduce((sum, item) => sum + item.bytes, 0)
    while (entries.size && (bytes + reserve > maximum || entries.size >= 24)) {
      const [oldKey, old] = Array.from(entries.entries()).sort((a, b) => a[1].used - b[1].used)[0]
      bytes -= old.bytes; remove(old.path); entries.delete(oldKey)
    }
    const file = folder + '/media-' + Date.now() + '-' + (++sequence) + (original ? '.img' : '.jpg')
    const url = config.BASE_URL + (original ? '/api/videos/file' : '/api/videos/poster') + '?videoId=' + encodeURIComponent(mediaId)
    let task, oversized = false
    task = wx.downloadFile({ url, filePath: file, header: { Authorization: 'Bearer ' + api.token() }, timeout: 30000,
      success(result) {
        let size = 0
        try { size = fs.statSync(file).size } catch (_) {}
        if (result.statusCode !== 200 || !size || size > reserve || generation !== epoch || getApp().globalData.userId !== user) {
          remove(file); reject(new Error(result.statusCode === 200 ? '媒体缓存空间不足或登录已变化' : '媒体暂不可查看')); return
        }
        entries.set(key, { path: file, bytes: size, used: Date.now() }); resolve(file)
      }, fail(error) { remove(file); reject(new Error(oversized ? '媒体超出缓存大小限制' : error.errMsg || '媒体下载失败')) }
    })
    if (task && task.onProgressUpdate) task.onProgressUpdate(result => { if (result.totalBytesWritten > reserve || result.totalBytesExpectedToWrite > reserve) { oversized = true; task.abort() } })
  })).finally(() => { if (jobs.get(key) === promise) jobs.delete(key) })
  jobs.set(key, promise); tail = promise.catch(() => {}); return promise
}
module.exports = { get, clear }