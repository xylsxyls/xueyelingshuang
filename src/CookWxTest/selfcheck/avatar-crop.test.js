const test = require('node:test')
const assert = require('node:assert/strict')
const fs = require('node:fs')
const path = require('node:path')
const vm = require('node:vm')
const cropper = require('../../CookWx/utils/avatarCrop')
const config = require('../../CookWx/utils/config')

function fixture() {
  const app = { globalData: { userId: 'owner' } }
  const calls = { updates: [], toast: [], back: [], uploaded: [], timers: new Map() }
  let definition, timerId = 0
  const context = { save() {}, restore() {}, setTransform() {}, translate() {}, scale() {}, transform() {}, drawImage() {}, fillRect() {}, clearRect() {} }
  const media = {
    nativeCall: async () => ({ tempFiles: [{ tempFilePath: 'photo' }] }),
    prepare: async () => ({ width: 600, height: 800, orientation: 'up', path: 'photo' }),
    loadImage: async () => ({}), exportBytes: async () => new ArrayBuffer(8), removePrepared() {}
  }
  const api = { uploadAvatar: async (data, options) => { calls.uploaded.push({ data, options }); return { ok: true, profile: { avatarUrl: '/api/users/avatar' } } }, isStaleError: error => !!error.stale }
  const wx = { chooseMedia() {}, showToast: value => calls.toast.push(value), navigateBack: value => calls.back.push(value) }
  const file = path.resolve(__dirname, '../../CookWx/pages/avatarCrop/avatarCrop.js')
  vm.runInNewContext(fs.readFileSync(file, 'utf8'), {
    Page: value => { definition = value }, getApp: () => app, wx,
    setTimeout: callback => { calls.timers.set(++timerId, callback); return timerId }, clearTimeout: id => calls.timers.delete(id),
    require: name => name.endsWith('/api') ? api : name.endsWith('/avatarMedia') ? media : require(path.resolve(path.dirname(file), name))
  }, { filename: file })
  const page = { ...definition, data: JSON.parse(JSON.stringify(definition.data)),
    setData(value) { Object.assign(this.data, value); calls.updates.push(value) } }
  page.onLoad()
  page.viewport = { width: 300, height: 400 }
  page.preview = page.output = { getContext: () => context, width: 512, height: 512 }
  page.ratio = 1
  return { page, app, calls, media, api, wx }
}

test('裁剪按比例居中且拒绝无效尺寸', () => {
  assert.deepEqual(cropper.fit(600, 800, 300, 500), { x: 0, y: 50, width: 300, height: 400 })
  for (const size of [0, -1, NaN, Infinity]) assert.equal(cropper.fit(size, 800, 300, 500), null)
})

test('拖动和缩放保持方形、最小尺寸及四边边界', () => {
  const image = { x: 30, y: 20, width: 260, height: 400 }
  const initial = cropper.initial(image, 48)
  const move = cropper.gesture([{ clientX: 0, clientY: 0 }], 'move', initial)
  const last = cropper.update(move, [{ clientX: 9999, clientY: -9999 }], image, 48)
  assert.equal(last.x + last.size, image.x + image.width)
  assert.equal(last.y, 20)
  const minimum = cropper.resize(initial, -900, image, 48, false)
  assert.equal(minimum.size, 48)
  const maximum = cropper.resize(initial, 9999, image, 48, true)
  assert.equal(maximum.size, 260)
  assert.equal(maximum.x, 30)
})

test('双指缩放和移动仍限制在图片范围', () => {
  const image = { x: 0, y: 0, width: 300, height: 400 }, square = { x: 50, y: 60, size: 100 }
  const gesture = cropper.gesture([{ clientX: 0, clientY: 0 }, { clientX: 100, clientY: 0 }], 'move', square)
  const result = cropper.update(gesture, [{ clientX: 0, clientY: 40 }, { clientX: 200, clientY: 40 }], image, 48)
  assert.equal(result.size, 200)
  assert.equal(result.x, 50)
  assert.equal(result.y, 50)
})

test('八种照片方向将四角映射到预期方向且不越界', () => {
  const expected = {
    up: [[0, 0], [600, 0], [600, 800], [0, 800]],
    'up-mirrored': [[600, 0], [0, 0], [0, 800], [600, 800]],
    down: [[600, 800], [0, 800], [0, 0], [600, 0]],
    'down-mirrored': [[0, 800], [600, 800], [600, 0], [0, 0]],
    left: [[0, 600], [0, 0], [800, 0], [800, 600]],
    right: [[800, 0], [800, 600], [0, 600], [0, 0]],
    'left-mirrored': [[0, 0], [0, 600], [800, 600], [800, 0]],
    'right-mirrored': [[800, 600], [800, 0], [0, 0], [0, 600]]
  }
  for (const [orientation, corners] of Object.entries(expected)) {
    const [a, b, c, d, e, f] = cropper.orientationMatrix(600, 800, orientation)
    const mapped = [[0, 0], [600, 0], [600, 800], [0, 800]].map(([x, y]) => [a * x + c * y + e, b * x + d * y + f])
    assert.deepEqual(mapped, corners)
  }
})

test('取消换图与图片处理失败保留当前裁剪', async () => {
  const f = fixture(); await f.page.choose()
  const before = JSON.stringify(f.page.data.crop)
  f.media.nativeCall = async () => { throw Object.assign(new Error('cancel'), { cancelled: true }) }
  await f.page.choose()
  assert.equal(JSON.stringify(f.page.data.crop), before)
  assert.equal(f.calls.toast.length, 0)
  f.media.nativeCall = async () => ({ tempFiles: [{ tempFilePath: 'bad' }] })
  f.media.prepare = async () => { throw new Error('bad image') }
  await f.page.choose()
  assert.equal(JSON.stringify(f.page.data.crop), before)
  assert.equal(f.page.data.ready, true)
  assert.equal(f.page.data.loading, false)
})

test('高频移动合并setData，结束手势刷新最后一次坐标', async () => {
  const f = fixture(); await f.page.choose()
  const event = x => ({ currentTarget: { dataset: { mode: 'move' } }, touches: [{ clientX: x, clientY: 0 }] })
  f.page.touchStart(event(0)); const updates = f.calls.updates.length
  for (let i = 0; i < 500; i++) f.page.touchMove(event(i))
  assert.equal(f.calls.timers.size, 1)
  assert.equal(f.calls.updates.length, updates)
  f.page.touchEnd({ touches: [] })
  assert.equal(f.calls.timers.size, 0)
  assert.equal(f.page.data.crop.x + f.page.data.crop.size, 300)
})

test('保存只上传裁剪二进制，成功后刷新资料并返回', async () => {
  const f = fixture(); await f.page.choose(); await f.page.save()
  assert.equal(f.calls.uploaded.length, 1)
  assert.equal(f.calls.uploaded[0].data.byteLength, 8)
  assert.equal(f.calls.back.length, 1)
  assert.equal(f.app.globalData.socialStateVersion, 1)
})

test('服务器拒绝头像时不返回且允许重试', async () => {
  const f = fixture(); await f.page.choose()
  f.api.uploadAvatar = async () => ({ ok: false, message: 'disk failed' })
  await f.page.save()
  assert.equal(f.calls.back.length, 0)
  assert.equal(f.page.data.saving, false)
  assert.equal(f.page.data.ready, true)
})

test('退出或切换账号后拒绝晚到上传响应', async () => {
  for (const exit of ['unload', 'account']) {
    const f = fixture(); await f.page.choose()
    let resolve
    f.api.uploadAvatar = (_, options) => { f.calls.uploaded.push(options); return new Promise(done => { resolve = done }) }
    const pending = f.page.save()
    await new Promise(done => setImmediate(done))
    if (exit === 'unload') f.page.onUnload()
    else f.app.globalData.userId = 'another'
    assert.equal(f.calls.uploaded[0].isStale(), true)
    resolve({ ok: true, profile: {} }); await pending
    assert.equal(f.calls.back.length, 0)
    assert.equal(f.calls.toast.length, 0)
    assert.equal(f.app.globalData.socialStateVersion, undefined)
  }
})

test('解码回调晚到不能恢复已销毁页面', async () => {
  const f = fixture(); let resolve
  f.media.prepare = () => new Promise(done => { resolve = done })
  const pending = f.page.choose(); await new Promise(done => setImmediate(done))
  f.page.onUnload(); const updates = f.calls.updates.length
  resolve({ width: 10, height: 10, path: 'photo' }); await pending
  assert.equal(f.calls.updates.length, updates)
  assert.equal(f.page.data.ready, false)
})

test('同一保存请求不会重复上传', async () => {
  const f = fixture(); await f.page.choose(); let resolve
  f.media.exportBytes = () => new Promise(done => { resolve = done })
  const first = f.page.save(); await new Promise(done => setImmediate(done)); await f.page.save()
  resolve(new ArrayBuffer(4)); await first
  assert.equal(f.calls.uploaded.length, 1)
})

test('头像配置有明确输出和流量上限', () => {
  assert.equal(config.AVATAR_OUTPUT_SIDE, 512)
  assert.equal(config.AVATAR_MAX_BYTES, 512 * 1024)
  assert.ok(config.AVATAR_JPEG_QUALITY > 0 && config.AVATAR_JPEG_QUALITY <= 1)
})

test('过期压缩结果与页面当前压缩图均会清理，原图不删除', async () => {
  const f = fixture(), removed = []
  f.media.removePrepared = info => { if (info && info.ownedPath) removed.push(info.ownedPath) }
  f.media.prepare = async () => ({ width: 600, height: 800, path: 'compressed', ownedPath: 'compressed' })
  await f.page.choose(); f.page.onUnload()
  assert.deepEqual(removed, ['compressed'])
  const late = fixture(); let resolve
  late.media.removePrepared = f.media.removePrepared
  late.media.prepare = () => new Promise(done => { resolve = done })
  const pending = late.page.choose(); await new Promise(done => setImmediate(done))
  late.page.onUnload(); resolve({ width: 600, height: 800, path: 'late', ownedPath: 'late' }); await pending
  assert.deepEqual(removed, ['compressed', 'late'])
})

test('实际导出读取前校验字节上限并清理本次导出文件', async () => {
  const media = require('../../CookWx/utils/avatarMedia')
  const previous = global.wx, removed = [], sizes = [], read = []
  let length = config.AVATAR_MAX_BYTES + 1
  const filesystem = {
    stat: options => options.success({ stats: { size: length } }),
    readFile: options => { read.push(options.filePath); options.success({ data: new ArrayBuffer(20) }) },
    unlink: options => removed.push(options.filePath)
  }
  global.wx = { getFileSystemManager: () => filesystem, canvasToTempFilePath: options => { sizes.push([options.width, options.height, options.destWidth, options.destHeight]); options.success({ tempFilePath: 'export.jpg' }) } }
  try {
    await assert.rejects(media.exportBytes({}, {}), /过大/)
    assert.deepEqual(read, [])
    length = 20
    assert.equal((await media.exportBytes({}, {})).byteLength, 20)
    assert.deepEqual(sizes[0], [512, 512, 512, 512])
    assert.deepEqual(removed, ['export.jpg', 'export.jpg'])
  } finally { global.wx = previous }
})