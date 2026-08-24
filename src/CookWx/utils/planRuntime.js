const time = require('./time')

const REST_WAIT_OFFSET_SECONDS = 1
const REST_WAIT_BUCKET_SECONDS = 5

function toNumber(value, fallback) {
  const number = Number(value)
  return isFinite(number) ? number : (fallback || 0)
}

function ceilMinutes(seconds) {
  return Math.ceil(Math.max(0, seconds || 0) / 60)
}

function clampPercent(value) {
  if (value < 0) return 0
  if (value > 100) return 100
  return Math.round(value)
}

function cloneTask(task, index) {
  const startSeconds = Math.max(0, toNumber(task.startSeconds, 0))
  const inputDuration = toNumber(task.durationSeconds, -1)
  const inputEnd = toNumber(task.endSeconds, startSeconds + Math.max(0, inputDuration))
  const endSeconds = Math.max(startSeconds, inputEnd)
  const durationSeconds = inputDuration >= 0 ? Math.max(0, inputDuration) : Math.max(0, endSeconds - startSeconds)
  return Object.assign({}, task, {
    order: index,
    startSeconds,
    endSeconds: Math.max(endSeconds, startSeconds + durationSeconds),
    durationSeconds,
    originalStartSeconds: startSeconds,
    originalEndSeconds: Math.max(endSeconds, startSeconds + durationSeconds),
    originalDurationSeconds: durationSeconds,
    dependencies: Array.isArray(task.dependencies) ? task.dependencies.slice() : []
  })
}

function sortTimeline(timeline) {
  return timeline.sort((left, right) => {
    if (left.startSeconds !== right.startSeconds) return left.startSeconds - right.startSeconds
    if (left.active !== right.active) return left.active ? -1 : 1
    return left.order - right.order
  })
}

function create(plan) {
  const sourceTimeline = Array.isArray(plan.timeline) ? plan.timeline : []
  const timeline = sortTimeline(sourceTimeline.map((task, index) => cloneTask(task, index)))
  const runtime = {
    plan: Object.assign({}, plan),
    timeline,
    completedTaskIds: {},
    skippedTaskIds: {},
    actualTimeMap: {},
    extendedSecondsMap: {},
    finishTypeMap: {},
    elapsedSeconds: 0,
    totalSeconds: 0,
    activeSeconds: 0
  }
  recalculate(runtime)
  return runtime
}

function recalculate(runtime) {
  let totalSeconds = 0
  let activeSeconds = 0
  runtime.timeline.forEach((task) => {
    task.durationSeconds = Math.max(0, task.endSeconds - task.startSeconds)
    totalSeconds = Math.max(totalSeconds, task.endSeconds)
    if (task.active && !runtime.skippedTaskIds[task.id]) {
      activeSeconds += task.durationSeconds
    }
  })
  runtime.totalSeconds = totalSeconds
  runtime.activeSeconds = activeSeconds
  runtime.elapsedSeconds = Math.max(0, Math.min(runtime.elapsedSeconds, totalSeconds))
  const summary = Object.assign({}, runtime.plan.summary || {})
  summary.totalSeconds = totalSeconds
  summary.totalMinutes = ceilMinutes(totalSeconds)
  summary.activeSeconds = activeSeconds
  summary.activeMinutes = ceilMinutes(activeSeconds)
  summary.headline = `预计 ${summary.totalMinutes} 分钟吃上，真正动手约 ${summary.activeMinutes} 分钟`
  runtime.plan.summary = summary
}

function hasDependencyData(runtime) {
  return runtime.timeline.some((task) => task.dependencies && task.dependencies.length)
}

function findTask(runtime, taskId) {
  return runtime.timeline.find((task) => task.id === taskId) || null
}

function isManualFinished(runtime, task) {
  if (!task || !task.id) return false
  return !!runtime.completedTaskIds[task.id] || !!runtime.skippedTaskIds[task.id]
}

function isBackgroundTask(task) {
  if (!task) return false
  if (task.taskMode === 'background' || task.background === true) return true
  if (task.taskMode === 'foreground' || task.foreground === true) return false
  return !task.active
}

function canSkipTask(runtime, task) {
  if (!task || !task.id || task.manualSkippable === false || isBackgroundTask(task)) return false
  if (isFinished(runtime, task)) return false
  const currentTask = findCurrentTask(runtime)
  return !!currentTask && currentTask.id === task.id && runtime.elapsedSeconds >= task.startSeconds
}

function canFinishTask(runtime, task) {
  if (!task || !task.id || isBackgroundTask(task)) return false
  if (isFinished(runtime, task)) return false
  const currentTask = findCurrentTask(runtime)
  return !!currentTask && currentTask.id === task.id && runtime.elapsedSeconds >= task.startSeconds
}

function canAddTimeTask(runtime, task) {
  if (!task || !task.id || isBackgroundTask(task) || isFinished(runtime, task)) return false
  const currentTask = findCurrentTask(runtime)
  return !!currentTask && currentTask.id === task.id && runtime.elapsedSeconds >= task.startSeconds
}

function isFinished(runtime, task) {
  if (!task || !task.id) return false
  return isManualFinished(runtime, task) || runtime.elapsedSeconds >= task.endSeconds
}

function buildAffectedIds(runtime, sourceTask, boundarySeconds) {
  const affected = {}
  const hasDependencies = hasDependencyData(runtime)
  if (hasDependencies) {
    affected[sourceTask.id] = true
    let changed = true
    while (changed) {
      changed = false
      runtime.timeline.forEach((task) => {
        if (affected[task.id]) return
        const dependencies = task.dependencies || []
        if (dependencies.some((dependencyId) => affected[dependencyId])) {
          affected[task.id] = true
          changed = true
        }
      })
    }
    delete affected[sourceTask.id]
    runtime.timeline.forEach((task) => {
      if (task.id !== sourceTask.id && task.startSeconds >= boundarySeconds) {
        affected[task.id] = true
      }
    })
    return affected
  }

  runtime.timeline.forEach((task) => {
    if (task.id !== sourceTask.id && task.startSeconds >= boundarySeconds) {
      affected[task.id] = true
    }
  })
  return affected
}

function repairDependencies(runtime, lockedTaskIds) {
  const taskMap = {}
  runtime.timeline.forEach((task) => {
    taskMap[task.id] = task
  })

  for (let round = 0; round < runtime.timeline.length; round += 1) {
    let changed = false
    runtime.timeline.forEach((task) => {
      if (lockedTaskIds && lockedTaskIds[task.id]) return
      let requiredStart = 0
      ;(task.dependencies || []).forEach((dependencyId) => {
        const dependency = taskMap[dependencyId]
        if (dependency) requiredStart = Math.max(requiredStart, dependency.endSeconds)
      })
      if (task.startSeconds < requiredStart) {
        const delta = requiredStart - task.startSeconds
        task.startSeconds += delta
        task.endSeconds += delta
        changed = true
      }
    })
    if (!changed) break
  }
}

function shiftAffectedTasks(runtime, sourceTask, deltaSeconds, boundarySeconds) {
  if (!deltaSeconds) return
  const affected = buildAffectedIds(runtime, sourceTask, boundarySeconds)
  const lockedBackgroundMap = {}
  runtime.timeline.forEach((task) => {
    if (isBackgroundTask(task) && runtime.elapsedSeconds >= task.startSeconds && runtime.elapsedSeconds < task.endSeconds) {
      lockedBackgroundMap[task.id] = {
        startSeconds: task.startSeconds,
        endSeconds: task.endSeconds,
        durationSeconds: task.durationSeconds
      }
    }
  })
  runtime.timeline.forEach((task) => {
    if (!affected[task.id]) return
    if (lockedBackgroundMap[task.id]) return
    task.startSeconds = Math.max(0, task.startSeconds + deltaSeconds)
    task.endSeconds = Math.max(task.startSeconds, task.endSeconds + deltaSeconds)
  })
  repairDependencies(runtime, lockedBackgroundMap)
  runtime.timeline.forEach((task) => {
    const locked = lockedBackgroundMap[task.id]
    if (!locked) return
    task.startSeconds = locked.startSeconds
    task.endSeconds = locked.endSeconds
    task.durationSeconds = locked.durationSeconds
  })
  repairDependencies(runtime, lockedBackgroundMap)
  sortTimeline(runtime.timeline)
  recalculate(runtime)
}

function findCurrentTask(runtime) {
  const elapsed = runtime.elapsedSeconds
  const runningTasks = runtime.timeline.filter((task) => {
    if (isManualFinished(runtime, task)) return false
    return elapsed >= task.startSeconds && elapsed < task.endSeconds
  })
  const foregroundTask = runningTasks.find((task) => !isBackgroundTask(task))
  if (foregroundTask) return foregroundTask
  const nextForegroundTask = runtime.timeline.find((task) => !isBackgroundTask(task) && !isManualFinished(runtime, task) && !isFinished(runtime, task))
  if (nextForegroundTask) return nextForegroundTask
  return runningTasks[0] || runtime.timeline.find((task) => !isManualFinished(runtime, task) && elapsed < task.startSeconds) || null
}

function runningBackgroundTasks(runtime) {
  return runtime.timeline.filter((task) => {
    if (!isBackgroundTask(task) || isFinished(runtime, task)) return false
    return runtime.elapsedSeconds >= task.startSeconds && runtime.elapsedSeconds < task.endSeconds
  })
}

function backgroundWaitName(task) {
  const title = task && task.title ? task.title : '后台步骤'
  if (title.indexOf('启动煮饭') >= 0 || title.indexOf('煮饭') >= 0) return '煮饭'
  if (title.indexOf('焖') >= 0) return title
  if (title.indexOf('炖') >= 0) return title
  return title.replace(/^启动/, '')
}

function canLeaveKitchenDuring(task) {
  return backgroundWaitMode(task) === 'free'
}

function backgroundWaitMode(task) {
  if (!task || isBackgroundTask(task) === false) return ''
  if (task.backgroundWaitMode === 'free') return 'free'
  if (task.backgroundWaitMode === 'watch') return 'watch'
  return task.canLeaveKitchen === true && task.safetyLevel !== 'attention' && task.safetyLevel !== 'danger' ? 'free' : 'watch'
}

function formatVoiceWaitDuration(seconds) {
  return time.formatDurationText(Math.max(0, Math.floor(seconds || 0)))
}

function adjustRestWaitSeconds(seconds) {
  const safeSeconds = Math.max(0, Math.floor(seconds || 0) - REST_WAIT_OFFSET_SECONDS)
  return Math.floor(safeSeconds / REST_WAIT_BUCKET_SECONDS) * REST_WAIT_BUCKET_SECONDS
}

function buildRestDisplayWaitText(canLeaveKitchen, seconds) {
  const waitText = formatVoiceWaitDuration(seconds)
  return canLeaveKitchen
    ? `预计还可以休息${waitText}。`
    : `预计还需要等待${waitText}。`
}

function buildRestDisplayCountdownText(canLeaveKitchen, seconds) {
  const waitText = formatVoiceWaitDuration(seconds)
  return canLeaveKitchen
    ? `预计还可以休息 ${waitText}`
    : `预计还需要等待 ${waitText}`
}

function buildRestWaitPrefixText(canLeaveKitchen) {
  return canLeaveKitchen ? '预计还可以休息。' : '预计还需要等待。'
}

function buildRestWaitDurationText(seconds) {
  return `${formatVoiceWaitDuration(adjustRestWaitSeconds(seconds))}。`
}

function buildPromptTasks(runtime, currentTask) {
  if (!currentTask || !currentTask.id || runtime.elapsedSeconds < currentTask.startSeconds) return []
  const startSeconds = currentTask.startSeconds
  return runtime.timeline
    .filter((task) => {
      if (isManualFinished(runtime, task)) return false
      return task.startSeconds === startSeconds && runtime.elapsedSeconds >= task.startSeconds && runtime.elapsedSeconds < task.endSeconds
    })
    .sort((left, right) => {
      const leftBackground = isBackgroundTask(left)
      const rightBackground = isBackgroundTask(right)
      if (leftBackground !== rightBackground) return leftBackground ? -1 : 1
      return left.order - right.order
    })
}

function buildStartingPromptTasks(runtime) {
  const elapsed = runtime.elapsedSeconds
  return runtime.timeline
    .filter((task) => {
      if (isManualFinished(runtime, task)) return false
      return task.startSeconds === elapsed && elapsed < task.endSeconds
    })
    .sort((left, right) => {
      const leftBackground = isBackgroundTask(left)
      const rightBackground = isBackgroundTask(right)
      if (leftBackground !== rightBackground) return leftBackground ? -1 : 1
      return left.order - right.order
    })
}

function buildRestHint(runtime, currentTask, backgroundTasks) {
  if (!backgroundTasks.length) return null
  const elapsed = runtime.elapsedSeconds
  const hasRunningForeground = runtime.timeline.some((task) => {
    if (isBackgroundTask(task) || isFinished(runtime, task)) return false
    return elapsed >= task.startSeconds && elapsed < task.endSeconds
  })
  if (hasRunningForeground) return null
  if (currentTask && !isBackgroundTask(currentTask) && currentTask.startSeconds <= elapsed) return null

  const endSeconds = currentTask && currentTask.startSeconds > elapsed
    ? currentTask.startSeconds
    : Math.min.apply(null, backgroundTasks.map((task) => task.endSeconds))
  const waitNames = backgroundTasks.map((task) => backgroundWaitName(task))
  const uniqueNames = waitNames.filter((name, index) => waitNames.indexOf(name) === index)
  const waitText = uniqueNames.length ? uniqueNames.join('、') : '后台步骤'
  const remainingSeconds = Math.max(0, endSeconds - elapsed)
  const canLeaveKitchen = backgroundTasks.every((task) => canLeaveKitchenDuring(task))
  const restKind = canLeaveKitchen ? 'free' : 'kitchen'
  const title = canLeaveKitchen ? '可以休息一下' : '留在厨房等一下'
  const detail = canLeaveKitchen
    ? `当前没有新的手动操作，可以离开厨房，等待${waitText}完成。`
    : `当前没有新的手动操作，但${waitText}还在进行，请留在厨房，等待它完成。`
  const displayWaitText = buildRestDisplayWaitText(canLeaveKitchen, remainingSeconds)
  const waitPrefixText = buildRestWaitPrefixText(canLeaveKitchen)
  const waitDurationText = buildRestWaitDurationText(remainingSeconds)
  const countdownText = buildRestDisplayCountdownText(canLeaveKitchen, remainingSeconds)
  return {
    id: `rest_${backgroundTasks.map((task) => task.id).join('_')}_${currentTask && currentTask.id ? currentTask.id : 'finish'}`,
    title,
    detail,
    detailText: `${detail}${displayWaitText}`,
    countdownText,
    voiceText: `${title}。${detail}`,
    instantVoiceText: `${title}。`,
    fixedVoiceText: `${title}。${detail}`,
    waitPrefixVoiceText: waitPrefixText,
    waitDurationVoiceText: waitDurationText,
    followVoiceText: `${detail}${waitPrefixText}${waitDurationText}`,
    recipeTitle: currentTask && currentTask.recipeTitle ? currentTask.recipeTitle : '',
    startSeconds: elapsed,
    endSeconds,
    durationSeconds: remainingSeconds,
    remainingSeconds,
    progressPercent: 0,
    rest: true,
    restKind,
    background: false,
    foreground: false,
    active: false,
    canPause: false,
    continuesDuringPause: true,
    canLeaveKitchen,
    canSkip: false,
    canFinish: false,
    canAddTime: false,
    taskTypeText: canLeaveKitchen ? '休息时间' : '厨房等待',
    stateText: canLeaveKitchen ? '可离开厨房' : '留在厨房'
  }
}

function advanceToNext(runtime) {
  const current = findCurrentTask(runtime)
  if (current) {
    if (runtime.elapsedSeconds < current.startSeconds) {
      if (runningBackgroundTasks(runtime).length > 0) {
        return false
      }
      runtime.elapsedSeconds = current.startSeconds
    }
    return false
  }
  runtime.elapsedSeconds = runtime.totalSeconds
  return true
}

function progressOf(runtime, task) {
  if (runtime.skippedTaskIds[task.id] || runtime.completedTaskIds[task.id] || runtime.elapsedSeconds >= task.endSeconds) return 100
  if (runtime.elapsedSeconds <= task.startSeconds) return 0
  if (task.durationSeconds <= 0) return 100
  return clampPercent(((runtime.elapsedSeconds - task.startSeconds) / task.durationSeconds) * 100)
}

function detailWithExpectedTime(task) {
  const detail = (task.detail || '').trim()
  const expectedText = `预计用时${time.formatDurationText(task.durationSeconds)}。`
  if (!detail) return expectedText
  if (detail.indexOf('预计用时') >= 0) return detail
  const hasEnd = /[。！？!?]$/.test(detail)
  return `${detail}${hasEnd ? '' : '。'}${expectedText}`
}

function finishTask(runtime, taskId, finishType) {
  const task = findTask(runtime, taskId)
  if (!canFinishTask(runtime, task)) return false
  const oldEnd = task.endSeconds
  const elapsed = Math.max(runtime.elapsedSeconds, task.startSeconds)
  if (elapsed < task.endSeconds) {
    task.endSeconds = elapsed
    task.durationSeconds = Math.max(0, task.endSeconds - task.startSeconds)
    shiftAffectedTasks(runtime, task, task.endSeconds - oldEnd, oldEnd)
  }
  runtime.completedTaskIds[task.id] = true
  runtime.actualTimeMap[task.id] = Math.max(0, Math.min(elapsed, oldEnd) - task.startSeconds)
  runtime.finishTypeMap[task.id] = finishType || (elapsed < oldEnd ? 'early' : 'normal')
  recalculate(runtime)
  return advanceToNext(runtime)
}

function skipTask(runtime, taskId) {
  const task = findTask(runtime, taskId)
  if (!canSkipTask(runtime, task)) return false
  const oldEnd = task.endSeconds
  const newEnd = runtime.elapsedSeconds >= task.startSeconds ? Math.max(task.startSeconds, runtime.elapsedSeconds) : task.startSeconds
  task.endSeconds = Math.min(oldEnd, newEnd)
  task.durationSeconds = Math.max(0, task.endSeconds - task.startSeconds)
  runtime.completedTaskIds[task.id] = true
  runtime.skippedTaskIds[task.id] = true
  runtime.actualTimeMap[task.id] = 0
  runtime.finishTypeMap[task.id] = 'skipped'
  shiftAffectedTasks(runtime, task, task.endSeconds - oldEnd, oldEnd)
  recalculate(runtime)
  return advanceToNext(runtime)
}

function addTime(runtime, taskId, seconds) {
  const task = findTask(runtime, taskId)
  const deltaSeconds = Math.max(0, Math.floor(seconds || 0))
  if (!canAddTimeTask(runtime, task) || !deltaSeconds || runtime.skippedTaskIds[task.id]) return false
  const oldEnd = task.endSeconds
  task.endSeconds += deltaSeconds
  task.durationSeconds = Math.max(0, task.endSeconds - task.startSeconds)
  runtime.extendedSecondsMap[task.id] = (runtime.extendedSecondsMap[task.id] || 0) + deltaSeconds
  runtime.finishTypeMap[task.id] = 'extended'
  shiftAffectedTasks(runtime, task, deltaSeconds, oldEnd)
  recalculate(runtime)
  return true
}

function tick(runtime, seconds) {
  const deltaSeconds = Math.max(0, Math.floor(seconds || 0))
  if (!deltaSeconds) return false
  runtime.elapsedSeconds = Math.min(runtime.totalSeconds, runtime.elapsedSeconds + deltaSeconds)
  return runtime.elapsedSeconds >= runtime.totalSeconds
}

function buildView(runtime) {
  recalculate(runtime)
  const totalSeconds = Math.max(1, runtime.totalSeconds)
  const timelineItemWidth = 220
  const timelineItemGap = 14
  const timelineWidth = Math.max(690, runtime.timeline.length * (timelineItemWidth + timelineItemGap))
  const currentTask = findCurrentTask(runtime)
  const rawBackgroundTasks = runningBackgroundTasks(runtime)
  const pendingRawTasks = []
  const addPendingTask = (task) => {
    if (task && !pendingRawTasks.some((visibleTask) => visibleTask.id === task.id)) {
      pendingRawTasks.push(task)
    }
  }
  runtime.timeline.forEach((task) => {
    if (isFinished(runtime, task)) return
    addPendingTask(task)
  })
  sortTimeline(pendingRawTasks)
  const visibleRawTasks = pendingRawTasks.slice(0, 8)
  rawBackgroundTasks.forEach((task) => {
    if (!visibleRawTasks.some((visibleTask) => visibleTask.id === task.id)) {
      visibleRawTasks.push(task)
    }
  })
  sortTimeline(visibleRawTasks)
  const visibleTasks = visibleRawTasks.map((task) => decorateTask(runtime, task))
  const backgroundTasks = rawBackgroundTasks.map((task) => decorateTask(runtime, task))
  const restHint = buildRestHint(runtime, currentTask, rawBackgroundTasks)
  const displayCurrentTask = restHint || (currentTask ? decorateTask(runtime, currentTask) : {})
  const promptTasks = (restHint ? buildStartingPromptTasks(runtime) : buildPromptTasks(runtime, currentTask)).map((task) => decorateTask(runtime, task))
  const timelineBars = runtime.timeline.map((task) => {
    return Object.assign(decorateTask(runtime, task), {
      width: timelineItemWidth,
      itemGap: timelineItemGap
    })
  })
  return {
    plan: runtime.plan,
    currentTask: displayCurrentTask,
    promptTasks,
    restHint,
    visibleTasks,
    backgroundTasks,
    timelineBars,
    timelineWidth,
    timelinePointerLeft: Math.round((runtime.elapsedSeconds / totalSeconds) * timelineWidth),
    elapsedSeconds: runtime.elapsedSeconds,
    totalSeconds: runtime.totalSeconds,
    progressPercent: clampPercent((runtime.elapsedSeconds / totalSeconds) * 100),
    completedCount: runtime.timeline.filter((task) => isFinished(runtime, task)).length,
    totalCount: runtime.timeline.length,
    isComplete: runtime.timeline.length > 0 && runtime.timeline.every((task) => isFinished(runtime, task))
  }
}

function decorateTask(runtime, task) {
  const currentTask = findCurrentTask(runtime)
  const done = isFinished(runtime, task)
  const skipped = !!runtime.skippedTaskIds[task.id]
  const current = !!currentTask && currentTask.id === task.id && runtime.elapsedSeconds >= task.startSeconds && !done
  const progressPercent = progressOf(runtime, task)
  const background = isBackgroundTask(task)
  const waitMode = backgroundWaitMode(task)
  const running = !done && !skipped && runtime.elapsedSeconds >= task.startSeconds && runtime.elapsedSeconds < task.endSeconds
  const remainingSeconds = running
    ? Math.max(0, task.endSeconds - runtime.elapsedSeconds)
    : Math.max(0, task.startSeconds - runtime.elapsedSeconds)
  const canSkip = canSkipTask(runtime, task)
  const canFinish = canFinishTask(runtime, task)
  const canAddTime = canAddTimeTask(runtime, task)
  const runningTimeText = running ? `剩余 ${time.formatSeconds(remainingSeconds)}` : time.formatSeconds(Math.max(0, task.startSeconds - runtime.elapsedSeconds))
  const timelineText = running
    ? `${runningTimeText} · ${background ? (waitMode === 'free' ? '后台计时' : '厨房守候') : '进行中'}`
    : `${time.formatSeconds(task.startSeconds)} · ${skipped ? '跳过' : (done ? '完成' : '未开始')}`
  const listTimeText = running ? runningTimeText : time.formatSeconds(task.startSeconds)
  return Object.assign({}, task, {
    done,
    skipped,
    current,
    running,
    runningBackground: running && background,
    currentForeground: current && !background,
    background,
    foreground: !background,
    backgroundWaitMode: waitMode,
    canSkip,
    canFinish,
    canAddTime,
    taskTypeText: background ? (waitMode === 'free' ? '后台休息' : '后台守候') : '前台操作',
    progressPercent,
    remainingSeconds,
    timelineText,
    listTimeText,
    detailText: detailWithExpectedTime(task),
    stateText: skipped ? '跳过' : (done ? '完成' : (background ? (waitMode === 'free' ? '计时中 · 可离开' : '计时中 · 留意厨房') : '动手'))
  })
}

module.exports = {
  create,
  buildView,
  tick,
  finishTask,
  skipTask,
  addTime,
  findCurrentTask
}
