const test = require('node:test')
const assert = require('node:assert/strict')
const path = require('node:path')
const fs = require('node:fs')
const vm = require('node:vm')
const assets = require('../../CookWx/utils/recipeAssets')
const voice = require('../../CookWx/utils/cookVoiceText')
const root = path.resolve(__dirname, '../../CookWx')
const flush = () => new Promise(resolve => setImmediate(resolve))

function fixture(route, apiOverrides = {}) {
  let page
  const app = { globalData: { userId: 'self', selectedRecipeIds: ['meal'] } }
  const calls = { routes: [], toasts: [], modals: [], purchase: [], joins: [], remove: [] }
  const api = {
    isStaleError: error => error && error.stale,
    joinRecipe: async id => { calls.joins.push(id); return { ok: true } },
    purchaseRecipe: async id => { calls.purchase.push(id); return { ok: true } },
    removeMenuRecipe: async id => { calls.remove.push(id); return { ok: true } },
    getRecipes: async () => ({ ok: true, recipes: [] }),
    getRecipeDetail: async () => ({ ok: true, recipe: { id: 'paid', priceType: 'coin', priceAmount: 18, owned: false } }),
    previewPlan: async () => ({ ok: true, summary: {} }),
    ...apiOverrides
  }
  const wx = {
    showToast: value => calls.toasts.push(value), showModal: value => calls.modals.push(value),
    navigateTo: value => calls.routes.push(value), navigateBack: value => calls.routes.push(value), switchTab: value => calls.routes.push(value)
  }
  const modules = new Map()
  function moduleFrom(file) {
    if (file === path.join(root, 'utils/api.js')) return api
    if (modules.has(file)) return modules.get(file).exports
    const module = { exports: {} }; modules.set(file, module)
    vm.runInNewContext(fs.readFileSync(file, 'utf8'), {
      module, exports: module.exports, wx, getApp: () => app, console, setTimeout, clearTimeout,
      require: name => {
        const target = path.resolve(path.dirname(file), name) + '.js'
        if (target === path.join(root, 'utils/api.js')) return api
        if (target.endsWith('utils/debugLog.js')) return { info() {}, warn() {}, error() {} }
        if (target.endsWith('utils/tabBar.js')) return { showHome() {} }
        if (target.endsWith('utils/cookPersonalizationSync.js')) return { showPending() {} }
        return moduleFrom(target)
      }
    }, { filename: file })
    return module.exports
  }
  const filename = path.join(root, 'pages', route, route + '.js')
  vm.runInNewContext(fs.readFileSync(filename, 'utf8'), {
    Page: value => { page = value }, wx, getApp: () => app, console, setTimeout, clearTimeout,
    require: name => moduleFrom(path.resolve(path.dirname(filename), name) + '.js')
  }, { filename })
  page.data = JSON.parse(JSON.stringify(page.data)); page.pageActive = true
  page.setData = function(values, callback) {
    Object.entries(values).forEach(([key, value]) => {
      const names = key.replace(/\[(\d+)\]/g, '.$1').split('.'); let target = this.data
      names.slice(0, -1).forEach(name => { target = target[name] })
      target[names[names.length - 1]] = value
    })
    if (callback) callback()
  }
  return { page, app, calls, api }
}

test('正方形封面优先，失败退到视频封面，旧图片错误不得清掉新封面', () => {
  const recipe = assets.normalize({ id: 'r', coverImageUrl: '/image', posterUrl: '/poster', category: '荤菜' })
  assert.ok(recipe.thumbnailUrl.endsWith('/image'))
  const fallback = assets.thumbnailFallback(recipe, recipe.thumbnailUrl)
  assert.equal(fallback.thumbnailUrl, fallback.posterUrl)
  assert.equal(assets.thumbnailFallback(fallback, recipe.thumbnailUrl).thumbnailUrl, fallback.posterUrl)
  assert.equal(assets.thumbnailFallback(fallback, fallback.posterUrl).thumbnailUrl, '')
})

test('系统付费菜不因systemRecipe获得制作权限；买按钮和免费移除分开', () => {
  const paid = assets.normalize({ id: 'paid', systemRecipe: true, inMenu: true, priceType: 'coin', priceAmount: 18 })
  assert.equal(paid.accessible, false); assert.equal(paid.canBuy, true); assert.equal(paid.canRemove, false)
  const owned = assets.normalize({ purchased: true, owned: true, inMenu: true, priceAmount: 18, priceType: 'coin' })
  assert.equal(owned.canBuy, false); assert.equal(owned.canRemove, false)
  assert.equal(assets.normalize({ inMenu: true, priceType: 'free' }).canRemove, true)
})

test('菜单五类连续展示，点击左侧不丢其他类别，滚动改变选中类别', () => {
  const { page } = fixture('menu')
  page.data.recipes = assets.CATEGORIES.map((category, index) => ({ id: String(index), category, title: category }))
  page.data.selectedMap = {}
  page.applyFilter()
  assert.equal(page.data.menuSections.length, 5)
  assert.equal(page.data.filteredRecipes.length, 5)
  page.selectCategory({ currentTarget: { dataset: { name: '汤饮' } } })
  assert.equal(page.data.scrollTarget, 'menu-section-3')
  assert.equal(page.data.filteredRecipes.length, 5)
  page.sectionOffsets = [0, 100, 200, 300, 400]
  page.onRecipeScroll({ detail: { scrollTop: 210 } })
  assert.equal(page.data.activeCategory, '蔬菜')
  page.onRecipeScrollBottom(); assert.equal(page.data.activeCategory, '主食')
})

test('我的菜单排除官方免费、自己的免费创作和未加入菜，付费未买仍展示且最新在上', () => {
  const { page } = fixture('mine')
  const recipes = [
    { id: 'default', systemRecipe: true, priceType: 'free', inMenu: true },
    { id: 'my-free', authorUserId: 'self', priceType: 'free', inMenu: true, menuPosition: 0 },
    { id: 'absent', priceAmount: 18, priceType: 'coin', owned: true, inMenu: false },
    { id: 'paid', title: '付费菜', inMenu: true, priceAmount: 18, priceType: 'coin', menuPosition: 1 },
    { id: 'free', title: '免费菜', inMenu: true, priceType: 'free', menuPosition: 0 }
  ]
  const groups = page.buildAssetGroups({ userId: 'self' }, recipes, [], [], [])
  assert.deepEqual(Array.from(groups.menu, item => item.recipeId), ['free', 'paid'])
  assert.equal(groups.menu[0].canRemove, true); assert.equal(groups.menu[1].canBuy, true)
  page.data.assetGroups = groups; page.refreshAssetItems()
  assert.deepEqual(Array.from(page.data.assetTabs, item => item.key), ['liked', 'favorite', 'created', 'menu', 'personal'])
  assert.ok(page.data.assetTabs.every(item => !Object.hasOwn(item, 'count')))
})

test('加入未购买菜不扣费也不加入本顿做菜选择；重复点击只请求一次', async () => {
  let resolve
  const { page, app, calls } = fixture('recipe', { joinRecipe: id => { calls.joins.push(id); return new Promise(done => { resolve = done }) } })
  page.data.recipe = assets.normalize({ id: 'paid', inMenu: false, priceAmount: 18, priceType: 'coin' })
  const work = page.addToMenu(); page.addToMenu()
  assert.deepEqual(calls.joins, ['paid']); assert.equal(calls.purchase.length, 0)
  resolve({ ok: true }); await work
  assert.equal(page.data.recipe.inMenu, true)
  assert.deepEqual(app.globalData.selectedRecipeIds, ['meal'])
})

test('取消购买或确认前离开不会扣费；确认后只调用购买接口', async () => {
  const { page, calls } = fixture('recipe')
  page.data.recipe = assets.normalize({ id: 'paid', title: '付费菜', priceAmount: 18, priceType: 'coin' })
  let work = page.purchase(); calls.modals[0].success({ confirm: false }); await work
  assert.equal(calls.purchase.length, 0)
  work = page.purchase(); page.onHide(); calls.modals[1].success({ confirm: true }); await work
  assert.equal(calls.purchase.length, 0)
  page.pageActive = true; page.setData({ buying: false })
  work = page.purchase(); calls.modals[2].success({ confirm: true }); await work; await flush()
  assert.deepEqual(calls.purchase, ['paid']); assert.equal(calls.joins.length, 0)
})

test('移出免费菜需确认，取消保留菜单和本顿选择', async () => {
  const { page, app, calls } = fixture('mine')
  page.data.assetItems = [{ id: 'recipe_meal', recipeId: 'meal', title: '免费菜', canRemove: true }]
  const work = page.removeMenuAsset({ currentTarget: { dataset: { index: 0 } } })
  calls.modals[0].success({ confirm: false }); await work
  assert.equal(calls.remove.length, 0); assert.deepEqual(app.globalData.selectedRecipeIds, ['meal'])
})

test('详情异步响应在退出后不能恢复页面；免费和付费菜单项均能看公开摘要', async () => {
  let resolve
  const { page } = fixture('recipe', { getRecipeDetail: () => new Promise(done => { resolve = done }) })
  page.data.recipeId = 'late'; page.loadRecipe(); page.onHide()
  resolve({ ok: true, recipe: { id: 'late' } }); await flush()
  assert.equal(page.data.recipe, null)
  const menu = fixture('menu')
  menu.page.data.recipes = [{ id: 'paid', accessible: false, priceAmount: 18 }]
  menu.page.openRecipeFromCard({ currentTarget: { dataset: { id: 'paid' } } })
  assert.equal(menu.calls.routes[0].url, '/pages/recipe/recipe?id=paid&selection=meal')
})

test('菜谱原视频入口保留视频身份和URL，跳转复用缓存播放器', () => {
  const { page, calls } = fixture('recipe')
  page.data.recipe = assets.normalize({ id: 'recipe', videoId: 'original', videoUrl: '/api/videos/file?videoId=original', posterUrl: '/poster' })
  page.openVideo()
  assert.ok(calls.routes[0].url.startsWith('/pages/videoPlayer/videoPlayer?videoId=original&src='))
  assert.equal(new URLSearchParams(calls.routes[0].url.split('?')[1]).get('src'), page.data.recipe.videoUrl)
})

test('短步骤只播标题，解释与预计时间不混入，Unicode名称最多八字', () => {
  assert.equal(voice.buildTaskVoiceText({ title: '鸡腿煎至熟透', detail: '这里是长说明', detailText: '预计五分钟。', durationSeconds: 300 }), '鸡腿煎至熟透')
  assert.equal(voice.buildTaskVoiceText({ title: '一二三四五六七八九十', detail: '解释' }), '一二三四五六七八')
  assert.equal(Array.from(voice.buildTaskVoiceText({ title: '😀😀😀😀😀😀😀😀😀' })).length, 8)
})

test('公开主页仅支持创作和喜欢，切换时旧网络结果不得覆盖当前类别', async () => {
  let resolve
  const { page } = fixture('profile', {
    getUserProfile: async () => ({ ok: true, profile: { userId: 'peer', nickname: '作者' } }),
    getUserFeed: () => new Promise(done => { resolve = done })
  })
  page.data.userId = 'peer'; page.load(false)
  page.switchTab({ currentTarget: { dataset: { key: 'favorite' } } })
  assert.equal(page.data.activeTab, 'created')
  const oldResolve = resolve
  page.switchTab({ currentTarget: { dataset: { key: 'liked' } } })
  oldResolve({ ok: true, items: [{ videoId: 'old' }] }); await flush()
  assert.equal(page.data.items.length, 0)
  resolve({ ok: true, items: [{ videoId: 'liked' }], hasMore: false }); await flush()
  assert.equal(page.data.items[0].videoId, 'liked'); page.onUnload()
})

test('私信列表双向历史按身份标记，阅读只对当前会话提交', async () => {
  const reads = []
  const { page } = fixture('conversation', {
    getConversation: async () => ({ ok: true, profile: { userId: 'peer', following: true }, messages: [{ id: 'a', fromUserId: 'self', text: '发出' }, { id: 'b', fromUserId: 'peer', text: '收到' }] }),
    readConversation: async id => { reads.push(id); return { ok: true } }
  })
  page.data.userId = 'peer'; await page.load(false)
  assert.deepEqual(Array.from(page.data.messages, item => item.outgoing), [true, false])
  assert.deepEqual(reads, ['peer']); page.onUnload()
})

test('旧私信轮询晚到不得覆盖已发送消息', async () => {
  const resolves = []
  const { page } = fixture('conversation', {
    getConversation: () => new Promise(done => resolves.push(done)),
    sendMessage: async () => ({ ok: true, messageItem: { id: 'sent', fromUserId: 'self', text: '刚发出的' } }),
    readConversation: async () => ({ ok: true })
  })
  page.data.userId = 'peer'; page.data.profile = { following: true }; page.data.text = '刚发出的'
  const old = page.load(false)
  await page.send()
  assert.equal(page.data.messages[0].id, 'sent')
  resolves[0]({ ok: true, profile: { following: true }, messages: [] }); await old
  assert.equal(page.data.messages[0].id, 'sent')
  resolves[1]({ ok: true, profile: { following: true }, messages: [{ id: 'sent', fromUserId: 'self', text: '刚发出的' }] }); await flush()
  assert.equal(page.data.messages.length, 1); page.onUnload()
})

test('退出再返回后旧发送回调不能清空新草稿，失败入口仍可重试', async () => {
  let resolve
  const { page } = fixture('conversation', { sendMessage: () => new Promise(done => { resolve = done }) })
  page.data.profile = { following: true }; page.data.text = '旧消息'
  const work = page.send(); page.onHide(); page.pageActive = true; page.data.text = '新草稿'
  resolve({ ok: true, messageItem: { id: 'old' } }); await work
  assert.equal(page.data.text, '新草稿'); assert.equal(page.data.messages.length, 0)
  let older = null; page.load = value => { older = value }; page.retry(); assert.equal(older, false)
})

test('我的菜单购买使用真实菜谱编号，资产行编号不得发给服务端', async () => {
  const { page, calls } = fixture('mine')
  page.load = () => {}
  page.data.assetItems = [{ id: 'recipe_paid', recipeId: 'paid', title: '付费菜', priceAmount: 18, priceType: 'coin', canBuy: true }]
  const work = page.buyAsset({ currentTarget: { dataset: { index: 0 } } })
  calls.modals[0].success({ confirm: true }); await work
  assert.deepEqual(calls.purchase, ['paid']); assert.equal(calls.joins.length, 0)
  assert.equal(page.data.menuActionId, '')
})

test('确认移出使用真实菜谱编号，并清除本顿已选菜，重复点击不重复请求', async () => {
  const { page, app, calls } = fixture('mine')
  let reloads = 0; page.load = () => { reloads++ }
  page.data.assetItems = [{ id: 'recipe_meal', recipeId: 'meal', title: '免费菜', canRemove: true }]
  const event = { currentTarget: { dataset: { index: 0 } } }
  const work = page.removeMenuAsset(event); page.removeMenuAsset(event)
  assert.equal(calls.modals.length, 1)
  calls.modals[0].success({ confirm: true }); await work
  assert.deepEqual(calls.remove, ['meal']); assert.deepEqual(app.globalData.selectedRecipeIds, [])
  assert.equal(reloads, 1); assert.equal(page.data.menuActionId, '')
})