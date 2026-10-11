const test = require('node:test')
const assert = require('node:assert/strict')
const fs = require('node:fs')
const path = require('node:path')
const vm = require('node:vm')
const Renderer = require('../../CookWx/utils/videoFrameRenderer')

function webgl() {
  const gl = {
    getShaderParameter: () => true, getProgramParameter: () => true,
    createShader: () => ({}), createProgram: () => ({}), createTexture: () => ({}), createBuffer: () => ({}),
    shaderSource() {}, compileShader() {}, attachShader() {}, linkProgram() {}, deleteShader() {},
    bindTexture() {}, texParameteri() {}, bindBuffer() {}, bufferData() {}
  }
  return gl
}

test('可用WebGL必须通过真实渲染器和组件Canvas准备，不误报基础库不支持', async () => {
  const gl = webgl()
  const canvas = { getContext: kind => kind === 'webgl' ? gl : null }
  let definition
  const playerRoot = path.resolve(__dirname, '../../CookWx/components/cachedVideoPlayer')
  vm.runInNewContext(fs.readFileSync(path.join(playerRoot, 'cachedVideoPlayer.js'), 'utf8'), {
    Component: value => { definition = value },
    require: name => name.includes('videoFrameRenderer') ? Renderer : {},
    console, setTimeout, clearTimeout
  })
  const component = {
    ...definition.methods,
    createSelectorQuery: () => ({ select: () => ({ fields: () => ({ exec: callback => callback([{ node: canvas }]) }) }) })
  }
  await component.createCanvas()
  assert.equal(component._renderer.supported, true)
  assert.equal(component._renderer.gl, gl)
  assert.equal(component._canvas, canvas)
})

test('无WebGL但有2D时渲染器仍可用，两种上下文均缺失时明确不可用', () => {
  const context = {}
  const renderer2d = new Renderer({ getContext: kind => kind === '2d' ? context : null })
  assert.equal(renderer2d.supported, true)
  assert.equal(renderer2d.context2d, context)
  const unavailable = new Renderer({ getContext: () => null })
  assert.equal(unavailable.supported, false)
})

test('首页必须记录组件传出的播放失败原因，兼容原生errMsg', () => {
  let definition
  const warnings = []
  const toasts = []
  const source = fs.readFileSync(path.resolve(__dirname, '../../CookWx/pages/home/home.js'), 'utf8')
  vm.runInNewContext(source, {
    Page: value => { definition = value },
    require: name => name.includes('debugLog') ? { warn: (...args) => warnings.push(args) } : {},
    wx: { showToast: value => toasts.push(value) }, console
  })
  const page = { data: { currentIndex: 0, feed: [{ videoId: 'video' }] }, clearWatchTimer() {} }
  for (const detail of [{ message: '视频解码操作超时：start' }, { errMsg: 'downloadFile:fail timeout' }]) {
    definition.onVideoError.call(page, { currentTarget: { dataset: { index: 0 } }, detail })
    assert.equal(warnings[warnings.length - 1][2].errMsg, detail.message || detail.errMsg)
  }
  assert.equal(toasts.length, 2)
})

function standalonePage() {
  let definition
  vm.runInNewContext(fs.readFileSync(path.resolve(__dirname, '../../CookWx/pages/videoPlayer/videoPlayer.js'), 'utf8'), {
    Page: value => { definition = value }, console, require: name => require(path.resolve(__dirname, '../../CookWx/pages/videoPlayer', name))
  })
  const page = { ...definition, data: { ...definition.data } }
  page.setData = values => { Object.assign(page.data, values) }
  return page
}

test('独立播放页必须解码导航参数中的完整视频地址、封面和中文标题', () => {
  const page = standalonePage()
  const source = 'http://127.0.0.1:5207/api/videos/file?videoId=video-1&version=1'
  const poster = 'http://127.0.0.1:5207/poster.jpg'
  page.onLoad({ videoId: 'video-1', src: encodeURIComponent(source), poster: encodeURIComponent(poster), title: encodeURIComponent('第一道菜') })
  assert.equal(page.data.videoUrl, source)
  assert.equal(page.data.posterUrl, poster)
  assert.equal(page.data.title, '第一道菜')
})

test('独立播放页遇到缺失或非法百分号参数时不崩溃', () => {
  const page = standalonePage()
  assert.doesNotThrow(() => page.onLoad({ src: '%', title: '%E4' }))
  assert.equal(page.data.videoUrl, '%')
  assert.equal(page.data.title, '%E4')
  page.onLoad({})
  assert.equal(page.data.videoUrl, '')
})

test('点击暂停时同步到达的pause事件不能把目标状态反转为播放', () => {
  const page = standalonePage()
  const calls = []
  page.player = () => ({
    pause() { calls.push('pause'); page.onPause() },
    play() { calls.push('play'); page.onPlay() }
  })
  page.togglePlayback()
  assert.equal(page.data.paused, true)
  page.togglePlayback()
  assert.equal(page.data.paused, false)
  assert.deepEqual(calls, ['pause', 'play'])
})