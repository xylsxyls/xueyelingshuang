const api = require('../../utils/api')
const actions = require('../../utils/menuActions')
const social = require('../../utils/socialActions')

Page({
  data: { users: [], hasMore: true, loading: false, error: '' },
  onShow() { this.pageActive = true; this.cursor = ''; this.setData({ users: [], hasMore: true }, () => this.load()) },
  onHide() { this.pageActive = false; this.requestId = (this.requestId || 0) + 1; this.busy = false },
  onUnload() { this.onHide() },
  onReachBottom() { this.load() },
  async load() {
    if (this.busy || !this.data.hasMore || !this.pageActive) return
    this.busy = true
    const requestId = this.requestId = (this.requestId || 0) + 1
    const isStale = () => !this.pageActive || this.requestId !== requestId
    this.setData({ loading: true, error: '' })
    try {
      const response = actions.requireOk(await api.getFans({ data: { count: 30, cursor: this.cursor || '' }, isStale }))
      if (isStale()) return
      const used = {}; this.data.users.forEach(user => { used[user.userId] = true })
      const incoming = (response.users || []).filter(user => !used[user.userId])
      this.cursor = response.cursor || this.cursor
      this.setData({ users: this.data.users.concat(incoming), hasMore: !!response.hasMore && incoming.length > 0 })
    } catch (err) { if (!isStale()) this.setData({ error: err.message || '粉丝加载失败' }) }
    finally { if (!isStale()) { this.busy = false; this.setData({ loading: false }) } }
  },
  openUser(event) { social.openProfile(event.currentTarget.dataset.id) }
})