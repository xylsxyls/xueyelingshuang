const test = require('node:test')
const assert = require('node:assert/strict')
const flush = () => new Promise(resolve => setImmediate(resolve))

function complete(operation, ok = true) {
  const result = { errMsg: ok ? 'ok' : 'controlled failure' }
  if (ok && operation.success) operation.success(result)
  if (!ok && operation.fail) operation.fail(result)
  if (operation.complete) operation.complete(result)
}

const { fixture } = require('./fixtures/cookPage')

test('开始计时后返回必须等原生提醒关闭；连续点击只导航一次', async () => {
  const { page, calls } = fixture('cook')
  page.toggleRunning()
  complete(calls.enable[0])
  page.backMenu()
  page.backMenu()
  await flush()
  assert.equal(calls.routes.length, 0, '关闭提醒之前不能切页')
  assert.equal(calls.disable.length, 1)
  complete(calls.disable[0])
  await flush()
  assert.equal(calls.routes.length, 1)
  assert.equal(calls.routes[0].url, '/pages/menu/menu')
  assert.equal(calls.timers.size, 0)
})

test('快速开始后返回，晚到的开启提醒回调不能遗留在菜单', async () => {
  const { page, calls } = fixture('cook')
  page.toggleRunning()
  page.backMenu()
  await flush()
  assert.equal(calls.routes.length, 0)
  complete(calls.enable[0])
  await flush()
  assert.equal(calls.disable.length, 1, '开启回调晚到仍必须关闭')
  complete(calls.disable[0])
  await flush()
  assert.equal(calls.routes.length, 1)
})

test('关闭原生提醒失败保留流程并允许返回重试', async () => {
  const { page, calls } = fixture('cook')
  page.toggleRunning()
  complete(calls.enable[0])
  page.backMenu()
  await flush()
  complete(calls.disable[0], false)
  await flush()
  assert.equal(calls.routes.length, 0)
  assert.ok(page.runtime, '失败时不能提前清掉做菜流程')
  page.backMenu()
  await flush()
  assert.equal(calls.disable.length, 2)
  complete(calls.disable[1])
  await flush()
  assert.equal(calls.routes.length, 1)
})

test('系统卸载后晚到的渲染回调不能重建计时器或播报', async () => {
  const { page, calls } = fixture('cook', { deferCallbacks: true })
  page.toggleRunning()
  complete(calls.enable[0])
  page.onUnload()
  if (calls.disable.length) complete(calls.disable[0])
  await flush()
  if (calls.disable.length) complete(calls.disable[calls.disable.length - 1])
  while (calls.callbacks.length) calls.callbacks.shift()()
  assert.equal(calls.timers.size, 0)
  assert.deepEqual(calls.speaks.filter(value => value !== 'stop'), [])
  assert.equal(calls.routes.length, 0, '卸载生命周期不发起新的导航')
})

test('从计划进入计时页替换当前页，系统返回不经过二次跳转', async () => {
  const { page, calls } = fixture('plan')
  page.startCook()
  await flush()
  assert.equal(calls.routes.length, 1)
  assert.equal(calls.routes[0].type, 'redirectTo', '菜单→计划→计时的返回栈应为菜单→计时')
  assert.equal(calls.routes[0].url, '/pages/cook/cook')
})

test('返回点单后即使目录请求失败，旧选择和预览复位且搜索加减可用', async () => {
  const { page, app, calls, recipe } = fixture('menu', { catalogFails: true })
  page.data.recipes = [recipe]
  page.data.categories = ['家常菜']
  page.data.selectedMap = { recipe: true }
  page.data.selectedRecipes = [recipe]
  page.data.plan = { summary: { totalSeconds: 30 } }
  page.data.previewing = true
  page.data.cartOpen = true
  app.globalData.selectedRecipeIds = []
  page.onShow()
  await flush()
  assert.equal(page.data.previewing, false)
  assert.equal(page.data.cartOpen, false)
  assert.equal(page.data.selectedRecipes.length, 0)
  assert.ok(!page.data.plan.summary)
  page.selectCategory({ currentTarget: { dataset: { name: '家常菜' } } })
  page.onSearch({ detail: { value: '测试菜' } })
  assert.equal(page.data.filteredRecipes.length, 1)
  page.addRecipe({ currentTarget: { dataset: { id: 'recipe' } } })
  assert.equal(page.data.selectedRecipes.length, 1)
  page.removeRecipe({ currentTarget: { dataset: { id: 'recipe' } } })
  assert.equal(page.data.selectedRecipes.length, 0)
  assert.equal(page.data.previewing, false)
  page.onUnload()
  assert.equal(calls.timers.size, 0)
})

test('原生提醒关闭无回调在期限内失败，随后可以重试', async () => {
  const { createCookExitAlert } = require('../../CookWx/utils/cookExitAlert')
  const operations = []
  const guard = createCookExitAlert({
    enableAlertBeforeUnload: options => complete(options),
    disableAlertBeforeUnload: options => operations.push(options)
  }, () => {}, 20)
  guard.enable('退出？')
  await assert.rejects(guard.disable(), /超时/)
  const retry = guard.disable()
  await flush()
  complete(operations[1])
  await retry
  assert.equal(operations.length, 2)
})

test('旧基础库缺少提醒接口可以退出；从未开始无需调用原生关闭', async () => {
  const { createCookExitAlert } = require('../../CookWx/utils/cookExitAlert')
  const guard = createCookExitAlert({}, () => {})
  guard.enable('退出？')
  await guard.disable()
  const { page, calls } = fixture('cook')
  page.backMenu()
  await flush()
  assert.equal(calls.enable.length, 0)
  assert.equal(calls.disable.length, 0)
  assert.equal(calls.routes.length, 1)
})

test('提醒开启超时后的晚到成功仍收回提醒，不重新开启', async () => {
  const { createCookExitAlert } = require('../../CookWx/utils/cookExitAlert')
  const operations = { enable: [], disable: [] }
  const guard = createCookExitAlert({
    enableAlertBeforeUnload: options => operations.enable.push(options),
    disableAlertBeforeUnload: options => { operations.disable.push(options); complete(options) }
  }, () => {}, 20)
  guard.enable('退出？')
  await guard.disable()
  assert.equal(operations.disable.length, 1)
  complete(operations.enable[0])
  await flush()
  assert.equal(operations.disable.length, 2)
  guard.enable('再次开启？')
  assert.equal(operations.enable.length, 1)
})

test('返回导航失败后可再次返回；不会重复清理或重新启动流程', async () => {
  const { page, calls } = fixture('cook')
  page.toggleRunning()
  complete(calls.enable[0])
  page.backMenu()
  await flush()
  complete(calls.disable[0])
  await flush()
  calls.routes[0].fail({ errMsg: 'controlled navigation failure' })
  page.toggleRunning()
  assert.equal(calls.timers.size, 0)
  page.backMenu()
  await flush()
  assert.equal(calls.routes.length, 2)
  assert.equal(calls.disable.length, 1)
  assert.equal(calls.speaks.filter(value => value === 'stop').length, 1)
})

test('上一次流程超时后的晚到回调不能关闭下一次流程的退出提醒', async () => {
  const { createCookExitAlert } = require('../../CookWx/utils/cookExitAlert')
  const operations = { enable: [], disable: [] }
  const platform = {
    enableAlertBeforeUnload: options => operations.enable.push(options),
    disableAlertBeforeUnload: options => { operations.disable.push(options); complete(options) }
  }
  const old = createCookExitAlert(platform, () => {}, 20)
  old.enable('旧流程')
  await old.disable()
  const next = createCookExitAlert(platform, () => {}, 100)
  next.enable('新流程')
  complete(operations.enable[1])
  complete(operations.enable[0])
  await flush()
  assert.equal(operations.disable.length, 1, '旧回调不能操作新流程的原生提醒')
  await next.disable()
  assert.equal(operations.disable.length, 2)
})