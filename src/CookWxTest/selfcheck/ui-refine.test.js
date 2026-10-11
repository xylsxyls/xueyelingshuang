const test = require('node:test')
const assert = require('node:assert/strict')
const fs = require('node:fs')
const path = require('node:path')
const vm = require('node:vm')
const flow = require('../../CookWx/utils/personalizationFlow')
const viewport = require('../../CookWx/utils/menuViewport')
const presentation = require('../../CookWx/utils/videoPresentation')
const flush = () => new Promise(resolve => setImmediate(resolve))

test('所有登记的模拟器用例都有实际执行入口', () => {
  const { catalog } = require('../lib/catalog'), suites = require('../suites')
  for (const item of catalog) assert.equal(typeof suites[item.id], 'function', 'Missing case ' + item.id)
})

function fixture(route, api = {}) {
  const root = path.resolve(__dirname, '../../CookWx'), cache = new Map(), calls = { routes: [], toasts: [] }
  const app = { globalData: { userId: 'self', selectedRecipeIds: [] } }
  const wx = { navigateTo: options => calls.routes.push(options), switchTab: options => calls.routes.push(options), showToast: options => calls.toasts.push(options), showModal: options => { calls.modal = options } }
  function load(file) {
    if (file === path.join(root, 'utils/api.js')) return api
    if (file.endsWith('utils/debugLog.js')) return { info() {}, warn() {}, error() {} }
    if (file.endsWith('utils/tabBar.js')) return { showHome() {} }
    if (file.endsWith('utils/cookPersonalizationSync.js')) return { showPending() {} }
    if (cache.has(file)) return cache.get(file).exports
    const module = { exports: {} }; cache.set(file, module)
    vm.runInNewContext(fs.readFileSync(file, 'utf8'), { module, exports: module.exports, wx, getApp: () => app, console, setTimeout, clearTimeout, require: name => load(path.resolve(path.dirname(file), name) + '.js') }, { filename: file })
    return module.exports
  }
  let page
  const file = path.join(root, 'pages', route + '.js')
  vm.runInNewContext(fs.readFileSync(file, 'utf8'), { Page: value => { page = value }, wx, getApp: () => app, console, setTimeout, clearTimeout, require: name => load(path.resolve(path.dirname(file), name) + '.js') }, { filename: file })
  page.data = JSON.parse(JSON.stringify(page.data)); page.pageActive = true
  page.setData = (values, callback) => { Object.entries(values).forEach(([key, value]) => { const parts = key.replace(/\[(\d+)\]/g, '.$1').split('.'); let target = page.data; parts.slice(0, -1).forEach(name => { target = target[name] }); target[parts.at(-1)] = value }); if (callback) callback() }
  return { page, calls, wx, app }
}

test('简介输入20字、保存返回后更新，拒绝保存保留编辑草稿', async () => {
  let saved, ok = true
  const { page } = fixture('mine/mine', { updateIntroduction: async value => { saved = value; return { ok, profile: { introduction: value }, message: 'failed' } } })
  page.editIntroduction(); assert.equal(page.data.introductionDraft, '开心做大厨')
  page.onIntroductionInput({ detail: { value: '厨'.repeat(21) } }); assert.equal(Array.from(page.data.introductionDraft).length, 20)
  await page.saveIntroduction(); assert.equal(saved, '厨'.repeat(20)); assert.equal(page.data.profile.introduction, saved); assert.equal(page.data.editingIntroduction, false)
  page.editIntroduction(); ok = false; page.onIntroductionInput({ detail: { value: '新草稿' } }); await page.saveIntroduction()
  assert.equal(page.data.editingIntroduction, true); assert.equal(page.data.introductionDraft, '新草稿'); assert.equal(page.data.profile.introduction, '厨'.repeat(20))
})

test('简介保存晚返回不得覆盖重新打开后的新编辑状态，重复点击只提交一次', async () => {
  let resolve, requests = 0
  const { page } = fixture('mine/mine', { updateIntroduction: () => { requests++; return new Promise(done => { resolve = done }) } })
  page.pauseAdCountdown = () => {}
  page.editIntroduction(); page.data.introductionDraft = '旧提交'
  const work = page.saveIntroduction(); await page.saveIntroduction(); assert.equal(requests, 1)
  page.onHide(); page.pageActive = true; page.data.profile = { introduction: '重新加载' }; page.data.savingIntroduction = false
  page.editIntroduction(); resolve({ ok: true, profile: { introduction: '旧提交' } }); await work
  assert.equal(page.data.profile.introduction, '重新加载'); assert.equal(page.data.introductionDraft, '重新加载'); assert.equal(page.data.editingIntroduction, true)
})

test('旧账户加载响应不能覆盖已保存的新简介，其他账户数据仍正常加载', async () => {
  let resolve
  const { page } = fixture('mine/mine', { getAccount: () => new Promise(done => { resolve = done }), getRecipes: async () => ({ ok: true, recipes: [] }), getFeed: async () => ({ ok: true, feed: [] }), updateIntroduction: async introduction => ({ ok: true, profile: { introduction } }) })
  page.load(); page.data.introductionDraft = '新简介'; await page.saveIntroduction()
  resolve({ ok: true, account: { userId: 'self', coins: 42 }, profile: { introduction: '旧简介' } }); await flush()
  assert.equal(page.data.profile.introduction, '新简介'); assert.equal(page.data.account.coins, 42); assert.equal(page.data.loading, false)
})

const base = { id: 'r', tasks: [{ id: 'r.a', shortId: 'a', title: '洗菜', durationSeconds: 10, active: true }, { id: 'r.b', shortId: 'b', title: '下锅', durationSeconds: 20, active: true }] }
test('带点的合法短步骤ID不与其他同后缀步骤混淆', () => {
  const recipe = { id: 'r', tasks: [{ id: 'r.prep.a', shortId: 'prep.a', durationSeconds: 10 }, { id: 'r.a', shortId: 'a', durationSeconds: 20 }] }
  const steps = flow.merge(recipe, { taskOrder: ['prep.a', 'a'], durationOverrides: [{ taskId: 'r.prep.a', durationSeconds: 12 }] }, String)
  assert.deepEqual(steps.map(flow.key), ['prep.a', 'a']); assert.equal(steps[0].seconds, 12); assert.equal(steps[1].seconds, 20)
  assert.deepEqual(flow.payload('r', steps).taskOrder, ['prep.a', 'a'])
})
test('完整顺序合并、保存和重开不丢新增步骤位置或时长', () => {
  const config = { addedTasks: [{ id: 'head', title: '备锅', durationSeconds: 5 }, { id: 'middle', title: '检查', durationSeconds: 6 }, { id: 'tail', title: '装盘', durationSeconds: 7 }], taskOrder: ['head', 'a', 'middle', 'b', 'tail'], durationOverrides: [{ taskId: 'r.a', durationSeconds: 12 }], disabledTaskIds: ['b'] }
  const steps = flow.merge(base, config, String)
  assert.deepEqual(steps.map(flow.key), config.taskOrder); assert.deepEqual(steps.map(item => item.index), [1, 2, 3, 4, 5]); assert.equal(steps[1].seconds, 12); assert.equal(steps[3].disabled, true)
  const payload = flow.payload('r', steps)
  const reopened = flow.merge(base, payload, String)
  assert.deepEqual(reopened.map(flow.key), config.taskOrder); assert.equal(reopened[1].seconds, 12); assert.equal(reopened[3].disabled, true)
  assert.ok(payload.addedTasks.every(item => item.voiceText === item.title && !Object.hasOwn(item, 'index')))
})

test('新增步骤立即插入原流程，返回保持草稿、连续编号且只可删除新增项', () => {
  const { page, calls } = fixture('personalization/personalization')
  page.data.steps = flow.merge(base, {}, String); page.data.recipeId = 'r'; page._loadedOnce = true
  page.addStep()
  const handlers = {}, emissions = []
  calls.routes[0].success({ eventChannel: { on: (name, callback) => { handlers[name] = callback }, emit: (name, value) => emissions.push([name, value]) } })
  assert.equal(emissions[0][1].steps.length, 2)
  page.onHide()
  handlers.taskCreated({ id: 'new', title: '检查', durationSeconds: 6, insertRelativeTo: 'b', insertPosition: 'before' })
  page.onShow()
  assert.deepEqual(Array.from(page.data.steps, flow.key), ['a', 'new', 'b']); assert.deepEqual(Array.from(page.data.steps, item => item.index), [1, 2, 3])
  page.removeAddedTask({ currentTarget: { dataset: { index: 0 } } }); assert.equal(page.data.steps.length, 3)
  page.removeAddedTask({ currentTarget: { dataset: { index: 1 } } }); assert.deepEqual(Array.from(page.data.steps, flow.key), ['a', 'b']); assert.equal(page.data.steps[1].index, 2)
})

test('编辑器将第几步之前之后的选择带回真实事件通道', () => {
  const { page, wx } = fixture('personalization/taskEditor')
  const handlers = {}, events = []
  page.getOpenerEventChannel = () => ({ on: (name, callback) => { handlers[name] = callback }, emit: (name, value) => events.push([name, value]) })
  wx.navigateBack = () => {}
  page.onLoad(); handlers.configureInsertion({ steps: [{ key: 'a', label: '1. 洗菜' }, { key: 'b', label: '2. 下锅' }] })
  page.onInsertionStepChange({ detail: { value: 0 } }); page.onInsertionPositionChange({ detail: { value: 0 } })
  page.data.title = '检查'; page.data.seconds = 5; page.save()
  assert.equal(events[0][0], 'taskCreated'); assert.equal(events[0][1].insertRelativeTo, 'a'); assert.equal(events[0][1].insertPosition, 'before')
})

test('个性化保存成功后解除忙状态，重置的旧响应不覆盖重新打开页面', async () => {
  let resetResolve
  const { page, calls } = fixture('personalization/personalization', { savePersonalization: async () => ({ ok: true }), getPersonalizationDetail: async () => ({ ok: true, recipe: base }), resetPersonalization: () => new Promise(done => { resetResolve = done }) })
  page.data.recipeId = 'r'; page.load(); await flush()
  await page.save(); await flush(); assert.equal(page.data.saving, false)
  page.reset(); const work = calls.modal.success({ confirm: true })
  page.onHide(); page.pageActive = true; page.data.steps = [{ id: 'new-draft' }]; page.data.saving = false
  resetResolve({ ok: true }); await work; assert.equal(page.data.steps[0].id, 'new-draft')
})

test('五类全部显示时左栏固定，超过视口只做保持当前类可见的最小滚动', () => {
  assert.equal(viewport.ensureVisible({ top: 300, bottom: 375 }, 500, 40, 375), 0)
  assert.equal(viewport.ensureVisible({ top: 400, bottom: 450 }, 250, 100, 600), 200)
  assert.equal(viewport.ensureVisible({ top: 100, bottom: 150 }, 250, 200, 600), 100)
  assert.equal(viewport.ensureVisible({ top: 200, bottom: 250 }, 250, 100, 600), 100)
})

test('右侧内容底部留白覆盖浮动做菜栏高度，右侧切类不把五类左栏滚走', () => {
  const { page } = fixture('menu/menu')
  page.data.activeCategory = '主食'; page.categoryCurrentScrollTop = 100
  const result = [{ top: 100, bottom: 600 }, { scrollTop: 200 }, [{ top: -100 }, { top: 0 }, { top: 100 }, { top: 200 }, { top: 300 }], { top: 100, height: 500 }, { scrollTop: 0 }, [0, 1, 2, 3, 4].map(i => ({ top: 100 + i * 50, bottom: 150 + i * 50 })), { top: 470 }]
  page.createSelectorQuery = () => { const query = { select: () => query, selectAll: () => query, boundingClientRect: () => query, scrollOffset: () => query, exec: callback => callback(result) }; return query }
  page.measureSections(); assert.equal(page.data.recipeBottomSpace, 146); assert.equal(page.categoryCurrentScrollTop, 0)
  page.onRecipeScroll({ detail: { scrollTop: 210 } }); assert.equal(page.data.activeCategory, '蔬菜'); assert.equal(page.categoryCurrentScrollTop, 0)
})

test('作者图标与关注状态统一，会员状态只接受服务端布尔值', () => {
  const item = presentation.fromServer({ author: '系统厨友', authorUserId: 'system', authorFollowing: true, likes: 17, liked: true, favorite: true, inMenu: false })
  assert.equal(item.authorInitial, '系'); assert.equal(item.canFollowAuthor, false); assert.equal(item.likes, 17); assert.equal(item.menuText, '加入菜单')
  assert.equal(presentation.fromServer({ owned: true, inMenu: 'false' }).inMenu, false)
})

test('原视频点赞采用服务端总数，旧元数据返回不得覆盖刚完成的互动', async () => {
  let resolve
  const { page } = fixture('videoPlayer/videoPlayer', { getVideoDetail: () => new Promise(done => { resolve = done }), toggleLike: async () => ({ ok: true, liked: true, likes: 28 }) })
  page.data.active = true; page.data.videoId = 'video'; page.data.videoUrl = 'decoder-source'; page.data.item = { liked: false, likes: 4 }
  const load = page.loadMetadata(); await page.toggleLike()
  resolve({ ok: true, item: { videoId: 'video', liked: false, likes: 3, videoUrl: 'other-source' } }); await load
  assert.equal(page.data.item.liked, true); assert.equal(page.data.item.likes, 28); assert.equal(page.data.videoUrl, 'decoder-source')
})

test('原视频底部导航实际跳转到原来的首页菜单我的页面', () => {
  let component
  const calls = []
  vm.runInNewContext(fs.readFileSync(path.resolve(__dirname, '../../CookWx/components/bottomNavigation/bottomNavigation.js'), 'utf8'), { Component: value => { component = value }, wx: { switchTab: value => calls.push(value) } })
  for (const index of [0, 1, 4]) component.methods.switchTab.call(component, { currentTarget: { dataset: { index } } })
  assert.deepEqual(calls.map(item => item.url), ['/pages/home/home', '/pages/menu/menu', '/pages/mine/mine'])
})

const mealRecipe = { id: 'rice', title: '米饭', priceType: 'free', inMenu: true, category: '主食', tasks: [] }
function mealDetail(api = {}, recipe = mealRecipe) {
  const result = fixture('recipe/recipe', { getRecipeDetail: async () => ({ ok: true, recipe }), ...api })
  result.page.onLoad({ id: recipe.id, selection: 'meal' })
  result.page.onShow()
  return result
}

test('底部菜单的各个详情入口明确使用本次做菜选择，浏览不自动选菜', () => {
  const { page, calls, app } = fixture('menu/menu')
  page.data.recipes = [mealRecipe]
  const event = { currentTarget: { dataset: { id: 'rice' } } }
  page.openRecipeFromCard(event); page.openRecipe(event)
  page.data.recipes = [{ id: 'locked', priceType: 'coin', priceAmount: 18 }]
  page.addRecipe({ currentTarget: { dataset: { id: 'locked' } } })
  assert.ok(calls.routes.every(item => item.url.includes('selection=meal')))
  assert.deepEqual(app.globalData.selectedRecipeIds, [])
})

test('长期菜单中的未选菜在详情可加入本次列表，重复点击不重复加入或发HTTP', async () => {
  let joins = 0
  const { page, app } = mealDetail({ joinRecipe: async () => { joins++; return { ok: true } } })
  app.globalData.selectedRecipeIds = ['other']; app.globalData.selectedRecipes = [{ id: 'other', title: '其他菜' }]
  app.globalData.lastPreview = { summary: { totalSeconds: 300 } }; app.globalData.lastPreviewRecipeKey = 'other'
  await flush()
  assert.equal(page.data.mealSelection, true); assert.equal(page.data.recipe.inMenu, true); assert.equal(page.data.selectedForMeal, false)
  await page.addToMenu(); await page.addToMenu()
  assert.deepEqual(Array.from(app.globalData.selectedRecipeIds), ['other', 'rice'])
  assert.deepEqual(Array.from(app.globalData.selectedRecipes, item => item.id), ['other', 'rice'])
  assert.equal(page.data.selectedForMeal, true); assert.equal(joins, 0); assert.equal(app.globalData.lastPreview, null); assert.equal(app.globalData.lastPreviewRecipeKey, '')
})

test('选菜页减去后再进入详情可重新加入，其他已选菜保留', async () => {
  const detail = mealDetail()
  detail.app.globalData.selectedRecipeIds = ['rice', 'other']; detail.app.globalData.selectedRecipes = [mealRecipe, { id: 'other' }]
  await flush(); assert.equal(detail.page.data.selectedForMeal, true)
  const { page: menuPage, app } = fixture('menu/menu'); app.globalData = detail.app.globalData
  menuPage.data.recipes = [mealRecipe, { id: 'other', priceType: 'free' }]; menuPage.data.selectedMap = { rice: true, other: true }
  menuPage.schedulePreview = () => {}
  menuPage.removeRecipe({ currentTarget: { dataset: { id: 'rice' } } })
  detail.page.onHide(); detail.page.onShow(); await flush()
  assert.equal(detail.page.data.selectedForMeal, false); assert.deepEqual(Array.from(app.globalData.selectedRecipeIds), ['other'])
  await detail.page.addToMenu(); assert.deepEqual(Array.from(app.globalData.selectedRecipeIds), ['other', 'rice'])
})

test('展开本顿菜单连续减菜，同步选择和卡片、删完收起，晚预览不能恢复旧摘要', { timeout: 2000 }, async context => {
  let resolvePreview, notifyPreview
  const requests = [], started = new Promise(resolve => { notifyPreview = resolve })
  const { page, app, calls } = fixture('menu/menu', {
    previewPlan: (ids, options) => {
      requests.push({ ids: Array.from(ids), options })
      return new Promise(resolve => { resolvePreview = resolve; notifyPreview() })
    },
    isStaleError: () => false
  })
  context.after(() => page.onHide())
  const soup = { id: 'soup', title: '蔬菜汤', priceType: 'free', inMenu: true, category: '汤饮' }
  page.data.recipes = [mealRecipe, soup].map(item => Object.assign({}, item, { selected: true }))
  page.data.selectedMap = { rice: true, soup: true }; page.data.selectedRecipes = page.data.recipes.slice()
  page.data.plan = { summary: { totalSeconds: 300 } }; page.data.planRecipeKey = 'rice|soup'
  page.timeBlocksRecipeKey = 'rice|soup'; page.timeBlocksPlan = { timeSegments: [{}], freeBlocks: [] }
  Object.assign(app.globalData, { selectedRecipeIds: ['rice', 'soup'], selectedRecipes: page.data.recipes.slice(), lastPreview: page.data.plan, lastPreviewRecipeKey: 'rice|soup' })
  page.toggleCart(); assert.equal(page.data.cartOpen, true)
  page.removeRecipe({ currentTarget: { dataset: { id: 'rice' } } })
  assert.deepEqual(Array.from(app.globalData.selectedRecipeIds), ['soup'])
  assert.deepEqual(Array.from(page.data.selectedRecipes, item => item.id), ['soup'])
  assert.equal(page.data.recipes[0].selected, false); assert.equal(page.data.recipes[1].selected, true)
  assert.equal(page.data.cartOpen, true); assert.equal(page.data.previewing, true); assert.equal(page.data.plan.summary, undefined)
  assert.equal(app.globalData.lastPreview, null); assert.equal(page.timeBlocksPlan, null)
  await started
  assert.deepEqual(requests[0].ids, ['soup']); assert.equal(requests[0].options.isStale(), false)
  page.removeRecipe({ currentTarget: { dataset: { id: 'soup' } } })
  assert.deepEqual(Array.from(app.globalData.selectedRecipeIds), []); assert.equal(app.globalData.selectedRecipes.length, 0)
  assert.equal(page.data.selectedRecipes.length, 0); assert.equal(page.data.cartOpen, false); assert.equal(page.data.previewing, false)
  assert.equal(requests[0].options.isStale(), true)
  resolvePreview({ ok: true, summary: { totalSeconds: 200 } }); await flush()
  assert.equal(page.data.plan.summary, undefined); assert.equal(page.data.planRecipeKey, '')
  assert.equal(page.data.cartOpen, false); assert.equal(page.timeBlocksPlan, null); assert.equal(requests.length, 1)
  assert.ok(page.data.recipes.every(item => item.inMenu && !item.selected)); assert.equal(calls.toasts.length, 0)
  page.removeRecipe({ currentTarget: { dataset: { id: 'soup' } } })
  assert.equal(page.data.selectedRecipes.length, 0); assert.equal(page.data.cartOpen, false)
})

test('普通详情加入仍只写长期菜单，失败可重试且不改变本次做菜选择', async () => {
  let ok = false, joins = 0
  const { page, app } = fixture('recipe/recipe', { joinRecipe: async () => { joins++; return { ok, message: '网络失败' } } })
  page.onLoad({ id: 'paid' }); page.data.recipe = page.normalizeRecipe({ id: 'paid', priceType: 'coin', priceAmount: 18, inMenu: false })
  app.globalData.selectedRecipeIds = ['other']
  await page.addToMenu(); assert.equal(page.data.recipe.inMenu, false); assert.equal(page.data.joining, false)
  ok = true; await page.addToMenu(); assert.equal(page.data.recipe.inMenu, true); assert.equal(joins, 2)
  assert.deepEqual(app.globalData.selectedRecipeIds, ['other']); assert.equal(page.data.selectedForMeal, false)
})

test('未购买的付费菜和隐藏页面不能加入本次列表，也不触发购买或长期加入', async () => {
  let requests = 0
  const { page, app } = mealDetail({ joinRecipe: async () => { requests++; return { ok: true } }, purchaseRecipe: async () => { requests++; return { ok: true } } }, { id: 'locked', priceType: 'coin', priceAmount: 18, inMenu: true })
  await flush(); await page.addToMenu(); assert.deepEqual(app.globalData.selectedRecipeIds, []); assert.equal(requests, 0)
  page.data.recipe = page.normalizeRecipe(mealRecipe); page.onHide(); await page.addToMenu()
  assert.deepEqual(app.globalData.selectedRecipeIds, []); assert.equal(requests, 0)
})

test('旧菜谱读取或加入响应不能反转新选择和重新打开后的页面状态', async () => {
  let resolve
  const { page, app } = mealDetail({ getRecipeDetail: () => new Promise(done => { resolve = done }) })
  page.data.recipe = page.normalizeRecipe(mealRecipe)
  await page.addToMenu(); resolve({ ok: true, recipe: mealRecipe }); await flush()
  assert.equal(page.data.selectedForMeal, true); assert.deepEqual(Array.from(app.globalData.selectedRecipeIds), ['rice'])
  const other = fixture('recipe/recipe', { joinRecipe: () => new Promise(done => { resolve = done }) })
  other.page.data.recipe = other.page.normalizeRecipe({ id: 'paid', priceType: 'coin', priceAmount: 18, inMenu: false })
  const pending = other.page.addToMenu(); other.page.onHide(); other.page.pageActive = true
  other.page.data.recipe = other.page.normalizeRecipe({ id: 'new', inMenu: false })
  resolve({ ok: true }); await pending; assert.equal(other.page.data.recipe.inMenu, false)
})

test('system、中文和Unicode作者在视频、本人及公开主页使用同一首字和头像地址', async () => {
  for (const nickname of ['system', '系统厨友', '👨‍🍳厨友']) {
    const raw = { userId: 'system', nickname, avatarUrl: '/api/users/avatar?id=system' }
    const video = presentation.fromServer({ authorUserId: 'system', author: nickname, authorAvatarUrl: raw.avatarUrl })
    const publicPage = fixture('profile/profile', { getUserProfile: async () => ({ ok: true, profile: raw }), getUserFeed: async () => ({ ok: true, items: [] }) }).page
    publicPage.data.userId = 'system'; await publicPage.load(false)
    const mine = fixture('mine/mine', { getAccount: async () => ({ ok: true, account: raw, profile: raw }), getRecipes: async () => ({ ok: true, recipes: [] }), getFeed: async () => ({ ok: true, feed: [] }) }).page
    mine.load(); await flush()
    assert.equal(publicPage.data.profile.avatarInitial, video.authorInitial); assert.equal(mine.data.profile.avatarInitial, video.authorInitial)
    assert.equal(publicPage.data.profile.avatarUrl, video.authorAvatarUrl); assert.equal(mine.data.profile.avatarUrl, video.authorAvatarUrl)
    assert.equal(publicPage.data.profile.avatarInitial, Array.from(nickname)[0])
  }
})