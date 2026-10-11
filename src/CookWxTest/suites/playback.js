const { freshAccount, videoPage, realFrame, until, sleep, assert } = require('./helpers')

async function frameCount(context) { return context.simulator.mini.evaluate(() => wx.__cookTestFrameCount || 0) }
async function newFrame(context, previous) { return until(async () => (await frameCount(context)) > previous, '本次解码帧/原生本地预览就绪', 50000, context.signal) }
async function confirmedSeek(context, seconds, after) {
  return until(() => context.simulator.mini.evaluate((target, started) => {
    const page = getCurrentPages().slice(-1)[0]
    const player = page && page.selectComponent('#standaloneVideoPlayer')
    return player && (wx.__cookTestSeeks || []).find(item => item.at >= started && item.generation === player._generation &&
      Math.abs(item.requested - target) < 0.001 && item.actual >= target - 0.05 && item.actual <= target + 0.5)
  }, seconds, after), '最新目标的真实定位完成事件', 50000, context.signal)
}
async function seed(context, video, start, duration) {
  return context.simulator.mini.evaluate(async (id, url, from, length) => {
    const entry = await getApp().__cookTest.cache.getSegment(id, url, from, length)
    return { path: entry.path, bytes: entry.bytes, start: entry.startSeconds, mediaDurationSeconds: entry.mediaDurationSeconds }
  }, video.videoId, video.url, start, duration)
}
async function seek(context, page, seconds) {
  const before = await frameCount(context)
  const started = Date.now()
  await (await context.simulator.element(page, 'slider')).slideTo(seconds)
  await confirmedSeek(context, seconds, started)
  await newFrame(context, before)
  return context.simulator.data(page, data => data.currentSeconds >= seconds && data.currentSeconds < seconds + 3, '定位后真实进度')
}
async function noDownloads(context) {
  await context.simulator.mini.evaluate(() => { wx.__cookTestBlockedDownloads = 0 })
  await context.simulator.mock('downloadFile', function(options) {
    wx.__cookTestBlockedDownloads = (wx.__cookTestBlockedDownloads || 0) + 1
    options.fail({ errMsg: 'downloadFile:fail controlled offline' })
    return { abort() {}, onHeadersReceived() {}, offHeadersReceived() {} }
  })
}

module.exports = {
  27: async context => {
    const user = await freshAccount(context)
    const video = await videoPage(context, user.token, Date.now().toString(36))
    const actual = await realFrame(context, video.page, 1)
    assert.ok(await frameCount(context) > 0)
    const observations = await context.simulator.mini.evaluate(id => (wx.__cookTestFrames || []).filter(item => item.videoId === id), video.videoId)
    const actualFrames = observations.filter(item => item.backend === 'decoder').length
    return { videoId: video.videoId, duration: actual.durationSeconds, actualFrames,
      localPreviewReady: observations.filter(item => item.backend === 'local-preview').length, realDecoder: actualFrames > 0,
      incomplete: actualFrames ? '' : '本平台仅验证原生本地预览；VideoDecoder/WebGL逐帧解码未执行' }
  },
  28: async context => {
    const user = await freshAccount(context)
    const video = await videoPage(context, user.token, Date.now().toString(36))
    await realFrame(context, video.page, 7)
    const requests = (await context.simulator.mediaNetwork()).slice(-4)
    const segments = requests.map(item => ({ start: Number(new URL(item.url).searchParams.get('startSeconds')), duration: Number(new URL(item.url).searchParams.get('durationSeconds')), startedAt: item.startedAt }))
    const zero = segments.findIndex(item => item.start === 0)
    assert.ok(zero >= 0)
    assert.deepEqual(segments.slice(zero, zero + 3).map(item => [item.start, item.duration]), [[0, 3], [3, 5], [8, 5]])
    assert.ok(segments[zero + 1].startedAt - segments[zero].startedAt >= 700, '首段尚未到触发点不能立即下载后段')
    await context.simulator.tap(video.page, '#standaloneVideoPlayer')
    return { requested: segments, configuredRule: [3, 1, 5, 2] }
  },
  29: async context => {
    const user = await freshAccount(context)
    const video = await videoPage(context, user.token, Date.now().toString(36))
    await realFrame(context, video.page)
    await seed(context, video, 3, 5)
    await context.simulator.tap(video.page, '#standaloneVideoPlayer')
    await noDownloads(context)
    await seek(context, video.page, 4)
    await sleep(400, context.signal)
    assert.equal(await context.simulator.mini.evaluate(() => wx.__cookTestBlockedDownloads), 0, '命中缓存不能调用媒体下载')
    await context.simulator.tap(video.page, '#standaloneVideoPlayer')
    return { cachedSeekActualFrame: true, downloadsWhileOffline: 0 }
  },
  30: async context => {
    const user = await freshAccount(context)
    const video = await videoPage(context, user.token, Date.now().toString(36))
    await realFrame(context, video.page)
    await seed(context, video, 3, 5)
    await context.simulator.tap(video.page, '#standaloneVideoPlayer')
    const before = (await context.simulator.mediaNetwork()).length
    await seek(context, video.page, 15)
    await context.simulator.tap(video.page, '#standaloneVideoPlayer')
    const fetched = (await context.simulator.mediaNetwork()).slice(before)
    assert.ok(fetched.some(item => new URL(item.url).searchParams.get('startSeconds') === '13'))
    assert.ok(!fetched.some(item => new URL(item.url).searchParams.get('startSeconds') === '8'), '定位15秒不能补下载跳过的8秒段')
    const afterSeek = (await context.simulator.mediaNetwork()).length
    await seek(context, video.page, 5)
    assert.equal((await context.simulator.mediaNetwork()).length, afterSeek, '回到5秒先命中缓存')
    await realFrame(context, video.page, 6.5)
    await until(async () => (await context.simulator.mediaNetwork()).slice(afterSeek).some(item => new URL(item.url).searchParams.get('startSeconds') === '8'), '到缺口前预取8至13秒', 15000, context.signal)
    return { jumpedToSegment: 13, cachedReturn: 5, gapFetchedAt: 8 }
  },
  31: async context => {
    const user = await freshAccount(context)
    const video = await videoPage(context, user.token, Date.now().toString(36))
    await realFrame(context, video.page)
    const slider = await context.simulator.element(video.page, 'slider')
    const before = await frameCount(context)
    const started = Date.now()
    await slider.slideTo(15)
    await slider.slideTo(5)
    await slider.slideTo(1)
    const completion = await confirmedSeek(context, 1, started)
    await newFrame(context, before)
    await context.simulator.data(video.page, data => data.currentSeconds >= 1 && data.currentSeconds < 4, '最后定位请求生效')
    await context.simulator.mini.navigateTo('/pages/voice/voice')
    await sleep(500, context.signal)
    const count = await frameCount(context)
    await sleep(700, context.signal)
    assert.equal(await frameCount(context), count, '页面隐藏后不能继续渲染旧帧')
    await context.simulator.mini.navigateBack()
    await newFrame(context, count)
    return { latestSeek: 1, completion, hiddenRenderingStopped: true, resumedActualFrame: true }
  },
  32: async context => {
    const user = await freshAccount(context)
    await noDownloads(context)
    const video = await videoPage(context, user.token, Date.now().toString(36))
    const component = await context.simulator.element(video.page, '#standaloneVideoPlayer')
    await until(async () => String(await component.data('status')).includes('offline'), '真实失败状态可观察', 15000, context.signal)
    assert.equal(await context.simulator.mini.evaluate(() => wx.__cookTestBlockedDownloads), 1, '不能无限重试下载')
    await context.simulator.restoreMocks()
    const before = await frameCount(context)
    await context.simulator.tap(video.page, '#standaloneVideoPlayer')
    await context.simulator.tap(video.page, '#standaloneVideoPlayer')
    await newFrame(context, before)
    await realFrame(context, video.page)
    return { controlledFailure: true, restoredNetworkActualFrame: true }
  },
  33: async context => {
    const user = await freshAccount(context)
    const version = Date.now().toString(36)
    let video = await videoPage(context, user.token, version)
    await realFrame(context, video.page)
    const entry = await seed(context, video, 3, 5)
    await context.simulator.page('/pages/voice/voice')
    await noDownloads(context)
    const previous = await frameCount(context)
    video = await videoPage(context, user.token, version)
    await newFrame(context, previous)
    const info = await context.simulator.mini.evaluate(() => {
      const cache = getApp().__cookTest.cache
      const stored = wx.getStorageSync('cook_video_segment_cache_v1') || {}
      const entries = Object.keys(stored).map(key => stored[key])
      const directory = wx.env.USER_DATA_PATH + '/cook-video-cache'
      const fs = wx.getFileSystemManager()
      const files = fs.readdirSync(directory).map(name => ({ name, bytes: fs.statSync(directory + '/' + name).size }))
      return { bytes: entries.reduce((sum, item) => sum + item.bytes, 0), diskBytes: files.reduce((sum, item) => sum + item.bytes, 0),
        partFiles: files.filter(item => item.name.endsWith('.part')).length,
        existingFiles: entries.filter(item => cache.fileExists(item.path)).length, entries: entries.length, pinned: Object.keys(cache.pinnedPaths).length }
    })
    assert.ok(info.bytes <= 100 * 1024 * 1024)
    assert.ok(info.diskBytes <= 100 * 1024 * 1024)
    assert.equal(info.partFiles, 0)
    assert.equal(info.existingFiles, info.entries)
    assert.ok(info.pinned >= 1)
    assert.equal(await context.simulator.mini.evaluate(() => wx.__cookTestBlockedDownloads), 0)
    return { persistedFile: entry.path, ...info, incomplete: '本项仅重开页面，完整进程重启与模拟器满容量未覆盖；LRU边界另有Node契约测试' }
  },
  35: async context => {
    const user = await freshAccount(context)
    const latencies = []
    for (let index = 0; index < context.config.stressLoops; index += 1) {
      const start = Date.now()
      const video = await videoPage(context, user.token, 'stress')
      await realFrame(context, video.page)
      await seek(context, video.page, index % 2 ? 1 : 5)
      await context.simulator.page('/pages/menu/menu', true)
      assert.equal((await context.simulator.mini.currentPage()).path, 'pages/menu/menu')
      latencies.push(Date.now() - start)
    }
    return { loops: latencies.length, minMs: Math.min(...latencies), maxMs: Math.max(...latencies), meanMs: latencies.reduce((sum, value) => sum + value, 0) / latencies.length, latencies }
  }
}