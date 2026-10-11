const { freshAccount, until, assert } = require('./helpers')

module.exports = {
  50: async context => {
    const { simulator: sim, http } = context
    const user = await freshAccount(context)
    const png = Buffer.from([137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,1,0,0,0,1,8,2,0,0,0,144,119,83,222,0,0,0,12,73,68,65,84,120,156,99,104,104,104,0,0,3,4,1,129,75,211,210,16,0,0,0,0,73,69,78,68,174,66,96,130])
    const initial = await http.json('/api/users/avatar/upload', { token: user.token, body: png, headers: { 'Content-Type': 'application/octet-stream' } })
    const photo = await sim.mini.evaluate(url => new Promise((resolve, reject) => {
      wx.downloadFile({ url, success: response => response.statusCode === 200 ? resolve(response.tempFilePath) : reject(new Error('头像素材下载失败')), fail: reject })
    }), 'http://127.0.0.1:' + context.config.serverPort + initial.profile.avatarUrl)
    await sim.mock('chooseMedia', { type: 'image', tempFiles: [{ tempFilePath: photo, fileType: 'image', size: png.length }] })
    const mine = await sim.page('/pages/mine/mine', true)
    await sim.data(mine, data => data.account.userId === user.account.userId, '头像入口真实账号')
    await sim.tap(mine, '[bindtap="editAvatar"]')
    const page = await until(async () => { const current = await sim.mini.currentPage(); return current.path === 'pages/avatarCrop/avatarCrop' && current }, '独立裁剪界面', 15000, context.signal)
    const loaded = await sim.data(page, data => data.ready && !data.loading, '实际Canvas图片解码')
    const box = await sim.element(page, '.crop-box')
    await box.trigger('touchstart', { touches: [{ clientX: 30, clientY: 50 }] })
    await box.trigger('touchmove', { touches: [{ clientX: 45, clientY: 70 }] })
    await box.trigger('touchend', { touches: [] })
    const moved = await page.data()
    assert.equal(moved.crop.size, loaded.crop.size)
    await sim.tap(page, '.save-button')
    const returned = await until(async () => { const current = await sim.mini.currentPage(); return current.path === 'pages/mine/mine' && current }, '保存真实头像返回', 30000, context.signal)
    const profile = (await http.json('/api/users/profile?userId=' + user.account.userId, { token: user.token })).profile
    assert.notEqual(profile.avatarUrl, initial.profile.avatarUrl)
    const bytes = await http.request(profile.avatarUrl)
    assert.equal(bytes.status, 200)
    assert.equal(bytes.data[0], 255); assert.equal(bytes.data[1], 216)
    assert.ok(bytes.data.length > 0 && bytes.data.length <= 512 * 1024)
    await sim.data(returned, data => data.profile.avatarUrl.endsWith(profile.avatarUrl), '本人页重新加载真实头像')
    return { actualCanvasExportAndUpload: true, bytes: bytes.data.length,
      nativeChooser: '官方替身；Canvas解码、裁剪导出和服务器存储实际执行', incomplete: '相册/拍照方向与真实双指触摸仍需手机验收' }
  }
}