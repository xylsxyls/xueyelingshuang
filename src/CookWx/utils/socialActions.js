const api = require('./api')
const menuActions = require('./menuActions')

function openProfile(userId) {
  if (!userId) return
  if (userId === getApp().globalData.userId) wx.switchTab({ url: '/pages/mine/mine' })
  else wx.navigateTo({ url: `/pages/profile/profile?userId=${encodeURIComponent(userId)}` })
}

function recordFollow(userId, following) {
  const state = getApp().globalData
  if (!state.followStateChanges) state.followStateChanges = {}
  state.followStateChanges[userId] = !!following
  state.socialStateVersion = Number(state.socialStateVersion || 0) + 1
}

async function toggleFollow(profile, isActive) {
  if (profile.following && !await menuActions.confirm('取消关注', `确定取消关注${profile.nickname || '这位作者'}吗？`, '取消关注')) return null
  if (isActive && !isActive()) return null
  const result = menuActions.requireOk(await api.toggleFollow(profile.userId))
  recordFollow(profile.userId, result.following)
  return result
}

module.exports = { openProfile, recordFollow, toggleFollow }