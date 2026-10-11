const test = require('node:test')
const assert = require('node:assert/strict')
const { fixture } = require('./fixtures/cookPage')
const runtime = require('../../CookWx/utils/planRuntime')
const flush = () => new Promise(resolve => setImmediate(resolve))

function fixtureSync() {
  const file = require.resolve('../../CookWx/utils/cookPersonalizationSync')
  delete require.cache[file]
  const sync = require(file)
  const app = { globalData: { userId: 'owner', pendingCookSyncReports: [] } }
  const calls = { ticks: [], modals: [], writes: [], toasts: [] }
  const platform = {
    nextTick: callback => calls.ticks.push(callback),
    showModal: value => calls.modals.push(value), showToast: value => calls.toasts.push(value)
  }
  const api = { syncPersonalizationTimes: (items, options) => { calls.writes.push({ items, options }); return Promise.resolve({ ok: true }) } }
  const clock = { timeline: [{ id: 'done', recipeId: 'recipe', endSeconds: 5 }, { id: 'future', recipeId: 'recipe', endSeconds: 10 }], actualTimeMap: { done: 5 }, completedTaskIds: {}, skippedTaskIds: {}, elapsedSeconds: 5 }
  return { sync, app, platform, api, calls, clock }
}

test('退出队列保存快照，菜单渲染后才询问，同一时刻只显示一份', () => {
  const { sync, app, platform, api, calls, clock } = fixtureSync()
  sync.enqueue(app, clock, 'owner')
  clock.actualTimeMap.done = 99
  assert.equal(sync.showPending(app, platform, api), true)
  assert.equal(calls.modals.length, 0)
  assert.equal(sync.showPending(app, platform, api), false)
  calls.ticks.shift()()
  assert.equal(calls.modals[0].title, '同步到个性化？')
  calls.modals[0].success({ confirm: false })
  assert.equal(calls.writes.length, 0)
  assert.equal(app.globalData.pendingCookSyncReports.length, 0)
})

test('重复确认只发一次同步；接口使用快照且检查账号归属', async () => {
  const { sync, app, platform, api, calls, clock } = fixtureSync()
  sync.enqueue(app, clock, 'owner')
  sync.showPending(app, platform, api)
  calls.ticks.shift()()
  calls.modals[0].success({ confirm: true })
  calls.modals[0].success({ confirm: true })
  clock.actualTimeMap.done = 99
  await flush()
  assert.equal(calls.writes.length, 1)
  assert.equal(calls.writes[0].items[0].durationOverrides[0].durationSeconds, 5)
  assert.equal(calls.writes[0].options.isStale(), false)
  app.globalData.userId = 'another'
  assert.equal(calls.writes[0].options.isStale(), true)
})

test('失败保留实际用时，重试成功；取消重试不写入且移除队列', async () => {
  for (const retryConfirm of [false, true]) {
    const { sync, app, platform, api, calls, clock } = fixtureSync()
    let attempts = 0
    api.syncPersonalizationTimes = items => {
      calls.writes.push(items)
      return ++attempts === 1 ? Promise.reject(new Error('offline')) : Promise.resolve({ ok: true })
    }
    sync.enqueue(app, clock, 'owner')
    sync.showPending(app, platform, api)
    calls.ticks.shift()()
    calls.modals[0].success({ confirm: true })
    await flush()
    assert.equal(calls.modals[1].title, '同步失败')
    assert.equal(app.globalData.pendingCookSyncReports.length, 1)
    calls.modals[1].success({ confirm: retryConfirm })
    if (retryConfirm) {
      calls.ticks.shift()()
      calls.modals[2].success({ confirm: true })
      await flush()
      assert.equal(calls.writes.length, 2)
    } else assert.equal(calls.writes.length, 1)
    assert.equal(app.globalData.pendingCookSyncReports.length, 0)
  }
})

test('渲染前页面隐藏不弹框，重回可见页可询问；请求失败晚到不在旧页弹框', async () => {
  const { sync, app, platform, api, calls, clock } = fixtureSync()
  let visible = true
  sync.enqueue(app, clock, 'owner')
  sync.showPending(app, platform, api, { isActive: () => visible })
  visible = false
  calls.ticks.shift()()
  assert.equal(calls.modals.length, 0)
  assert.equal(app.globalData.pendingCookSyncReports.length, 1)
  let reject
  api.syncPersonalizationTimes = () => new Promise((resolve, fail) => { reject = fail })
  visible = true
  sync.showPending(app, platform, api, { isActive: () => visible })
  calls.ticks.shift()()
  calls.modals[0].success({ confirm: true })
  await flush()
  visible = false
  reject(new Error('offline'))
  await flush()
  assert.equal(calls.modals.length, 1)
  assert.equal(app.globalData.pendingCookSyncReports.length, 1)
})

test('账号切换不得写入旧流程，零步骤仅提示不发送请求', async () => {
  const { sync, app, platform, api, calls, clock } = fixtureSync()
  sync.enqueue(app, clock, 'another')
  assert.equal(app.globalData.pendingCookSyncReports.length, 0)
  sync.enqueue(app, clock, 'owner')
  sync.showPending(app, platform, api)
  calls.ticks.shift()()
  app.globalData.userId = 'another'
  calls.modals[0].success({ confirm: true })
  await flush()
  assert.equal(calls.writes.length, 0)
  app.globalData.userId = 'owner'
  sync.enqueue(app, { ...clock, actualTimeMap: {} }, 'owner')
  sync.showPending(app, platform, api)
  calls.ticks.shift()()
  assert.equal(calls.modals[1].showCancel, false)
  calls.modals[1].success({ confirm: true })
  assert.equal(calls.writes.length, 0)
})

test('主动退出等待提醒关闭，退出途中记录实际完成任务；从未弹框阻塞导航', async () => {
  const timeline = [{ id: 'done', recipeId: 'recipe', startSeconds: 0, endSeconds: 2, active: true, taskMode: 'foreground', canPause: true }, { id: 'next', recipeId: 'recipe', startSeconds: 2, endSeconds: 30, active: true, taskMode: 'foreground', canPause: true }]
  const { page, app, calls } = fixture('cook', { plan: { timeline, summary: { totalSeconds: 30 }, cookOptions: { syncPersonalization: true } } })
  page.toggleRunning()
  calls.enable[0].success({})
  page.lastTickAt = Date.now() - 3000
  page.backMenu()
  await flush()
  assert.equal(calls.routes.length, 0)
  calls.disable[0].success({})
  await flush()
  assert.equal(calls.routes.length, 1)
  assert.equal(calls.modals.length, 0)
  assert.equal(app.globalData.pendingCookSyncReports[0].items[0].durationOverrides[0].durationSeconds, 2)
  assert.equal(page.runtime, null)
})