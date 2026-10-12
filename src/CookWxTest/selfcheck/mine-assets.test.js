const test = require('node:test')
const assert = require('node:assert/strict')
const fs = require('node:fs')
const path = require('node:path')
const vm = require('node:vm')
const media = require('../../CookWx/utils/mineMediaAssets')
const flush = () => new Promise(resolve => setImmediate(resolve))
const video = (id, fields = {}) => ({ videoId: id, title: '糖醋排骨', videoUrl: '/video?id=' + id, posterUrl: '/poster?id=' + id, authorUserId: 'self', recipeIds: ['ribs'], ...fields })

function fixture(overrides = {}, pageSize = 3) {
  let page
  const requests = []
  const routes = []
  const api = {
    getAccount: async () => ({ ok: true, account: { userId: 'self', favoriteRecipeIds: ['ribs'] } }),
    getRecipes: async () => ({ ok: true, recipes: [{ id: 'ribs', title: '糖醋排骨', owned: true, inMenu: true, custom: true, authorUserId: 'peer', priceType: 'free', personalizationApplied: true }] }),
    getFeed: async options => { requests.push(options); return { ok: true, feed: [video('ribs-video')] } },
    isStaleError: error => error && error.stale,
    ...overrides
  }
  vm.runInNewContext(fs.readFileSync(path.join(__dirname, '../../CookWx/pages/mine/mine.js'), 'utf8'), {
    Page: definition => { page = definition },
    require: name => ({
      '../../utils/api': api,
      '../../utils/mineRecipeReviews': require('../../CookWx/utils/mineRecipeReviews'),
      '../../utils/privateMediaCache': require('../../CookWx/utils/privateMediaCache'),
      '../../utils/config': { MINE_MEDIA_PAGE_SIZE: pageSize },
      '../../utils/mineMediaAssets': media,
      '../../utils/creationAssets': require('../../CookWx/utils/creationAssets'),
      '../../utils/recipeAssets': require('../../CookWx/utils/recipeAssets'),
      '../../utils/userPresentation': require('../../CookWx/utils/userPresentation'),
      '../../utils/menuActions': require('../../CookWx/utils/menuActions'),
      '../../utils/time': { formatDurationText: String },
      '../../utils/debugLog': { info() {}, warn() {} },
      '../../utils/tabBar': { showHome() {} },
      '../../utils/cookPersonalizationSync': { showPending() {} }
    })[name],
    wx: { showToast() {}, navigateTo: options => routes.push(options) },
    getApp: () => ({ globalData: {} }), setTimeout, clearTimeout
  })
  page.data = JSON.parse(JSON.stringify(page.data))
  page.pageActive = true
  page.setData = function(values, callback) {
    for (const [key, value] of Object.entries(values)) {
      const parts = key.replace(/\[(\d+)\]/g, '.$1').split('.')
      let target = this.data
      for (let i = 0; i < parts.length - 1; i += 1) target = target[parts[i]]
      target[parts[parts.length - 1]] = value
    }
    if (callback) callback()
  }
  return { page, requests, routes }
}

test('收藏只取视频，菜谱与视频双收藏不生成重复卡片；同名不同视频保留', () => {
  const { page } = fixture()
  const groups = page.buildAssetGroups({ userId: 'self', favoriteRecipeIds: ['ribs'] }, [{ id: 'ribs', title: '糖醋排骨', owned: true, inMenu: true }], [], [video('ribs-video'), video('feed_ribs-video'), video('second-video')], [])
  assert.deepEqual(Array.from(groups.favorite, item => item.videoId), ['ribs-video', 'second-video'])
  assert.equal(groups.menu.length, 1)
  assert.equal(groups.menu[0].type, 'recipe')
})

test('创作仅显示本人视频，包含未关联菜谱作品；空身份不展示推荐作品', () => {
  const { page } = fixture()
  const feed = [video('own', { recipeIds: [] }), video('other', { authorUserId: 'peer' }), video('unknown', { authorUserId: '' })]
  assert.deepEqual(Array.from(page.buildAssetGroups({ userId: 'self' }, [], [], [], feed).created, item => item.videoId), ['own'])
  assert.equal(page.buildAssetGroups({}, [], [], [], feed).created.length, 0)
  assert.equal(media.mediaMode('__proto__'), '')
  assert.equal(media.toMediaAssets([null, {}]).length, 0)
})

test('我的页请求喜欢、收藏合集、本人作品；封面打开已有缓存播放器，菜单仍打开菜谱', async () => {
  const { page, requests, routes } = fixture()
  page.load()
  await flush()
  assert.deepEqual(requests.map(item => [item.mode, item.count]), [['liked', 3], ['collection', 3], ['created', 3]])
  assert.equal(page.data.assetItems.length, 1)
  assert.equal(page.data.mediaGrid, true)
  page.openAsset({ currentTarget: { dataset: { index: 0 } } })
  assert.match(routes[0].url, /^\/pages\/videoPlayer\/videoPlayer\?videoId=ribs-video&src=/)
  page.switchAssetTab({ currentTarget: { dataset: { key: 'menu' } } })
  assert.equal(page.data.mediaGrid, false)
  page.openAsset({ currentTarget: { dataset: { index: 0 } } })
  assert.equal(routes[1].url, '/pages/recipe/recipe?id=ribs')
  page.switchAssetTab({ currentTarget: { dataset: { key: 'personal' } } })
  page.openAsset({ currentTarget: { dataset: { index: 0 } } })
  assert.equal(routes[2].url, '/pages/personalization/personalization?id=ribs')
})

test('翻页排除已加载视频并去重，无新增结果停止请求；并发滚动只请求一次', async () => {
  let release
  let requests = 0
  let options
  const { page } = fixture({ getFeed: value => { requests += 1; options = value; return new Promise(resolve => { release = resolve }) } })
  const items = media.toMediaAssets([video('one'), video('two'), video('three')])
  page.data.assetGroups = { favorite: items }
  page.data.assetHasMore.favorite = true
  page.loadRequestId = 1
  const pending = page.loadMoreAssets(false)
  page.onReachBottom()
  assert.equal(requests, 1)
  assert.deepEqual(Array.from(options.excludeIds), ['one', 'two', 'three'])
  release({ ok: true, feed: [video('feed_one'), video('two'), video('three')] })
  await pending
  assert.equal(page.data.assetGroups.favorite.length, 3)
  assert.equal(page.data.assetHasMore.favorite, false)
  page.onReachBottom()
  assert.equal(requests, 1)
})

test('收藏请求晚到时更新对应分区，切换后不能把视频塞进菜单；隐藏后禁止更新', async () => {
  let release
  const { page } = fixture({ getFeed: () => new Promise(resolve => { release = resolve }) })
  page.data.assetGroups = { favorite: [], menu: [{ id: 'recipe_ribs', type: 'recipe' }] }
  page.data.assetHasMore.favorite = true
  page.loadRequestId = 1
  const pending = page.loadMoreAssets(false)
  page.switchAssetTab({ currentTarget: { dataset: { key: 'menu' } } })
  release({ ok: true, feed: [video('late')] })
  await pending
  assert.equal(page.data.assetItems[0].id, 'recipe_ribs')
  assert.equal(page.data.assetGroups.favorite[0].videoId, 'late')
  page.switchAssetTab({ currentTarget: { dataset: { key: 'favorite' } } })
  const hidden = page.loadMoreAssets(true)
  page.onHide()
  release({ ok: true, feed: [video('hidden')] })
  await hidden
  assert.equal(page.data.assetGroups.favorite[0].videoId, 'late')
})

test('封面失败显示占位；旧分区、旧URL的错误不会污染当前封面', () => {
  const { page } = fixture()
  const item = media.toMediaAssets([video('one')])[0]
  page.data.assetGroups = { favorite: [item] }
  page.refreshAssetItems()
  const event = fields => ({ currentTarget: { dataset: { index: 0, id: item.id, tab: 'favorite', url: item.posterUrl, ...fields } } })
  page.onAssetCoverError(event({ tab: 'liked' }))
  page.onAssetCoverError(event({ url: '/obsolete-cover' }))
  assert.equal(item.coverFailed, false)
  page.onAssetCoverError(event({}))
  assert.equal(item.coverFailed, true)
})

test('初次分区网络失败可重试，失败不误报空收藏；页大小限制控制流量', async () => {
  let failing = true
  const { page } = fixture({ getFeed: async () => failing ? Promise.reject(new Error('offline')) : { ok: true, feed: [video('recovered')] } }, 100000)
  assert.equal(page.mediaPageSize(), 60)
  page.load()
  await flush()
  assert.equal(page.data.assetErrors.favorite, true)
  failing = false
  await page.loadMoreAssets(true)
  assert.equal(page.data.assetErrors.favorite, false)
  assert.equal(page.data.assetItems[0].videoId, 'recovered')
})