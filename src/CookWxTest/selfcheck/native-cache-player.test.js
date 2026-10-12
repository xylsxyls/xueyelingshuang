const test = require('node:test')
const assert = require('node:assert/strict')
const fs = require('node:fs')
const path = require('node:path')
const vm = require('node:vm')
const flush = () => new Promise(resolve => setImmediate(resolve))

function fixture() {
  const root = path.resolve(__dirname, '../../CookWx'), modules = new Map(), contexts = [], events = []
  let definition, tick, owner
  const entries = [0, 3, 8, 13].map((start, index) => ({ path: 'wxfile://usr/cook-video-cache/v_' + start + '.mp4',
    startSeconds: start, durationSeconds: start ? 5 : 3, actualStartMilliseconds: index ? (start - 1) * 1000 : 0, mediaDurationSeconds: 18, pins: 0 }))
  const reads = []
  const cache = { findAt: (id, url, time) => entries.find(entry => time >= entry.startSeconds && time < entry.startSeconds + entry.durationSeconds),
    getAt: async (id, url, time, pin, wanted) => { reads.push(time); const entry = cache.findAt(id, url, time); if (!wanted()) throw new Error('canceled'); if (pin) entry.pins++; return entry },
    unpin: entry => { if (entry) { entry.pins--; assert.ok(entry.pins >= 0, '不能重复释放缓存文件') } }, cancelUnused() {} }
  const wx = { env: { USER_DATA_PATH: 'wxfile://usr' }, getDeviceInfo: () => ({ platform: 'android' }),
    createVideoContext(id) { const context = { id, plays: 0, pauses: 0, seeks: [], play() { this.plays++ }, pause() { this.pauses++ }, seek(value) { this.seeks.push(value) } }; contexts.push(context); return context } }
  function load(file) {
    if (file.endsWith('videoSegmentCache.js')) return cache
    if (file.endsWith('config.js')) return { VIDEO_PLAYER_BACKEND: 'local-native', VIDEO_CACHE_INITIAL_PREFETCH_AT_SECONDS: 1, VIDEO_CACHE_NEXT_SECONDS: 5, VIDEO_CACHE_PREFETCH_LEAD_SECONDS: 2 }
    if (modules.has(file)) return modules.get(file).exports
    const module = { exports: {} }; modules.set(file, module)
    vm.runInNewContext(fs.readFileSync(file, 'utf8'), { module, wx, console, setTimeout, clearTimeout, Date, Promise, require: name => load(path.resolve(path.dirname(file), name) + '.js') })
    return module.exports
  }
  const componentFile = path.join(root, 'components/cachedVideoPlayer/cachedVideoPlayer.js')
  vm.runInNewContext(fs.readFileSync(componentFile, 'utf8'), { Component: value => { definition = value }, wx, console, setTimeout, clearTimeout,
    setInterval: callback => { tick = callback; return 1 }, clearInterval() {}, Date, require: name => load(path.resolve(path.dirname(componentFile), name) + '.js') })
  owner = { ...definition.methods, properties: { videoId: 'v', videoUrl: 'https://server/api/videos/file?videoId=v', active: true, loop: false },
    data: { previewSegments: [], status: '' }, _alive: true, _pageVisible: true, _desiredPlaying: true, _previewMode: true,
    _readyPromise: Promise.resolve(), _generation: 1, _durationSeconds: 18, _positionSeconds: 0,
    triggerEvent: (name, detail) => events.push({ name, detail }),
    setData(values, callback) { Object.assign(this.data, values); assert.ok(this.data.previewSegments.length <= 2, '节点必须有界'); if (callback) callback() } }
  function event(session, type, detail = {}) { owner.onPreviewEvent({ type, detail, currentTarget: { dataset: { generation: session.generation } } }) }
  async function ready(session, local) { event(session, 'loadedmetadata', { width: 720, height: 1280 }); event(session, 'play'); event(session, 'timeupdate', { currentTime: local }); await flush() }
  async function start() { const pending = owner.loadAndPlay(0); await flush(); await ready(owner._preview, 0.05); await pending }
  return { owner, cache, entries, reads, contexts, events, definition, event, ready, start, tick: () => tick() }
}

test('下一本地片段提前准备后暂停静音待命，交接不重新读取或创建节点', async () => {
  const f = fixture(); await f.start()
  f.event(f.owner._preview, 'timeupdate', { currentTime: 1.1 }); f.tick(); await flush()
  const next = f.owner._nextSegment
  assert.ok(next && next.preview)
  await f.ready(next.preview, 1.25); await next.promise
  assert.equal(f.owner.data.previewSegments.length, 2)
  assert.equal(f.owner.data.previewSegments[1].ready, false)
  assert.ok(f.contexts[1].pauses > 0, '未来片段就绪后必须暂停')
  assert.equal(f.contexts[1].seeks.length, 2, '预加载确认后要回到片段起点，不能跳过预加载时已走过的画面')
  assert.equal(next.preview.currentPosition(), 3)
  f.event(next.preview, 'timeupdate', { currentTime: 1.4 })
  assert.equal(next.preview.currentPosition(), 3, '待命时的晚到时间事件不推进未来片段')
  const readCount = f.reads.length, contextCount = f.contexts.length
  await f.owner.onSegmentEnded(f.owner._generation)
  assert.equal(f.reads.length, readCount)
  assert.equal(f.contexts.length, contextCount)
  assert.equal(f.owner.data.previewSegments.length, 1)
  assert.equal(f.entries[0].pins, 0)
  assert.equal(f.entries[1].pins, 1)
  assert.ok(f.owner.currentPosition() >= 3 && f.owner.currentPosition() < 3.5)
  f.owner.pause()
})

test('同一缓存片段内定位复用现有节点，连续反向定位只确认最新目标', async () => {
  const f = fixture(); await f.start()
  const session = f.owner._preview, reads = f.reads.length, count = f.contexts.length
  f.owner.seek(2.2); f.owner.seek(0.8)
  f.event(session, 'timeupdate', { currentTime: 2.2 }); await flush()
  assert.equal(f.events.filter(item => item.name === 'seekcomplete').length, 1, '旧定位不能完成新目标')
  f.event(session, 'timeupdate', { currentTime: 0.85 }); await flush()
  assert.equal(f.reads.length, reads)
  assert.equal(f.contexts.length, count)
  assert.ok(f.owner.currentPosition() < 1)
  const completed = f.events.filter(item => item.name === 'seekcomplete').pop()
  assert.equal(completed.detail.requestedPosition, 0.8)
  f.owner.pause()
})

test('预加载原生节点尚未就绪时，前台定位不等它的15秒超时', async () => {
  const f = fixture(); await f.start()
  f.event(f.owner._preview, 'timeupdate', { currentTime: 1.1 }); f.tick(); await flush()
  const warming = f.owner._nextSegment.preview
  const pending = f.owner.onSegmentEnded(f.owner._generation); await flush()
  assert.notEqual(f.owner._preview, warming)
  assert.equal(warming.stopped, true)
  assert.ok(f.owner._preview && f.contexts.length === 3, '必须立即启动前台节点而不是等待旧预加载完成')
  await f.ready(f.owner._preview, 1.05); await pending
  assert.equal(f.entries[1].pins, 1)
  f.owner.pause()
})

test('切到后台同时取消预加载、释放两段文件，旧事件不能再次播放', async () => {
  const f = fixture(); await f.start()
  f.event(f.owner._preview, 'timeupdate', { currentTime: 1.1 }); f.tick(); await flush()
  const warming = f.owner._nextSegment.preview, old = f.owner._preview
  f.definition.pageLifetimes.hide.call(f.owner)
  await flush()
  assert.equal(f.entries[0].pins, 0)
  assert.equal(f.entries[1].pins, 0)
  const events = f.events.length
  f.event(warming, 'timeupdate', { currentTime: 1.05 }); f.event(old, 'ended')
  assert.equal(f.events.length, events)
})

test('预加载就绪后解码失败只释放未来片段，正在播放的节点继续运行', async () => {
  const f = fixture(); await f.start()
  const current = f.owner._preview
  f.event(current, 'timeupdate', { currentTime: 1.1 }); f.tick(); await flush()
  const next = f.owner._nextSegment
  await f.ready(next.preview, 1.05); await next.promise
  f.event(next.preview, 'error', { errMsg: 'preload failed' }); await flush()
  assert.equal(next.closed, true)
  assert.equal(f.owner._preview, current)
  assert.equal(f.owner._desiredPlaying, true)
  assert.equal(f.entries[1].pins, 0)
  assert.equal(next.take(), null)
  f.owner.pause()
})

test('同段定位后原生错误按操作代次处理，不能混用节点编号而忽略错误', async () => {
  const f = fixture(); await f.start()
  const current = f.owner._preview
  f.owner.seek(0.8)
  f.event(current, 'timeupdate', { currentTime: 0.85 }); await flush()
  assert.notEqual(current.generation, f.owner._generation)
  f.event(current, 'error', { errMsg: 'decode failed after seek' }); await flush()
  assert.equal(f.owner._desiredPlaying, false)
  assert.equal(f.owner._preview, null)
  assert.equal(f.owner.data.status, 'decode failed after seek')
  assert.equal(f.entries[0].pins, 0)
})

test('原生路径只挂本地缓存文件；默认真机不创建Canvas解码器', async () => {
  const f = fixture(); await f.start()
  assert.ok(f.owner.data.previewSegments.every(item => item.path.startsWith('wxfile://usr/cook-video-cache/')))
  assert.equal(f.owner._decoder, null)
  f.owner.pause()
})

test('Android默认选择本地原生节点而不是创建Canvas渲染器', async () => {
  const f = fixture()
  f.definition.lifetimes.ready.call(f.owner)
  await f.owner._readyPromise
  assert.equal(f.owner._previewMode, true)
  assert.equal(f.owner.data.localPreview, true)
  assert.equal(f.owner._renderer, null)
})

test('终点定位在可播放确认前到达ended，不得丢失完成或卡住加载', async () => {
  const f = fixture()
  const pending = f.owner.loadAndPlay(17.8)
  await flush()
  const session = f.owner._preview
  f.event(session, 'loadedmetadata', { width: 720, height: 1280 })
  f.event(session, 'play')
  f.event(session, 'timeupdate', { currentTime: 5.8 })
  f.event(session, 'ended')
  await pending; await flush()
  assert.ok(f.events.some(item => item.name === 'seekcomplete'))
  assert.ok(f.events.some(item => item.name === 'ended'))
  assert.equal(f.owner._loadInFlight, false)
  assert.equal(f.owner._desiredPlaying, false)
})

test('按实际下载耗时提前预取，绘制限帧也持续排出旧解码帧', () => {
  const timing = require('../../CookWx/utils/videoPlaybackTiming')
  assert.equal(timing.prefetchLead(2, 4, true, 5), 5)
  assert.equal(timing.prefetchLead(2, 4, false, 5), 2)
  let remaining = 6
  const frame = timing.latestFrame({ getFrameData() { return remaining ? { data: new ArrayBuffer(4), index: remaining-- } : null } }, 4)
  assert.equal(frame.index, 3)
  assert.equal(remaining, 2)
  let previous = 1000, paints = 0
  for (let tick = 1; tick <= 60; tick++) { const next = timing.renderTime(previous, 1000 + tick * 1000 / 60, 1000 / 30); if (next !== previous) paints++; previous = next }
  assert.ok(paints >= 29 && paints <= 30, '60Hz绘制不能因每次丢掉小数变成20fps')
})