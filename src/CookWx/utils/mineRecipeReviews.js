const api = require('./api')
const review = require('./recipeReview')
const media = require('./privateMediaCache')
const config = require('./config')
function allowed(account) { return account && ['admin', 'super_admin'].includes(account.role) }
function tabs(base, account, state) { return allowed(account) ? base.concat({ key: 'review', title: state === 'processed' ? '已审核' : '未审核' }) : base }
async function load(page, more = false) {
  if (!allowed(page.data.account) || page.pageActive === false || page.reviewBusy || (more && !page.reviewMore)) return
  page.reviewBusy = true
  const state = page.data.reviewState || 'pending', generation = page.reviewGeneration || 0, owner = getApp().globalData.userId
  const stale = () => page.pageActive === false || generation !== (page.reviewGeneration || 0) || owner !== getApp().globalData.userId || page.data.reviewState !== state
  page.setData({ loadingMore: true })
  try {
    const result = await api.getRecipeReviews(state, more ? page.reviewCursor : '', { isStale: stale })
    if (stale()) return
    if (!result.ok) { page.setData({ 'assetErrors.review': true }); return }
    const rows = (result.items || []).map(row => Object.assign({}, row, { recipeId: row.recipeId, badge: '第' + row.reviewNumber + '次审核', subtitle: row.author + ' · ' + review.status(row) + ' · ' + review.date(state === 'processed' ? row.reviewedAtSeconds : row.submittedAtSeconds) }))
    const previous = more ? page.data.assetGroups.review || [] : []
    const known = new Set(previous.map(row => row.id))
    const items = previous.concat(rows.filter(row => !known.has(row.id)))
    page.reviewCursor = result.nextCursor || ''; page.reviewMore = !!result.hasMore
    page.setData({ 'assetGroups.review': items, 'assetHasMore.review': !!result.hasMore, 'assetErrors.review': false }, () => page.refreshAssetItems())
    for (const row of rows) {
      if (stale()) break
      const id = row.coverImageId || row.videoId || (row.mediaIds || [])[0]
      if (!id) continue
      try { const file = await media.get(id); if (!stale()) { const index = (page.data.assetGroups.review || []).findIndex(item => item.id === row.id); if (index >= 0) page.setData({ ['assetGroups.review[' + index + '].thumbnailUrl']: file }, () => page.refreshAssetItems()) } } catch (_) {}
    }
  } catch (_) { if (!stale()) page.setData({ 'assetErrors.review': true }) }
  finally { if (generation === (page.reviewGeneration || 0)) page.reviewBusy = false; if (!stale()) page.setData({ loadingMore: false }) }
}
function stop(page) { clearTimeout(page.reviewTimer); page.reviewTimer = null; page.reviewBusy = false; page.reviewGeneration = (page.reviewGeneration || 0) + 1 }
function poll(page) {
  clearTimeout(page.reviewTimer)
  if (page.pageActive === false || page.data.activeAssetTab !== 'review') return
  page.reviewTimer = setTimeout(async () => { await load(page); poll(page) }, config.REVIEW_POLL_INTERVAL_MS || 15000)
}
module.exports = { allowed, tabs, load, stop, poll }