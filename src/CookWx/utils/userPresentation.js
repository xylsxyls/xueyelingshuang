const assets = require('./recipeAssets')

// 视频作者、本人与公开主页从同一昵称和头像产生显示数据。
function fromServer(value, fallback) {
  const profile = value || {}, defaults = fallback || {}
  const nickname = String(profile.nickname || defaults.nickname || defaults.account || profile.userId || defaults.userId || '厨友').trim() || '厨友'
  return Object.assign({}, profile, {
    nickname,
    avatarInitial: Array.from(nickname)[0],
    avatarUrl: assets.url(String(profile.avatarUrl || defaults.avatarUrl || '').trim())
  })
}

module.exports = { fromServer }