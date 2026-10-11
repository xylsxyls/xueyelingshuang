const api = require('../../utils/api')
const assets = require('../../utils/recipeAssets')
const actions = require('../../utils/menuActions')
const social = require('../../utils/socialActions')
const users = require('../../utils/userPresentation')

Page({
  data: { userId: '', profile: {}, ownProfile: false, activeTab: 'created', items: [], loading: false, loadingMore: false, hasMore: false, followingBusy: false, error: '' },
  onLoad(options) { this.setData({ userId: options.userId || '' }) },
  onShow() { this.pageActive = true; this.setData({ followingBusy: false }); this.load(false) },
  onHide() { this.invalidate() },
  onUnload() { this.invalidate() },
  invalidate() { this.pageActive = false; this.pageGeneration = (this.pageGeneration || 0) + 1; this.requestId = (this.requestId || 0) + 1; this.loadingRequest = false },
  async load(append) {
    if (this.loadingRequest || !this.pageActive || (append && !this.data.hasMore)) return
    const requestId = this.requestId = (this.requestId || 0) + 1
    const mode = this.data.activeTab
    const isStale = () => !this.pageActive || this.requestId !== requestId || this.data.activeTab !== mode
    const previous = append ? this.data.items : []
    this.loadingRequest = true
    this.setData({ loading: !append, loadingMore: !!append, error: '' })
    try {
      const [profile, feed] = await Promise.all([
        api.getUserProfile(this.data.userId, { isStale }),
        api.getUserFeed(this.data.userId, mode, { count: 18, excludeIds: previous.map(item => item.videoId), isStale })
      ])
      actions.requireOk(profile); actions.requireOk(feed)
      if (isStale()) return
      const used = {}; previous.forEach(item => { used[item.videoId] = true })
      const incoming = (feed.items || []).filter(item => !used[item.videoId]).map(item => Object.assign({}, item, { posterUrl: assets.url(item.posterUrl) }))
      this.setData({ profile: users.fromServer(profile.profile), ownProfile: this.data.userId === getApp().globalData.userId, items: previous.concat(incoming), hasMore: !!feed.hasMore && incoming.length > 0 })
    } catch (err) { if (!isStale()) this.setData({ error: err.message || '加载失败' }) }
    finally { if (!isStale()) { this.loadingRequest = false; this.setData({ loading: false, loadingMore: false }) } }
  },
  switchTab(event) {
    const key = event.currentTarget.dataset.key
    if (key === this.data.activeTab || ['created', 'liked'].indexOf(key) < 0) return
    this.requestId = (this.requestId || 0) + 1; this.loadingRequest = false
    this.setData({ activeTab: key, items: [], hasMore: false }, () => this.load(false))
  },
  onReachBottom() { this.load(true) },
  retry() { this.load(false) },
  openVideo(event) { const item = this.data.items[Number(event.currentTarget.dataset.index)]; if (item) wx.navigateTo({ url: assets.videoPage(item) }) },
  async follow() {
    if (this.data.followingBusy || this.data.ownProfile || !this.data.profile.userId) return
    const generation = this.pageGeneration || 0
    const isActive = () => this.pageActive && generation === (this.pageGeneration || 0)
    this.requestId = (this.requestId || 0) + 1
    this.loadingRequest = false
    this.setData({ followingBusy: true, loading: false, loadingMore: false })
    try {
      const result = await social.toggleFollow(this.data.profile, isActive)
      if (result && isActive()) await this.load(false)
    } catch (err) { if (isActive()) wx.showToast({ title: err.message || '关注失败', icon: 'none' }) }
    finally { if (isActive()) this.setData({ followingBusy: false }) }
  },
  sendMessage() { if (this.data.profile.following) wx.navigateTo({ url: `/pages/conversation/conversation?userId=${encodeURIComponent(this.data.userId)}` }) },
  onPosterError(event) {
    const index = Number(event.currentTarget.dataset.index)
    if (this.data.items[index] && this.data.items[index].videoId === event.currentTarget.dataset.id) this.setData({ [`items[${index}].posterUrl`]: '' })
  }
})