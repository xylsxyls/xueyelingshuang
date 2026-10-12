const test = require('node:test')
const assert = require('node:assert/strict')
const review = require('../../CookWx/utils/recipeReview')
const mine = require('../../CookWx/utils/mineRecipeReviews')
const create = require('../../CookWx/utils/newCreation')
const base = { id: 'r', title: '菜', priceType: 'coin', priceAmount: 5, mediaIds: ['first'], tasks: [
  { id: 'r.a', title: '切小块', detail: '原说明', active: true, durationSeconds: 10 },
  { id: 'r.b', title: '煮熟', detail: '原煮法', active: false, durationSeconds: 30 }
] }
test('审核比较保留原价格、原步骤时间说明及已删除步骤', () => {
  const recipe = { ...base, priceAmount: 8, mediaIds: ['second'], tasks: [{ ...base.tasks[0], detail: '新说明', durationSeconds: 12 }, { id: 'r.c', title: '拌匀', active: true, durationSeconds: 4 }] }
  const result = review.presentation({ recipe, previousApprovedRecipe: base, review: { state: 'pending', submittedAtSeconds: 1700000000 } })
  assert.equal(result.changes.find(item => item.field === 'priceAmount').previous, '5')
  assert.equal(result.changes.find(item => item.field === 'priceAmount').current, '8')
  assert.ok(result.steps[0].changes.includes('原作者时间：10秒'))
  assert.ok(result.steps[0].changes.includes('原说明：原说明'))
  assert.ok(result.steps[1].changes.includes('本次新增步骤'))
  assert.deepEqual(result.removedSteps.map(step => step.id), ['r.b'])
  assert.equal(result.reviewStatus, '审核中')
})
test('管理员第六固定标签切换状态，普通账户只保留原五项', () => {
  const tabs = Array.from({ length: 5 }, (_, i) => ({ key: String(i) }))
  assert.equal(mine.tabs(tabs, { role: 'user' }, 'pending').length, 5)
  assert.equal(mine.tabs(tabs, { role: 'admin' }, 'pending')[5].title, '未审核')
  assert.equal(mine.tabs(tabs, { role: 'super_admin' }, 'processed')[5].title, '已审核')
})
test('历史版本清楚显示审核次数、未通过及替代，免费版本不伪装审核', () => {
  const result = review.presentation({ history: [{ id: 'a', reviewNumber: 2, requiresReview: true, state: 'rejected' }, { id: 'b', submissionNumber: 3, requiresReview: false, state: 'approved' }] })
  assert.match(result.history[0].label, /第2次审核.*不通过/)
  assert.match(result.history[1].label, /免费发布第3版/)
  assert.equal(review.status({ state: 'superseded' }), '已被新提交替代')
})
test('新创作食材用克，提交使用客户端防重复标识，响应采用服务端ID', () => {
  const source = create.draft()
  const body = { priceType: 'coin', priceAmount: 3, mediaIds: ['uploaded'], steps: source.tasks }
  const result = create.publication(source, body, { titleDraft: '青椒土豆丝', subtitleDraft: '少油', ingredientsDraft: '土豆 150克\n盐 2g', categoryIndex: 2 })
  assert.equal(result.category, '蔬菜')
  assert.deepEqual(result.ingredients, [{ name: '土豆', amount: '150克' }, { name: '盐', amount: '2克' }])
  assert.equal(result.id, source.id)
  assert.ok(source.id.startsWith('new_'))
  assert.equal(create.canonicalBody(body, source.id, 'recipe_server').steps[0].id, 'recipe_server.prepare')
  assert.throws(() => create.ingredients('盐 0克'))
  assert.throws(() => create.ingredients('盐 一勺'))
  assert.throws(() => create.publication(source, { ...body, mediaIds: [] }, { titleDraft: '菜' }), /上传/)
})