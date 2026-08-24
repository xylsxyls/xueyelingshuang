const STORAGE_KEY = 'cook_debug_logs'
const MAX_LOGS = 500

function nowText() {
  const date = new Date()
  const pad = (value, length) => String(value).padStart(length, '0')
  return `${pad(date.getHours(), 2)}:${pad(date.getMinutes(), 2)}:${pad(date.getSeconds(), 2)}.${pad(date.getMilliseconds(), 3)}`
}

function safeDetail(detail) {
  if (detail === undefined || detail === null) return ''
  if (typeof detail === 'string') return detail
  try {
    return JSON.stringify(detail)
  } catch (err) {
    return String(detail)
  }
}

function readLogs() {
  try {
    const logs = wx.getStorageSync(STORAGE_KEY)
    return Array.isArray(logs) ? logs : []
  } catch (err) {
    return []
  }
}

function writeLogs(logs) {
  try {
    wx.setStorageSync(STORAGE_KEY, logs.slice(Math.max(0, logs.length - MAX_LOGS)))
  } catch (err) {
  }
}

function push(level, tag, message, detail) {
  const entry = {
    time: nowText(),
    level,
    tag: tag || 'APP',
    message: message || '',
    detail: safeDetail(detail)
  }
  const logs = readLogs()
  logs.push(entry)
  writeLogs(logs)

  const text = `[CookLog][${entry.time}][${entry.level}][${entry.tag}] ${entry.message}`
  if (level === 'ERROR' && console.error) {
    console.error(text, entry.detail)
  } else if (level === 'WARN' && console.warn) {
    console.warn(text, entry.detail)
  } else if (console.log) {
    console.log(text, entry.detail)
  }
}

function info(tag, message, detail) {
  push('INFO', tag, message, detail)
}

function warn(tag, message, detail) {
  push('WARN', tag, message, detail)
}

function error(tag, message, detail) {
  push('ERROR', tag, message, detail)
}

function clear() {
  try {
    wx.removeStorageSync(STORAGE_KEY)
  } catch (err) {
  }
}

module.exports = {
  STORAGE_KEY,
  info,
  warn,
  error,
  readLogs,
  clear
}
