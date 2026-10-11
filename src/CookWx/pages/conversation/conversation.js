const api = require('../../utils/api')
const actions = require('../../utils/menuActions')

Page({
  data: { userId: '', profile: {}, messages: [], text: '', sending: false, loading: false, hasMore: false, error: '', scrollTarget: '' },
  onLoad(options) { this.setData({ userId: options.userId || '' }) },
  onShow() { this.pageActive = true; this.setData({ sending: false }); this.load(false); this.startPolling() },
  onHide() { this.invalidate() },
  onUnload() { this.invalidate() },
  invalidate() { this.pageActive = false; this.pageGeneration = (this.pageGeneration || 0) + 1; this.requestId = (this.requestId || 0) + 1; this.busy = false; if (this.timer) clearTimeout(this.timer); this.timer = null },
  startPolling() {
    if (this.timer) clearTimeout(this.timer)
    this.timer = setTimeout(() => { this.timer = null; if (this.pageActive) { this.load(false); this.startPolling() } }, 15000)
  },
  async load(older) {
    if (this.busy || !this.pageActive || (older && !this.data.hasMore)) return
    this.busy = true
    const requestId = this.requestId = (this.requestId || 0) + 1
    const isStale = () => !this.pageActive || this.requestId !== requestId
    const beforeId = older && this.data.messages.length ? this.data.messages[0].id : ''
    this.setData({ loading: true, error: '' })
    try {
      const response = actions.requireOk(await api.getConversation(this.data.userId, { data: { userId: this.data.userId, beforeId }, isStale }))
      if (isStale()) return
      const old = this.data.messages
      const used = {}; old.forEach(item => { used[item.id] = true })
      const incoming = (response.messages || []).map(item => Object.assign({}, item, { outgoing: item.fromUserId === getApp().globalData.userId }))
      const messages = older ? incoming.filter(item => !used[item.id]).concat(old) : (old.length > 60 ? old.filter(item => !incoming.some(next => next.id === item.id)).concat(incoming) : incoming)
      const last = messages.length ? messages[messages.length - 1].id : ''
      const changed = !old.length || last !== old[old.length - 1].id
      this.setData({ profile: response.profile || {}, messages, hasMore: older || old.length <= 60 ? !!response.hasMore : this.data.hasMore, scrollTarget: !older && changed ? 'chat-' + last : this.data.scrollTarget })
      if (!older) api.readConversation(this.data.userId).catch(() => {})
    } catch (err) { if (!isStale()) this.setData({ error: err.message || '私信加载失败' }) }
    finally { if (!isStale()) { this.busy = false; this.setData({ loading: false }) } }
  },
  loadOlder() { this.load(true) },
  retry() { this.load(false) },
  onInput(event) { this.setData({ text: event.detail.value || '' }) },
  async send() {
    const text = String(this.data.text || '').trim()
    if (this.data.sending || !text || !this.data.profile.following) return
    this.setData({ sending: true })
    const userId = this.data.userId
    const generation = this.pageGeneration || 0
    const isStale = () => !this.pageActive || generation !== (this.pageGeneration || 0)
    try {
      const response = actions.requireOk(await api.sendMessage(userId, text))
      if (isStale()) return
      // 已返回的发送结果使旧轮询失效，避免旧响应覆盖刚发出的消息。
      this.requestId = (this.requestId || 0) + 1
      this.busy = false
      const item = Object.assign({}, response.messageItem, { outgoing: true })
      this.setData({ text: '', messages: this.data.messages.concat(item), scrollTarget: 'chat-' + item.id })
      this.load(false)
    } catch (err) { if (!isStale()) wx.showToast({ title: err.message || '发送失败', icon: 'none' }) }
    finally { if (!isStale()) this.setData({ sending: false }) }
  }
})