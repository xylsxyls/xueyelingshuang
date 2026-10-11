const assets = require('./recipeAssets')

// 本次做菜选择归App所有；长期菜单归属仍由服务端inMenu表达。
function ids(app) {
  const values = app.globalData.selectedRecipeIds
  return Array.from(new Set((Array.isArray(values) ? values : []).filter(id => typeof id === 'string' && id)))
}

function has(app, recipeId) { return ids(app).indexOf(recipeId) >= 0 }

function replace(app, values, recipes) {
  const state = app.globalData
  const previous = ids(app)
  const next = Array.from(new Set((values || []).filter(id => typeof id === 'string' && id)))
  const byId = new Map((recipes || []).filter(item => item && item.id).map(item => [item.id, item]))
  state.selectedRecipeIds = next
  state.selectedRecipes = next.map(id => byId.get(id)).filter(Boolean)
  if (previous.length !== next.length || previous.some((id, index) => id !== next[index])) {
    state.lastPreview = null
    state.lastPreviewRecipeKey = ''
    state.lastPreviewNeedsTimeBlocks = false
  }
}

function add(app, recipe) {
  if (!recipe || !recipe.id || !assets.normalize(recipe).accessible || has(app, recipe.id)) return false
  replace(app, ids(app).concat(recipe.id), (app.globalData.selectedRecipes || []).concat(recipe))
  return true
}

module.exports = { has, replace, add }