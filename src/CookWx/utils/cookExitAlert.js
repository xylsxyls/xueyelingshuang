// 一个做菜流程只持有一个退出提醒。先等开启结束，再关闭，关闭成功后才允许页面跳转。
let activeOwner = null

function createCookExitAlert(platform, report, timeoutMilliseconds = 3000) {
  const owner = {}
  let opening = Promise.resolve()
  let closing = null
  let requested = false
  let closed = false
  let disabled = false

  function invoke(method, options, lateSuccess) {
    return new Promise((resolve, reject) => {
      let settled = false
      const finish = (error) => {
        if (settled) return
        settled = true
        clearTimeout(timer)
        if (error) reject(error)
        else resolve()
      }
      const timer = setTimeout(() => finish(new Error('退出提醒操作超时')), timeoutMilliseconds)
      try {
        platform[method](Object.assign({}, options, {
          success() {
            if (settled) {
              if (lateSuccess) lateSuccess()
              return
            }
            finish()
          },
          fail(error) { finish(new Error(error && error.errMsg ? error.errMsg : '退出提醒操作失败')) }
        }))
      } catch (error) {
        finish(error)
      }
    })
  }

  function disable() {
    closed = true
    if (closing) return closing
    if (disabled || !requested) return Promise.resolve()
    if (typeof platform.disableAlertBeforeUnload !== 'function') return Promise.reject(new Error('当前微信版本无法关闭退出提醒'))
    closing = opening.then(() => {
      if (activeOwner !== owner) return
      return invoke('disableAlertBeforeUnload')
    }).then(() => {
      disabled = true
    }).catch((error) => {
      closing = null
      throw error
    })
    return closing
  }

  function enable(message) {
    if (closed || requested || typeof platform.enableAlertBeforeUnload !== 'function') return
    requested = true
    activeOwner = owner
    opening = invoke('enableAlertBeforeUnload', { message }, () => {
      // 开启超时后回调晚到，仍须收回本流程的提醒，防止留在下一页。
      disabled = false
      closing = null
      if (closed && activeOwner === owner) disable().catch(report)
    }).catch(report)
  }

  return { enable, disable }
}

module.exports = { createCookExitAlert }