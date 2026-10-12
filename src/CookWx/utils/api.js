const config = require('./config')
const debugLog = require('./debugLog')

const TOKEN_KEY = 'cook_token'
const ACCOUNT_KEY = 'cook_account'
const REQUEST_RUN_ID = `${Date.now().toString(36)}_${Math.floor(Math.random() * 100000)}`
let requestSerial = 0
let requestQueueSerial = 0
let runningRequestCount = 0
let runningLowPriorityCount = 0
const requestQueue = []

function numberOrDefault(value, defaultValue) {
  const numberValue = Number(value)
  return isFinite(numberValue) ? numberValue : defaultValue
}

function maxRequestConcurrency() {
  return Math.max(1, Math.floor(numberOrDefault(config.API_REQUEST_MAX_CONCURRENCY, 4)))
}

function maxBackgroundRequestConcurrency() {
  return Math.max(1, Math.floor(numberOrDefault(config.API_BACKGROUND_REQUEST_MAX_CONCURRENCY, 2)))
}

function lowPriorityValue() {
  return numberOrDefault(config.API_REQUEST_PRIORITY_LOW, 10)
}

function highPriorityValue() {
  return numberOrDefault(config.API_REQUEST_PRIORITY_HIGH, 100)
}

function normalPriorityValue() {
  return numberOrDefault(config.API_REQUEST_PRIORITY_NORMAL, 50)
}

function requestPriority(options) {
  const explicit = Number(options && options.priority)
  if (isFinite(explicit)) return explicit
  const purpose = options && options.purpose ? String(options.purpose) : ''
  if (purpose === 'background_prefetch') return lowPriorityValue()
  if (purpose === 'voice_fetch' || purpose === 'voice_tts') return normalPriorityValue()
  return highPriorityValue()
}

function isLowPriority(priority) {
  return Number(priority) <= lowPriorityValue()
}

function hasHighPriorityWorkQueuedOrRunning() {
  if (runningRequestCount - runningLowPriorityCount > 0) return true
  return requestQueue.some((task) => !isLowPriority(task.priority))
}

function findNextQueuedRequestIndex() {
  if (runningRequestCount >= maxRequestConcurrency()) return -1
  const highWorkExists = hasHighPriorityWorkQueuedOrRunning()
  let bestIndex = -1
  let bestPriority = -Infinity
  let bestSerial = Number.MAX_SAFE_INTEGER
  for (let i = 0; i < requestQueue.length; i += 1) {
    const task = requestQueue[i]
    const low = isLowPriority(task.priority)
    if (low && runningLowPriorityCount >= maxBackgroundRequestConcurrency()) continue
    if (low && highWorkExists) continue
    if (task.priority > bestPriority || (task.priority === bestPriority && task.queueSerial < bestSerial)) {
      bestIndex = i
      bestPriority = task.priority
      bestSerial = task.queueSerial
    }
  }
  return bestIndex
}

function pumpRequestQueue() {
  let index = findNextQueuedRequestIndex()
  while (index >= 0) {
    const task = requestQueue.splice(index, 1)[0]
    runningRequestCount += 1
    if (isLowPriority(task.priority)) runningLowPriorityCount += 1
    task.start()
    index = findNextQueuedRequestIndex()
  }
}

function enqueueRequestTask(task) {
  requestQueue.push(task)
  pumpRequestQueue()
}

function finishRequestTask(priority) {
  runningRequestCount = Math.max(0, runningRequestCount - 1)
  if (isLowPriority(priority)) {
    runningLowPriorityCount = Math.max(0, runningLowPriorityCount - 1)
  }
  pumpRequestQueue()
}

function appState() {
  const app = getApp()
  return app && app.globalData ? app.globalData : {}
}

function saveSession(res) {
  if (!res || !res.token) return
  const state = appState()
  const nextUserId = res.account && res.account.userId ? res.account.userId : ''
  if (state.userId && nextUserId && state.userId !== nextUserId) resetUserScopedState(state)
  state.token = res.token
  state.account = res.account || {}
  state.userId = state.account.userId || ''
  wx.setStorageSync(TOKEN_KEY, res.token)
  wx.setStorageSync(ACCOUNT_KEY, state.account)
}

function resetUserScopedState(state) {
  if (!state) return
  state.selectedRecipeIds = []
  state.selectedRecipes = []
  state.lastPreview = null
  state.lastPreviewRecipeKey = ''
  state.lastPreviewNeedsTimeBlocks = false
  state.lastPlan = null
  state.currentCookVoiceGender = ''
  state.pendingCookVoiceTexts = []
  state.pendingCookSyncReports = []
  state.mineInitialTab = ''
  state.messageInitialTab = ''
  state.messageTargetUserId = ''
  state.followStateChanges = {}
  state.socialStateVersion = Number(state.socialStateVersion || 0) + 1
}

function clearSession() {
  require('./privateMediaCache').clear()
  const state = appState()
  resetUserScopedState(state)
  state.token = ''
  state.account = null
  state.userId = ''
  wx.removeStorageSync(TOKEN_KEY)
  wx.removeStorageSync(ACCOUNT_KEY)
}

function token() {
  const state = appState()
  return state.token || wx.getStorageSync(TOKEN_KEY) || ''
}

function isPublicRequest(options) {
  const safeOptions = options || {}
  if (safeOptions.skipAuthCheck === true) return true
  const method = safeOptions.method || 'GET'
  const url = safeOptions.url || ''
  return url === '/api/health' ||
    url === '/api/voice/audio' ||
    (method === 'POST' && url === '/api/auth/register') ||
    (method === 'POST' && url === '/api/auth/login')
}

function shouldRedirectForAuth(options) {
  const safeOptions = options || {}
  return safeOptions.silentAuthFailure !== true && safeOptions.purpose !== 'background_prefetch'
}

function staleError() {
  const err = new Error('stale request')
  err.stale = true
  return err
}

function isStaleError(err) {
  return !!(err && err.stale)
}

function isRequestStale(options) {
  if (options && options.__authBound === true && token() !== options.__authToken) return true
  if (!options || typeof options.isStale !== 'function') return false
  try {
    return !!options.isStale()
  } catch (err) {
    debugLog.warn('API', 'STALE_CHECK_FAIL', {
      url: options.url || '',
      error: err && err.message ? err.message : String(err)
    })
    return false
  }
}

function request(options) {
  const safeOptions = Object.assign({}, options || {})
  if (safeOptions.__authBound !== true && !isPublicRequest(safeOptions)) {
    safeOptions.__authBound = true
    safeOptions.__authToken = token()
  }
  const method = safeOptions.method || 'GET'
  const timeout = safeOptions.timeout || config.DEFAULT_REQUEST_TIMEOUT_MS || 30000
  const requestId = safeOptions.__requestId || ++requestSerial
  const attempt = Math.max(1, Number(safeOptions.__attempt || 1))
  const defaultMaxAttempts = method === 'GET' ? (config.DEFAULT_REQUEST_MAX_ATTEMPTS || 3) : 1
  const maxAttempts = Math.max(1, Number(safeOptions.maxAttempts || defaultMaxAttempts))
  const queueEnterTime = Date.now()
  const purpose = safeOptions.purpose || ''
  const traceId = `${REQUEST_RUN_ID}:${requestId}:${attempt}`
  const priority = requestPriority(safeOptions)
  const queueSerial = ++requestQueueSerial
  const data = safeOptions.rawBody === true
    ? safeOptions.data
    : Object.assign({}, safeOptions.data || {}, {
      clientType: appState().clientType || 'wechat_mini',
      clientVersion: appState().clientVersion || '0.2.0'
    })

  debugLog.info('API', 'REQUEST_START', {
    runId: REQUEST_RUN_ID,
    requestId,
    attempt,
    maxAttempts,
    traceId,
    purpose,
    priority,
    method,
    url: safeOptions.url,
    timeout,
    queuedRequests: requestQueue.length,
    runningRequests: runningRequestCount
  })
  return new Promise((resolve, reject) => {
    enqueueRequestTask({
      priority,
      queueSerial,
      start() {
        const sendTime = Date.now()
        const queueWaitMilliseconds = sendTime - queueEnterTime
        const headers = Object.assign({
          'content-type': 'application/json'
        }, safeOptions.headers || {})
        const currentToken = safeOptions.__authBound ? safeOptions.__authToken : token()
        if (safeOptions.__authBound === true && token() !== currentToken) {
          const finishQueue = () => finishRequestTask(priority)
          finishQueue()
          reject(staleError())
          return
        }
        if (!isPublicRequest(safeOptions) && !currentToken) {
          const finishQueue = () => finishRequestTask(priority)
          debugLog.warn('API', 'AUTH_REQUIRED_LOCAL', {
            runId: REQUEST_RUN_ID,
            requestId,
            attempt,
            traceId,
            purpose,
            method,
            url: safeOptions.url,
            queueWaitMilliseconds
          })
          finishQueue()
          if (shouldRedirectForAuth(safeOptions)) {
            wx.reLaunch({ url: '/pages/auth/login/login' })
          }
          reject(new Error('请先登录'))
          return
        }
        if (currentToken) headers.Authorization = `Bearer ${currentToken}`
        headers['X-Cook-Run-Id'] = REQUEST_RUN_ID
        headers['X-Cook-Request-Id'] = String(requestId)
        headers['X-Cook-Attempt'] = String(attempt)
        headers['X-Cook-Trace-Id'] = traceId
        headers['X-Cook-Request-Begin-Ms'] = String(sendTime)
        headers['X-Cook-Queue-Wait-Ms'] = String(queueWaitMilliseconds)
        if (purpose) headers['X-Cook-Purpose'] = purpose

        debugLog.info('API', 'REQUEST_SEND', {
          runId: REQUEST_RUN_ID,
          requestId,
          attempt,
          maxAttempts,
          traceId,
          purpose,
          priority,
          method,
          url: safeOptions.url,
          timeout,
          queueWaitMilliseconds,
          queuedRequests: requestQueue.length,
          runningRequests: runningRequestCount,
          runningLowPriorityRequests: runningLowPriorityCount
        })

        const finishQueue = () => finishRequestTask(priority)
        if (isRequestStale(safeOptions)) {
          debugLog.info('API', 'REQUEST_STALE_QUEUE_CANCEL', {
            runId: REQUEST_RUN_ID,
            requestId,
            attempt,
            traceId,
            purpose,
            method,
            url: safeOptions.url,
            queueWaitMilliseconds
          })
          finishQueue()
          reject(staleError())
          return
        }

        try {
          wx.request({
          url: `${config.BASE_URL}${safeOptions.url}`,
          method,
          data,
          timeout,
          header: headers,
          success(res) {
            const durationMilliseconds = Date.now() - sendTime
            const totalMilliseconds = Date.now() - queueEnterTime
            finishQueue()
        if (isRequestStale(safeOptions)) {
          debugLog.info('API', 'REQUEST_STALE_DONE', { runId: REQUEST_RUN_ID, requestId, attempt, traceId, purpose, method, url: safeOptions.url, statusCode: res.statusCode, durationMilliseconds, queueWaitMilliseconds, totalMilliseconds })
          reject(staleError())
          return
        }
        debugLog.info('API', 'REQUEST_DONE', { runId: REQUEST_RUN_ID, requestId, attempt, traceId, purpose, method, url: safeOptions.url, statusCode: res.statusCode, durationMilliseconds, queueWaitMilliseconds, totalMilliseconds })
        if (res.statusCode === 401) {
          debugLog.warn('API', 'UNAUTHORIZED', { runId: REQUEST_RUN_ID, requestId, attempt, traceId, purpose, url: safeOptions.url })
          clearSession()
          if (shouldRedirectForAuth(safeOptions)) {
            wx.reLaunch({ url: '/pages/auth/login/login' })
          }
          reject(new Error('请先登录'))
          return
        }
        if (res.statusCode >= 200 && res.statusCode < 300) {
          resolve(res.data || {})
          return
        }
        const message = res.data && res.data.message ? res.data.message : `HTTP ${res.statusCode}`
        debugLog.warn('API', 'HTTP_ERROR', { runId: REQUEST_RUN_ID, requestId, attempt, traceId, purpose, url: safeOptions.url, statusCode: res.statusCode, message })
        const error = new Error(message)
        error.statusCode = res.statusCode
        reject(error)
      },
      fail(err) {
        const durationMilliseconds = Date.now() - sendTime
        const totalMilliseconds = Date.now() - queueEnterTime
        finishQueue()
        const failDetail = {
          runId: REQUEST_RUN_ID,
          requestId,
          attempt,
          maxAttempts,
          traceId,
          purpose,
          url: safeOptions.url,
          method,
          durationMilliseconds,
          queueWaitMilliseconds,
          totalMilliseconds,
          errMsg: err && err.errMsg ? err.errMsg : ''
        }
        if (isRequestStale(safeOptions)) {
          debugLog.info('API', 'REQUEST_STALE_FAIL', failDetail)
          reject(staleError())
          return
        }
        if (attempt < maxAttempts) {
          const retryDelayMs = Math.max(0, Number(safeOptions.retryDelayMs !== undefined ? safeOptions.retryDelayMs : 300))
          debugLog.warn('API', 'REQUEST_FAIL_RETRYING', Object.assign({}, failDetail, {
            attempt,
            maxAttempts,
            retryDelayMs
          }))
          setTimeout(() => {
            if (isRequestStale(safeOptions)) {
              debugLog.info('API', 'REQUEST_STALE_RETRY_CANCEL', failDetail)
              reject(staleError())
              return
            }
            request(Object.assign({}, safeOptions, {
              __requestId: requestId,
              __attempt: attempt + 1
            })).then(resolve).catch(reject)
          }, retryDelayMs)
          return
        }
        const finalFailDetail = Object.assign({}, failDetail, { willRetry: false })
        if (safeOptions.silentFailure === true || safeOptions.purpose === 'background_prefetch') {
          debugLog.info('API', 'REQUEST_FAIL_SILENT', finalFailDetail)
        } else if (safeOptions.url === '/api/voice/tts' ||
          safeOptions.url === '/api/voice/fetch') {
          debugLog.warn('API', 'REQUEST_FAIL_FINAL', finalFailDetail)
        } else {
          debugLog.error('API', 'REQUEST_FAIL', finalFailDetail)
        }
        reject(new Error(err && err.errMsg ? err.errMsg : '网络请求失败'))
      }
          })
        } catch (err) {
          const durationMilliseconds = Date.now() - sendTime
          const totalMilliseconds = Date.now() - queueEnterTime
          finishQueue()
          debugLog.error('API', 'REQUEST_THROW', {
            runId: REQUEST_RUN_ID,
            requestId,
            attempt,
            maxAttempts,
            traceId,
            purpose,
            url: safeOptions.url,
            method,
            durationMilliseconds,
            queueWaitMilliseconds,
            totalMilliseconds,
            error: err && err.message ? err.message : String(err)
          })
          reject(new Error(err && err.message ? err.message : '网络请求失败'))
        }
      }
    })
  })
}

function healthCheck() {
  return request({ url: '/api/health' })
}

function register(account, password, nickname) {
  return request({ url: '/api/auth/register', method: 'POST', data: { account, password, nickname } })
    .then((res) => {
      if (res.ok) saveSession(res)
      return res
    })
}

function login(account, password) {
  return request({ url: '/api/auth/login', method: 'POST', data: { account, password } })
    .then((res) => {
      if (res.ok) saveSession(res)
      return res
    })
}

function logout() {
  const logoutToken = token()
  clearSession()
  if (!logoutToken) {
    return Promise.resolve({ ok: true, message: 'logout ok' })
  }
  return request({
    url: '/api/auth/logout',
    method: 'POST',
    skipAuthCheck: true,
    silentAuthFailure: true,
    headers: {
      Authorization: `Bearer ${logoutToken}`
    }
  }).catch((err) => {
    debugLog.warn('API', 'LOGOUT_REMOTE_FAIL_LOCAL_CLEARED', {
      error: err && err.message ? err.message : String(err)
    })
    return { ok: false, message: err && err.message ? err.message : 'logout request failed' }
  })
}

function session() {
  return request({ url: '/api/auth/session' })
}

function getFeed(options) {
  const safeOptions = options || {}
  const data = Object.assign({}, safeOptions.data || {})
  if (safeOptions.count !== undefined) data.count = safeOptions.count
  if (safeOptions.mode !== undefined) data.mode = safeOptions.mode
  if (safeOptions.currentVideoId !== undefined) data.currentVideoId = safeOptions.currentVideoId
  if (Array.isArray(safeOptions.excludeIds)) data.excludeIds = safeOptions.excludeIds.filter((id) => !!id).join(',')
  else if (safeOptions.excludeIds !== undefined) data.excludeIds = safeOptions.excludeIds
  return request(Object.assign({ url: '/api/feed', maxAttempts: 3, retryDelayMs: 300 }, safeOptions, { data }))
}

function markVideoWatched(videoId) {
  return request({ url: '/api/feed/watch', method: 'POST', data: { videoId }, purpose: 'feed_watch' })
}

function getRecipes(options) {
  return request(Object.assign({ url: options && options.publicCatalog ? '/api/recipes?scope=public_catalog' : options && options.catalog ? '/api/recipes?scope=catalog' : '/api/recipes', maxAttempts: 3, retryDelayMs: 300 }, options || {}))
}

function getRecipeDetail(recipeId, options) {
  return request(Object.assign({ url: `/api/recipes/detail?recipeId=${encodeURIComponent(recipeId)}` }, options || {}))
}

function getAccount(options) {
  return request(Object.assign({ url: '/api/account' }, options || {}))
}

function checkin() {
  return request({ url: '/api/wallet/checkin', method: 'POST' })
}

function startAdReward() {
  return request({ url: '/api/wallet/ad/start', method: 'POST' })
}

function finishAdReward(adToken) {
  return request({ url: '/api/wallet/ad/finish', method: 'POST', data: { adToken } })
}

function makeIdempotencyKey(prefix, id) {
  return `${prefix}_${id}_${Date.now()}_${Math.floor(Math.random() * 100000)}`
}

function joinRecipe(recipeId, idempotencyKey) {
  const key = idempotencyKey || makeIdempotencyKey('join', recipeId)
  return request({
    url: '/api/recipes/join',
    method: 'POST',
    headers: { 'X-Idempotency-Key': key },
    data: { recipeId, idempotencyKey: key }
  })
}

function purchaseRecipe(recipeId, idempotencyKey) {
  const key = idempotencyKey || makeIdempotencyKey('purchase', recipeId)
  return request({ url: '/api/recipes/purchase', method: 'POST', headers: { 'X-Idempotency-Key': key }, data: { recipeId, idempotencyKey: key } })
}

function removeMenuRecipe(recipeId) { return request({ url: '/api/recipes/menu/remove', method: 'POST', data: { recipeId } }) }
function updateIntroduction(introduction, options) { return request(Object.assign({ url: '/api/users/profile/update', method: 'POST', data: { introduction } }, options || {})) }
function uploadAvatar(data, options) {
  return request(Object.assign({}, options || {}, {
    url: '/api/users/avatar/upload', method: 'POST', data, rawBody: true,
    headers: { 'content-type': 'application/octet-stream' }, maxAttempts: 1
  }))
}
function getUserProfile(userId, options) { return request(Object.assign({ url: '/api/users/profile', data: { userId } }, options || {})) }
function getUserFeed(userId, mode, options) {
  const settings = options || {}
  return request(Object.assign({}, settings, { url: '/api/users/feed', data: { userId, mode, count: settings.count || 18, excludeIds: (settings.excludeIds || []).join(',') } }))
}
function getFans(options) { return request(Object.assign({ url: '/api/users/fans', data: { count: 30 } }, options || {})) }
function getVideoDetail(videoId, options) { return request(Object.assign({ url: '/api/videos/detail', data: { videoId } }, options || {})) }
function getConversation(userId, options) { return request(Object.assign({ url: '/api/messages/conversation', data: { userId } }, options || {})) }
function readConversation(targetUserId) { return request({ url: '/api/messages/read', method: 'POST', data: { targetUserId } }) }

function toggleFavorite(recipeId, targetType) {
  const type = targetType || 'recipe'
  return request({ url: '/api/favorites/toggle', method: 'POST', data: { targetType: type, targetId: recipeId, recipeId } })
}

function toggleLike(targetType, targetId) {
  return request({ url: '/api/likes/toggle', method: 'POST', data: { targetType, targetId } })
}

function listComments(targetType, targetId, options) {
  return request(Object.assign({ url: `/api/comments/list?targetType=${encodeURIComponent(targetType)}&targetId=${encodeURIComponent(targetId)}` }, options || {}))
}

function createComment(targetType, targetId, content, score) {
  return request({ url: '/api/comments/create', method: 'POST', data: { targetType, targetId, content, score } })
}

function toggleFollow(targetUserId) {
  return request({ url: '/api/follows/toggle', method: 'POST', data: { targetUserId } })
}

function listMessages(options) {
  return request(Object.assign({ url: '/api/messages/list' }, options || {}))
}

function sendMessage(targetUserId, text) {
  return request({ url: '/api/messages/send', method: 'POST', data: { targetUserId, text } })
}

function previewPlan(recipeIds, options) {
  const safeOptions = options || {}
  const data = Object.assign({ recipeIds }, safeOptions.data || {})
  if (safeOptions.summaryOnly !== undefined) {
    data.summaryOnly = !!safeOptions.summaryOnly
  }
  if (safeOptions.includeTimeBlocks !== undefined) {
    data.includeTimeBlocks = !!safeOptions.includeTimeBlocks
  }
  const requestOptions = Object.assign({}, safeOptions)
  delete requestOptions.summaryOnly
  delete requestOptions.includeTimeBlocks
  delete requestOptions.data
  return request(Object.assign({
    url: '/api/plan/preview',
    method: 'POST',
    data,
    timeout: config.PLAN_PREVIEW_REQUEST_TIMEOUT_MS || 30000,
    maxAttempts: 3,
    retryDelayMs: 300
  }, requestOptions))
}

function startPlan(recipeIds, options) {
  const safeOptions = options || {}
  const data = Object.assign({ recipeIds, lite: false }, safeOptions.data || {})
  const requestOptions = Object.assign({}, safeOptions)
  delete requestOptions.data
  return request(Object.assign({
    url: '/api/plan/start',
    method: 'POST',
    data,
    timeout: config.PLAN_START_REQUEST_TIMEOUT_MS || 30000,
    maxAttempts: config.PLAN_START_MAX_ATTEMPTS || 3,
    retryDelayMs: config.PLAN_START_RETRY_DELAY_MS || 300
  }, requestOptions))
}

function publishCustomRecipe(recipeJson, options = {}) {
  return request({ url: '/api/recipes/custom/publish', method: 'POST', data: { recipeJson }, maxAttempts: 1, isStale: options.isStale })
}

function getCreationRecipe(recipeId, options = {}) {
  return request({ url: '/api/recipes/creation/detail?recipeId=' + encodeURIComponent(recipeId), isStale: options.isStale })
}

function saveCreationSteps(data, options = {}) {
  return request({ url: '/api/recipes/creation/save', method: 'POST', data, maxAttempts: 1, isStale: options.isStale })
}

function getRecipeReviews(state, cursor = '', options = {}) {
  return request({ url: '/api/recipes/reviews/list?state=' + encodeURIComponent(state) + '&cursor=' + encodeURIComponent(cursor) + '&count=18', isStale: options.isStale })
}
function getRecipeReview(recipeId, reviewId = '', options = {}) {
  return request({ url: '/api/recipes/reviews/detail?recipeId=' + encodeURIComponent(recipeId || '') + '&reviewId=' + encodeURIComponent(reviewId), isStale: options.isStale })
}
function getRecipeReviewHistory(recipeId, cursor = '', options = {}) {
  return request({ url: '/api/recipes/reviews/history?recipeId=' + encodeURIComponent(recipeId) + '&cursor=' + encodeURIComponent(cursor), isStale: options.isStale })
}
function decideRecipeReview(reviewId, decision, reason) {
  return request({ url: '/api/recipes/reviews/decide', method: 'POST', data: { reviewId, decision, reason }, maxAttempts: 1 })
}

function savePersonalization(data) {
  return request({ url: '/api/recipes/personalization/save', method: 'POST', data })
}

function getPersonalizationDetail(recipeId) {
  return request({ url: `/api/recipes/personalization/detail?recipeId=${encodeURIComponent(recipeId)}` })
}

function resetPersonalization(recipeId) {
  return request({ url: '/api/recipes/personalization/reset', method: 'POST', data: { recipeId } })
}

function syncPersonalizationTimes(items, options = {}) {
  return request({
    url: '/api/recipes/personalization/sync-times',
    method: 'POST',
    data: { items: Array.isArray(items) ? items : [] },
    timeout: config.DEFAULT_REQUEST_TIMEOUT_MS || 30000,
    maxAttempts: 1,
    isStale: options.isStale
  })
}

function uploadVideoStart(data) {
	return request({ url: '/api/videos/upload/start', method: 'POST', data, maxAttempts: 1 })
}

function uploadVideoChunk(uploadId, offset, data) {
	return request({
		url: `/api/videos/upload/chunk?uploadId=${encodeURIComponent(uploadId)}&offset=${offset}`,
		method: 'POST',
		data,
		rawBody: true,
		headers: { 'content-type': 'application/octet-stream' },
		timeout: config.DEFAULT_REQUEST_TIMEOUT_MS || 30000,
		maxAttempts: 1
	})
}

function uploadVideoComplete(uploadId) {
	return request({ url: '/api/videos/upload/complete', method: 'POST', data: { uploadId }, maxAttempts: 1 })
}

function cancelVideoUpload(uploadId) {
	return request({ url: '/api/videos/upload/cancel', method: 'POST', data: { uploadId }, maxAttempts: 1 })
}

function publishVideo(data) {
  return request({ url: '/api/videos/publish', method: 'POST', data })
}

function publishCookResult(data) {
  return request({ url: '/api/cook/result/publish', method: 'POST', data })
}

function voiceTts(text, gender, options) {
  const data = { text, gender }
  if (options && options.inlineAudio) data.inlineAudio = true
  return request({
    url: '/api/voice/tts',
    method: 'POST',
    timeout: options && options.timeout !== undefined ? options.timeout : (config.VOICE_TTS_REQUEST_TIMEOUT_MS || 30000),
    maxAttempts: options && options.maxAttempts !== undefined ? options.maxAttempts : (config.VOICE_TTS_MAX_ATTEMPTS || 3),
    retryDelayMs: options && options.retryDelayMs !== undefined ? options.retryDelayMs : (config.VOICE_TTS_RETRY_DELAY_MS || 300),
    purpose: options && options.purpose ? options.purpose : '',
    priority: options && options.priority !== undefined ? options.priority : undefined,
    data
  })
}

function voicePrepare(texts, gender, requiredCount, planId, options) {
  const safeOptions = options || {}
  return request({
    url: '/api/voice/prepare',
    method: 'POST',
    timeout: config.VOICE_PREPARE_REQUEST_TIMEOUT_MS || 30000,
    maxAttempts: config.VOICE_PREPARE_MAX_ATTEMPTS || 3,
    retryDelayMs: config.VOICE_PREPARE_RETRY_DELAY_MS || 300,
    purpose: safeOptions.purpose || 'voice_prepare',
    data: {
      texts: texts || [],
      gender,
      requiredCount: requiredCount || 0,
      planId: planId || '',
      includeRequiredAudio: safeOptions.includeRequiredAudio === true,
      requiredAudioWaitMilliseconds: safeOptions.requiredAudioWaitMilliseconds || config.VOICE_FETCH_WAIT_MS || 30000
    }
  })
}

function voiceFetch(voiceId, options) {
  const waitMilliseconds = options && options.waitMilliseconds !== undefined ? options.waitMilliseconds : (config.VOICE_FETCH_WAIT_MS || 30000)
  const timeout = options && options.timeout !== undefined ? options.timeout : Math.max(config.VOICE_TTS_REQUEST_TIMEOUT_MS || 30000, waitMilliseconds + 1000)
  return request({
    url: '/api/voice/fetch',
    method: 'POST',
    timeout,
    maxAttempts: options && options.maxAttempts !== undefined ? options.maxAttempts : (config.VOICE_FETCH_MAX_ATTEMPTS || 3),
    retryDelayMs: options && options.retryDelayMs !== undefined ? options.retryDelayMs : (config.VOICE_TTS_RETRY_DELAY_MS || 300),
    purpose: options && options.purpose ? options.purpose : '',
    priority: options && options.priority !== undefined ? options.priority : undefined,
    data: {
      voiceId,
      inlineAudio: !!(options && options.inlineAudio),
      waitMilliseconds
    }
  })
}

module.exports = {
  getRecipeReviews, getRecipeReview, getRecipeReviewHistory, decideRecipeReview,
  uploadAvatar,
  updateIntroduction, removeMenuRecipe, getUserProfile, getUserFeed, getFans, getVideoDetail, getConversation, readConversation,
  TOKEN_KEY,
  ACCOUNT_KEY,
  saveSession,
  clearSession,
  token,
  isStaleError,
  healthCheck,
  register,
  login,
  logout,
  session,
  getFeed,
  markVideoWatched,
  getRecipes,
  getRecipeDetail,
  getAccount,
  checkin,
  startAdReward,
  finishAdReward,
  joinRecipe,
  purchaseRecipe,
  toggleFavorite,
  toggleLike,
  listComments,
  createComment,
  toggleFollow,
  listMessages,
  sendMessage,
  previewPlan,
  startPlan,
  publishCustomRecipe,
  getCreationRecipe,
  saveCreationSteps,
  savePersonalization,
  getPersonalizationDetail,
  resetPersonalization,
  syncPersonalizationTimes,
  uploadVideoStart,
  uploadVideoChunk,
  uploadVideoComplete,
  cancelVideoUpload,
  publishVideo,
  publishCookResult,
  voiceTts,
  voicePrepare,
  voiceFetch
}