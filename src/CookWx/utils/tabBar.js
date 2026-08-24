const debugLog = require('./debugLog')

function setHomeTabText(text) {
  if (typeof wx === 'undefined' || typeof wx.setTabBarItem !== 'function') return
  wx.setTabBarItem({
    index: 0,
    text,
    fail(err) {
      debugLog.warn('TAB_BAR', 'SET_HOME_TEXT_FAIL', {
        text,
        error: err && err.errMsg ? err.errMsg : String(err)
      })
    }
  })
}

function showRefresh() {
  setHomeTabText('刷新')
}

function showHome() {
  setHomeTabText('首页')
}

module.exports = {
  showRefresh,
  showHome
}
