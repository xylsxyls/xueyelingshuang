const test = require('node:test')
const assert = require('node:assert/strict')
const fs = require('node:fs')
const path = require('node:path')
const vm = require('node:vm')
const MiB = 1024 * 1024
const folder = '/fixture/cook-video-cache'
const sourceUrl = 'http://fixture/api/videos/file?videoId=v'

function fixture(options = {}) {
  const files = new Map()
  const storage = { cook_video_segment_cache_v1: {} }
  const blocked = new Set()
  const moves = []
  const downloads = []
  let directoryExists = true
  let moving = 0
  const manager = {
    accessSync(file) { if ((file === folder && directoryExists) || files.has(file)) return; throw new Error('ENOENT') },
    mkdirSync() { directoryExists = true },
    readdirSync() { return [...files.keys()].filter(file => file.startsWith(folder + '/')).map(file => file.slice(folder.length + 1)) },
    statSync(file) { if (!files.has(file)) throw new Error('ENOENT'); return { size: files.get(file), isFile: () => true } },
    unlinkSync(file) { if (blocked.has(file)) throw new Error('EBUSY'); files.delete(file) },
    stat(operation) { queueMicrotask(() => { try { operation.success({ stats: this.statSync(operation.path) }) } catch (error) { operation.fail(error) } }) },
    readdir(operation) { queueMicrotask(() => operation.success({ files: this.readdirSync(operation.dirPath) })) },
    unlink(operation) { queueMicrotask(() => { try { this.unlinkSync(operation.filePath); if (operation.success) operation.success() } catch (error) { if (operation.fail) operation.fail(error) } }) },
    rename(operation) {
      moves.push(operation)
      moving += 1
      const perform = () => {
        moving -= 1
        const bytes = files.get(operation.oldPath)
        if (options.moveError) {
          operation.fail(options.moveError)
          return
        }
        files.set(operation.newPath, bytes)
        files.delete(operation.oldPath)
        operation.success()
      }
      if (options.deferMove) operation.finish = perform
      else queueMicrotask(perform)
    }
  }
  const wx = {
    env: { USER_DATA_PATH: '/fixture' },
    getStorageSync: key => storage[key], setStorageSync: (key, value) => { storage[key] = value },
    getFileSystemManager: () => manager,
    downloadFile(operation) {
      downloads.push(operation)
      const persistent = [...files].filter(([file]) => file.startsWith(folder + '/')).reduce((sum, [, size]) => sum + size, 0)
      if (persistent + (options.incomingBytes || 3 * MiB) > (options.quota || Infinity)) {
        queueMicrotask(() => operation.fail({ errMsg: 'downloadFile:fail the maximum size of the file storage limit is exceeded' }))
        return { onHeadersReceived() {} }
      }
      const file = operation.filePath
      files.set(file, options.incomingBytes || 3 * MiB)
      let headers
      const finish = () => {
        const start = Number(new URL(operation.url).searchParams.get('startSeconds'))
        if (headers) headers({ header: { 'X-Video-Segment-Start-Ms': String(start * 1000), 'X-Video-Duration-Ms': '60000' } })
        operation.success({ statusCode: 200, filePath: file })
      }
      if (options.deferDownload) operation.finish = finish
      else queueMicrotask(finish)
      return { onHeadersReceived(callback) { headers = callback }, abort() { queueMicrotask(() => operation.fail({ errMsg: 'downloadFile:fail abort' })) } }
    }
  }
  const settings = { VIDEO_CACHE_MAX_BYTES: 10 * MiB, VIDEO_CACHE_MAX_SEGMENT_BYTES: 8 * MiB, VIDEO_CACHE_DOWNLOAD_START_DELAY_MS: 0, ...options.config }
  const sandbox = { wx, module: { exports: {} }, console: { warn() {} }, require: name => name === './config' ? settings : require('../../CookWx/utils/' + name.slice(2)) }
  vm.runInNewContext(fs.readFileSync(path.join(__dirname, '../../CookWx/utils/videoSegmentCache.js'), 'utf8'), sandbox)
  const cache = sandbox.module.exports
  function add(key, bytes, indexed = true, lastAccess = 0) {
    const file = folder + '/' + key + '.mp4'
    files.set(file, bytes)
    if (indexed) storage.cook_video_segment_cache_v1[key] = { key, path: file, bytes, lastAccess }
    return file
  }
  return { cache, files, storage, blocked, moves, downloads, add, manager, setDirectoryMissing() { directoryExists = false }, get moving() { return moving } }
}

test('下载必须指定可回收文件路径，不能复制只读临时文件造成双份占用', async () => {
  const env = fixture()
  const unlink = env.manager.unlink.bind(env.manager)
  env.manager.unlink = operation => {
    if (!operation.filePath.startsWith(folder + '/')) queueMicrotask(() => operation.fail({ errMsg: 'unlink:fail permission denied' }))
    else unlink(operation)
  }
  await env.cache.getAt('v', sourceUrl, 0)
  assert.ok(env.downloads[0].filePath && env.downloads[0].filePath.startsWith(folder + '/'))
  assert.ok([...env.files.keys()].every(file => file.startsWith(folder + '/')))
})

test('磁盘超配额且有孤儿片段时，保存前回收并保留正在播放片段，只下载一次', async () => {
  const env = fixture({ quota: 20 * MiB })
  const pinned = env.add('aaaaaaaa_0_3', 4 * MiB, true, 1)
  const oldest = env.add('bbbbbbbb_0_3', 4 * MiB, true, 2)
  const orphan = env.add('cccccccc_0_3', 6 * MiB, false)
  env.add('dddddddd_0_3', 4 * MiB, false)
  env.cache.pin({ path: pinned })
  const entry = await env.cache.getSegment('v', sourceUrl, 0, 3)
  assert.equal(env.downloads.length, 1)
  assert.equal(env.moves.length, 1)
  assert.equal(entry.bytes, 3 * MiB)
  assert.ok(env.files.has(pinned))
  assert.ok(!env.files.has(oldest))
  assert.ok(!env.files.has(orphan))
  assert.ok([...env.files].filter(([file]) => file.startsWith(folder + '/')).reduce((sum, [, size]) => sum + size, 0) <= 10 * MiB)
})

test('删除失败不能扣减真实占用或删除索引', async () => {
  const env = fixture()
  const blocked = env.add('aaaaaaaa_0_3', 8 * MiB, true, 1)
  const pinned = env.add('bbbbbbbb_0_3', 4 * MiB, true, 2)
  env.blocked.add(blocked)
  env.cache.pin({ path: pinned })
  const result = await env.cache.evictOldEntries('')
  assert.ok(env.files.has(blocked))
  assert.ok(env.cache.loadIndex().aaaaaaaa_0_3, '删除失败仍须保留文件索引')
  assert.equal(result.totalBytes, 12 * MiB)
  assert.equal(result.fits, false)
})

test('写入失败清理临时文件、释放预留并不提交索引，保留微信原始错误', async () => {
  const env = fixture({ moveError: { errMsg: 'rename:fail permission denied' } })
  await assert.rejects(env.cache.getSegment('v', sourceUrl, 0, 3), /permission denied/)
  assert.equal(env.files.size, 0)
  assert.equal(Object.keys(env.cache.pending).length, 0)
  assert.equal(Object.keys(env.cache.loadIndex()).length, 0)
})

test('清理只处理本工程片段，不删除未知文件或索引指向的外部文件', async () => {
  const env = fixture()
  const unknown = folder + '/user-notes.txt'
  const foreign = '/outside/private.mp4'
  env.files.set(unknown, MiB)
  env.files.set(foreign, MiB)
  env.storage.cook_video_segment_cache_v1.unsafe = { key: 'unsafe', path: foreign, bytes: MiB }
  env.add('aaaaaaaa_0_3', 2 * MiB, false)
  await env.cache.evictOldEntries('')
  assert.ok(env.files.has(unknown))
  assert.ok(env.files.has(foreign))
  assert.ok(!env.files.has(folder + '/aaaaaaaa_0_3.mp4'))
})

test('并发保存纳入同一个容量预算，不能让两个片段都挤占最后的空间', async () => {
  const env = fixture({ deferMove: true, incomingBytes: 6 * MiB })
  const first = env.cache.getAt('one', sourceUrl, 0, true)
  const second = env.cache.getSegment('two', sourceUrl, 0, 3)
  const result = Promise.allSettled([first, second])
  await new Promise(resolve => setImmediate(resolve))
  assert.equal(env.moving, 1, '10MiB预算只能同时接纳一个6MiB片段')
  env.moves[0].finish()
  const outcomes = await result
  assert.equal(outcomes.filter(item => item.status === 'fulfilled').length, 1)
  assert.equal(outcomes.filter(item => item.status === 'rejected').length, 1)
  assert.ok([...env.files.values()].reduce((sum, bytes) => sum + bytes, 0) <= 10 * MiB)
})

test('单片段超限在移动前拒绝，释放本工程暂存文件', async () => {
  const env = fixture({ incomingBytes: 9 * MiB })
  await assert.rejects(env.cache.getSegment('v', sourceUrl, 0, 3), /片段过大/)
  assert.equal(env.moves.length, 0)
  assert.equal(env.files.size, 0)
})

test('下载触发原生配额时回收LRU，仅重试一次且不复制临时文件', async () => {
  const env = fixture({ quota: 5 * MiB, config: { VIDEO_CACHE_DOWNLOAD_RESERVE_BYTES: MiB } })
  env.add('aaaaaaaa_0_3', 4 * MiB, true)
  const entry = await env.cache.getSegment('v', sourceUrl, 0, 3)
  assert.equal(entry.bytes, 3 * MiB)
  assert.equal(env.downloads.length, 2)
  assert.equal(env.moves.length, 1)
})

test('只剩正在播放文件仍无空间时，有界失败，保护已播放缓存', async () => {
  const env = fixture({ quota: 5 * MiB })
  const pinned = env.add('aaaaaaaa_0_3', 4 * MiB, true)
  env.cache.pin({ path: pinned })
  await assert.rejects(env.cache.getSegment('v', sourceUrl, 0, 3), /空间不足/)
  assert.ok(env.files.has(pinned))
  assert.equal(env.downloads.length, 1)
  assert.ok(env.moves.length <= 2)
})

test('文件系统删掉缓存目录后，下次请求重新创建目录', async () => {
  const env = fixture()
  env.cache.ensureBasePath()
  env.setDirectoryMissing()
  let mkdirCount = 0
  const mkdir = env.manager.mkdirSync
  env.manager.mkdirSync = () => { mkdirCount += 1; mkdir() }
  await env.cache.getSegment('v', sourceUrl, 0, 3)
  assert.equal(mkdirCount, 1)
})

test('缓存命中在交给播放器前固定，其他保存不能淘汰交接中的片段', async () => {
  const env = fixture({ incomingBytes: 6 * MiB })
  const first = await env.cache.getAt('v', sourceUrl, 0, true)
  assert.equal(env.cache.pinnedPaths[first.path], 1)
  await assert.rejects(env.cache.getSegment('two', sourceUrl, 0, 3), /空间不足/)
  assert.ok(env.files.has(first.path))
  const again = await env.cache.getAt('v', sourceUrl, 0, true)
  assert.equal(env.cache.pinnedPaths[first.path], 2)
  assert.equal(env.downloads.length, 2, '返回缓存没有发起第三次下载')
  env.cache.unpin(again)
  env.cache.unpin(first)
  await env.cache.evictOldEntries('')
  assert.equal(Object.keys(env.cache.pinnedPaths).length, 0)
})

test('同一片段并发请求合并下载，失败后允许重新请求且没有预留泄漏', async () => {
  const env = fixture({ moveError: { errMsg: 'rename:fail permission denied' } })
  const results = await Promise.allSettled([env.cache.getAt('v', sourceUrl, 0), env.cache.getAt('v', sourceUrl, 0)])
  assert.ok(results.every(result => result.status === 'rejected'))
  assert.equal(env.downloads.length, 1)
  assert.equal(env.moves.length, 1)
  assert.equal(Object.keys(env.cache.reservations).length, 0)
  await assert.rejects(env.cache.getAt('v', sourceUrl, 0), /permission denied/)
  assert.equal(env.downloads.length, 2)
})

test('扫描遇到权限错误不会提交半份磁盘清单，下次恢复后仍能回收孤儿文件', async () => {
  const env = fixture()
  const orphan = env.add('aaaaaaaa_0_3', 4 * MiB, false)
  const stat = env.manager.stat.bind(env.manager)
  let fail = true
  env.manager.stat = operation => {
    if (fail && operation.path === orphan) queueMicrotask(() => operation.fail({ errMsg: 'stat:fail permission denied' }))
    else stat(operation)
  }
  await assert.rejects(env.cache.evictOldEntries(''), error => error.errMsg === 'stat:fail permission denied')
  assert.ok(env.files.has(orphan))
  fail = false
  await env.cache.evictOldEntries('')
  assert.ok(!env.files.has(orphan))
})

test('拒绝覆盖固定文件时，只删除本次暂存文件，保留原视频和索引', async () => {
  const env = fixture()
  const key = env.cache.segmentKey(env.cache.videoKey('v', sourceUrl), 0, 3)
  const original = env.add(key, 4 * MiB)
  env.cache.pin({ path: original })
  await assert.rejects(env.cache.getSegment('v', sourceUrl, 0, 3), /不能覆盖/)
  assert.equal(env.files.get(original), 4 * MiB)
  assert.ok(env.cache.loadIndex()[key])
  assert.ok(!env.files.has(original + '.part'))
  assert.equal(env.moves.length, 0)
})

test('移动留下半个文件时将其回收，删除失败时仍计入真实占用', async () => {
  const env = fixture()
  env.manager.rename = operation => {
    env.files.set(operation.newPath, MiB)
    env.blocked.add(operation.newPath)
    queueMicrotask(() => operation.fail({ errMsg: 'rename:fail permission denied' }))
  }
  await assert.rejects(env.cache.getSegment('v', sourceUrl, 0, 3), /permission denied/)
  const result = await env.cache.evictOldEntries('')
  assert.equal(result.totalBytes, MiB)
  assert.equal(env.files.size, 1)
  assert.equal(Object.keys(env.cache.reservations).length, 0)
  assert.equal(Object.keys(env.cache.loadIndex()).length, 0)
  env.blocked.clear()
  await env.cache.evictOldEntries('')
  assert.equal(env.files.size, 0)
})

test('清理进行时新写入的片段不会被扫描的旧快照丢掉', async () => {
  const env = fixture({ deferMove: true })
  const request = env.cache.getSegment('v', sourceUrl, 0, 3)
  await new Promise(resolve => setImmediate(resolve))
  const stat = env.manager.stat.bind(env.manager)
  let resume
  env.files.set(folder + '/unknown.txt', 1)
  env.manager.stat = operation => {
    if (operation.path.endsWith('/unknown.txt')) resume = () => stat(operation)
    else stat(operation)
  }
  const scanning = env.cache.files.refresh(env.cache.loadIndex())
  await new Promise(resolve => setImmediate(resolve))
  env.moves[0].finish()
  const entry = await request
  resume()
  await scanning
  assert.equal(env.cache.files.files[entry.path].bytes, 3 * MiB)
  assert.ok(env.cache.loadIndex()[entry.key])
})

test('两百轮切换和预取后占用有界，释放临时文件、预留与固定引用', async () => {
  const env = fixture()
  for (let id = 0; id < 200; id += 1) {
    const entry = await env.cache.getAt('video-' + id, sourceUrl, 0, true)
    const prefetched = await env.cache.getSegment('video-' + id, sourceUrl, 3, 5)
    assert.ok(env.files.has(entry.path))
    assert.ok(env.files.has(prefetched.path))
    env.cache.unpin(entry)
  }
  const result = await env.cache.evictOldEntries('')
  assert.ok(result.totalBytes <= 10 * MiB)
  assert.equal(Object.keys(env.cache.pending).length, 0)
  assert.equal(Object.keys(env.cache.reservations).length, 0)
  assert.equal(Object.keys(env.cache.pinnedPaths).length, 0)
  assert.ok([...env.files.keys()].every(file => file.startsWith(folder + '/')))
  assert.equal(env.files.size, Object.keys(env.cache.loadIndex()).length)
  assert.ok(Object.keys(env.cache.files.pathRevisions).length <= env.files.size)
})

test('首次直接命中已有缓存也启动校准，不需要先下载才能回收遗留文件', async () => {
  const env = fixture()
  const videoKey = env.cache.videoKey('v', sourceUrl)
  const key = env.cache.segmentKey(videoKey, 0, 3)
  const file = env.add(key, 4 * MiB)
  Object.assign(env.storage.cook_video_segment_cache_v1[key], { videoKey, videoId: 'v', sourceUrl, startSeconds: 0, durationSeconds: 3 })
  env.add('aaaaaaaa_0_3', 6 * MiB, false)
  env.add('bbbbbbbb_0_3', 4 * MiB, false)
  const entry = await env.cache.getAt('v', sourceUrl, 0, true)
  const result = await env.cache.evictOldEntries('')
  assert.equal(env.downloads.length, 0)
  assert.equal(entry.path, file)
  assert.equal(env.cache.pinnedPaths[file], 1)
  assert.equal(env.files.size, 1)
  assert.equal(result.totalBytes, 4 * MiB)
})

test('临时下载也受配额限制时，首次校准须在网络下载之前完成', async () => {
  const env = fixture({ quota: 20 * MiB, enforceDownloadQuota: true })
  env.add('aaaaaaaa_0_3', 8 * MiB)
  env.add('bbbbbbbb_0_3', 8 * MiB, false)
  env.add('cccccccc_0_3', 3 * MiB, false)
  const entry = await env.cache.getAt('v', sourceUrl, 0)
  assert.equal(env.downloads.length, 1)
  assert.equal(env.moves.length, 1)
  assert.equal(entry.bytes, 3 * MiB)
  assert.ok((await env.cache.evictOldEntries('')).totalBytes <= 10 * MiB)
})

test('移动成功但统计与删除失败时，保留占用记录并要求重新校准', async () => {
  const env = fixture()
  const stat = env.manager.stat.bind(env.manager)
  env.manager.stat = operation => {
    if (operation.path.startsWith(folder + '/') && !operation.path.endsWith('.part') && env.files.has(operation.path)) {
      env.blocked.add(operation.path)
      queueMicrotask(() => operation.fail({ errMsg: 'stat:fail permission denied' }))
    } else stat(operation)
  }
  await assert.rejects(env.cache.getAt('v', sourceUrl, 0), /permission denied/)
  const destination = [...env.files.keys()][0]
  assert.equal(env.files.get(destination), 3 * MiB)
  assert.equal(env.cache.files.files[destination] && env.cache.files.files[destination].bytes, 3 * MiB, '未提交索引的文件也不能失去占用记录')
  assert.equal(env.cache.files.needsRefresh, true)
  assert.equal(Object.keys(env.cache.reservations).length, 0)
  assert.equal(Object.keys(env.cache.loadIndex()).length, 0)
  env.manager.stat = stat
  env.blocked.clear()
  await env.cache.evictOldEntries('')
  assert.equal(env.files.size, 0)
})

test('连续未命中定位取消旧下载，旧片段不固定且部分文件被清理', async () => {
  const env = fixture({ deferDownload: true })
  let generation = 1
  const old = env.cache.getAt('v', sourceUrl, 1, true, () => generation === 1)
  const canceled = assert.rejects(old, /取消/)
  await new Promise(resolve => setImmediate(resolve))
  assert.equal(env.downloads.length, 1)
  generation = 2
  const current = env.cache.getAt('v', sourceUrl, 27, true, () => generation === 2)
  await new Promise(resolve => setImmediate(resolve))
  await canceled
  assert.equal(env.downloads.length, 2)
  assert.equal(env.files.size, 1, '仅剩当前在途暂存')
  env.downloads[1].finish()
  const entry = await current
  assert.equal(entry.startSeconds, 23)
  assert.equal(env.files.size, 1)
  assert.equal(env.cache.pinnedPaths[entry.path], 1)
  assert.equal(Object.keys(env.cache.pending).length, 0)
  assert.equal(Object.keys(env.cache.reservations).length, 0)
  env.cache.unpin(entry)
})

test('已缓存读者失效不固定文件；残留暂存文件启动时回收', async () => {
  const env = fixture()
  const entry = await env.cache.getAt('v', sourceUrl, 1)
  await assert.rejects(env.cache.getAt('v', sourceUrl, 1, true, () => false), /取消/)
  assert.equal(Object.keys(env.cache.pinnedPaths).length, 0)
  const partial = entry.path + '.part'
  env.files.set(partial, MiB)
  env.cache.files.needsRefresh = true
  await env.cache.evictOldEntries('')
  assert.ok(env.files.has(entry.path))
  assert.ok(!env.files.has(partial))
  assert.equal(env.cache.loadIndex()[entry.key].path, entry.path)
})