function key(task) { if (task.shortId) return String(task.shortId); const id = String(task.id || ''), prefix = String(task.recipeId || '') + '.'; return task.recipeId && id.startsWith(prefix) ? id.slice(prefix.length) : id.split('.').pop() }
function fields(seconds) {
  const value = Math.max(0, Math.floor(Number(seconds) || 0))
  return { hours: Math.floor(value / 3600), minutes: Math.floor(value % 3600 / 60), seconds: value % 60 }
}
function seconds(step) { return Math.max(0, Math.floor(Number(step.hours) || 0)) * 3600 + Math.max(0, Math.min(59, Math.floor(Number(step.minutes) || 0))) * 60 + Math.max(0, Math.min(59, Math.floor(Number(step.seconds) || 0))) }
function merge(recipe, personalization, format) {
  const config = personalization || {}, disabled = config.disabledTaskIds || [], overrides = config.durationOverrides || []
  const items = (recipe.tasks || []).map(task => Object.assign({}, task, { added: false })).concat((config.addedTasks || []).map(task => Object.assign({}, task, { added: true })))
  const byKey = new Map(items.map(task => [key(task), task]))
  const ordered = [], used = new Set()
  const resolveId = value => { const id = String(value), prefix = String(recipe.id || '') + '.'; return byKey.has(id) ? id : (id.startsWith(prefix) ? id.slice(prefix.length) : id) }
  ;(config.taskOrder || []).concat(items.map(key)).forEach(id => {
    const short = resolveId(id), task = byKey.get(short)
    if (!task || used.has(short)) return
    used.add(short)
    const original = Math.max(0, Math.floor(Number(task.durationSeconds) || 0))
    const override = overrides.find(item => resolveId(item.taskId) === short)
    const duration = override ? Math.max(0, Number(override.durationSeconds) || 0) : original
    ordered.push(Object.assign({}, task, fields(duration), {
      index: ordered.length + 1, originalDurationSeconds: original, durationSeconds: duration,
      durationText: format(duration), originalDurationText: format(original),
      disabled: disabled.some(id => resolveId(id) === short),
      modeText: task.active || task.taskMode === 'foreground' ? '前台操作' : (task.canLeaveKitchen ? '可离开等待' : '厨房守候')
    }))
  })
  return ordered
}
function renumber(items) { return items.map((item, index) => Object.assign({}, item, { index: index + 1 })) }
function payload(recipeId, steps) {
  const durationOverrides = [], disabledTaskIds = [], addedTasks = []
  steps.forEach(step => {
    const durationSeconds = seconds(step)
    if (step.added) { const task = Object.assign({}, step, { id: key(step), durationSeconds, voiceText: step.title }); ['index', 'added', 'hours', 'minutes', 'seconds', 'originalDurationSeconds', 'originalDurationText', 'durationText', 'modeText', 'disabled'].forEach(field => { delete task[field] }); addedTasks.push(task) }
    else if (step.disabled) disabledTaskIds.push(key(step))
    else if (durationSeconds !== step.originalDurationSeconds) durationOverrides.push({ taskId: key(step), durationSeconds })
  })
  return { recipeId, disabledTaskIds, durationOverrides, addedTasks, taskOrder: steps.map(key) }
}
module.exports = { key, fields, seconds, merge, renumber, payload }