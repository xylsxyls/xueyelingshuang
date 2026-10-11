const api = require('../../utils/api')
const assets = require('../../utils/recipeAssets')
const actions = require('../../utils/menuActions')
const selection = require('../../utils/mealSelection')
const viewport = require('../../utils/menuViewport')
const config = require('../../utils/config')
const debugLog = require('../../utils/debugLog')
const tabBar = require('../../utils/tabBar')
const personalizationSync = require('../../utils/cookPersonalizationSync')

Page({
  data: {
    categories: assets.CATEGORIES,
    menuSections: [],
    scrollTarget: '',
    categoryScrollTop: 0,
    recipeBottomSpace: 24,
    buyingRecipeId: '',
    activeCategory: '',
    recipes: [],
    categoryGroups: [],
    filteredRecipes: [],
    keyword: '',
    selectedMap: {},
    selectedRecipes: [],
    plan: {},
    planRecipeKey: '',
    previewError: '',
    cartOpen: false,
    previewing: false,
    starting: false
  },

  onShow() {
    tabBar.showHome()
    this.pageActive = true
    const selectedMap = {}
    ;(getApp().globalData.selectedRecipeIds || []).forEach((id) => { selectedMap[id] = true })
    this.setData({ starting: false, previewing: false, buyingRecipeId: '', cartOpen: false, selectedMap }, () => {
      this.refreshRecipeSelectionState()
      this.refreshSelected(false)
      if (!this.currentSelectedIds().length) this.schedulePreview([])
    })
    this.loadRecipes()
    personalizationSync.showPending(getApp(), wx, api, { isActive: () => this.pageActive !== false })
  },

  onHide() {
    this.invalidatePageRequests()
  },

  onUnload() {
    this.invalidatePageRequests()
  },

  invalidatePageRequests() {
    this.pageActive = false
    this.loadRecipesRequestId = (this.loadRecipesRequestId || 0) + 1
    this.previewRequestId = (this.previewRequestId || 0) + 1
    this.openCookRequestId = (this.openCookRequestId || 0) + 1
    this.resetTimeBlocksPlan()
    this.clearPreviewTimer()
  },

  loadRecipes() {
    debugLog.info('MENU', 'LOAD_RECIPES_START')
    this.loadRecipesRequestId = (this.loadRecipesRequestId || 0) + 1
    const requestId = this.loadRecipesRequestId
    const isStale = () => requestId !== this.loadRecipesRequestId || this.pageActive === false
    api.getRecipes({ isStale })
      .then((res) => {
        if (!res.ok) throw new Error(res.message)
        if (isStale()) {
          if (this.pageActive !== false) this.setData({ starting: false })
          return
        }
        const app = getApp()
        const selectedIds = app.globalData.selectedRecipeIds || []
        const selectedMap = {}
        selectedIds.forEach((id) => { selectedMap[id] = true })
        const recipes = this.decorateRecipes(res.recipes || [], selectedMap)
        Object.keys(selectedMap).forEach(id => { if (!recipes.some(item => item.id === id && item.accessible)) delete selectedMap[id] })
        app.globalData.selectedRecipeIds = Object.keys(selectedMap)
        const categoryGroups = this.decorateCategoryGroups(res.categoryGroups || [], selectedMap)
        const categories = assets.CATEGORIES.concat(Array.from(new Set(recipes.map(item => item.category))).filter(name => assets.CATEGORIES.indexOf(name) < 0))
        const activeCategory = this.chooseActiveCategory(categories, recipes)
        this.setData({
          categories,
          categoryGroups,
          recipes,
          activeCategory,
          selectedMap
        }, () => {
          this.refreshSelected()
          this.applyFilter()
        })
        debugLog.info('MENU', 'LOAD_RECIPES_OK', { requestId, recipeCount: recipes.length, selectedCount: selectedIds.length })
      })
      .catch((err) => {
        if (api.isStaleError(err) || isStale()) {
          debugLog.info('MENU', 'LOAD_RECIPES_STALE', { requestId })
          return
        }
        debugLog.error('MENU', 'LOAD_RECIPES_FAIL', { error: err && err.message ? err.message : String(err) })
        wx.showToast({ title: '服务器连接失败', icon: 'none' })
      })
  },

  decorateRecipes(recipes, selectedMap) {
    return recipes.map((item) => {
      const id = item.id || ''
      const priceCoins = Number(item.priceCoins || 0)
      const priceAmount = Number(item.priceAmount || priceCoins || 0)
      const priceType = item.priceType || (priceCoins > 0 ? 'coin' : 'free')
      const owned = !!item.owned
      const accessible = owned || priceType === 'free' || priceAmount <= 0
      const normalized = assets.normalize(item)
      const priceText = normalized.priceText
      return Object.assign({}, normalized, {
        id,
        title: item.title || '',
        subtitle: item.subtitle || '',
        category: normalized.category,
        tags: Array.isArray(item.tags) ? item.tags : [],
        selected: !!selectedMap[id],
        owned,
        accessible,
        priceCoins,
        priceAmount,
        priceType,
        priceText
      })
    })
  },

  decorateCategoryGroups(groups, selectedMap) {
    return (groups || []).map((group) => {
      const recipes = this.decorateRecipes(group.recipes || [], selectedMap)
      return {
        name: group.name || '',
        count: Number(group.count || recipes.length || 0),
        recipes
      }
    })
  },

  chooseActiveCategory(categories, recipes) {
    return categories.indexOf(this.data.activeCategory) >= 0 ? this.data.activeCategory : (categories[0] || '')
  },

  applyFilter() {
    const key = String(this.data.keyword || '').trim()
    const list = (this.data.recipes || []).filter(item => !key || [item.title, item.subtitle, item.category].some(text => String(text || '').indexOf(key) >= 0))
      .map(item => Object.assign({}, item, { selected: !!this.data.selectedMap[item.id] }))
    const menuSections = this.data.categories.map((name, index) => ({ name, id: 'menu-section-' + index, recipes: list.filter(item => item.category === name) }))
    this.setData({ filteredRecipes: list, menuSections }, () => this.measureSections())
  },

  measureSections() {
    if (!this.pageActive || !this.createSelectorQuery) return
    const requestId = this.measureRequestId = (this.measureRequestId || 0) + 1
    const query = this.createSelectorQuery()
    query.select('#recipeScroll').boundingClientRect()
    query.select('#recipeScroll').scrollOffset()
    query.selectAll('.recipe-group').boundingClientRect()
    query.select('#categoryScroll').boundingClientRect()
    query.select('#categoryScroll').scrollOffset()
    query.selectAll('.cat').boundingClientRect()
    query.select('#menuCart').boundingClientRect()
    query.exec(result => {
      if (!this.pageActive || requestId !== this.measureRequestId || !result || !result[0] || !result[1]) return
      this.sectionOffsets = (result[2] || []).map(rect => rect.top - result[0].top + result[1].scrollTop)
      if (result[3] && result[4]) {
        this.categoryHeight = result[3].height
        this.categoryCurrentScrollTop = result[4].scrollTop || 0
        this.categoryRects = (result[5] || []).map(rect => ({ top: rect.top - result[3].top + this.categoryCurrentScrollTop, bottom: rect.bottom - result[3].top + this.categoryCurrentScrollTop }))
        this.categoryContentHeight = this.categoryRects.length ? this.categoryRects[this.categoryRects.length - 1].bottom : 0
        this.ensureCategoryVisible(this.data.categories.indexOf(this.data.activeCategory))
      }
      const space = result[6] ? Math.max(24, Math.ceil(result[0].bottom - result[6].top + 16)) : 24
      if (space !== this.data.recipeBottomSpace) this.setData({ recipeBottomSpace: space })
    })
  },

  selectCategory(event) {
    const name = event.currentTarget.dataset.name
    const index = this.data.categories.indexOf(name)
    if (index < 0) return
    this.ensureCategoryVisible(index)
    this.setData({ activeCategory: name, scrollTarget: '' }, () => this.setData({ scrollTarget: 'menu-section-' + index }))
  },

  onRecipeScroll(event) {
    const offsets = this.sectionOffsets || []
    if (!offsets.length) return
    const top = Math.max(0, Number(event.detail.scrollTop) || 0) + 8
    let index = 0
    while (index + 1 < offsets.length && offsets[index + 1] <= top) index += 1
    const name = this.data.categories[index]
    if (name !== this.data.activeCategory) { this.setData({ activeCategory: name }); this.ensureCategoryVisible(index) }
  },

  onRecipeScrollBottom() {
    const index = this.data.categories.length - 1
    this.setData({ activeCategory: this.data.categories[index] }); this.ensureCategoryVisible(index)
  },
  onResize() { this.measureSections() },
  onCategoryScroll(event) { this.categoryCurrentScrollTop = Math.max(0, Number(event.detail.scrollTop) || 0) },
  ensureCategoryVisible(index) {
    const rects = this.categoryRects || []
    const current = this.categoryCurrentScrollTop || 0
    const next = viewport.ensureVisible(rects[index], this.categoryHeight, current, this.categoryContentHeight)
    if (next !== current) { this.categoryCurrentScrollTop = next; this.setData({ categoryScrollTop: next }) }
  },

  onCoverError(event) {
    const { id, url } = event.currentTarget.dataset
    const recipes = this.data.recipes.map(item => item.id === id ? assets.thumbnailFallback(item, url) : item)
    this.setData({ recipes }, () => this.applyFilter())
  },

  async buyRecipe(event) {
    if (this.data.buyingRecipeId) return
    const recipe = this.data.recipes.find(item => item.id === event.currentTarget.dataset.id)
    if (!recipe) return
    this.setData({ buyingRecipeId: recipe.id })
    try { if (await actions.purchase(recipe, () => this.pageActive)) { if (this.pageActive) this.loadRecipes() } }
    catch (err) { if (this.pageActive) wx.showToast({ title: err.message || '购买失败', icon: 'none' }) }
    finally { if (this.pageActive) this.setData({ buyingRecipeId: '' }) }
  },

  onSearch(e) {
    this.setData({ keyword: e.detail.value }, () => this.applyFilter())
  },

  addRecipe(e) {
    const id = e.currentTarget.dataset.id
    const recipe = this.data.recipes.find((item) => item.id === id)
    if (!recipe || !assets.normalize(recipe).accessible) {
      this.openRecipe(e)
      return
    }
    const selectedMap = Object.assign({}, this.data.selectedMap)
    selectedMap[id] = true
    debugLog.info('MENU', 'ADD_RECIPE', { recipeId: id, title: recipe.title })
    this.setData({ selectedMap }, () => {
      this.refreshRecipeSelectionState()
      this.syncSelectedIds()
    })
  },

  removeRecipe(e) {
    const id = e.currentTarget.dataset.id
    const selectedMap = Object.assign({}, this.data.selectedMap)
    delete selectedMap[id]
    debugLog.info('MENU', 'REMOVE_RECIPE', { recipeId: id })
    this.setData({ selectedMap }, () => {
      this.refreshRecipeSelectionState()
      this.syncSelectedIds()
    })
  },

  refreshRecipeSelectionState() {
    const selectedMap = this.data.selectedMap
    const recipes = this.data.recipes.map((item) => Object.assign({}, item, {
      selected: !!selectedMap[item.id]
    }))
    this.setData({ recipes }, () => this.applyFilter())
  },

  currentSelectedIds() {
    return Object.keys(this.data.selectedMap || {})
  },

  recipeIdsKey(ids) {
    return (ids || []).slice().sort().join('|')
  },

  hasTimeBlocks(plan) {
    return !!(plan &&
      Array.isArray(plan.freeBlocks) &&
      Array.isArray(plan.timeSegments) &&
      plan.timeSegments.length)
  },

  clearPreviewTimer() {
    if (this.previewTimer) {
      clearTimeout(this.previewTimer)
      this.previewTimer = null
    }
  },

  resetTimeBlocksPlan() {
    this.timeBlocksRequestId = (this.timeBlocksRequestId || 0) + 1
    this.timeBlocksRecipeKey = ''
    this.timeBlocksPlan = null
    this.timeBlocksPromise = null
  },

  ensureTimeBlocksPlan(ids, recipeKey, options) {
    const safeOptions = options || {}
    if (this.hasTimeBlocks(this.timeBlocksPlan) && this.timeBlocksRecipeKey === recipeKey) {
      debugLog.info('MENU', 'TIME_BLOCKS_CACHE_HIT', { recipeKey })
      return Promise.resolve(this.timeBlocksPlan)
    }
    if (this.timeBlocksPromise && this.timeBlocksRecipeKey === recipeKey) {
      debugLog.info('MENU', 'TIME_BLOCKS_WAIT_IN_FLIGHT', { recipeKey })
      return this.timeBlocksPromise
    }

    this.timeBlocksRequestId = (this.timeBlocksRequestId || 0) + 1
    const requestId = this.timeBlocksRequestId
    const snapshotIds = (ids || []).slice()
    this.timeBlocksRecipeKey = recipeKey
    this.timeBlocksPlan = null
    const isStale = () => {
      return requestId !== this.timeBlocksRequestId ||
        this.pageActive === false ||
        this.recipeIdsKey(this.currentSelectedIds()) !== recipeKey
    }
    debugLog.info('MENU', safeOptions.prefetch ? 'TIME_BLOCKS_PREFETCH_START' : 'TIME_BLOCKS_REQUEST', {
      requestId,
      recipeIds: snapshotIds,
      recipeKey
    })

    this.timeBlocksPromise = api.previewPlan(snapshotIds, {
      isStale,
      summaryOnly: true,
      includeTimeBlocks: true,
      timeout: config.PLAN_TIME_BLOCKS_REQUEST_TIMEOUT_MS || 30000,
      maxAttempts: config.PLAN_TIME_BLOCKS_MAX_ATTEMPTS || 3,
      retryDelayMs: 500
    })
      .then((res) => {
        if (!res.ok) throw new Error(res.message)
        if (isStale()) throw new Error('stale time blocks')
        if (!this.hasTimeBlocks(res)) throw new Error('\u4f11\u606f\u5b89\u6392\u8fd4\u56de\u4e0d\u5b8c\u6574')
        this.timeBlocksPlan = res
        this.timeBlocksRecipeKey = recipeKey
        this.timeBlocksPromise = null
        debugLog.info('MENU', safeOptions.prefetch ? 'TIME_BLOCKS_PREFETCH_OK' : 'TIME_BLOCKS_OK', {
          requestId,
          recipeKey,
          freeBlockCount: Array.isArray(res.freeBlocks) ? res.freeBlocks.length : 0,
          timeSegmentCount: Array.isArray(res.timeSegments) ? res.timeSegments.length : 0
        })
        return res
      })
      .catch((err) => {
        const stale = api.isStaleError(err) || isStale() || (err && err.message === 'stale time blocks')
        if (requestId === this.timeBlocksRequestId) {
          this.timeBlocksPromise = null
          if (!stale) this.timeBlocksPlan = null
        }
        debugLog[stale ? 'info' : 'warn']('MENU', stale ? 'TIME_BLOCKS_STALE' : 'TIME_BLOCKS_FAIL', {
          requestId,
          recipeKey,
          error: err && err.message ? err.message : String(err)
        })
        throw err
      })
    return this.timeBlocksPromise
  },

  syncSelectedIds() {
    const ids = this.currentSelectedIds()
    selection.replace(getApp(), ids, this.data.recipes)
    this.refreshSelected(false)
    this.schedulePreview(ids)
  },

  schedulePreview(ids) {
    if (this.pageActive === false) return
    const snapshotIds = (ids || []).slice()
    const snapshotKey = this.recipeIdsKey(snapshotIds)
    if (this.timeBlocksRecipeKey && this.timeBlocksRecipeKey !== snapshotKey) {
      this.resetTimeBlocksPlan()
    }
    this.previewRequestId = (this.previewRequestId || 0) + 1
    const requestId = this.previewRequestId
    const isStale = () => {
      return requestId !== this.previewRequestId ||
        this.pageActive === false ||
        this.recipeIdsKey(this.currentSelectedIds()) !== snapshotKey
    }
    this.clearPreviewTimer()

    if (!snapshotIds.length) {
      this.resetTimeBlocksPlan()
      this.setData({
        plan: {},
        planRecipeKey: '',
        previewError: '',
        cartOpen: false,
        previewing: false
      }, () => this.measureSections())
      return
    }

    this.setData({
      plan: {},
      planRecipeKey: '',
      previewError: '',
      previewing: true
    }, () => this.measureSections())
    debugLog.info('MENU', 'PREVIEW_SCHEDULED', { requestId, recipeIds: snapshotIds, recipeKey: snapshotKey })

    this.previewTimer = setTimeout(() => {
      this.previewTimer = null
      if (isStale()) return
      debugLog.info('MENU', 'PREVIEW_REQUEST', { requestId, recipeIds: snapshotIds, recipeKey: snapshotKey })
      api.previewPlan(snapshotIds, { isStale, summaryOnly: true })
        .then((res) => {
          if (!res.ok) throw new Error(res.message)
          if (isStale()) return
          this.setData({
            plan: res,
            planRecipeKey: snapshotKey,
            previewError: '',
            previewing: false
          }, () => this.measureSections())
          debugLog.info('MENU', 'PREVIEW_OK', {
            requestId,
            recipeKey: snapshotKey,
            totalSeconds: res.summary && res.summary.totalSeconds,
            activeSeconds: res.summary && res.summary.activeSeconds
          })
          this.ensureTimeBlocksPlan(snapshotIds, snapshotKey, { prefetch: true }).catch(() => {})
        })
        .catch((err) => {
          if (api.isStaleError(err) || isStale()) {
            debugLog.info('MENU', 'PREVIEW_STALE', { requestId, recipeKey: snapshotKey })
            return
          }
          this.setData({
            plan: {},
            planRecipeKey: '',
            previewError: err.message || '计划生成失败',
            previewing: false
          }, () => this.measureSections())
          debugLog.error('MENU', 'PREVIEW_FAIL', { requestId, recipeKey: snapshotKey, error: err.message || '计划生成失败' })
          wx.showToast({ title: err.message || '计划生成失败', icon: 'none' })
        })
    }, 160)
  },

  refreshSelected(needPreview) {
    const ids = this.currentSelectedIds()
    const selectedRecipes = this.data.recipes.filter((item) => ids.indexOf(item.id) >= 0)
    getApp().globalData.selectedRecipes = selectedRecipes
    this.setData({ selectedRecipes }, () => this.measureSections())
    if (needPreview !== false) {
      this.schedulePreview(ids)
    }
  },

  toggleCart() {
    this.setData({ cartOpen: !this.data.cartOpen })
  },

  canOpenRecipeDetail(recipe) {
    if (!recipe) return false
    return true
  },

  openRecipeFromCard(e) {
    const id = e.currentTarget.dataset.id
    const recipe = this.data.recipes.find((item) => item.id === id) ||
      this.data.filteredRecipes.find((item) => item.id === id)
    if (!this.canOpenRecipeDetail(recipe)) {
      debugLog.warn('MENU', 'OPEN_RECIPE_BLOCKED_NOT_OWNED', { recipeId: id })
      wx.showToast({ title: '请先购买菜谱', icon: 'none' })
      return
    }
    debugLog.info('MENU', 'OPEN_RECIPE_DETAIL', { recipeId: id, owned: !!recipe.owned, accessible: !!recipe.accessible })
    this.openRecipe(e)
  },

  openRecipe(e) {
    wx.navigateTo({ url: `/pages/recipe/recipe?id=${encodeURIComponent(e.currentTarget.dataset.id)}&selection=meal` })
  },

  openPlanPage(ids, selectedKey, previewPlan) {
    const app = getApp()
    app.globalData.lastPreview = previewPlan
    app.globalData.lastPreviewRecipeKey = selectedKey
    app.globalData.lastPreviewNeedsTimeBlocks = false
    debugLog.info('MENU', 'OPEN_COOK', {
      recipeIds: ids,
      recipeKey: selectedKey,
      freeBlockCount: Array.isArray(previewPlan.freeBlocks) ? previewPlan.freeBlocks.length : 0,
      timeSegmentCount: Array.isArray(previewPlan.timeSegments) ? previewPlan.timeSegments.length : 0,
      needsTimeBlocks: false
    })
    this.setData({ starting: false }, () => {
      wx.navigateTo({ url: '/pages/plan/plan' })
    })
  },

  openCook() {
    if (this.data.starting) return
    const ids = this.currentSelectedIds()
    if (!ids.length) {
      wx.showToast({ title: '先选几道菜', icon: 'none' })
      return
    }
    const selectedKey = this.recipeIdsKey(ids)
    if (this.data.previewing) {
      debugLog.warn('MENU', 'OPEN_COOK_BLOCKED_PREVIEWING', { recipeIds: ids })
      wx.showToast({ title: '计划还在计算', icon: 'none' })
      return
    }
    if (!this.data.plan || !this.data.plan.summary || this.data.planRecipeKey !== selectedKey) {
      this.schedulePreview(ids)
      debugLog.warn('MENU', 'OPEN_COOK_BLOCKED_PLAN_NOT_READY', { recipeIds: ids, selectedKey, planRecipeKey: this.data.planRecipeKey })
      wx.showToast({ title: this.data.previewError || '计划还在计算', icon: 'none' })
      return
    }
    const cachedTimeBlocks = this.hasTimeBlocks(this.timeBlocksPlan) && this.timeBlocksRecipeKey === selectedKey ? this.timeBlocksPlan : null
    if (cachedTimeBlocks) {
      debugLog.info('MENU', 'OPEN_COOK_TIME_BLOCKS_CACHE_READY', { recipeIds: ids, recipeKey: selectedKey })
      this.openPlanPage(ids, selectedKey, cachedTimeBlocks)
      return
    }

    this.openCookRequestId = (this.openCookRequestId || 0) + 1
    const openRequestId = this.openCookRequestId
    const isOpenStale = () => {
      return openRequestId !== this.openCookRequestId ||
        this.pageActive === false ||
        this.recipeIdsKey(this.currentSelectedIds()) !== selectedKey
    }
    debugLog.info('MENU', 'OPEN_COOK_TIME_BLOCKS_WAIT', { recipeIds: ids, recipeKey: selectedKey })
    this.setData({ starting: true })
    this.ensureTimeBlocksPlan(ids, selectedKey, { open: true })
      .then((previewPlan) => {
        if (isOpenStale()) {
          debugLog.info('MENU', 'OPEN_COOK_STALE_AFTER_TIME_BLOCKS', { recipeIds: ids, recipeKey: selectedKey })
          return
        }
        this.openPlanPage(ids, selectedKey, previewPlan)
      })
      .catch((err) => {
        if (api.isStaleError(err) || isOpenStale() || (err && err.message === 'stale time blocks')) {
          debugLog.info('MENU', 'OPEN_COOK_TIME_BLOCKS_STALE', { recipeIds: ids, recipeKey: selectedKey })
          if (this.pageActive !== false) this.setData({ starting: false })
          return
        }
        debugLog.error('MENU', 'OPEN_COOK_TIME_BLOCKS_FAIL', {
          recipeIds: ids,
          recipeKey: selectedKey,
          error: err && err.message ? err.message : String(err)
        })
        this.setData({ starting: false })
        wx.showToast({ title: err.message || '\u4f11\u606f\u5b89\u6392\u751f\u6210\u5931\u8d25', icon: 'none' })
      })
  }
})