const test = require('node:test')
const assert = require('node:assert/strict')
const fs = require('node:fs')
const path = require('node:path')
const vm = require('node:vm')
const { EventEmitter } = require('node:events')

function playerFixture(settings = {}) {
  let definition
  const native = new EventEmitter()
  native.off = native.removeListener.bind(native)
  native.start = () => undefined
  native.seek = value => { native.seekPosition = value }
  native.stop = () => undefined
  native.remove = () => undefined
  const events = []
  const requests = []
  const cache = { cancelUnused() {}, pin: entry => { if (entry) entry.pins = (entry.pins || 0) + 1 }, unpin: entry => { if (entry) entry.pins -= 1 }, getSegment: (...args) => { requests.push(args); return Promise.resolve() } }
  const playerDirectory = path.resolve(__dirname, '../../CookWx/components/cachedVideoPlayer')
  const sandbox = {
    Component: value => { definition = value }, wx: { createVideoDecoder: () => native },
    require: name => name.includes('videoSegmentCache') ? cache : name.includes('config') ? settings : name.includes('videoLocalPreview') ? { isDeveloperTools: () => false } : name.includes('Lifecycle') ? require(path.resolve(playerDirectory, name)) : function Renderer() {},
    setTimeout, clearTimeout, Date, Number, Promise, console
  }
  vm.runInNewContext(fs.readFileSync(path.join(playerDirectory, 'cachedVideoPlayer.js'), 'utf8'), sandbox)
  const player = {
    ...definition.methods, properties: { active: true }, data: { previewSegments: [] }, _alive: true, _generation: 1, _desiredPlaying: true,
    _renderer: { resize: (width, height) => { player.resolution = [width, height] } },
    setData() {}, triggerEvent: name => events.push(name), startFrameLoop() {}, startTimeUpdates() {}, schedulePrefetch() {}
  }
  return { player, native, events, definition, requests, cache }
}

test('原生 start/seek 返回 void 时必须等真实完成事件才通知可播放', async () => {
  const { player, native, events } = playerFixture()
  const promise = player.startDecoder({ path: '/local.mp4', actualStartMilliseconds: 2000 }, 5, 1)
  await new Promise(resolve => setImmediate(resolve))
  assert.ok(!events.includes('seekcomplete'), '不能在原生 start 完成前宣称定位完成')
  native.emit('start', { width: 640, height: 360 })
  await new Promise(resolve => setImmediate(resolve))
  assert.equal(native.seekPosition, 3000)
  assert.ok(!events.includes('seekcomplete'), '不能在原生 seek 完成前通知页面')
  native.emit('seek')
  await promise
  assert.ok(events.includes('seekcomplete'))
  assert.deepEqual(player.resolution, [640, 360])
})

test('暂停或切到后台后不再固定不参与解码的缓存文件', async () => {
  const { player } = playerFixture()
  const entry = { path: '/local.mp4', pins: 1 }
  player._currentEntry = entry
  player.pause()
  await new Promise(resolve => setImmediate(resolve))
  assert.equal(entry.pins, 0)
  assert.equal(player._currentEntry, null)
})

test('解码事件缺失或取消时清理监听，不留下无界等待', async () => {
  const { decoderOperation } = require('../../CookWx/utils/videoDecoderLifecycle')
  const native = new EventEmitter()
  const missing = decoderOperation(native, 'start', () => undefined, 20)
  await assert.rejects(missing.promise, /超时/)
  assert.equal(native.listenerCount('start'), 0)
  const canceled = decoderOperation(native, 'seek', () => undefined, 10000)
  canceled.cancel()
  await assert.rejects(canceled.promise, /取消/)
  assert.equal(native.listenerCount('seek'), 0)
})

test('首段触发点和后续提前量都由配置控制，触发前不下载、结尾不预取', () => {
  for (const scenario of [
    { settings: {}, first: 3, next: 5, trigger: 1, lead: 2 },
    { settings: { VIDEO_CACHE_INITIAL_PREFETCH_AT_SECONDS: 0.5, VIDEO_CACHE_NEXT_SECONDS: 4, VIDEO_CACHE_PREFETCH_LEAD_SECONDS: 1 }, first: 2, next: 4, trigger: 0.5, lead: 1 }
  ]) {
    const { player, definition, requests } = playerFixture(scenario.settings)
    const prefetch = () => definition.methods.schedulePrefetch.call(player)
    player._durationSeconds = 18
    player._currentEntry = { startSeconds: 0, durationSeconds: scenario.first }
    player._positionSeconds = scenario.trigger - 0.01
    prefetch()
    assert.equal(requests.length, 0)
    player._positionSeconds = scenario.trigger
    prefetch()
    assert.deepEqual(requests[0].slice(2, 4), [scenario.first, scenario.next])
    for (let repeat = 0; repeat < 20; repeat += 1) prefetch()
    assert.equal(requests.length, 1, '时钟重复触发不能反复预取或同步写索引')
    player._currentEntry = { startSeconds: scenario.first, durationSeconds: scenario.next }
    player._positionSeconds = scenario.first + scenario.next - scenario.lead - 0.01
    prefetch()
    assert.equal(requests.length, 1)
    player._positionSeconds += 0.01
    prefetch()
    assert.deepEqual(requests[1].slice(2, 4), [scenario.first + scenario.next, scenario.next])
    player._currentEntry = { startSeconds: 15, durationSeconds: 3 }
    player._positionSeconds = 17
    prefetch()
    assert.equal(requests.length, 2)
  }
})

test('暂停期间的旧时钟不能让恢复位置跳过暂停的时间', () => {
  const { player } = playerFixture()
  player.properties.videoUrl = '/video.mp4'
  player._positionSeconds = 5
  player._clockStartPosition = 5
  player._clockStartAt = Date.now() - 10000
  player._desiredPlaying = false
  let resumedAt
  player.loadAndPlay = value => { resumedAt = value }
  player.play()
  assert.equal(resumedAt, 5)
})

test('新片段交接期间取消播放必须释放借用引用，不能泄漏 pin', async () => {
  const { player, cache } = playerFixture()
  const entry = { path: '/handoff.mp4', pins: 0 }
  let releaseDecoder
  cache.getAt = async (id, url, position, pinOnRead) => { assert.equal(pinOnRead, true); cache.pin(entry); return entry }
  player.stopDecoder = () => new Promise(resolve => { releaseDecoder = resolve })
  const request = player.loadAndPlay(0)
  await new Promise(resolve => setImmediate(resolve))
  assert.equal(entry.pins, 1, '关闭旧解码器时新片段已固定')
  player._generation += 1
  player._desiredPlaying = false
  releaseDecoder()
  await request
  assert.equal(entry.pins, 0)
})

test('同一片段恢复播放只保留一个固定引用，切换时释放旧片段', async () => {
  const { player, cache } = playerFixture()
  const previous = { path: '/previous.mp4', pins: 1 }
  const next = { path: '/next.mp4', pins: 0 }
  let requested = previous
  cache.getAt = async () => { cache.pin(requested); return requested }
  player._currentEntry = previous
  player.stopDecoder = async () => { cache.unpin(player._currentEntry); player._currentEntry = null }
  player.startDecoder = async () => {}
  await player.loadAndPlay(0)
  assert.equal(previous.pins, 1)
  requested = next
  await player.loadAndPlay(5)
  assert.equal(previous.pins, 0)
  assert.equal(next.pins, 1)
  assert.equal(player._currentEntry, next)
})

test('拖到进度条终点读取最后可播放帧，不请求媒体结束之后的位置', () => {
  for (const duration of [0.1, 18, 41.233]) {
    const { player } = playerFixture()
    player._durationSeconds = duration
    let target
    player.loadAndPlay = position => { target = position }
    player.seek(duration)
    assert.ok(target >= 0 && target < duration)
    assert.equal(player._requestedPosition, duration, '完成事件仍关联用户提交的终点目标')
  }
})

test('最后片段结束保留最后画面，停止解码并释放固定引用', async () => {
  const { player, events } = playerFixture()
  const entry = { path: '/last.mp4', startSeconds: 18, durationSeconds: 5, pins: 1 }
  let kept
  player._pageVisible = true
  player._durationSeconds = 20
  player._currentEntry = entry
  player._preview = { stop: keep => { kept = keep } }
  await player.onSegmentEnded(1)
  assert.equal(kept, true)
  assert.equal(entry.pins, 0)
  assert.equal(player._currentEntry, null)
  assert.equal(player._desiredPlaying, false)
  assert.ok(events.includes('ended'))
})

test('隐藏页面不接受播放或定位；恢复可见后才允许播放', () => {
  const { player, definition } = playerFixture()
  const requested = []
  player.properties.videoUrl = '/video.mp4'
  player.properties.autoplay = true
  player._desiredPlaying = false
  player._pageVisible = false
  player.loadAndPlay = position => requested.push(position)
  player.play()
  player.seek(5)
  assert.equal(requested.length, 0)
  definition.pageLifetimes.show.call(player)
  assert.equal(requested.length, 1)
})

test('ready晚于页面隐藏时不能重置可见状态或启动延迟自动播放', async () => {
  const { player, definition } = playerFixture()
  const requested = []
  player.properties.videoUrl = '/video.mp4'
  player.properties.autoplay = true
  player._pageVisible = false
  player.createCanvas = () => Promise.resolve()
  player.loadAndPlay = position => requested.push(position)
  definition.lifetimes.ready.call(player)
  await player._readyPromise
  await Promise.resolve()
  assert.equal(player._pageVisible, false)
  assert.equal(requested.length, 0)
})