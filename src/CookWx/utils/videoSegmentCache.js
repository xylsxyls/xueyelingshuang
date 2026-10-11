const config = require('./config')
const VideoCacheFiles = require('./videoCacheFiles')
const { VideoDownloadQueue, canceled } = require('./videoDownloadQueue')
const downloadToPath = require('./videoSegmentDownload')

const STORAGE_KEY = 'cook_video_segment_cache_v1'
const CACHE_FOLDER = '/cook-video-cache'
const DEFAULT_MAX_CACHE_BYTES = 100 * 1024 * 1024
const DEFAULT_MAX_SINGLE_SEGMENT_BYTES = 48 * 1024 * 1024

function safeNumber(value, fallback) {
  const number = Number(value)
  return isFinite(number) ? number : fallback
}

function hashText(text) {
  let hash1 = 2166136261
  let hash2 = 5381
  const value = String(text || '')
  for (let i = 0; i < value.length; i += 1) {
    const code = value.charCodeAt(i)
    hash1 ^= code
    hash1 = Math.imul(hash1, 16777619)
    hash2 = Math.imul(hash2, 33) ^ code
  }
  return (hash1 >>> 0).toString(16) + (hash2 >>> 0).toString(16)
}

function readHeader(headers, name) {
  const source = headers || {}
  const expected = String(name || '').toLowerCase()
  const keys = Object.keys(source)
  for (let i = 0; i < keys.length; i += 1) {
    if (keys[i].toLowerCase() === expected) return source[keys[i]]
  }
  return ''
}

function getFileSystem() {
  try { return wx.getFileSystemManager() } catch (error) { return null }
}

function isQuotaError(error) {
  return /maximum size|storage limit|ENOSPC|no space|quota/i.test(error && (error.errMsg || error.message) || '')
}

class VideoSegmentCache {
  constructor() {
    this.index = null
    this.downloads = new VideoDownloadQueue(safeNumber(config.VIDEO_CACHE_DOWNLOAD_CONCURRENCY, 1),
      safeNumber(config.VIDEO_CACHE_DOWNLOAD_START_DELAY_MS, 150))
    this.pending = this.downloads.jobs
    this.pinnedPaths = Object.create(null)
    this.basePath = ''
    this.files = null
    this.reservations = Object.create(null)
    this.prepareQueue = Promise.resolve()
  }

  loadIndex() {
    if (this.index) return this.index
    let stored = {}
    try { stored = wx.getStorageSync(STORAGE_KEY) || {} } catch (error) {}
    this.index = stored && typeof stored === 'object' ? stored : {}
    return this.index
  }

  videoKey(videoId, sourceUrl) {
    return hashText(String(videoId || '') + '|' + String(sourceUrl || ''))
  }

  segmentKey(videoKey, startSeconds, durationSeconds) {
    return videoKey + '_' + Math.floor(startSeconds) + '_' + Math.floor(durationSeconds)
  }

  ensureBasePath() {
    const root = wx.env && wx.env.USER_DATA_PATH
    if (!root) throw new Error('本地缓存目录不可用')
    const path = root + CACHE_FOLDER
    const fs = getFileSystem()
    if (!fs) throw new Error('本地文件系统不可用')
    try { fs.accessSync(path) } catch (accessError) {
      this.files = null
      try { fs.mkdirSync(path) } catch (error) {
        try { fs.accessSync(path) } catch (verifyError) { throw new Error('无法创建视频缓存目录') }
      }
    }
    this.basePath = path
    return path
  }

  fileExists(path) {
    if (!path) return false
    if (this.files && this.files.deletingPaths[path]) return false
    const fs = getFileSystem()
    if (!fs) return false
    try { fs.accessSync(path); return true } catch (error) { return false }
  }

  saveIndex() {
    try { wx.setStorageSync(STORAGE_KEY, this.loadIndex()) } catch (error) {}
  }

  findAt(videoId, sourceUrl, positionSeconds) {
    const videoKey = this.videoKey(videoId, sourceUrl)
    const index = this.loadIndex()
    const position = Math.max(0, safeNumber(positionSeconds, 0))
    let selected = null
    Object.keys(index).forEach((key) => {
      const entry = index[key]
      if (!entry || entry.videoKey !== videoKey || entry.videoId !== String(videoId || '') ||
          entry.sourceUrl !== String(sourceUrl || '') || !this.fileExists(entry.path)) return
      const start = safeNumber(entry.startSeconds, -1)
      const end = start + safeNumber(entry.durationSeconds, 0)
      if (start >= 0 && position >= start && position < end && (!selected || start > selected.startSeconds)) selected = entry
    })
    if (selected) {
      selected.lastAccess = Date.now()
      this.saveIndex()
    }
    return selected
  }

  segmentStartFor(positionSeconds) {
    const firstDuration = Math.max(1, Math.floor(safeNumber(config.VIDEO_CACHE_INITIAL_SECONDS, 3)))
    const nextDuration = Math.max(1, Math.floor(safeNumber(config.VIDEO_CACHE_NEXT_SECONDS, 5)))
    const position = Math.max(0, safeNumber(positionSeconds, 0))
    if (position < firstDuration) return 0
    return firstDuration + Math.floor((position - firstDuration) / nextDuration) * nextDuration
  }

  segmentDurationFor(startSeconds) {
    if (startSeconds === 0) return Math.max(1, Math.floor(safeNumber(config.VIDEO_CACHE_INITIAL_SECONDS, 3)))
    return Math.max(1, Math.floor(safeNumber(config.VIDEO_CACHE_NEXT_SECONDS, 5)))
  }

  getAt(videoId, sourceUrl, positionSeconds, pinOnRead = false, wanted = () => true) {
    if (!wanted()) return Promise.reject(canceled())
    const cached = this.findAt(videoId, sourceUrl, positionSeconds)
    if (cached) {
      if (pinOnRead) this.pin(cached)
      if (pinOnRead && (!this.files || this.files.needsRefresh)) {
        this.evictOldEntries('').catch(error => { if (console.warn) console.warn('[CookVideoCache] CLEANUP_FAILED', error && (error.errMsg || error.message)) })
      }
      return Promise.resolve(cached)
    }
    const start = this.segmentStartFor(positionSeconds)
    return this.getSegment(videoId, sourceUrl, start, this.segmentDurationFor(start), wanted, 100).then(entry => {
      if (!wanted()) throw canceled()
      if (pinOnRead) this.pin(entry)
      return entry
    })
  }

  getSegment(videoId, sourceUrl, startSeconds, durationSeconds, wanted = () => true, priority = 0) {
    if (!wanted()) return Promise.reject(canceled())
    const videoKey = this.videoKey(videoId, sourceUrl)
    const start = Math.max(0, Math.floor(safeNumber(startSeconds, 0)))
    const duration = Math.max(1, Math.floor(safeNumber(durationSeconds, 1)))
    const key = this.segmentKey(videoKey, start, duration)
    const existing = this.loadIndex()[key]
    if (existing && existing.videoKey === videoKey && existing.videoId === String(videoId || '') &&
        existing.sourceUrl === String(sourceUrl || '') && this.fileExists(existing.path)) {
      existing.lastAccess = Date.now()
      this.saveIndex()
      return Promise.resolve(existing)
    }
    return this.downloads.request(key, job => this.downloadSegment(key, videoKey, videoId, sourceUrl, start, duration, job), wanted, priority)
  }

  cancelUnused() { this.downloads.prune() }

  segmentUrl(videoId, sourceUrl, startSeconds, durationSeconds) {
    const marker = '/api/videos/file?'
    const markerIndex = String(sourceUrl || '').indexOf(marker)
    if (markerIndex < 0) throw new Error('视频地址不支持分段缓存')
    const origin = String(sourceUrl).slice(0, markerIndex)
    return origin + '/api/videos/segment?videoId=' + encodeURIComponent(videoId) +
      '&startSeconds=' + startSeconds + '&durationSeconds=' + durationSeconds
  }

  async downloadSegment(key, videoKey, videoId, sourceUrl, startSeconds, durationSeconds, job) {
    const url = this.segmentUrl(videoId, sourceUrl, startSeconds, durationSeconds)
    const destination = this.ensureBasePath() + '/' + key + '.mp4'
    const staging = destination + '.part'
    const fs = getFileSystem()
    const maxBytes = Math.max(1024 * 1024, safeNumber(config.VIDEO_CACHE_MAX_SEGMENT_BYTES, DEFAULT_MAX_SINGLE_SEGMENT_BYTES))
    const reserveBytes = Math.min(maxBytes, Math.max(0, safeNumber(config.VIDEO_CACHE_DOWNLOAD_RESERVE_BYTES, 4 * 1024 * 1024)))
    const stat = path => new Promise((resolve, reject) => fs.stat({ path, success: result => resolve(Number(result.stats && result.stats.size)), fail: reject }))
    let saved = false
    let stageStarted = false
    let destinationStarted = false
    let fileStore
    try {
      const prepare = this.prepareQueue.then(async () => {
        if (!job.isWanted()) throw canceled()
        const result = await this.evictOldEntries(destination)
        fileStore = this.files
        if (fileStore.files[staging] && !await fileStore.remove(staging, this.loadIndex())) throw new Error('旧视频暂存文件被占用，暂时无法下载')
        this.reservations[staging] = reserveBytes
        const space = await this.evictOldEntries(destination)
        if (!result.fits || !space.fits) throw new Error('本地缓存空间不足，正在使用的视频片段暂时无法回收')
      })
      this.prepareQueue = prepare.then(() => {}, () => {})
      await prepare
      for (let attempt = 0; attempt < 2; attempt += 1) {
        if (!job.isWanted()) throw canceled()
        stageStarted = true
        // 暂存归本工程管理；下载失败也要清理，不复制只读临时文件。
        fileStore.record(staging, reserveBytes)
        let headers
        try {
          headers = await downloadToPath(wx, url, staging, job, maxBytes,
            Math.max(1000, safeNumber(config.VIDEO_CACHE_DOWNLOAD_TIMEOUT_MS, 30000)))
        } catch (error) {
          await this.removePartial(staging, fileStore, stat)
          if (!job.isWanted()) throw canceled()
          if (attempt || !isQuotaError(error)) throw error
          await fileStore.refresh(this.loadIndex())
          const current = await this.evictOldEntries(destination)
          const retry = await this.evictOldEntries(destination, Math.max(0, current.totalBytes - reserveBytes))
          if (!retry.removed) throw error
          continue
        }
        if (!job.isWanted()) throw canceled()
        const size = await stat(staging)
        if (!isFinite(size) || size <= 0 || size > maxBytes) throw new Error(size > maxBytes ? '视频片段过大，已停止缓存' : '视频片段文件为空')
        this.reservations[staging] = size
        fileStore.record(staging, size)
        const space = await this.evictOldEntries(destination)
        if (!space.fits) throw new Error('本地缓存空间不足，正在使用的视频片段暂时无法回收')
        if (!job.isWanted()) throw canceled()
        if (this.pinnedPaths[destination]) throw new Error('正在使用的视频缓存不能覆盖')
        if (fileStore.files[destination] && !await fileStore.remove(destination, this.loadIndex())) throw new Error('旧视频缓存被占用，暂时无法替换')
        const startHeader = readHeader(headers, 'X-Video-Segment-Start-Ms')
        const actualStart = Number(startHeader)
        const mediaDuration = Number(readHeader(headers, 'X-Video-Duration-Ms'))
        if (startHeader === '' || !isFinite(actualStart) || actualStart < 0 || actualStart > startSeconds * 1000 || !isFinite(mediaDuration) || mediaDuration <= 0) throw new Error('视频片段时间信息无效')
        destinationStarted = true
        fileStore.record(destination, size)
        await new Promise((resolve, reject) => fs.rename({ oldPath: staging, newPath: destination, success: resolve, fail: reject }))
        // 移动不增加容量；先移除暂存清单，再登记目标，避免双计数。
        fileStore.forget(staging)
        delete this.reservations[staging]
        fileStore.record(destination, size)
        const actualBytes = await stat(destination)
        if (actualBytes !== size) throw new Error('视频缓存文件不完整')
        if (!job.isWanted()) throw canceled()
        const entry = {
          key, videoKey, videoId: String(videoId || ''), sourceUrl: String(sourceUrl || ''), path: destination,
          startSeconds, durationSeconds, actualStartMilliseconds: actualStart,
          mediaDurationSeconds: mediaDuration / 1000, bytes: size, lastAccess: Date.now()
        }
        this.loadIndex()[key] = entry
        this.saveIndex()
        saved = true
        return entry
      }
    } catch (error) {
      if (error && error.code === 'VIDEO_REQUEST_CANCELED') throw error
      const detail = error && (error.errMsg || error.message) || String(error)
      if (console.warn) console.warn('[CookVideoCache] SAVE_FAILED', detail)
      throw new Error(isQuotaError(error) ? '本地存储空间不足，无法保存视频缓存：' + detail : detail)
    } finally {
      if (stageStarted && fileStore) await this.removePartial(staging, fileStore, stat)
      delete this.reservations[staging]
      if (!saved && destinationStarted && fileStore && !this.pinnedPaths[destination]) await this.removePartial(destination, fileStore, stat)
    }
  }

  async removePartial(path, store, stat) {
    try { store.record(path, await stat(path)) } catch (error) {
      if (/ENOENT|no such file|not found/i.test(error && (error.errMsg || error.message) || '')) { store.forget(path); return }
      store.needsRefresh = true
    }
    await store.remove(path, this.loadIndex())
  }

  pin(entry) {
    if (entry && entry.path) this.pinnedPaths[entry.path] = (this.pinnedPaths[entry.path] || 0) + 1
  }

  unpin(entry) {
    if (!entry || !entry.path || !this.pinnedPaths[entry.path]) return
    this.pinnedPaths[entry.path] -= 1
    if (this.pinnedPaths[entry.path] <= 0) delete this.pinnedPaths[entry.path]
    this.evictOldEntries('').catch(error => { if (console.warn) console.warn('[CookVideoCache] CLEANUP_FAILED', error && (error.errMsg || error.message)) })
  }

  async evictOldEntries(protectedPath, limit) {
    const directory = this.ensureBasePath()
    if (!this.files) this.files = new VideoCacheFiles(getFileSystem(), directory)
    const maxBytes = Math.max(10 * 1024 * 1024, safeNumber(config.VIDEO_CACHE_MAX_BYTES, DEFAULT_MAX_CACHE_BYTES))
    const pendingPaths = () => {
      const paths = Object.create(null)
      Object.keys(this.pending).forEach(key => { paths[directory + '/' + key + '.mp4'] = true; paths[directory + '/' + key + '.mp4.part'] = true })
      return paths
    }
    const result = await this.files.trim(this.loadIndex(), this.pinnedPaths, pendingPaths, this.reservations,
      limit === undefined ? maxBytes : limit, protectedPath)
    this.saveIndex()
    return result
  }
}

module.exports = new VideoSegmentCache()