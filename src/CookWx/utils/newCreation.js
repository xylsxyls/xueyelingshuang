const assets = require('./recipeAssets')
function draft() {
  const id = 'new_' + Date.now().toString(36) + '_' + Math.random().toString(36).slice(2, 10)
  return { id, currentRevisionId: id + '_r1', title: '', subtitle: '', category: assets.CATEGORIES[0], priceType: 'free', priceAmount: 0,
    ingredients: [], mediaIds: [], tasks: [{ id: id + '.prepare', shortId: 'prepare', title: '准备食材', detail: '', durationSeconds: 60, active: true }] }
}
function ingredients(text) {
  const lines = String(text || '').split(/\r?\n/).map(line => line.trim()).filter(Boolean)
  if (!lines.length || lines.length > 100) throw new Error('请填写1至100项食材，每行填食材名和克数')
  return lines.map(line => {
    const match = line.match(/^(.+?)\s+(\d+(?:\.\d+)?)\s*(?:克|g)?$/)
    if (!match || !Number.isFinite(Number(match[2])) || Number(match[2]) <= 0 || Array.from(match[1]).length > 40) throw new Error('食材请按“土豆 150克”填写，克数必须大于0')
    return { name: match[1].trim(), amount: match[2] + '克' }
  })
}
function publication(recipe, body, data) {
  const title = String(data.titleDraft || '').trim(), subtitle = String(data.subtitleDraft || '').trim()
  if (!title || Array.from(title).length > 40 || Array.from(subtitle).length > 200) throw new Error('菜名须为1至40字，介绍最多200字')
  if (!body.mediaIds || !body.mediaIds.length) throw new Error('请先上传原视频或图片组')
  const categories = assets.CATEGORIES
  return { schemaVersion: '1.0', id: recipe.id, currentRevisionId: recipe.currentRevisionId, title, subtitle,
    category: categories[Number(data.categoryIndex)] || categories[0], ingredients: ingredients(data.ingredientsDraft),
    priceType: body.priceType, priceAmount: body.priceAmount, mediaIds: body.mediaIds,
    tasks: body.steps.map((step, index) => ({ ...step, resource: step.active ? 'board' : 'stove', dependsOn: index ? [body.steps[index - 1].id] : [] })) }
}
function canonicalBody(body, sourceId, serverId) {
  return { ...body, steps: body.steps.map(step => ({ ...step, id: serverId + step.id.slice(sourceId.length) })) }
}
module.exports = { draft, ingredients, publication, canonicalBody }