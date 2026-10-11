const { freshAccount, account, until, assert } = require('./helpers')

async function selectFixture(context, page) {
  const path = await context.simulator.mini.evaluate(async baseUrl => {
    return new Promise((resolve, reject) => wx.downloadFile({ url: baseUrl + '/api/videos/file?videoId=cook_000001', success: response => response.statusCode === 200 ? resolve(response.tempFilePath) : reject(new Error('测试素材下载失败')), fail: reject }))
  }, `http://127.0.0.1:${context.config.serverPort}`)
  const bytes = context.reporter.report.environment.fixture.bytes
  await context.simulator.mock('chooseMedia', { type: 'video', tempFiles: [{ tempFilePath: path, size: bytes, fileType: 'video' }] })
  await context.simulator.tap(page, '.media')
  await context.simulator.data(page, data => data.mediaPath === path && data.mediaFileSizeBytes === bytes, '原生选择结果进入真实页面')
  return { path, bytes }
}

module.exports = {
  25: async context => {
    const user = await freshAccount(context)
    const page = await context.simulator.page('/pages/publish/publish', true)
    const fixture = await selectFixture(context, page)
    const caption = '不关联菜谱的测试视频 ' + Date.now()
    await context.simulator.input(page, '.caption', caption)
    await context.simulator.mock('showModal', { confirm: true, cancel: false })
    await context.simulator.tap(page, '.publish-btn')
    await until(async () => (await context.simulator.network()).some(item => item.url.includes('/api/videos/publish') && item.status === 200), '真实上传发布请求完成', 55000, context.signal)
    await context.simulator.data(page, data => !data.publishing && !data.mediaPath, '发布成功后清理表单')
    const published = await context.http.json('/api/feed?mode=published&count=100', { token: user.token })
    const video = published.feed.find(item => item.description === caption || item.caption === caption || item.title === caption)
    assert.ok(video, '实际发布作品必须可查询')
    assert.equal((video.recipeIds || []).length, 0)
    const network = await context.simulator.network()
    const chunks = network.filter(item => item.url.includes('/api/videos/upload/chunk'))
    assert.ok(chunks.length > 0 && chunks.every(item => item.status === 200))
    assert.equal(chunks.length, Math.ceil(fixture.bytes / (512 * 1024)))
    return { actualBytes: fixture.bytes, chunks: chunks.length, videoId: video.videoId || video.id, nativeChooser: '官方替身；二进制和发布真实执行' }
  },
  26: async context => {
    const user = await freshAccount(context)
    const page = await context.simulator.page('/pages/publish/publish', true)
    await context.simulator.mock('chooseMedia', function(options) { options.fail({ errMsg: 'chooseMedia:fail cancel' }) })
    await context.simulator.tap(page, '.media')
    assert.equal((await page.data()).mediaPath, '')
    await context.simulator.tap(page, '.publish-btn')
    assert.equal((await context.simulator.network()).filter(item => item.url.includes('/api/videos/upload/start')).length, 0)
    const started = await context.http.json('/api/videos/upload/start', { token: user.token, body: { fileName: 'cancel.mp4', fileSizeBytes: 4, mediaType: 'video' } })
    await context.http.json('/api/videos/upload/cancel', { token: user.token, body: { uploadId: started.uploadId } })
    const complete = await context.http.request('/api/videos/upload/complete', { token: user.token, body: { uploadId: started.uploadId } })
    assert.equal(complete.json.ok, false)
    return { chooserCancelDoesNotUpload: true, canceledSessionCannotComplete: true, incomplete: '尚需真实在途上传取消检查' }
  }
}