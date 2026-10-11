const assets = require('./recipeAssets')
const users = require('./userPresentation')

// 所有视频入口使用同一份服务端元数据，刷新不替换解码源。
function fromServer(value) {
  const item = value || {}
  const identity = users.fromServer({ nickname: item.author || item.authorUserId || '厨友', avatarUrl: item.authorAvatarUrl })
  const author = identity.nickname
  const recipeIds = Array.isArray(item.recipeIds) ? item.recipeIds.filter(Boolean) : []
  return Object.assign({}, item, {
    author, authorInitial: identity.avatarInitial,
    authorAvatarUrl: identity.avatarUrl,
    videoUrl: assets.url(item.videoUrl), posterUrl: assets.url(item.posterUrl),
    recipeIds, primaryRecipeId: recipeIds[0] || '',
    liked: item.liked === true, favorite: item.favorite === true, inMenu: item.inMenu === true,
    likes: Math.max(0, Math.floor(Number(item.likes) || 0)), comments: Math.max(0, Math.floor(Number(item.comments) || 0)),
    authorSelf: item.authorSelf === true, authorFollowing: item.authorFollowing === true,
    canFollowAuthor: !!item.authorUserId && item.authorSelf !== true && item.authorFollowing !== true,
    followConfirmVisible: false,
    actionText: recipeIds.length ? '查看菜谱' : '暂无菜谱', canAddToMenu: recipeIds.length > 0,
    menuText: item.inMenu === true ? '已加入菜单' : '加入菜单'
  })
}

function interactionPatch(item) {
  const value = fromServer(item), result = {}
  ;['author', 'authorInitial', 'authorAvatarUrl', 'authorUserId', 'authorSelf', 'authorFollowing', 'canFollowAuthor', 'followConfirmVisible', 'likes', 'comments', 'liked', 'favorite', 'inMenu', 'menuText'].forEach(key => { result[key] = value[key] })
  return result
}

module.exports = { fromServer, interactionPatch }