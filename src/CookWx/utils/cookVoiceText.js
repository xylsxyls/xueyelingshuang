const time = require('./time')

const REST_VOICE_WAIT_OFFSET_SECONDS = 1
const VOICE_WAIT_BUCKET_SECONDS = 5
const VOICE_CONNECTOR_AND = '并且。'

function isBackgroundTask(task) {
  if (!task) return false
  if (task.taskMode === 'background' || task.background === true) return true
  if (task.taskMode === 'foreground' || task.foreground === true) return false
  return task.active === false
}

function detailWithExpectedTime(task) {
  const detail = (task && task.detail ? task.detail : '').trim()
  const expectedText = `预计用时${time.formatDurationText(task && task.durationSeconds)}。`
  if (!detail) return expectedText
  if (detail.indexOf('预计用时') >= 0) return detail
  const hasEnd = /[。！？!?]$/.test(detail)
  return `${detail}${hasEnd ? '' : '。'}${expectedText}`
}

function buildTaskVoiceText(task) {
  if (!task) return ''
  if (task.rest && task.voiceText) return task.voiceText
  const title = task.title || ''
  const detail = task.detailText || detailWithExpectedTime(task)
  if (!title) return detail
  if (!detail) return title
  return `${title}。${detail}`
}

function buildTaskVoiceTexts(task) {
  if (!task) return []
  if (task.rest) {
    const parts = buildRestVoicePartsFromTask(task)
    return [
      parts.instantText,
      parts.detailText,
      parts.waitPrefixText,
      parts.waitDurationText
    ].filter((text) => !!text)
  }
  const text = buildTaskVoiceText(task)
  return text ? [text] : []
}

function sortPromptTasks(tasks) {
  return (tasks || []).slice().sort((left, right) => {
    const leftBackground = isBackgroundTask(left)
    const rightBackground = isBackgroundTask(right)
    if (leftBackground !== rightBackground) return leftBackground ? -1 : 1
    return (left.order || 0) - (right.order || 0)
  })
}

function buildPromptTitle(tasks) {
  const titles = (tasks || []).map((task) => task && task.title).filter((title) => !!title)
  if (!titles.length) return '下一步'
  if (titles.length === 1) return titles[0]
  return titles.join('，并')
}

function buildPromptVoiceText(tasks) {
  const texts = buildPromptVoiceTexts(tasks)
  if (!texts.length) return ''
  if (texts.length === 1) return texts[0]
  return texts.join('')
}

function buildPromptVoiceTexts(tasks) {
  const texts = []
  sortPromptTasks(tasks).forEach((task) => {
    const taskTexts = buildTaskVoiceTexts(task)
    if (!taskTexts.length) return
    if (texts.length) texts.push(VOICE_CONNECTOR_AND)
    taskTexts.forEach((text) => texts.push(text))
  })
  return texts
}

function uniqueTexts(texts) {
  const result = []
  const map = {}
  ;(texts || []).forEach((text) => {
    const value = (text || '').trim()
    if (!value || map[value]) return
    map[value] = true
    result.push(value)
  })
  return result
}

function timelinePromptGroups(plan) {
  const timeline = Array.isArray(plan && plan.timeline) ? plan.timeline.slice() : []
  timeline.sort((left, right) => {
    if ((left.startSeconds || 0) !== (right.startSeconds || 0)) return (left.startSeconds || 0) - (right.startSeconds || 0)
    if (isBackgroundTask(left) !== isBackgroundTask(right)) return isBackgroundTask(left) ? -1 : 1
    return (left.order || 0) - (right.order || 0)
  })
  const groups = []
  timeline.forEach((task) => {
    const startSeconds = Math.max(0, Math.floor(task.startSeconds || 0))
    let group = groups.length ? groups[groups.length - 1] : null
    if (!group || group.startSeconds !== startSeconds) {
      group = { startSeconds, tasks: [] }
      groups.push(group)
    }
    group.tasks.push(task)
  })
  return groups
}

function buildInitialPromptVoiceTexts(plan, count) {
  const texts = []
  timelinePromptGroups(plan)
    .slice(0, Math.max(0, count || 0))
    .forEach((group) => {
      buildPromptVoiceTexts(group.tasks).forEach((text) => texts.push(text))
    })
  return uniqueTexts(texts)
}

function timelineTasks(plan) {
  return Array.isArray(plan && plan.timeline) ? plan.timeline.slice() : []
}

function backgroundWaitName(task) {
  const title = task && task.title ? task.title : '后台步骤'
  if (title.indexOf('启动煮饭') >= 0 || title.indexOf('煮饭') >= 0) return '煮饭'
  if (title.indexOf('焖') >= 0) return title
  if (title.indexOf('炖') >= 0) return title
  return title.replace(/^启动/, '')
}

function backgroundWaitMode(task) {
  if (!task || !isBackgroundTask(task)) return ''
  if (task.backgroundWaitMode === 'free') return 'free'
  if (task.backgroundWaitMode === 'watch') return 'watch'
  return task.canLeaveKitchen === true && task.safetyLevel !== 'attention' && task.safetyLevel !== 'danger' ? 'free' : 'watch'
}

function canLeaveKitchenDuring(task) {
  return backgroundWaitMode(task) === 'free'
}

function buildBackgroundEndVoiceTextParts(tasks) {
  const titles = (tasks || [])
    .map((task) => task && task.title ? `${task.title}结束` : '')
    .filter((title) => !!title)
  const uniqueTitles = titles.filter((title, index) => titles.indexOf(title) === index)
  const texts = []
  uniqueTitles.forEach((title, index) => {
    if (index > 0) texts.push(VOICE_CONNECTOR_AND)
    texts.push(`${title}。`)
  })
  return texts
}

function buildBackgroundEndVoiceText(tasks) {
  return buildBackgroundEndVoiceTextParts(tasks).join('')
}

function formatVoiceWaitDuration(seconds) {
  return time.formatDurationText(Math.max(0, Math.floor(seconds || 0)))
}

function adjustVoiceWaitSeconds(seconds) {
  const safeSeconds = Math.max(0, Math.floor(seconds || 0) - REST_VOICE_WAIT_OFFSET_SECONDS)
  return Math.floor(safeSeconds / VOICE_WAIT_BUCKET_SECONDS) * VOICE_WAIT_BUCKET_SECONDS
}

function buildRestVoiceWaitText(canLeaveKitchen, waitSeconds) {
  const safeWaitSeconds = adjustVoiceWaitSeconds(waitSeconds)
  return canLeaveKitchen
    ? `预计还可以休息${formatVoiceWaitDuration(safeWaitSeconds)}。`
    : `预计还需要等待${formatVoiceWaitDuration(safeWaitSeconds)}。`
}

function buildRestVoiceWaitPrefixText(canLeaveKitchen) {
  return canLeaveKitchen ? '预计还可以休息。' : '预计还需要等待。'
}

function buildRestVoiceWaitDurationText(waitSeconds) {
  return `${formatVoiceWaitDuration(adjustVoiceWaitSeconds(waitSeconds))}。`
}

function buildRestVoiceParts(backgroundTasks, waitSeconds) {
  const tasks = (backgroundTasks || []).filter((task) => task && task.id)
  if (!tasks.length) return {}
  const waitNames = tasks.map((task) => backgroundWaitName(task))
  const uniqueNames = waitNames.filter((name, index) => waitNames.indexOf(name) === index)
  const waitText = uniqueNames.length ? uniqueNames.join('、') : '后台步骤'
  const canLeaveKitchen = tasks.every((task) => canLeaveKitchenDuring(task))
  const title = canLeaveKitchen ? '可以休息一下' : '留在厨房等一下'
  const detail = canLeaveKitchen
    ? `当前没有新的手动操作，可以离开厨房，等待${waitText}完成。`
    : `当前没有新的手动操作，但${waitText}还在进行，请留在厨房，等待它完成。`
  const waitTextPart = waitSeconds !== undefined && waitSeconds !== null
    ? buildRestVoiceWaitText(canLeaveKitchen, waitSeconds)
    : ''
  const waitDurationText = waitSeconds !== undefined && waitSeconds !== null
    ? buildRestVoiceWaitDurationText(waitSeconds)
    : ''
  const waitPrefixText = buildRestVoiceWaitPrefixText(canLeaveKitchen)
  const fixedText = `${title}。${detail}`
  return {
    instantText: `${title}。`,
    detailText: detail,
    fixedText,
    waitPrefixText,
    waitDurationText,
    waitText: waitTextPart,
    fullText: `${fixedText}${waitTextPart}`
  }
}

function buildRestVoicePartsFromTask(task) {
  if (!task || !task.rest) return {}
  const title = task.title || ''
  const detail = task.detail || ''
  const canLeaveKitchen = task.canLeaveKitchen !== false
  const waitTextPart = task.remainingSeconds !== undefined && task.remainingSeconds !== null
    ? buildRestVoiceWaitText(canLeaveKitchen, task.remainingSeconds)
    : ''
  const waitDurationText = task.waitDurationVoiceText || (task.remainingSeconds !== undefined && task.remainingSeconds !== null
    ? buildRestVoiceWaitDurationText(task.remainingSeconds)
    : ''
  )
  const waitPrefixText = task.waitPrefixVoiceText || buildRestVoiceWaitPrefixText(canLeaveKitchen)
  const fixedText = task.fixedVoiceText || `${title ? `${title}。` : ''}${detail}`
  return {
    instantText: task.instantVoiceText || (title ? `${title}。` : ''),
    detailText: detail,
    fixedText,
    waitPrefixText,
    waitDurationText,
    waitText: waitTextPart,
    fullText: `${fixedText}${waitTextPart}`
  }
}

function buildPlanAheadVoiceTexts(plan, limit) {
  const texts = []
  const groups = timelinePromptGroups(plan)
  groups.slice(0, Math.max(0, limit || groups.length)).forEach((group) => {
    buildPromptVoiceTexts(group.tasks).forEach((text) => texts.push(text))
    const backgroundTasks = group.tasks.filter((task) => isBackgroundTask(task))
    if (backgroundTasks.length) {
      const restParts = buildRestVoiceParts(backgroundTasks, null)
      texts.push(restParts.instantText)
      texts.push(restParts.detailText)
      texts.push(restParts.waitPrefixText)
      buildBackgroundEndVoiceTextParts(backgroundTasks).forEach((text) => texts.push(text))
    }
  })
  return uniqueTexts(texts)
}

function buildRestFixedVoiceTextsForIntervals(plan) {
  const timeline = timelineTasks(plan)
  const timePointsMap = {}
  timeline.forEach((task) => {
    timePointsMap[Math.max(0, Math.floor(task.startSeconds || 0))] = true
    timePointsMap[Math.max(0, Math.floor(task.endSeconds || 0))] = true
  })
  const timePoints = Object.keys(timePointsMap).map((value) => Number(value)).sort((left, right) => left - right)
  const texts = []
  timePoints.forEach((seconds, index) => {
    if (index >= timePoints.length - 1 || seconds >= timePoints[index + 1]) return
    const runningBackgroundTasks = timeline.filter((task) => {
      return isBackgroundTask(task) && seconds >= (task.startSeconds || 0) && seconds < (task.endSeconds || 0)
    })
    if (!runningBackgroundTasks.length) return
    const hasForegroundTask = timeline.some((task) => {
      return !isBackgroundTask(task) && seconds >= (task.startSeconds || 0) && seconds < (task.endSeconds || 0)
    })
    if (hasForegroundTask) return
    const restParts = buildRestVoiceParts(runningBackgroundTasks, null)
    texts.push(restParts.instantText)
    texts.push(restParts.detailText)
    texts.push(restParts.waitPrefixText)
  })
  return texts
}

function buildBackgroundEndVoiceTexts(plan) {
  const endGroups = {}
  timelineTasks(plan).forEach((task) => {
    if (!isBackgroundTask(task)) return
    const key = String(Math.max(0, Math.floor(task.endSeconds || 0)))
    if (!endGroups[key]) endGroups[key] = []
    endGroups[key].push(task)
  })
  const texts = []
  Object.keys(endGroups).forEach((key) => {
    buildBackgroundEndVoiceTextParts(endGroups[key]).forEach((text) => texts.push(text))
  })
  return texts
}

function buildFixedPlanVoiceTexts(plan) {
  const promptTexts = []
  const restTexts = []
  const endTexts = []
  timelinePromptGroups(plan).forEach((group) => {
    buildPromptVoiceTexts(group.tasks).forEach((text) => promptTexts.push(text))
    const backgroundTasks = group.tasks.filter((task) => isBackgroundTask(task))
    if (backgroundTasks.length) {
      const restParts = buildRestVoiceParts(backgroundTasks, null)
      restTexts.push(restParts.instantText)
      restTexts.push(restParts.detailText)
      restTexts.push(restParts.waitPrefixText)
    }
  })
  buildRestFixedVoiceTextsForIntervals(plan).forEach((text) => restTexts.push(text))
  buildBackgroundEndVoiceTexts(plan).forEach((text) => endTexts.push(text))
  return uniqueTexts(promptTexts.concat(restTexts, endTexts))
}

module.exports = {
  REST_VOICE_WAIT_OFFSET_SECONDS,
  VOICE_WAIT_BUCKET_SECONDS,
  VOICE_CONNECTOR_AND,
  isBackgroundTask,
  detailWithExpectedTime,
  buildTaskVoiceText,
  buildTaskVoiceTexts,
  sortPromptTasks,
  buildPromptTitle,
  buildPromptVoiceText,
  buildPromptVoiceTexts,
  uniqueTexts,
  timelinePromptGroups,
  buildInitialPromptVoiceTexts,
  buildFixedPlanVoiceTexts,
  backgroundWaitName,
  backgroundWaitMode,
  canLeaveKitchenDuring,
  buildBackgroundEndVoiceTextParts,
  buildBackgroundEndVoiceText,
  buildRestVoiceWaitText,
  buildRestVoiceWaitPrefixText,
  buildRestVoiceWaitDurationText,
  buildRestVoiceParts,
  buildRestVoicePartsFromTask,
  buildPlanAheadVoiceTexts
}
