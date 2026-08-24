const config = require('./config')
const api = require('./api')
const debugLog = require('./debugLog')

const STORAGE_KEY = 'cook_voice_gender'
const TTS_RETRY_DELAYS = [300, 900]
const DEFAULT_PREFETCH_QUEUE_CONCURRENCY = 2
const SERVER_WAV_BYTES_PER_SECOND = 32000
const SERVER_WAV_HEADER_BYTES = 44
const ESTIMATE_AUDIO_BUFFER_SECONDS = 0.25

let audioContext = null
let ttsPlugin = null
let pluginLoaded = false
let speakToken = 0
let sequenceEndedHandler = null
let sequenceErrorHandler = null
let sequenceHandlerId = 0
let activeSequenceHandlerId = 0

const serverAudioCache = {}
const serverAudioUrlPending = {}
const serverAudioUrlPendingInline = {}
const serverAudioUrlPendingStartedAt = {}
const serverAudioDownloadPending = {}
const restDurationAudioPending = {}
const restDurationAudioPendingStartedAt = {}
const preparedVoiceMap = {}
const preparedVoiceIdMap = {}
const queuedPrefetchMap = {}
const prefetchQueue = []
let prefetchQueueRunningCount = 0
let preparedFetchWaitMs = config.VOICE_FETCH_WAIT_MS || 30000
let preparedFetchMaxAttempts = config.VOICE_FETCH_MAX_ATTEMPTS || 3
let preparedFetchConcurrency = config.VOICE_PREFETCH_QUEUE_CONCURRENCY || 2

function contentPreview(content) {
  const text = (content || '').replace(/\s+/g, ' ').trim()
  return text.length > 60 ? `${text.slice(0, 60)}...` : text
}

function genderLabel(gender) {
  return gender === 'male' ? '男声' : '女声'
}

function normalizeGender(gender) {
  return gender === 'male' ? 'male' : 'female'
}

function getGender() {
  const stored = wx.getStorageSync(STORAGE_KEY) || ''
  if (stored) return normalizeGender(stored)
  try {
    return normalizeGender(getApp().globalData.voiceGender)
  } catch (err) {
    return 'female'
  }
}

function setGender(gender) {
  const value = normalizeGender(gender)
  wx.setStorageSync(STORAGE_KEY, value)
  try {
    getApp().globalData.voiceGender = value
  } catch (err) {
  }
  return value
}

function provider() {
  return config.VOICE_TTS_PROVIDER || config.VOICE_TTS_PROVIDER_NONE
}

function providerLabel() {
  if (provider() === config.VOICE_TTS_PROVIDER_WECHAT_PLUGIN) return '前端插件'
  if (provider() === config.VOICE_TTS_PROVIDER_SERVER) return '服务端语音'
  return '未启用'
}

function loadPlugin() {
  if (provider() !== config.VOICE_TTS_PROVIDER_WECHAT_PLUGIN) return null
  if (pluginLoaded) return ttsPlugin
  pluginLoaded = true
  try {
    ttsPlugin = requirePlugin('WechatSI')
  } catch (err) {
    ttsPlugin = null
  }
  return ttsPlugin
}

function ensureAudioContext() {
  if (audioContext) return audioContext
  if (!wx.createInnerAudioContext) return null
  audioContext = wx.createInnerAudioContext()
  audioContext.obeyMuteSwitch = false
  audioContext.onError((err) => {
    debugLog.warn('VOICE_PLAY', 'AUDIO_ERROR', {
      errMsg: err && err.errMsg ? err.errMsg : '',
      src: audioContext && audioContext.src ? contentPreview(audioContext.src) : ''
    })
    if (sequenceErrorHandler) {
      const handler = sequenceErrorHandler
      sequenceErrorHandler = null
      try {
        handler(err)
      } catch (handlerErr) {
        debugLog.warn('VOICE_PLAY', 'AUDIO_ERROR_HANDLER_FAIL', {
          error: handlerErr && handlerErr.message ? handlerErr.message : String(handlerErr)
        })
      }
    }
    wx.showToast({ title: '语音播放失败', icon: 'none' })
  })
  return audioContext
}

function trimForTts(text) {
  const normalized = (text || '').replace(/\s+/g, ' ').trim()
  if (normalized.length <= 450) return normalized
  return `${normalized.slice(0, 450)}。`
}

function normalizeTexts(texts) {
  const result = []
  const map = {}
  ;(texts || []).forEach((text) => {
    const content = trimForTts(text)
    if (!content || map[content]) return
    map[content] = true
    result.push(content)
  })
  return result
}

function serverCacheKey(content, voiceGender) {
  return `${normalizeGender(voiceGender)}|${content}`
}

function serverCacheEntry(key) {
  if (!serverAudioCache[key]) {
    serverAudioCache[key] = {
      remoteUrl: '',
      localUrl: '',
      cacheHit: false,
      audioBytes: 0,
      format: '',
      contentType: '',
      audioKey: ''
    }
  }
  return serverAudioCache[key]
}

function playableUrl(entry) {
  return entry && (entry.localUrl || entry.remoteUrl) ? (entry.localUrl || entry.remoteUrl) : ''
}

function isRemoteUrl(url) {
  return /^https?:\/\//i.test(url || '')
}

function estimateTextDurationSeconds(content) {
  const text = trimForTts(content)
  if (!text) return 0
  const cjkCount = (text.match(/[\u3400-\u9fff]/g) || []).length
  const digitCount = (text.match(/[0-9]/g) || []).length
  const asciiCount = (text.match(/[A-Za-z]/g) || []).length
  const punctuationCount = (text.match(/[，。！？；：、,.!?;:]/g) || []).length
  return Math.max(1, cjkCount * 0.27 + digitCount * 0.16 + asciiCount * 0.08 + punctuationCount * 0.18 + 0.4)
}

function canEstimateByWavBytes(entry) {
  if (!entry) return false
  const marker = `${entry.format || ''} ${entry.contentType || ''} ${entry.audioKey || ''} ${entry.remoteUrl || ''}`.toLowerCase()
  if (marker.indexOf('mp3') >= 0 && marker.indexOf('wav') < 0) return false
  return Number(entry.audioBytes) > SERVER_WAV_HEADER_BYTES
}

function estimateEntryDurationSeconds(entry, content, gender) {
  if (canEstimateByWavBytes(entry)) {
    let seconds = (Number(entry.audioBytes) - SERVER_WAV_HEADER_BYTES) / SERVER_WAV_BYTES_PER_SECOND
    const voiceGender = normalizeGender(gender)
    if (voiceGender === 'male') seconds /= 0.95
    if (voiceGender === 'female') seconds /= 1.05
    return Math.max(0.5, seconds + ESTIMATE_AUDIO_BUFFER_SECONDS)
  }
  return estimateTextDurationSeconds(content)
}

function estimatePreparedDurationSeconds(text, gender) {
  const content = trimForTts(text)
  const voiceGender = normalizeGender(gender || getGender())
  if (!content) return 0
  const entry = serverAudioCache[serverCacheKey(content, voiceGender)]
  return estimateEntryDurationSeconds(entry, content, voiceGender)
}

function estimateTextsDurationSeconds(texts, gender) {
  const voiceGender = normalizeGender(gender || getGender())
  return (texts || []).reduce((sum, text) => {
    return sum + estimatePreparedDurationSeconds(text, voiceGender)
  }, 0)
}

function shouldWarmDownload() {
  return config.VOICE_PREFETCH_DOWNLOAD_AUDIO === true
}

function inlineAudioFileName(entry, content, voiceGender) {
  const rawName = entry.audioKey || serverCacheKey(content, voiceGender)
  return `cook_voice_${rawName.replace(/[^0-9A-Za-z_.-]/g, '_')}`
}

function isStorageLimitError(err) {
  const message = err && err.errMsg ? err.errMsg : String(err || '')
  return message.indexOf('storage limit') >= 0 || message.indexOf('maximum size') >= 0
}

function forgetLocalVoiceFile(filePath) {
  if (!filePath) return
  Object.keys(serverAudioCache).forEach((key) => {
    if (serverAudioCache[key] && serverAudioCache[key].localUrl === filePath) {
      serverAudioCache[key].localUrl = ''
    }
  })
}

function cleanVoiceCacheFiles(reason) {
  if (!wx.getFileSystemManager || !wx.env || !wx.env.USER_DATA_PATH) return Promise.resolve(0)
  const fs = wx.getFileSystemManager()
  const rootPath = wx.env.USER_DATA_PATH
  return new Promise((resolve) => {
    fs.readdir({
      dirPath: rootPath,
      success(res) {
        const files = (res && res.files ? res.files : [])
          .filter((fileName) => /^cook_voice_/i.test(fileName || ''))
        if (!files.length) {
          debugLog.warn('VOICE_TTS', 'CACHE_CLEAN_EMPTY', { reason })
          resolve(0)
          return
        }
        let finished = 0
        let removed = 0
        files.forEach((fileName) => {
          const filePath = `${rootPath}/${fileName}`
          fs.unlink({
            filePath,
            success() {
              removed += 1
              forgetLocalVoiceFile(filePath)
            },
            complete() {
              finished += 1
              if (finished >= files.length) {
                debugLog.warn('VOICE_TTS', 'CACHE_CLEAN_DONE', { reason, removed, total: files.length })
                resolve(removed)
              }
            }
          })
        })
      },
      fail(err) {
        debugLog.warn('VOICE_TTS', 'CACHE_CLEAN_FAIL', {
          reason,
          errMsg: err && err.errMsg ? err.errMsg : ''
        })
        resolve(0)
      }
    })
  })
}

function writeInlineAudioFile(entry, audioBase64, content, voiceGender) {
  if (!entry || !audioBase64 || !wx.getFileSystemManager || !wx.env || !wx.env.USER_DATA_PATH) {
    return Promise.resolve(false)
  }
  const filePath = `${wx.env.USER_DATA_PATH}/${inlineAudioFileName(entry, content, voiceGender)}`
  const writeOnce = (cleaned) => new Promise((resolve) => {
    wx.getFileSystemManager().writeFile({
      filePath,
      data: audioBase64,
      encoding: 'base64',
      success() {
        entry.localUrl = filePath
        debugLog.info('VOICE_TTS', 'INLINE_WRITE_OK', {
          gender: voiceGender,
          text: contentPreview(content),
          audioBytes: entry.audioBytes || 0,
          filePath: contentPreview(filePath)
        })
        resolve(true)
      },
      fail(err) {
        debugLog.warn('VOICE_TTS', 'INLINE_WRITE_FAIL', {
          gender: voiceGender,
          text: contentPreview(content),
          errMsg: err && err.errMsg ? err.errMsg : '',
          cleaned: !!cleaned
        })
        if (!cleaned && isStorageLimitError(err)) {
          cleanVoiceCacheFiles('inline_write_storage_limit').then(() => {
            writeOnce(true).then(resolve)
          })
          return
        }
        resolve(false)
      }
    })
  })
  return writeOnce(false)
}

function applyAudioResponseToEntry(entry, data) {
  if (!entry || !data) return entry
  entry.remoteUrl = data.audioUrl || data.url || entry.remoteUrl || ''
  entry.cacheHit = !!data.cacheHit
  entry.audioBytes = Number(data.audioBytes || 0)
  entry.format = data.format ? String(data.format) : ''
  entry.contentType = data.contentType ? String(data.contentType) : ''
  entry.audioKey = data.audioKey ? String(data.audioKey) : ''
  return entry
}

function preparedVoiceFor(content, voiceGender) {
  return preparedVoiceMap[serverCacheKey(content, voiceGender)] || null
}

function updatePreparedRuntimeConfig(data) {
  if (!data) return
  if (Number(data.fetchWaitMilliseconds) > 0) preparedFetchWaitMs = Number(data.fetchWaitMilliseconds)
  if (Number(data.fetchMaxAttempts) > 0) preparedFetchMaxAttempts = Number(data.fetchMaxAttempts)
  if (Number(data.clientConcurrency) > 0) {
    const clientLimit = Number(config.VOICE_PREFETCH_QUEUE_CONCURRENCY || 2)
    preparedFetchConcurrency = Math.max(1, Math.floor(Math.min(Number(data.clientConcurrency), isFinite(clientLimit) && clientLimit > 0 ? clientLimit : 2)))
  }
}

function rememberPreparedVoices(items, gender) {
  const voiceGender = normalizeGender(gender || getGender())
  ;(items || []).forEach((item) => {
    const content = trimForTts(item && item.text)
    const voiceId = item && item.voiceId ? String(item.voiceId) : ''
    if (!content || !voiceId) return
    const key = serverCacheKey(content, voiceGender)
    const info = {
      voiceId,
      text: content,
      gender: voiceGender,
      required: !!item.required,
      order: Number(item.order || 0)
    }
    preparedVoiceMap[key] = info
    preparedVoiceIdMap[voiceId] = info
    if (item && (item.audioUrl || item.url || item.audioKey)) {
      const entry = serverCacheEntry(key)
      applyAudioResponseToEntry(entry, item)
      delete serverAudioUrlPending[key]
      delete serverAudioUrlPendingInline[key]
      delete serverAudioUrlPendingStartedAt[key]
      debugLog.info('VOICE_PREPARE', 'ITEM_AUDIO_READY', {
        gender: voiceGender,
        text: contentPreview(content),
        voiceId,
        audioBytes: entry.audioBytes || 0,
        localFile: !!entry.localUrl
      })
    }
  })
}

function preparePlanVoiceTexts(texts, gender, requiredCount, planId, options) {
  const contents = normalizeTexts(texts)
  const voiceGender = normalizeGender(gender || getGender())
  if (!contents.length || provider() !== config.VOICE_TTS_PROVIDER_SERVER) return Promise.resolve({ ok: true, items: [] })
  debugLog.info('VOICE_PREPARE', 'REQUEST_START', {
    gender: voiceGender,
    count: contents.length,
    requiredCount,
    planId: planId || '',
    texts: contents.map((content) => contentPreview(content))
  })
  return api.voicePrepare(contents, voiceGender, requiredCount, planId, options || {})
    .then((data) => {
      updatePreparedRuntimeConfig(data)
      rememberPreparedVoices(data && data.items, voiceGender)
      debugLog.info('VOICE_PREPARE', 'REQUEST_OK', {
        gender: voiceGender,
        count: data && data.items ? data.items.length : 0,
        requiredCount: data && data.requiredCount,
        requiredAudioIncluded: !!(data && data.requiredAudioIncluded),
        requiredAudioReadyCount: Number(data && data.requiredAudioReadyCount ? data.requiredAudioReadyCount : 0),
        backgroundCount: data && data.backgroundVoiceIds ? data.backgroundVoiceIds.length : 0,
        concurrency: preparedFetchConcurrency,
        fetchWaitMilliseconds: preparedFetchWaitMs,
        fetchMaxAttempts: preparedFetchMaxAttempts
      })
      return data
    })
}

function clearSequenceEndedHandler(audio, handlerId) {
  if (handlerId && activeSequenceHandlerId && activeSequenceHandlerId !== handlerId) return
  if (audio && sequenceEndedHandler && audio.offEnded) {
    audio.offEnded(sequenceEndedHandler)
  }
  sequenceEndedHandler = null
  sequenceErrorHandler = null
  activeSequenceHandlerId = 0
}

function applyPlaybackRate(audio, gender) {
  if (audio.playbackRate !== undefined) {
    audio.playbackRate = normalizeGender(gender) === 'male' ? 0.95 : 1.05
  }
}

function stop() {
  if (audioContext) {
    clearSequenceEndedHandler(audioContext)
    audioContext.stop()
  }
}

function playAudioUrl(url, gender, currentToken) {
  const audio = ensureAudioContext()
  if (!url || !audio) return false
  if (currentToken !== speakToken) return false
  clearSequenceEndedHandler(audio)
  audio.stop()
  audio.src = url
  applyPlaybackRate(audio, gender)
  debugLog.info('VOICE_PLAY', 'PLAY_URL', {
    gender,
    localFile: !isRemoteUrl(url),
    url: contentPreview(url)
  })
  audio.play()
  return true
}

function speakByWechatPlugin(content, voiceGender, currentToken) {
  const plugin = loadPlugin()
  if (!content || !plugin || !plugin.textToSpeech) {
    return Promise.resolve(false)
  }

  return new Promise((resolve) => {
    plugin.textToSpeech({
      lang: 'zh_CN',
      tts: true,
      content,
      speaker: voiceGender === 'male' ? '1' : '0',
      voiceType: voiceGender === 'male' ? 1 : 0,
      success(res) {
        if (currentToken !== speakToken) {
          resolve(false)
          return
        }
        if (!res || (res.retcode !== undefined && res.retcode !== 0) || !res.filename) {
          resolve(false)
          return
        }
        resolve(playAudioUrl(res.filename, voiceGender, currentToken))
      },
      fail() {
        resolve(false)
      }
    })
  })
}

function delay(milliseconds) {
  return new Promise((resolve) => {
    setTimeout(resolve, Math.max(1, milliseconds || 1))
  })
}

function requestServerAudioWithRetry(content, voiceGender, attemptIndex, options) {
  return api.voiceTts(content, voiceGender, options)
}

function requestPreparedAudioWithRetry(content, voiceGender, prepared, options, attemptIndex) {
  const inlineAudio = !!(options && options.inlineAudio)
  const rawWaitMilliseconds = options && options.waitMilliseconds !== undefined
    ? Number(options.waitMilliseconds)
    : Number(preparedFetchWaitMs || config.VOICE_FETCH_WAIT_MS || 30000)
  const waitMilliseconds = Math.max(0, rawWaitMilliseconds)
  const maxAttempts = Math.max(1, Number((options && options.maxAttempts) || preparedFetchMaxAttempts || config.VOICE_FETCH_MAX_ATTEMPTS || 3))
  const timeout = options && options.timeout !== undefined
    ? Number(options.timeout)
    : Math.max(config.VOICE_TTS_REQUEST_TIMEOUT_MS || 30000, waitMilliseconds + 1000)
  return api.voiceFetch(prepared.voiceId, {
    inlineAudio,
    waitMilliseconds,
    timeout,
    maxAttempts,
    retryDelayMs: options && options.retryDelayMs !== undefined ? Number(options.retryDelayMs) : undefined,
    purpose: options && options.purpose ? options.purpose : ''
  })
    .then((data) => {
      if (data && data.ready && (data.audioUrl || data.url)) {
        return data
      }
      if (data && data.ok && !data.ready && attemptIndex + 1 < maxAttempts) {
        debugLog.warn('VOICE_TTS', 'PREPARED_PENDING_RETRY', {
          gender: voiceGender,
          voiceId: prepared.voiceId,
          attempt: attemptIndex + 1,
          maxAttempts,
          status: data.status || '',
          text: contentPreview(content)
        })
        return delay(200).then(() => requestPreparedAudioWithRetry(content, voiceGender, prepared, options, attemptIndex + 1))
      }
      throw new Error(data && data.message ? data.message : '语音还没有准备好')
    })
    .catch((err) => {
      if (options && options.optional) throw err
      if (attemptIndex + 1 >= maxAttempts) throw err
      debugLog.warn('VOICE_TTS', 'PREPARED_REQUEST_RETRY', {
        gender: voiceGender,
        voiceId: prepared.voiceId,
        attempt: attemptIndex + 1,
        maxAttempts,
        text: contentPreview(content),
        error: err && err.message ? err.message : String(err)
      })
      return delay(TTS_RETRY_DELAYS[Math.min(attemptIndex, TTS_RETRY_DELAYS.length - 1)] || 900)
        .then(() => requestPreparedAudioWithRetry(content, voiceGender, prepared, options, attemptIndex + 1))
    })
}

function requestServerAudioEntry(content, voiceGender, options) {
  if (!content || !config.VOICE_SERVER_TTS_PATH) return Promise.resolve(null)
  const key = serverCacheKey(content, voiceGender)
  const wantInlineAudio = !!(options && options.inlineAudio)
  const allowPendingReuse = !(options && options.allowPendingReuse === false)
  const maxPendingReuseMs = options && options.maxPendingReuseMs !== undefined ? Number(options.maxPendingReuseMs) : 0
  const forceDirect = !!(options && options.direct === true)
  const prepared = preparedVoiceFor(content, voiceGender)
  const cached = serverAudioCache[key]
  if (cached && playableUrl(cached)) {
    if (wantInlineAudio && !cached.localUrl) {
      debugLog.info('VOICE_TTS', 'CACHE_INLINE_REFRESH', {
        gender: voiceGender,
        text: contentPreview(content),
        audioBytes: cached.audioBytes || 0
      })
    } else {
      debugLog.info('VOICE_TTS', 'CACHE_HIT', {
        gender: voiceGender,
        text: contentPreview(content),
        audioBytes: cached.audioBytes || 0,
        estimatedDurationSeconds: Number(estimateEntryDurationSeconds(cached, content, voiceGender).toFixed(2)),
        localFile: !!cached.localUrl
      })
      return Promise.resolve(cached)
    }
  }
  if (serverAudioUrlPending[key]) {
    const pendingAgeMs = Date.now() - Number(serverAudioUrlPendingStartedAt[key] || 0)
    if (wantInlineAudio && !serverAudioUrlPendingInline[key]) {
      debugLog.info('VOICE_TTS', 'PENDING_INLINE_REFRESH', { gender: voiceGender, text: contentPreview(content) })
    } else if (allowPendingReuse && (!maxPendingReuseMs || pendingAgeMs <= maxPendingReuseMs)) {
      debugLog.info('VOICE_TTS', 'PENDING_REUSE', { gender: voiceGender, text: contentPreview(content), pendingAgeMs })
      return serverAudioUrlPending[key]
    } else {
      debugLog.warn('VOICE_TTS', 'PENDING_BYPASS', { gender: voiceGender, text: contentPreview(content), pendingAgeMs })
    }
  }

  debugLog.info('VOICE_TTS', 'REQUEST_START', {
    gender: voiceGender,
    text: contentPreview(content),
    purpose: options && options.purpose ? options.purpose : '',
    optional: !!(options && options.optional),
    inlineAudio: wantInlineAudio,
    prepared: !!prepared && !forceDirect,
    voiceId: prepared && !forceDirect && prepared.voiceId ? prepared.voiceId : ''
  })
  const requestOptions = Object.assign({}, options || {})
  delete requestOptions.allowPendingReuse
  delete requestOptions.maxPendingReuseMs
  delete requestOptions.direct
  if (!requestOptions.purpose) requestOptions.purpose = prepared && !forceDirect ? 'voice_fetch' : 'voice_tts'
  const requestPurpose = requestOptions.purpose || ''
  const optionalRequest = requestOptions.optional === true
  if (wantInlineAudio) requestOptions.inlineAudio = true
  serverAudioUrlPendingInline[key] = wantInlineAudio
  const requestPromise = prepared && !forceDirect
    ? requestPreparedAudioWithRetry(content, voiceGender, prepared, requestOptions, 0)
    : requestServerAudioWithRetry(content, voiceGender, 0, requestOptions)
  const pendingPromise = requestPromise
    .then((data) => {
      const url = data && data.ok ? (data.audioUrl || data.url || '') : ''
      if (serverAudioUrlPending[key] === pendingPromise) {
        delete serverAudioUrlPending[key]
        delete serverAudioUrlPendingInline[key]
        delete serverAudioUrlPendingStartedAt[key]
      }
      if (!url) {
        debugLog.warn('VOICE_TTS', 'REQUEST_NO_AUDIO', { gender: voiceGender, text: contentPreview(content), data })
        return null
      }
      const entry = serverCacheEntry(key)
      applyAudioResponseToEntry(entry, data)
      const finishRequest = () => {
        debugLog.info('VOICE_TTS', 'REQUEST_OK', {
          gender: voiceGender,
          text: contentPreview(content),
          purpose: requestPurpose,
          optional: optionalRequest,
          cacheHit: entry.cacheHit,
          audioBytes: entry.audioBytes,
          audioInline: !!(data && data.audioBase64),
          prepared: !!prepared && !forceDirect,
          voiceId: prepared && !forceDirect && prepared.voiceId ? prepared.voiceId : '',
          estimatedDurationSeconds: Number(estimateEntryDurationSeconds(entry, content, voiceGender).toFixed(2)),
          localFile: !!entry.localUrl,
          preparedOnly: !!prepared && !forceDirect
        })
        if (shouldWarmDownload()) {
          warmDownloadServerAudio(content, voiceGender)
        }
        return entry
      }
      if (data && data.audioBase64) {
        return writeInlineAudioFile(entry, data.audioBase64, content, voiceGender)
          .then(() => finishRequest())
      }
      return finishRequest()
    })
    .catch((err) => {
      if (serverAudioUrlPending[key] === pendingPromise) {
        delete serverAudioUrlPending[key]
        delete serverAudioUrlPendingInline[key]
        delete serverAudioUrlPendingStartedAt[key]
      }
      const failDetail = {
        gender: voiceGender,
        text: contentPreview(content),
        purpose: requestPurpose,
        optional: optionalRequest,
        error: err && err.message ? err.message : String(err)
      }
      if (optionalRequest) {
        debugLog.warn('VOICE_TTS', 'REQUEST_FAIL_OPTIONAL', failDetail)
      } else {
        debugLog.error('VOICE_TTS', 'REQUEST_FAIL', failDetail)
      }
      return null
    })
  serverAudioUrlPending[key] = pendingPromise
  serverAudioUrlPendingStartedAt[key] = Date.now()
  return pendingPromise
}

function warmDownloadServerAudio(content, voiceGender) {
  const key = serverCacheKey(content, voiceGender)
  const entry = serverCacheEntry(key)
  const url = entry.remoteUrl
  if (!url || !isRemoteUrl(url) || !wx.downloadFile) return Promise.resolve(playableUrl(entry))
  if (entry.localUrl) return Promise.resolve(entry.localUrl)
  if (serverAudioDownloadPending[key]) return serverAudioDownloadPending[key]

  debugLog.info('VOICE_TTS', 'DOWNLOAD_START', { gender: voiceGender, text: contentPreview(content) })
  serverAudioDownloadPending[key] = new Promise((resolve) => {
    const timeout = Number(config.VOICE_AUDIO_DOWNLOAD_TIMEOUT_MS) || 8000
    wx.downloadFile({
      url,
      timeout,
      success(res) {
        if (res.statusCode >= 200 && res.statusCode < 300 && res.tempFilePath) {
          entry.localUrl = res.tempFilePath
          debugLog.info('VOICE_TTS', 'DOWNLOAD_OK', { gender: voiceGender, text: contentPreview(content), statusCode: res.statusCode })
          resolve(entry.localUrl)
          return
        }
        debugLog.warn('VOICE_TTS', 'DOWNLOAD_HTTP_ERROR', { gender: voiceGender, text: contentPreview(content), statusCode: res.statusCode })
        resolve(playableUrl(entry))
      },
      fail(err) {
        debugLog.warn('VOICE_TTS', 'DOWNLOAD_FAIL', {
          gender: voiceGender,
          text: contentPreview(content),
          errMsg: err && err.errMsg ? err.errMsg : ''
        })
        resolve(playableUrl(entry))
      },
      complete() {
        delete serverAudioDownloadPending[key]
      }
    })
  })
  return serverAudioDownloadPending[key]
}

function timeoutPromise(timeoutMs) {
  return new Promise((resolve) => {
    setTimeout(() => resolve(false), Math.max(1, timeoutMs || 1))
  })
}

function withTimeout(promise, timeoutMs) {
  if (!timeoutMs || timeoutMs <= 0) return promise
  return Promise.race([promise, timeoutPromise(timeoutMs)])
}

function entryWithTimeout(promise, timeoutMs) {
  if (!timeoutMs || timeoutMs <= 0) return promise
  return new Promise((resolve) => {
    let settled = false
    const timer = setTimeout(() => {
      if (settled) return
      settled = true
      resolve(null)
    }, timeoutMs)
    promise
      .then((entry) => {
        if (settled) return
        settled = true
        clearTimeout(timer)
        resolve(entry)
      })
      .catch(() => {
        if (settled) return
        settled = true
        clearTimeout(timer)
        resolve(null)
      })
  })
}

function playSequenceEntry(audio, entry, content, voiceGender, currentToken, index, source) {
  const url = playableUrl(entry)
  if (!url || !audio || currentToken !== speakToken) return Promise.resolve(false)
  return new Promise((resolve) => {
    let finished = false
    let timer = null
    const handlerId = ++sequenceHandlerId
    const durationSeconds = estimateEntryDurationSeconds(entry, content, voiceGender)
    const isRestSequence = (source || '').indexOf('rest_') === 0
    const timeoutPaddingMs = isRestSequence
      ? Number(config.VOICE_REST_SEQUENCE_TIMEOUT_PADDING_MS || 1200)
      : Number(config.VOICE_SEQUENCE_TIMEOUT_PADDING_MS || 4000)
    const timeoutMs = Math.max(2500, Math.ceil(durationSeconds * 1000) + Math.max(500, timeoutPaddingMs))
    const finish = (ok, reason) => {
      if (finished) return
      finished = true
      if (timer) clearTimeout(timer)
      clearSequenceEndedHandler(audio, handlerId)
      debugLog.info('VOICE_PLAY', 'PLAY_SEQUENCE_DONE', {
        gender: voiceGender,
        index,
        source: source || '',
        reason,
        text: contentPreview(content)
      })
      resolve(ok)
    }
    timer = setTimeout(() => {
      finish(currentToken === speakToken, currentToken === speakToken ? 'timeout_guard' : 'cancelled_timeout')
    }, timeoutMs)
    clearSequenceEndedHandler(audio)
    audio.stop()
    audio.src = url
    applyPlaybackRate(audio, voiceGender)
    activeSequenceHandlerId = handlerId
    sequenceEndedHandler = () => {
      if (activeSequenceHandlerId !== handlerId) return
      finish(true, 'ended')
    }
    sequenceErrorHandler = (err) => {
      if (activeSequenceHandlerId !== handlerId) return
      debugLog.warn('VOICE_PLAY', 'PLAY_SEQUENCE_ERROR', {
        gender: voiceGender,
        index,
        source: source || '',
        text: contentPreview(content),
        errMsg: err && err.errMsg ? err.errMsg : ''
      })
      finish(false, 'error')
    }
    audio.onEnded(sequenceEndedHandler)
    debugLog.info('VOICE_PLAY', 'PLAY_SEQUENCE_ITEM', {
      gender: voiceGender,
      index,
      source: source || '',
      localFile: !isRemoteUrl(url),
      timeoutMs,
      text: contentPreview(content)
    })
    audio.play()
  })
}

function requestServerAudioBatch(contents, voiceGender, concurrency, options) {
  const limit = Math.max(1, Math.floor(concurrency || preparedFetchConcurrency || 2))
  const results = new Array(contents.length)
  let nextIndex = 0
  let runningCount = 0
  let completedCount = 0

  return new Promise((resolve) => {
    if (!contents.length) {
      resolve(results)
      return
    }
    const pump = () => {
      while (runningCount < limit && nextIndex < contents.length) {
        const index = nextIndex
        nextIndex += 1
        runningCount += 1
        requestServerAudioEntry(contents[index], voiceGender, options)
          .then((entry) => {
            results[index] = playableUrl(entry)
          })
          .catch(() => {
            results[index] = ''
          })
          .then(() => {
            runningCount -= 1
            completedCount += 1
            if (completedCount >= contents.length) {
              resolve(results)
              return
            }
            pump()
          })
      }
    }
    pump()
  })
}

function hasCache(text, gender) {
  const content = trimForTts(text)
  const voiceGender = normalizeGender(gender || getGender())
  if (!content || provider() !== config.VOICE_TTS_PROVIDER_SERVER) return false
  return !!playableUrl(serverAudioCache[serverCacheKey(content, voiceGender)])
}

function speakCachedByServer(content, voiceGender, currentToken) {
  return requestServerAudioEntry(content, voiceGender, { direct: true, maxPendingReuseMs: 2000 })
    .then((entry) => playAudioUrl(playableUrl(entry), voiceGender, currentToken))
    .catch(() => false)
}

function speakSequenceByServer(contents, voiceGender, currentToken, options) {
  const audio = ensureAudioContext()
  if (!audio || !contents.length) return Promise.resolve(false)

  const urlRequests = contents.map((content) => requestServerAudioEntry(content, voiceGender, { direct: true, maxPendingReuseMs: 2000 }))

  return new Promise((resolve) => {
    let playedAny = false
    const finish = (result) => {
      if (currentToken === speakToken) clearSequenceEndedHandler(audio)
      resolve(result)
    }
    const playNext = (index) => {
      if (currentToken !== speakToken) {
        finish(false)
        return
      }
      if (index >= urlRequests.length) {
        finish(playedAny)
        return
      }
      urlRequests[index]
        .then((entry) => {
          if (currentToken !== speakToken) {
            finish(false)
            return
          }
          if (!playableUrl(entry)) {
            playNext(index + 1)
            return
          }
          if (options && options.onItemStart) {
            try {
              options.onItemStart(index, contents[index])
            } catch (err) {
              debugLog.warn('VOICE_PLAY', 'SEQUENCE_CALLBACK_FAIL', {
                index,
                error: err && err.message ? err.message : String(err)
              })
            }
          }
          playSequenceEntry(audio, entry, contents[index], voiceGender, currentToken, index, 'normal')
            .then((ok) => {
              playedAny = playedAny || ok
              playNext(index + 1)
            })
        })
        .catch(() => playNext(index + 1))
    }
    playNext(0)
  })
}

function requestRestFixedAudioEntry(content, voiceGender) {
  const text = trimForTts(content)
  const gender = normalizeGender(voiceGender)
  if (!text) return Promise.resolve(null)
  const key = serverCacheKey(text, gender)
  const cached = serverAudioCache[key]
  if (playableUrl(cached)) {
    debugLog.info('VOICE_REST', 'FIXED_CACHE_HIT', {
      gender,
      text: contentPreview(text),
      localFile: !!cached.localUrl
    })
    if (!cached.localUrl && config.VOICE_REST_DURATION_WARM_DOWNLOAD !== false) {
      const waitMs = Math.max(0, Number(config.VOICE_REST_FIXED_LOCAL_WAIT_MS || 1200))
      debugLog.info('VOICE_REST', 'FIXED_WARM_DOWNLOAD', {
        gender,
        text: contentPreview(text),
        waitMs
      })
      return entryWithTimeout(
        warmDownloadServerAudio(text, gender).then(() => serverAudioCache[key] || cached),
        waitMs
      ).then((entry) => {
        const result = entry || cached
        debugLog.info('VOICE_REST', 'FIXED_WARM_DOWNLOAD_DONE', {
          gender,
          text: contentPreview(text),
          localFile: !!(result && result.localUrl),
          ready: !!playableUrl(result)
        })
        return result
      })
    }
    return Promise.resolve(cached)
  }
  return requestServerAudioEntry(text, gender, {
    maxPendingReuseMs: 1000,
    waitMilliseconds: 0,
    timeout: config.VOICE_REST_FIXED_FETCH_TIMEOUT_MS || 2000,
    maxAttempts: 1,
    retryDelayMs: 0,
    purpose: 'rest_fixed'
  }).then((entry) => {
    if (!entry || entry.localUrl || !playableUrl(entry) || config.VOICE_REST_DURATION_WARM_DOWNLOAD === false) return entry
    const waitMs = Math.max(0, Number(config.VOICE_REST_FIXED_LOCAL_WAIT_MS || 1200))
    return entryWithTimeout(
      warmDownloadServerAudio(text, gender).then(() => serverAudioCache[key] || entry),
      waitMs
    ).then((localEntry) => localEntry || entry)
  })
}

function requestRestDurationAudioEntry(content, voiceGender) {
  const text = trimForTts(content)
  const gender = normalizeGender(voiceGender)
  if (!text) return Promise.resolve(null)
  const key = serverCacheKey(text, gender)
  const cached = serverAudioCache[key]
  if (cached && playableUrl(cached)) {
    debugLog.info('VOICE_REST', 'DURATION_CACHE_HIT', {
      gender,
      text: contentPreview(text),
      localFile: !!cached.localUrl,
      audioBytes: cached.audioBytes || 0
    })
    if (!cached.localUrl && config.VOICE_REST_DURATION_WARM_DOWNLOAD !== false) {
      const waitMs = Math.max(0, Number(config.VOICE_REST_DURATION_LOCAL_WAIT_MS || 1200))
      return entryWithTimeout(
        warmDownloadServerAudio(text, gender).then(() => serverAudioCache[key] || cached),
        waitMs
      ).then((entry) => entry || cached)
    }
    return Promise.resolve(cached)
  }
  const pendingAgeMs = Date.now() - Number(restDurationAudioPendingStartedAt[key] || 0)
  if (restDurationAudioPending[key] && pendingAgeMs <= 45000) {
    debugLog.info('VOICE_REST', 'DURATION_PENDING_REUSE', {
      gender,
      text: contentPreview(text),
      pendingAgeMs
    })
    return restDurationAudioPending[key]
  }

  delete serverAudioUrlPending[key]
  delete serverAudioUrlPendingInline[key]
  delete serverAudioUrlPendingStartedAt[key]

  const startedAt = Date.now()
  const inlineAudio = config.VOICE_REST_DURATION_INLINE_AUDIO === true
  debugLog.info('VOICE_REST', 'DURATION_REQUEST_START', {
    gender,
    text: contentPreview(text),
    mode: inlineAudio ? 'direct_inline' : 'direct_url'
  })
  const request = api.voiceTts(text, gender, {
    inlineAudio,
    timeout: config.VOICE_REST_DURATION_REQUEST_TIMEOUT_MS || config.VOICE_TTS_REQUEST_TIMEOUT_MS || 30000,
    maxAttempts: config.VOICE_REST_DURATION_MAX_ATTEMPTS || config.VOICE_TTS_MAX_ATTEMPTS || 3,
    retryDelayMs: config.VOICE_REST_DURATION_RETRY_DELAY_MS !== undefined ? config.VOICE_REST_DURATION_RETRY_DELAY_MS : 200,
    purpose: 'rest_duration'
  })
    .then((data) => {
      const url = data && data.ok ? (data.audioUrl || data.url || '') : ''
      if (!url) {
        debugLog.warn('VOICE_REST', 'DURATION_NO_AUDIO', {
          gender,
          text: contentPreview(text),
          costMs: Date.now() - startedAt
        })
        return null
      }
      const entry = serverCacheEntry(key)
      applyAudioResponseToEntry(entry, data)
      if (data && data.audioBase64) {
        return writeInlineAudioFile(entry, data.audioBase64, text, gender).then(() => entry)
      }
      if (config.VOICE_REST_DURATION_WARM_DOWNLOAD !== false) {
        const waitMs = Math.max(0, Number(config.VOICE_REST_DURATION_LOCAL_WAIT_MS || 1200))
        debugLog.info('VOICE_REST', 'DURATION_WARM_DOWNLOAD', {
          gender,
          text: contentPreview(text),
          waitMs
        })
        return entryWithTimeout(
          warmDownloadServerAudio(text, gender).then(() => entry),
          waitMs
        ).then((localEntry) => {
          const result = localEntry || entry
          debugLog.info('VOICE_REST', inlineAudio ? 'DURATION_NO_INLINE_AUDIO' : 'DURATION_URL_READY', {
            gender,
            text: contentPreview(text),
            costMs: Date.now() - startedAt,
            hasRemoteUrl: !!result.remoteUrl,
            localFile: !!result.localUrl
          })
          return result
        })
      }
      debugLog.info('VOICE_REST', inlineAudio ? 'DURATION_NO_INLINE_AUDIO' : 'DURATION_URL_READY', {
        gender,
        text: contentPreview(text),
        costMs: Date.now() - startedAt,
        hasRemoteUrl: !!entry.remoteUrl,
        localFile: !!entry.localUrl
      })
      return entry
    })
    .then((entry) => {
      delete restDurationAudioPending[key]
      delete restDurationAudioPendingStartedAt[key]
      debugLog.info('VOICE_REST', 'DURATION_REQUEST_DONE', {
        gender,
        text: contentPreview(text),
        ready: !!playableUrl(entry),
        localFile: !!(entry && entry.localUrl),
        remoteFallback: !!(entry && !entry.localUrl && entry.remoteUrl),
        audioBytes: entry && entry.audioBytes ? entry.audioBytes : 0,
        costMs: Date.now() - startedAt
      })
      return entry
    })
    .catch((err) => {
      delete restDurationAudioPending[key]
      delete restDurationAudioPendingStartedAt[key]
      debugLog.warn('VOICE_REST', 'DURATION_REQUEST_FAIL', {
        gender,
        text: contentPreview(text),
        error: err && err.message ? err.message : String(err),
        costMs: Date.now() - startedAt
      })
      return null
    })
  restDurationAudioPending[key] = request
  restDurationAudioPendingStartedAt[key] = Date.now()
  return request
}

function speakRestSequenceByServer(fixedTexts, durationText, voiceGender, currentToken) {
  const audio = ensureAudioContext()
  const fixedContents = (fixedTexts || []).map((text) => trimForTts(text)).filter((text) => !!text)
  const durationContent = trimForTts(durationText)
  if (!audio || (!fixedContents.length && !durationContent)) return Promise.resolve(false)

  const durationPromise = durationContent
    ? requestRestDurationAudioEntry(durationContent, voiceGender)
    : Promise.resolve(null)

  debugLog.info('VOICE_REST', 'SEQUENCE_START', {
    gender: voiceGender,
    fixedCount: fixedContents.length,
    durationText: contentPreview(durationContent)
  })
  return new Promise((resolve) => {
    let playedAny = false
    const finish = (result, reason) => {
      if (currentToken === speakToken) clearSequenceEndedHandler(audio)
      debugLog.info('VOICE_REST', 'SEQUENCE_DONE', {
        gender: voiceGender,
        reason,
        playedAny: !!result,
        durationText: contentPreview(durationContent)
      })
      resolve(result)
    }
    const playFixed = (index) => {
      if (currentToken !== speakToken) {
        finish(false, 'cancelled')
        return
      }
      if (index >= fixedContents.length) {
        playDuration()
        return
      }
      requestRestFixedAudioEntry(fixedContents[index], voiceGender)
        .then((entry) => {
          if (currentToken !== speakToken) {
            finish(false, 'cancelled')
            return
          }
          if (!playableUrl(entry)) {
            debugLog.warn('VOICE_REST', 'FIXED_NOT_READY', {
              gender: voiceGender,
              index,
              text: contentPreview(fixedContents[index])
            })
            playFixed(index + 1)
            return
          }
          playSequenceEntry(audio, entry, fixedContents[index], voiceGender, currentToken, index, 'rest_fixed')
            .then((ok) => {
              playedAny = playedAny || ok
              playFixed(index + 1)
            })
        })
        .catch(() => playFixed(index + 1))
    }
    const playDuration = () => {
      if (!durationContent) {
        finish(playedAny, 'no_duration')
        return
      }
      const graceMs = Math.max(0, Number(config.VOICE_REST_DURATION_GRACE_MS || 2500))
      entryWithTimeout(durationPromise, graceMs)
        .then((entry) => {
          if (currentToken !== speakToken) {
            finish(false, 'cancelled')
            return
          }
          if (!entry || !playableUrl(entry)) {
            debugLog.warn('VOICE_REST', 'DURATION_NOT_READY', {
              gender: voiceGender,
              text: contentPreview(durationContent),
              graceMs,
              hasRemoteUrl: !!(entry && entry.remoteUrl)
            })
            durationPromise.then((lateEntry) => {
              debugLog.info('VOICE_REST', 'DURATION_LATE_RESULT', {
                gender: voiceGender,
                text: contentPreview(durationContent),
                ready: !!playableUrl(lateEntry),
                localFile: !!(lateEntry && lateEntry.localUrl)
              })
              if (currentToken === speakToken && playableUrl(lateEntry)) {
                debugLog.info('VOICE_REST', 'DURATION_LATE_PLAY', {
                  gender: voiceGender,
                  text: contentPreview(durationContent),
                  localFile: !!lateEntry.localUrl
                })
                playSequenceEntry(ensureAudioContext(), lateEntry, durationContent, voiceGender, currentToken, fixedContents.length, 'rest_duration_late')
              }
            }).catch(() => {})
            finish(playedAny, 'duration_waiting_late')
            return
          }
          playSequenceEntry(audio, entry, durationContent, voiceGender, currentToken, fixedContents.length, 'rest_duration')
            .then((ok) => finish(playedAny || ok, ok ? 'ended' : 'duration_error'))
        })
    }
    playFixed(0)
  })
}

function speak(text, gender) {
  const content = trimForTts(text)
  const voiceGender = normalizeGender(gender || getGender())
  const currentToken = ++speakToken
  if (!content) return Promise.resolve(false)
  debugLog.info('VOICE_PLAY', 'SPEAK', { gender: voiceGender, text: contentPreview(content) })
  if (provider() === config.VOICE_TTS_PROVIDER_WECHAT_PLUGIN) {
    return speakByWechatPlugin(content, voiceGender, currentToken)
  }
  if (provider() === config.VOICE_TTS_PROVIDER_SERVER) {
    return speakCachedByServer(content, voiceGender, currentToken)
  }
  return Promise.resolve(false)
}

function speakSequence(texts, gender, options) {
  const contents = (texts || []).map((text) => trimForTts(text)).filter((text) => !!text)
  const voiceGender = normalizeGender(gender || getGender())
  const currentToken = ++speakToken
  if (!contents.length) return Promise.resolve(false)
  debugLog.info('VOICE_PLAY', 'SPEAK_SEQUENCE', {
    gender: voiceGender,
    count: contents.length,
    texts: contents.map((content) => contentPreview(content))
  })
  if (provider() === config.VOICE_TTS_PROVIDER_WECHAT_PLUGIN) {
    return speakByWechatPlugin(contents.join(''), voiceGender, currentToken)
  }
  if (provider() === config.VOICE_TTS_PROVIDER_SERVER) {
    return speakSequenceByServer(contents, voiceGender, currentToken, options)
  }
  return Promise.resolve(false)
}

function prefetch(text, gender, options) {
  const content = trimForTts(text)
  const voiceGender = normalizeGender(gender || getGender())
  if (!content) return Promise.resolve(false)
  const download = !!(options && options.download)
  const warmDownload = !!(options && options.warmDownload)
  debugLog.info('VOICE_PREFETCH', 'DIRECT', { gender: voiceGender, text: contentPreview(content), download, warmDownload })
  if (provider() === config.VOICE_TTS_PROVIDER_SERVER) {
    return requestServerAudioEntry(content, voiceGender, {
      inlineAudio: download,
      direct: download,
      allowPendingReuse: !download
    }).then((entry) => {
      if (download) {
        if (entry && entry.localUrl) return true
        return warmDownloadServerAudio(content, voiceGender).then((url) => !!url)
      }
      if (warmDownload) {
        warmDownloadServerAudio(content, voiceGender)
      }
      return !!playableUrl(entry)
    })
  }
  return Promise.resolve(false)
}

function prefetchRequired(texts, gender, options) {
  const contents = normalizeTexts(texts)
  const voiceGender = normalizeGender(gender || getGender())
  const timeoutMs = options && options.timeoutMs ? options.timeoutMs : (config.VOICE_REQUIRED_PREFETCH_TIMEOUT_MS || 15000)
  const concurrency = options && options.concurrency ? options.concurrency : (preparedFetchConcurrency || 2)
  if (!contents.length || provider() !== config.VOICE_TTS_PROVIDER_SERVER) return Promise.resolve(true)
  debugLog.info('VOICE_PREFETCH', 'REQUIRED_START', {
    gender: voiceGender,
    count: contents.length,
    concurrency,
    mode: 'url_only',
    texts: contents.map((content) => contentPreview(content))
  })
  if (contents.every((content) => hasCache(content, voiceGender))) {
    debugLog.info('VOICE_PREFETCH', 'REQUIRED_CACHE_READY', {
      gender: voiceGender,
      count: contents.length
    })
    return Promise.resolve(true)
  }
  const request = requestServerAudioBatch(contents, voiceGender, concurrency, {
    purpose: options && options.purpose ? options.purpose : 'required_voice'
  })
    .then((urls) => urls.every((url) => !!url))
    .catch(() => false)
  return withTimeout(request, timeoutMs).then((ok) => {
    if (!ok) throw new Error('语音准备失败')
    return true
  })
}

function prefetchQueueConcurrency() {
  const value = Number(config.VOICE_BACKGROUND_PREFETCH_CONCURRENCY || preparedFetchConcurrency || config.VOICE_PREFETCH_QUEUE_CONCURRENCY)
  if (isFinite(value) && value > 0) return Math.max(1, Math.floor(value))
  return DEFAULT_PREFETCH_QUEUE_CONCURRENCY
}

function runPrefetchQueue() {
  const limit = prefetchQueueConcurrency()
  while (prefetchQueueRunningCount < limit && prefetchQueue.length) {
    const item = prefetchQueue.shift()
    const timeout = config.VOICE_BACKGROUND_FETCH_TIMEOUT_MS || 15000
    const maxAttempts = config.VOICE_BACKGROUND_FETCH_MAX_ATTEMPTS || 2
    prefetchQueueRunningCount += 1
    debugLog.info('VOICE_PREFETCH', 'QUEUE_ITEM_START', {
      gender: item.gender,
      text: contentPreview(item.content),
      timeout,
      maxAttempts,
      concurrency: limit,
      runningCount: prefetchQueueRunningCount,
      queuedCount: prefetchQueue.length
    })
    requestServerAudioEntry(item.content, item.gender, {
      waitMilliseconds: config.VOICE_BACKGROUND_FETCH_WAIT_MS,
      timeout,
      maxAttempts,
      retryDelayMs: 0,
      purpose: 'background_prefetch',
      optional: true
    })
      .then((entry) => {
        delete queuedPrefetchMap[item.key]
        if (playableUrl(entry)) {
          debugLog.info('VOICE_PREFETCH', 'QUEUE_ITEM_READY', {
            gender: item.gender,
            text: contentPreview(item.content),
            runningCount: prefetchQueueRunningCount,
            queuedCount: prefetchQueue.length
          })
        } else {
          debugLog.warn('VOICE_PREFETCH', 'QUEUE_ITEM_MISS', {
            gender: item.gender,
            text: contentPreview(item.content),
            runningCount: prefetchQueueRunningCount,
            queuedCount: prefetchQueue.length
          })
        }
      })
      .catch((err) => {
        delete queuedPrefetchMap[item.key]
        debugLog.warn('VOICE_PREFETCH', 'QUEUE_ITEM_FAIL', {
          gender: item.gender,
          text: contentPreview(item.content),
          error: err && err.message ? err.message : String(err),
          runningCount: prefetchQueueRunningCount,
          queuedCount: prefetchQueue.length
        })
      })
      .then(() => {
        prefetchQueueRunningCount = Math.max(0, prefetchQueueRunningCount - 1)
        runPrefetchQueue()
      })
  }
}

function prefetchQueued(texts, gender, options) {
  if (provider() !== config.VOICE_TTS_PROVIDER_SERVER) return
  const voiceGender = normalizeGender(gender || getGender())
  const items = []
  normalizeTexts(texts).forEach((content) => {
    const key = serverCacheKey(content, voiceGender)
    if (playableUrl(serverAudioCache[key]) || serverAudioUrlPending[key] || queuedPrefetchMap[key]) return
    queuedPrefetchMap[key] = true
    items.push({ key, content, gender: voiceGender })
  })
  if (items.length) {
    debugLog.info('VOICE_PREFETCH', options && options.priority ? 'QUEUE_PRIORITY' : 'QUEUE', {
      gender: voiceGender,
      count: items.length,
      mode: 'url_only',
      texts: items.map((item) => contentPreview(item.content))
    })
  }
  if (options && options.priority) {
    for (let i = items.length - 1; i >= 0; i -= 1) {
      prefetchQueue.unshift(items[i])
    }
  } else {
    items.forEach((item) => prefetchQueue.push(item))
  }
  runPrefetchQueue()
}

module.exports = {
  STORAGE_KEY,
  genderLabel,
  getGender,
  setGender,
  provider,
  providerLabel,
  preparePlanVoiceTexts,
  prefetch,
  prefetchRequired,
  prefetchQueued,
  hasCache,
  estimatePreparedDurationSeconds,
  estimateTextsDurationSeconds,
  speak,
  speakRestSequence: function(fixedTexts, durationText, gender) {
    const voiceGender = normalizeGender(gender || getGender())
    const currentToken = ++speakToken
    if (provider() === config.VOICE_TTS_PROVIDER_WECHAT_PLUGIN) {
      const content = normalizeTexts(fixedTexts).concat(trimForTts(durationText)).filter((text) => !!text).join('')
      return speakByWechatPlugin(content, voiceGender, currentToken)
    }
    if (provider() === config.VOICE_TTS_PROVIDER_SERVER) {
      return speakRestSequenceByServer(fixedTexts, durationText, voiceGender, currentToken)
    }
    return Promise.resolve(false)
  },
  speakSequence,
  stop
}
