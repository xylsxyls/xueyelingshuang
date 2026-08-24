function pad(number) {
  return number < 10 ? `0${number}` : `${number}`
}

function formatSeconds(seconds) {
  const safeSeconds = Math.max(0, Math.floor(seconds || 0))
  const minutes = Math.floor(safeSeconds / 60)
  const rest = safeSeconds % 60
  if (minutes >= 60) {
    const hours = Math.floor(minutes / 60)
    return `${hours}:${pad(minutes % 60)}:${pad(rest)}`
  }
  return `${minutes}:${pad(rest)}`
}

function formatDurationText(seconds) {
  const safeSeconds = Math.max(0, Math.floor(seconds || 0))
  const hours = Math.floor(safeSeconds / 3600)
  const minutes = Math.floor((safeSeconds % 3600) / 60)
  const rest = safeSeconds % 60
  if (hours > 0) {
    return `${hours}小时${minutes}分${rest}秒`
  }
  if (minutes > 0) {
    return `${minutes}分${rest}秒`
  }
  return `${rest}秒`
}

function formatMinutes(seconds) {
  return `${Math.ceil((seconds || 0) / 60)}分钟`
}

function taskWidth(task, totalSeconds) {
  if (!totalSeconds) return 0
  return Math.max(4, Math.round(((task.durationSeconds || 0) / totalSeconds) * 100))
}

function taskOffset(task, totalSeconds) {
  if (!totalSeconds) return 0
  return Math.round(((task.startSeconds || 0) / totalSeconds) * 100)
}

module.exports = {
  formatSeconds,
  formatDurationText,
  formatMinutes,
  taskWidth,
  taskOffset
}