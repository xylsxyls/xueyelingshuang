const test = require('node:test')
const assert = require('node:assert/strict')
const fs = require('node:fs')
const path = require('node:path')
const vm = require('node:vm')

function fixture(options = {}) {
  const calls = []
  let render
  const wx = {
    env: { USER_DATA_PATH: 'http://usr' },
    getDeviceInfo: () => ({ platform: options.platform || 'devtools' }),
    createVideoContext: (id, owner) => {
      calls.push(['context', id, owner])
      return { play: () => calls.push(['play']), pause: () => calls.push(['pause']), seek: seconds => calls.push(['seek', seconds]) }
    }
  }
  const sandbox = { wx, module: { exports: {} }, setTimeout, clearTimeout, console }
  vm.runInNewContext(fs.readFileSync(path.resolve(__dirname, '../../CookWx/utils/videoLocalPreview.js'), 'utf8'), sandbox)
  const owner = {
    _alive: true, _generation: 7, data: { previewSegments: [] },
    setData(values, callback) { Object.assign(this.data, values); if (callback) { if (options.delayed) render = callback; else callback() } },
    triggerEvent: (...args) => calls.push(['event', ...args]),
    onSegmentEnded: generation => calls.push(['ended', generation]),
    handlePlaybackError: (error, generation) => calls.push(['error', error.message, generation])
  }
  const entry = { path: 'http://usr/cook-video-cache/abc_3_5.mp4', actualStartMilliseconds: 2000, startSeconds: 3 }
  const session = new sandbox.module.exports.LocalVideoPreview(owner, entry, 5, 7, options.timeoutMs || 1000)
  const event = (type, detail = {}, generation = 7) => session.handle({ type, detail, currentTarget: { dataset: { generation } } })
  return { owner, session, entry, event, calls, render: () => render(), isDeveloperTools: sandbox.module.exports.isDeveloperTools }
}

test('开发工具虚拟本地路径可用，加载需等待真实元数据、播放和目标时间事件', async () => {
  const { owner, session, event, calls } = fixture()
  let ready = false
  const pending = session.start().then(value => { ready = true; return value })
  assert.equal(owner.data.previewSegments[0].startSeconds, 3)
  assert.equal(calls[0][1], 'localPreview7')
  event('loadedmetadata', { width: 720, height: 1280 })
  event('play')
  event('timeupdate', { currentTime: 0 })
  await Promise.resolve()
  assert.equal(ready, false)
  event('timeupdate', { currentTime: 3.2 })
  const metadata = await pending
  assert.equal(metadata.width, 720)
  assert.equal(session.currentPosition(), 5.2)
  event('ended')
  assert.deepEqual(calls[calls.length - 1], ['ended', 7])
  session.stop()
})

test('本地预览拒绝服务器完整视频地址，不把远程URL交给video组件', async () => {
  const { session, entry, calls } = fixture()
  entry.path = 'http://example.com/api/videos/file?videoId=video'
  await assert.rejects(session.start(), /只能读取本地/)
  assert.equal(calls.length, 0)
})

test('旧节点事件不能完成新定位，取消后晚到的渲染回调不再启动播放', async () => {
  const { session, event, render, calls, owner } = fixture({ delayed: true })
  const pending = session.start()
  const canceled = assert.rejects(pending, /已取消/)
  event('loadedmetadata', { width: 720 }, 6)
  event('play', {}, 6)
  event('timeupdate', { currentTime: 3 }, 6)
  assert.equal(session.started, false)
  session.stop()
  render()
  await canceled
  assert.equal(calls.length, 0)
  assert.equal(owner.data.previewSegments.length, 0)
})

test('缺失播放事件有期限，原生解码失败原样返回', async () => {
  const expired = fixture({ timeoutMs: 20 })
  await assert.rejects(expired.session.start(), /加载超时/)
  expired.session.stop()
  const broken = fixture()
  const pending = broken.session.start()
  broken.event('error', { errMsg: 'video:fail decode' })
  await assert.rejects(pending, /video:fail decode/)
  broken.session.stop()
})

test('旧会话清理不会删除新会话节点，停止只暂停已持有的本地上下文', async () => {
  const { session, owner, calls } = fixture()
  const pending = session.start()
  const canceled = assert.rejects(pending, /已取消/)
  owner.data.previewSegments = [{ generation: 8, path: 'http://usr/cook-video-cache/next.mp4' }]
  session.stop()
  await canceled
  assert.equal(owner.data.previewSegments[0].generation, 8)
  assert.deepEqual(calls[calls.length - 1], ['pause'])
})

test('仅开发工具走本地预览，真机保持VideoDecoder路径', () => {
  assert.equal(fixture().isDeveloperTools(), true)
  assert.equal(fixture({ platform: 'ios' }).isDeveloperTools(), false)
  assert.equal(fixture({ platform: 'android' }).isDeveloperTools(), false)
})

test('显式暂停可保留最后画面，已暂停节点的事件不能继续推进时间', async () => {
  const { session, owner, event, calls } = fixture()
  const pending = session.start()
  event('loadedmetadata', { width: 720 })
  event('play')
  event('timeupdate', { currentTime: 3 })
  await pending
  session.stop(true)
  assert.equal(owner.data.previewSegments.length, 1)
  assert.deepEqual(calls[calls.length - 1], ['pause'])
  event('timeupdate', { currentTime: 4 })
  assert.equal(session.currentPosition(), 5)
})

test('元数据后显式定位，越过目标很远的事件不能冒充定位成功', async () => {
  const { session, event, calls } = fixture()
  const pending = session.start()
  event('loadedmetadata', { duration: 8, width: 720 })
  assert.ok(calls.some(call => call[0] === 'seek' && call[1] === 3))
  event('play')
  event('timeupdate', { currentTime: 6 })
  assert.equal(session.started, false)
  event('timeupdate', { currentTime: 2.7 })
  assert.equal(session.started, false, '提前画面不可冒充目标')
  event('timeupdate', { currentTime: 3.1 })
  await pending
  session.stop()
})

test('读者代次已变化时旧片段的播放、结束与错误全部忽略', async () => {
  const { session, owner, event, calls } = fixture()
  const pending = session.start()
  const canceled = assert.rejects(pending, /已取消/)
  const before = calls.length
  owner._generation = 8
  event('loadedmetadata', { width: 720 })
  event('play')
  event('timeupdate', { currentTime: 3.1 })
  event('ended')
  event('error', { errMsg: 'late error' })
  assert.equal(session.started, false)
  assert.equal(calls.length, before)
  session.stop()
  await canceled
})

test('未就绪片段不会覆盖旧画面，加载中暂停保留已确认节点', async () => {
  const { session, owner } = fixture()
  owner.data.previewSegments = [{ generation: 6, ready: true, path: 'http://usr/cook-video-cache/old.mp4' }]
  const pending = session.start()
  const canceled = assert.rejects(pending, /已取消/)
  assert.equal(owner.data.previewSegments.length, 2)
  assert.equal(owner.data.previewSegments[1].ready, false)
  session.stop(true)
  await canceled
  assert.equal(owner.data.previewSegments.length, 1)
  assert.equal(owner.data.previewSegments[0].generation, 6)
  assert.equal(owner.data.previewSegments[0].ready, true)
})

test('目标时间确认后替换旧节点，启动加载不能提前露出画面', async () => {
  const { session, owner, event, calls } = fixture()
  owner.data.previewSegments = [{ generation: 6, ready: true, path: 'http://usr/cook-video-cache/old.mp4' }]
  const pending = session.start()
  assert.ok(calls.some(call => call[0] === 'play'), '启动媒体加载避免元数据死等')
  assert.equal(owner.data.previewSegments[1].ready, false)
  event('loadedmetadata', { width: 720 })
  event('play')
  event('timeupdate', { currentTime: 3.1 })
  await pending
  assert.equal(owner.data.previewSegments.length, 1)
  assert.equal(owner.data.previewSegments[0].generation, 7)
  assert.equal(owner.data.previewSegments[0].ready, true)
  session.stop()
})

test('解码错误后的晚到时间事件不会调用已清空的完成回调', async () => {
  const { session, event } = fixture()
  const pending = session.start()
  const failed = assert.rejects(pending, /decode failed/)
  event('error', { errMsg: 'decode failed' })
  assert.doesNotThrow(() => {
    event('loadedmetadata', { width: 720 })
    event('play')
    event('timeupdate', { currentTime: 3.1 })
  })
  await failed
  assert.equal(session.started, false)
  session.stop()
})