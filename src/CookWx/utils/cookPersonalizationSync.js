// 同步只使用运行时记录的完成用时，不用计划值补齐未做的步骤。
function buildItems(runtime) {
  if (!runtime || !Array.isArray(runtime.timeline)) return []
  const recipes = {}
  const skipped = runtime.skippedTaskIds || {}
  const actualTimes = runtime.actualTimeMap || {}
  const completed = runtime.completedTaskIds || {}
  runtime.timeline.forEach((task) => {
    if (!task || !task.id || !task.recipeId || task.rest || skipped[task.id]) return
    const seconds = Math.floor(Number(actualTimes[task.id]))
    if (!isFinite(seconds) || seconds <= 0) return
    if (!completed[task.id] && runtime.elapsedSeconds < task.endSeconds) return
    if (!recipes[task.recipeId]) recipes[task.recipeId] = { recipeId: task.recipeId, durationOverrides: [] }
    recipes[task.recipeId].durationOverrides.push({ taskId: task.id, durationSeconds: Math.floor(seconds) })
  })
  return Object.keys(recipes).map(id => recipes[id])
}

function enqueue(app, runtime, ownerId) {
  if (!app || !runtime || !ownerId || app.globalData.userId !== ownerId) return
  const reports = app.globalData.pendingCookSyncReports || []
  app.globalData.pendingCookSyncReports = reports
  reports.push({ ownerId, items: buildItems(runtime), elapsedSeconds: runtime.elapsedSeconds })
}

let activeReport = null

// 系统返回时先保存快照，再由可见页面呈现询问，卸载生命周期内不弹框或导航。
function showPending(app, platform, api, options = {}) {
  const reports = app.globalData.pendingCookSyncReports || []
  if (activeReport || !reports.length) return false
  const report = reports[0]
  if (report.ownerId !== app.globalData.userId) {
    reports.shift()
    return showPending(app, platform, api, options)
  }
  activeReport = report
  let sending = false
  const isActive = () => !options.isActive || options.isActive()
  const release = (consume) => {
    if (consume && reports[0] === report) reports.shift()
    if (activeReport === report) activeReport = null
    if (options.onSettled && isActive()) options.onSettled()
  }
  const ask = () => {
    if (!isActive() || app.globalData.userId !== report.ownerId) { release(false); return }
    const taskCount = report.items.reduce((count, item) => count + item.durationOverrides.length, 0)
    platform.showModal({
      title: '同步到个性化？',
      content: taskCount
        ? `是否将本次实际完成的${taskCount}个步骤用时同步到个性化？未完成和跳过的步骤不会覆盖原设置。`
        : '本次没有可同步的已完成步骤，个性化用时保持原设置。',
      confirmText: taskCount ? '同步' : '知道了',
      cancelText: '不同步',
      showCancel: taskCount > 0,
      success(result) {
        if (activeReport !== report || sending) return
        if (!result.confirm || !taskCount) { release(true); return }
        if (app.globalData.userId !== report.ownerId) { release(true); return }
        sending = true
        Promise.resolve().then(() => api.syncPersonalizationTimes(report.items, {
          isStale: () => app.globalData.userId !== report.ownerId
        })).then(response => {
          if (!response.ok) throw new Error(response.message || '同步失败')
          if (isActive() && app.globalData.userId === report.ownerId) platform.showToast({ title: '已同步到个性化', icon: 'success' })
          release(true)
        }).catch(() => {
          if (!isActive()) { release(false); return }
          if (app.globalData.userId === report.ownerId) {
            platform.showModal({
              title: '同步失败', content: '本次实际用时已保留，是否重试？', confirmText: '重试', cancelText: '不同步',
              success(retry) {
                if (retry.confirm && isActive()) { activeReport = null; showPending(app, platform, api, options) }
                else release(!retry.confirm)
              }, fail() { release(false) }
            })
          } else release(true)
        })
      },
      fail() { release(false) }
    })
  }
  if (platform.nextTick) platform.nextTick(ask)
  else setTimeout(ask, 0)
  return true
}

module.exports = { buildItems, enqueue, showPending }