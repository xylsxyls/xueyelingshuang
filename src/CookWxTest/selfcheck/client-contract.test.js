const test = require('node:test')
const assert = require('node:assert/strict')
const fs = require('node:fs')
const path = require('node:path')
const vm = require('node:vm')

// 验证真实客户端缓存模块的微信 API 契约；这是辅助测试，不代替模拟器解码。
function createCache(settings = {}) {
  const files = new Map()
  const storage = {}
  let captured
  const wx = {
    env: { USER_DATA_PATH: '/isolated-fixtures' },
    getStorageSync: key => storage[key],
    setStorageSync: (key, value) => { storage[key] = value },
    getFileSystemManager: () => ({
      accessSync: file => { if (!files.has(file) && file !== '/isolated-fixtures/cook-video-cache') throw new Error('not found') },
      mkdirSync() {}, statSync: file => ({ size: files.get(file) }),
      stat: options => { if (!files.has(options.path)) options.fail(new Error('ENOENT')); else options.success({ stats: { size: files.get(options.path), isFile: () => true } }) },
      readdir: options => options.success({ files: [...files.keys()].filter(file => file.startsWith(options.dirPath + '/')).map(file => file.slice(options.dirPath.length + 1)) }),
      rename: options => { files.set(options.newPath, files.get(options.oldPath)); files.delete(options.oldPath); options.success() },
      unlinkSync: file => { files.delete(file) }, unlink: options => { files.delete(options.filePath); if (options.success) options.success() }
    }),
    downloadFile: options => {
      captured = options
      let listener
      queueMicrotask(() => {
        if (listener) listener({ header: { 'x-video-duration-ms': '18000', 'X-Video-Segment-Start-Ms': '2000' } })
        // 按微信官方声明，success 不含 header。
        files.set(options.filePath, 4096)
        options.success({ statusCode: 200, filePath: options.filePath })
      })
      return { onHeadersReceived: callback => { listener = callback }, offHeadersReceived() {} }
    }
  }
  const sandbox = { wx, module: { exports: {} }, require: name => name === './config' ? { VIDEO_CACHE_MAX_BYTES: 10 * 1024 * 1024, VIDEO_CACHE_DOWNLOAD_START_DELAY_MS: 0, ...settings } : require('../../CookWx/utils/' + name.slice(2)), console, queueMicrotask }
  const source = fs.readFileSync(path.resolve(__dirname, '../../CookWx/utils/videoSegmentCache.js'), 'utf8')
  vm.runInNewContext(source, sandbox, { filename: 'videoSegmentCache.js' })
  return { cache: sandbox.module.exports, files, storage, request: () => captured }
}

test('下载响应头必须通过 DownloadTask 取得时长与关键帧起点', async () => {
  const { cache } = createCache()
  const entry = await cache.getSegment('video', 'http://127.0.0.1/api/videos/file?videoId=video', 3, 5)
  assert.equal(entry.mediaDurationSeconds, 18)
  assert.equal(entry.actualStartMilliseconds, 2000)
})
test('真实缓存模块 3 秒首段、后续 5 秒及边界映射', () => {
  const { cache } = createCache()
  assert.equal(cache.segmentStartFor(0), 0)
  assert.equal(cache.segmentStartFor(2.99), 0)
  assert.equal(cache.segmentStartFor(3), 3)
  assert.equal(cache.segmentStartFor(8), 8)
  assert.equal(cache.segmentStartFor(15), 13)
  assert.equal(cache.segmentDurationFor(0), 3)
  assert.equal(cache.segmentDurationFor(3), 5)
})
test('LRU 清理真实算法保留 pin 文件，淘汰后实际文件和索引一致', async () => {
  const { cache, files } = createCache()
  const bytes = 4 * 1024 * 1024
  cache.index = {}
  for (let id = 0; id < 3; id += 1) {
    const key = 'aaaaaaaa_' + id + '_3'
    const entry = { key, path: '/isolated-fixtures/cook-video-cache/' + key + '.mp4', bytes, lastAccess: id }
    cache.index[entry.key] = entry
    files.set(entry.path, bytes)
  }
  cache.pin(cache.index.aaaaaaaa_0_3)
  await cache.evictOldEntries('')
  assert.ok(files.has('/isolated-fixtures/cook-video-cache/aaaaaaaa_0_3.mp4'))
  assert.ok(!files.has('/isolated-fixtures/cook-video-cache/aaaaaaaa_1_3.mp4'))
  assert.ok(files.has('/isolated-fixtures/cook-video-cache/aaaaaaaa_2_3.mp4'))
  assert.equal(Object.keys(cache.index).length, 2)
  cache.unpin(cache.index.aaaaaaaa_0_3)
})

test('改变首段及后续配置时真实段映射随配置变化', () => {
  const { cache } = createCache({ VIDEO_CACHE_INITIAL_SECONDS: 2, VIDEO_CACHE_NEXT_SECONDS: 4 })
  assert.equal(cache.segmentStartFor(1.99), 0)
  assert.equal(cache.segmentStartFor(2), 2)
  assert.equal(cache.segmentStartFor(6), 6)
  assert.equal(cache.segmentStartFor(8), 6)
  assert.equal(cache.segmentDurationFor(0), 2)
  assert.equal(cache.segmentDurationFor(2), 4)
})