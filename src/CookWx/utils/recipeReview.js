const editing = require('./creationEditing')
const labels = { pending: '审核中', approved: '审核通过', rejected: '审核不通过', superseded: '已被新提交替代', published: '已发布', draft: '草稿' }
const fieldNames = { title: '菜名', subtitle: '介绍', category: '分类', difficulty: '难度', ingredients: '食材', tools: '厨具', tags: '标签', priceType: '收费方式', priceAmount: '价格', coverImageId: '封面', mediaIds: '原始视频或图片组' }
function status(recipe) { return labels[recipe.reviewState || recipe.state || recipe.status] || '我的创作' }
function date(seconds) { if (!seconds) return '尚未审核'; const d = new Date(Number(seconds) * 1000); return `${d.getFullYear()}/${d.getMonth() + 1}/${d.getDate()} ${String(d.getHours()).padStart(2, '0')}:${String(d.getMinutes()).padStart(2, '0')}:${String(d.getSeconds()).padStart(2, '0')}` }
function value(field, item) {
  if (item == null) return '无'
  if (field === 'priceType') return ({ free: '免费', coin: '金币', yuanbao: '元宝' })[item] || item
  if (field === 'mediaIds') return item.length ? item.length + '项媒体（点击上方查看）' : '无'
  if (Array.isArray(item)) return item.map(row => typeof row === 'object' ? Object.values(row).join(' ') : row).join('、') || '无'
  return String(item)
}
function presentation(result) {
  const recipe = result.recipe || {}, previous = result.previousApprovedRecipe
  const older = new Map((previous && previous.tasks || []).map(step => [step.id, step]))
  const currentIds = new Set((recipe.tasks || []).map(step => step.id))
  const steps = editing.steps(recipe).map(step => {
    const before = older.get(step.id)
    const changes = []
    if (previous && !before) changes.push('本次新增步骤')
    if (before) {
      if (step.title !== before.title) changes.push('原步骤名：' + before.title)
      if (step.detail !== before.detail) changes.push('原说明：' + (before.detail || '无'))
      if (step.durationSeconds !== before.durationSeconds) changes.push('原作者时间：' + before.durationSeconds + '秒')
      if (step.active !== before.active) changes.push('原性质：' + (before.active ? '前台' : '后台'))
      if ((previous.tasks || []).findIndex(item => item.id === step.id) !== step.index - 1) changes.push('原顺序：第' + ((previous.tasks || []).findIndex(item => item.id === step.id) + 1) + '步')
    }
    return Object.assign({}, step, { changes })
  })
  const changes = []
  if (previous) Object.keys(fieldNames).forEach(field => {
    if (JSON.stringify(recipe[field]) !== JSON.stringify(previous[field])) changes.push({ field, label: fieldNames[field], previous: value(field, previous[field]), current: value(field, recipe[field]) })
  })
  return { steps, changes, removedSteps: (previous && previous.tasks || []).filter(step => !currentIds.has(step.id)),
    history: (result.history || []).map(row => Object.assign({}, row, { label: (row.requiresReview ? '第' + row.reviewNumber + '次审核' : '免费发布第' + row.submissionNumber + '版') + ' · ' + status(row) + ' · ' + date(row.submittedAtSeconds) })),
    reviewStatus: result.review && result.review.requiresReview === false ? '已发布（无需审核）' : status(result.review || {}), submittedText: date((result.review || {}).submittedAtSeconds), reviewedText: result.review && result.review.requiresReview === false ? '无需审核' : date((result.review || {}).reviewedAtSeconds) }
}
module.exports = { status, date, presentation }