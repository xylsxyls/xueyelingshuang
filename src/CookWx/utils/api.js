const config = require('./config')

function appState() {
  const app = getApp()
  return app && app.globalData ? app.globalData : {}
}

function request(options) {
  const state = appState()
  const safeOptions = options || {}
  const method = safeOptions.method || 'GET'
  const data = Object.assign({}, safeOptions.data || {}, {
    userId: state.userId || 'demo_user',
    clientType: state.clientType || 'wechat_mini',
    clientVersion: state.clientVersion || '0.1.0'
  })

  return new Promise((resolve, reject) => {
    wx.request({
      url: `${config.BASE_URL}${safeOptions.url}`,
      method,
      data,
      timeout: 10000,
      header: {
        'content-type': 'application/json'
      },
      success(res) {
        if (res.statusCode >= 200 && res.statusCode < 300) {
          resolve(res.data || {})
          return
        }
        const message = res.data && res.data.message ? res.data.message : `HTTP ${res.statusCode}`
        reject(new Error(message))
      },
      fail(err) {
        reject(new Error(err && err.errMsg ? err.errMsg : '网络请求失败'))
      }
    })
  })
}

function healthCheck() {
  return request({ url: '/api/health' })
}

function getFeed() {
  return request({ url: '/api/feed' })
}

function getRecipes() {
  return request({ url: '/api/recipes' })
}

function getAccount() {
  return request({ url: '/api/account' })
}

function recharge(coins) {
  return request({ url: '/api/account/recharge', method: 'POST', data: { coins } })
}

function purchaseRecipe(recipeId) {
  return request({ url: '/api/recipes/purchase', method: 'POST', data: { recipeId } })
}

function toggleFavorite(recipeId) {
  return request({ url: '/api/favorites/toggle', method: 'POST', data: { recipeId } })
}

function previewPlan(recipeIds) {
  return request({ url: '/api/plan/preview', method: 'POST', data: { recipeIds } })
}

function startPlan(recipeIds) {
  return request({ url: '/api/plan/start', method: 'POST', data: { recipeIds } })
}

module.exports = {
  healthCheck,
  getFeed,
  getRecipes,
  getAccount,
  recharge,
  purchaseRecipe,
  toggleFavorite,
  previewPlan,
  startPlan
}