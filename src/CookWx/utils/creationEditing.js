const config = require('./config')
const order = require('./recipeStepOrder')

function fields(seconds) {
  const value = Math.max(0, Math.floor(Number(seconds) || 0))
  return { hours: Math.floor(value / 3600), minutes: Math.floor(value % 3600 / 60), seconds: value % 60 }
}

function steps(recipe) {
  return order.renumber((recipe.tasks || []).map(step => Object.assign({}, step, fields(step.durationSeconds), { active: step.active === true })))
}

function newStep(recipeId, task) {
  const short = String(task.id || '')
  return Object.assign({}, task, fields(task.durationSeconds), { id: recipeId + '.' + short, shortId: short, recipeId, active: task.active === true })
}

function matchesSaved(body, recipe) {
  const actual = recipe.tasks || []
  return actual.length === body.steps.length && actual.every((task, index) => {
    const wanted = body.steps[index]
    return task.id === wanted.id && task.title === wanted.title && task.detail === wanted.detail &&
      task.durationSeconds === wanted.durationSeconds && task.active === wanted.active
  })
}

function pricing(recipe) {
  const amount = Number(recipe.priceAmount || recipe.priceCoins || 0)
  const type = recipe.priceType || (amount > 0 ? 'coin' : 'free')
  return { priceType: type, priceAmount: type === 'free' ? '' : String(amount) }
}

function pricePayload(draft) {
  if (!draft || !['free', 'coin', 'yuanbao'].includes(draft.priceType)) throw new Error('请选择免费、金币或元宝')
  if (draft.priceType === 'free') return { priceType: 'free', priceAmount: 0 }
  const text = String(draft.priceAmount == null ? '' : draft.priceAmount).trim()
  const amount = Number(text)
  if (!/^\d+$/.test(text) || !Number.isInteger(amount) || amount < 1 || amount > 2147483647) throw new Error('价格须为1至2147483647之间的整数')
  return { priceType: draft.priceType, priceAmount: amount }
}

function loadError(error) {
  return error && (error.statusCode === 404 || /HTTP 404|api not found|API path not supported/i.test(error.message || ''))
    ? '服务器尚未支持创作编辑，请更新 CookServer 并重启后重试'
    : error && error.message || '创作加载失败，请重试'
}

function payload(recipe, edited, price = pricing(recipe)) {
  if (!recipe || !recipe.id || !recipe.currentRevisionId || !edited.length || edited.length > config.CREATION_MAX_STEPS) throw new Error('菜谱需保留1至' + config.CREATION_MAX_STEPS + '个步骤')
  const originals = new Set((recipe.tasks || []).map(task => task.id))
  const ids = new Set()
  const result = edited.map(step => {
    const title = String(step.title || '').trim(), detail = String(step.detail || '')
    const durationSeconds = Number(step.hours) * 3600 + Number(step.minutes) * 60 + Number(step.seconds)
    if (!step.id || ids.has(step.id) || !title || Array.from(title).length > 8 || Array.from(detail).length > 2000 ||
        typeof step.active !== 'boolean' || !Number.isInteger(durationSeconds) || durationSeconds < 0 || durationSeconds > 86400) throw new Error('请检查步骤名、说明和时长：步骤名最多8字，时长最多24小时')
    ids.add(step.id)
    const item = { id: step.id, title, detail, durationSeconds, active: step.active }
    if (!originals.has(step.id)) Object.assign(item, { safetyLevel: step.safetyLevel || 'normal',
      backgroundWaitMode: step.backgroundWaitMode || 'watch', canLeaveKitchen: step.canLeaveKitchen === true })
    return item
  })
  return Object.assign({ recipeId: recipe.id, currentRevisionId: recipe.currentRevisionId, editorVersion: 2, steps: result }, pricePayload(price))
}

module.exports = { fields, steps, newStep, matchesSaved, pricing, pricePayload, loadError, payload }