const fs = require('node:fs')
const path = require('node:path')
const vm = require('node:vm')
const sourceRoot = path.resolve(__dirname, '../../../CookWx')

function fixture(route, options = {}) {
  const plan = options.plan || {
    planId: 'navigation-plan', summary: { totalSeconds: 30, activeSeconds: 30 },
    timeline: [{ id: 'step', recipeId: 'recipe', title: '切菜', startSeconds: 0, endSeconds: 30, durationSeconds: 30, active: true, taskMode: 'foreground', canPause: true }],
    freeBlocks: [], timeSegments: [{ type: 'active', startSeconds: 0, durationSeconds: 30 }]
  }
  const recipe = { id: 'recipe', title: '测试菜', category: '家常菜', owned: true, tags: [] }
  const app = { globalData: { userId: 'owner', pendingCookSyncReports: [], lastPlan: plan, lastPreview: plan, selectedRecipeIds: ['recipe'], selectedRecipes: [recipe] } }
  const calls = { enable: [], disable: [], routes: [], speaks: [], timers: new Map(), callbacks: [], toasts: [], modals: [], sync: [] }
  let nextTimer = 0
  let definition
  const wx = {
    enableAlertBeforeUnload: value => calls.enable.push(value),
    disableAlertBeforeUnload: value => calls.disable.push(value),
    switchTab: value => calls.routes.push({ type: 'switchTab', ...value }),
    navigateTo: value => calls.routes.push({ type: 'navigateTo', ...value }),
    redirectTo: value => calls.routes.push({ type: 'redirectTo', ...value }),
    showToast: value => calls.toasts.push(value), showModal: value => calls.modals.push(value), nextTick: callback => callback(),
    showActionSheet() { throw new Error('微调不得打开操作菜单') }, vibrateShort() {}, setTabBarItem() {}
  }
  const voice = {
    getGender: () => 'female', genderLabel: () => '女声', stop: () => calls.speaks.push('stop'),
    speak: () => calls.speaks.push('speak'), speakSequence: () => calls.speaks.push('sequence'),
    preparePlanVoiceTexts: () => Promise.resolve(), prefetchRequired: () => Promise.resolve()
  }
  const api = {
    isStaleError: () => false, startPlan: () => Promise.resolve({ ok: true, ...plan }),
    getRecipes: () => options.catalogFails ? Promise.reject(new Error('offline')) : Promise.resolve({ ok: true, recipes: [recipe], categories: ['家常菜'] }),
    previewPlan: () => Promise.resolve({ ok: true, ...plan }),
    syncPersonalizationTimes(items) { calls.sync.push(items); return Promise.resolve({ ok: true }) }
  }
  const filename = path.join(sourceRoot, 'pages', route, route + '.js')
  const sandbox = {
    Page: value => { definition = value }, getApp: () => app, wx,
    require: name => {
      if (name.endsWith('/api')) return api
      if (name.endsWith('/voice')) return voice
      if (name.endsWith('/debugLog')) return { info() {}, warn() {}, error() {} }
      if (name.endsWith('/tabBar')) return { showHome() {} }
      return require(path.resolve(path.dirname(filename), name))
    },
    setTimeout: callback => { calls.timers.set(++nextTimer, callback); return nextTimer },
    clearTimeout: id => calls.timers.delete(id),
    setInterval: callback => { calls.timers.set(++nextTimer, callback); return nextTimer },
    clearInterval: id => calls.timers.delete(id)
  }
  vm.runInNewContext(fs.readFileSync(filename, 'utf8'), sandbox, { filename })
  const page = {
    ...definition, data: JSON.parse(JSON.stringify(definition.data)),
    setData(values, callback) {
      Object.assign(this.data, values)
      if (callback && options.deferCallbacks) calls.callbacks.push(callback)
      else if (callback) callback()
    }
  }
  if (page.onLoad) page.onLoad()
  while (calls.callbacks.length) calls.callbacks.shift()()
  return { page, app, calls, recipe, api, wx }
}

module.exports = { fixture }