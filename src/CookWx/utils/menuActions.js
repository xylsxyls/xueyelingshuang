const api = require('./api')

function confirm(title, content, confirmText) {
  return new Promise(resolve => wx.showModal({ title, content, confirmText: confirmText || '确定', success: result => resolve(!!result.confirm), fail: () => resolve(false) }))
}

function requireOk(result) {
  if (!result || !result.ok) throw new Error(result && result.message ? result.message : '操作失败')
  return result
}

async function purchase(recipe, isActive) {
  if (!recipe || recipe.accessible || recipe.owned) return null
  const amount = Number(recipe.priceAmount || recipe.priceCoins || 0)
  if (!await confirm('购买菜谱', `确认花费${amount}${recipe.priceType === 'yuanbao' ? '元宝' : '金币'}购买《${recipe.title}》吗？`, '购买')) return null
  if (isActive && !isActive()) return null
  return requireOk(await api.purchaseRecipe(recipe.recipeId || recipe.id))
}

async function remove(recipe, isActive) {
  if (!await confirm('移出菜单', `确定将《${recipe.title}》移出菜单吗？`, '移出')) return null
  if (isActive && !isActive()) return null
  return requireOk(await api.removeMenuRecipe(recipe.recipeId || recipe.id))
}

async function join(recipeId) { return requireOk(await api.joinRecipe(recipeId)) }

module.exports = { confirm, requireOk, purchase, remove, join }