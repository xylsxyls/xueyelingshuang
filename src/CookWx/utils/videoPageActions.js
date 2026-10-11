const api = require('./api')
const menu = require('./menuActions')
const social = require('./socialActions')

module.exports = {
  openAuthor() { social.openProfile(this.data.item.authorUserId) },
  openRecipe() {
    const id = (this.data.item.recipeIds || [])[0]
    if (id) wx.navigateTo({ url: `/pages/recipe/recipe?id=${encodeURIComponent(id)}` })
  },
  handleRecipeAction() { this.openRecipe() },
  openComments() {
    if (this.data.videoId) wx.navigateTo({ url: `/pages/comments/comments?targetType=video&targetId=${encodeURIComponent(this.data.videoId)}&title=${encodeURIComponent(this.data.item.title || '')}` })
  },
  async toggleLike() {
    if (this.actionBusy) return
    this.actionBusy = true
    this.metadataRequestId = (this.metadataRequestId || 0) + 1
    try {
      const result = menu.requireOk(await api.toggleLike('video', this.data.videoId))
      if (!this.data.active) return
      const item = this.data.item
      this.setData({ 'item.liked': !!result.liked, 'item.likes': Math.max(0, Number(result.likes) || 0) })
    } catch (err) { if (this.data.active) wx.showToast({ title: err.message || '点赞失败', icon: 'none' }) }
    finally { this.actionBusy = false }
  },
  async favorite() {
    if (this.favoriteBusy) return
    this.favoriteBusy = true
    this.metadataRequestId = (this.metadataRequestId || 0) + 1
    try {
      const result = menu.requireOk(await api.toggleFavorite(this.data.videoId, 'video'))
      if (this.data.active) this.setData({ 'item.favorite': !!result.favorite })
    } catch (err) { if (this.data.active) wx.showToast({ title: err.message || '收藏失败', icon: 'none' }) }
    finally { this.favoriteBusy = false }
  },
  async addToMenu() {
    const item = this.data.item
    const id = (item.recipeIds || [])[0]
    if (!id || item.inMenu || this.joinBusy) return
    this.joinBusy = true
    this.metadataRequestId = (this.metadataRequestId || 0) + 1
    try { await menu.join(id); if (this.data.active) this.setData({ 'item.inMenu': true, 'item.menuText': '已加入菜单' }) }
    catch (err) { if (this.data.active) wx.showToast({ title: err.message || '加入失败', icon: 'none' }) }
    finally { this.joinBusy = false }
  },
  async followAuthor() {
    const item = this.data.item
    if (!item.authorUserId || item.authorSelf || this.followBusy) return
    this.followBusy = true
    this.metadataRequestId = (this.metadataRequestId || 0) + 1
    try {
      const result = await social.toggleFollow({ userId: item.authorUserId, following: !!item.authorFollowing, nickname: item.author }, () => this.data.active)
      if (result && this.data.active) this.setData({ 'item.authorFollowing': !!result.following, 'item.canFollowAuthor': !result.following, 'item.followConfirmVisible': !!result.following })
    } catch (err) { if (this.data.active) wx.showToast({ title: err.message || '关注失败', icon: 'none' }) }
    finally { this.followBusy = false }
  },
  goBack() { wx.navigateBack({ delta: 1 }) }
}