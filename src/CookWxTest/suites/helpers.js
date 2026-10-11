const assert = require('node:assert/strict')
const { until, sleep } = require('../lib/wait')

async function freshAccount(context) {
  const { simulator: sim, http, signal } = context
  sim.signal = signal
  await sim.reset()
  const account = 'cw_' + Date.now().toString(36) + '_' + Math.random().toString(36).slice(2, 7)
  const page = await sim.page('/pages/auth/login/login')
  await sim.tap(page, '.ghost-btn')
  await sim.input(page, 'input[placeholder="账号"]', account)
  await sim.input(page, 'input[placeholder="密码"]', 'CookWxTest2026!')
  await sim.input(page, 'input[placeholder="昵称"]', '测试厨友')
  await sim.tap(page, '.primary-btn')
  await until(async () => { const current = await sim.mini.currentPage(); return current && current.path === 'pages/home/home' }, '注册后首页', 20000, signal)
  const token = await sim.token()
  assert.ok(token, '真实注册必须产生 token')
  const session = await http.json('/api/auth/session', { token })
  assert.equal(session.account.account, account)
  return { token, account: session.account, loginName: account }
}

async function account(context, token) { return (await context.http.json('/api/account', { token })).account }

async function recipes(context, token) { return (await context.http.json('/api/recipes', { token })).recipes }

async function ownedRecipe(context, token, paid = false) {
  const list = paid ? (await context.http.json('/api/recipes?scope=catalog', { token })).recipes : await recipes(context, token)
  const recipe = paid ? list.find(item => !item.owned && Number(item.priceAmount || item.priceCoins || 0) > 0) : list.find(item => item.owned)
  assert.ok(recipe, paid ? '测试目录必须有未拥有付费菜谱' : '测试目录必须有已拥有菜谱')
  return recipe
}

async function recipePage(context, recipeId) {
  const page = await context.simulator.page('/pages/recipe/recipe?id=' + encodeURIComponent(recipeId))
  await context.simulator.data(page, data => data.recipe && !data.loading, '菜谱详情实际加载')
  return page
}

async function menu(context) {
  const page = await context.simulator.page('/pages/menu/menu', true)
  await context.simulator.data(page, data => data.recipes && data.recipes.length > 0, '菜单实际加载')
  return page
}

async function pickRecipe(context, page, recipe) {
  await context.simulator.tap(page, `.cat[data-name="${recipe.category}"]`)
  await context.simulator.tap(page, `.plus[data-id="${recipe.id}"]`)
  return context.simulator.data(page, data => data.selectedRecipes.some(item => item.id === recipe.id) && data.plan.summary && !data.previewing, '选菜实际预览')
}

function verifyPlan(plan) {
  assert.ok(plan && plan.summary && Array.isArray(plan.timeline) && plan.timeline.length > 0, '完整时间线必需')
  const byId = new Map(plan.timeline.map(item => [item.id, item]))
  assert.equal(byId.size, plan.timeline.length, '步骤 ID 不重复')
  let activeEnd = 0
  const resourceEnd = new Map()
  for (const task of [...plan.timeline].sort((a, b) => a.startSeconds - b.startSeconds)) {
    assert.ok(task.endSeconds >= task.startSeconds && task.startSeconds >= 0)
    for (const dependency of task.dependencies || []) {
      const parent = byId.get(dependency)
      assert.ok(parent, `依赖必须存在 ${dependency}`)
      assert.ok(parent.endSeconds <= task.startSeconds, '依赖完成后开始')
    }
    if (task.active) { assert.ok(task.startSeconds >= activeEnd, '前台步骤不得重叠'); activeEnd = task.endSeconds }
    for (const resource of task.resources || (task.resource ? [task.resource] : [])) {
      const key = typeof resource === 'string' ? resource : resource.name
      assert.ok(task.startSeconds >= (resourceEnd.get(key) || 0), '独占资源不得重叠')
      resourceEnd.set(key, task.endSeconds)
    }
    if (task.safetyLevel === 'danger' || task.safetyLevel === 'attention') assert.equal(task.canLeaveKitchen, false)
  }
  assert.equal(plan.summary.totalSeconds, Math.max(...plan.timeline.map(item => item.endSeconds)))
}

async function cookPage(context, token) {
  const recipe = await ownedRecipe(context, token)
  const page = await menu(context)
  await pickRecipe(context, page, recipe)
  await context.simulator.tap(page, '.cart .primary-btn')
  const plan = await until(async () => { const current = await context.simulator.mini.currentPage(); return current && current.path === 'pages/plan/plan' && current }, '进入计划页', 15000, context.signal)
  await context.simulator.tap(plan, '.actions .primary-btn')
  return until(async () => { const current = await context.simulator.mini.currentPage(); return current && current.path === 'pages/cook/cook' && current }, '真实开始进入做菜', 40000, context.signal)
}

async function feed(context, token) {
  const result = await context.http.json('/api/feed?count=20&mode=recommend', { token })
  assert.ok(result.feed && result.feed.length)
  return result.feed[0]
}

async function videoPage(context, token, suffix = '') {
  const item = await feed(context, token)
  const videoId = String(item.videoId || item.id || '').replace(/^feed_/, '')
  const url = `http://127.0.0.1:${context.config.serverPort}/api/videos/file?videoId=${encodeURIComponent(videoId)}${suffix ? '&version=' + suffix : ''}`
  const page = await context.simulator.page(`/pages/videoPlayer/videoPlayer?videoId=${encodeURIComponent(videoId)}&src=${encodeURIComponent(url)}`)
  return { page, videoId, url }
}

async function realFrame(context, page, seconds = 0) {
  const data = await context.simulator.data(page, item => item.frameReady && item.durationSeconds > 0 && item.currentSeconds >= seconds, '真实解码帧或原生本地预览就绪与进度', 50000)
  const component = await context.simulator.element(page, '#standaloneVideoPlayer')
  assert.equal(await component.data('status'), '', '渲染不能处于失败状态')
  return data
}

async function secondaryAccount(context) {
  return context.http.json('/api/auth/register', { body: { account: 'peer_' + Date.now().toString(36) + '_' + Math.random().toString(36).slice(2, 6), password: 'CookWxPeer2026!', nickname: '第二位测试厨友' } })
}

module.exports = { freshAccount, account, recipes, ownedRecipe, recipePage, menu, pickRecipe, verifyPlan, cookPage, feed, videoPage, realFrame, secondaryAccount, until, sleep, assert }