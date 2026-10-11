const api = require('../../utils/api')
const debugLog = require('../../utils/debugLog')

function pad2(value) {
  const text = String(value)
  return text.length < 2 ? `0${text}` : text
}

Page({
  data: {
    targetType: 'video',
    targetId: '',
    comments: [],
    content: '',
    loading: false,
    submitting: false
  },

  onLoad(options) {
    const targetType = options.targetType === 'recipe' ? 'recipe' : 'video'
    const targetId = options.targetId || ''
    this.pageActive = true
    this.requestId = 0
    this.setData({ targetType, targetId })
    if (targetId) this.loadComments()
  },

  onUnload() {
    this.pageActive = false
    this.requestId += 1
  },

  loadComments() {
    if (!this.data.targetId || this.data.loading) return
    this.requestId += 1
    const requestId = this.requestId
    const isStale = () => !this.pageActive || requestId !== this.requestId
    this.setData({ loading: true })
    api.listComments(this.data.targetType, this.data.targetId, { isStale })
      .then((res) => {
        if (isStale()) return
        if (!res.ok) throw new Error(res.message || '评论加载失败')
        this.setData({ comments: this.decorateComments(res.comments || []), loading: false })
      })
      .catch((err) => {
        if (api.isStaleError(err) || isStale()) return
        this.setData({ loading: false })
        wx.showToast({ title: err.message || '评论加载失败', icon: 'none' })
      })
  },

  onInput(event) {
    this.setData({ content: String(event.detail.value || '').slice(0, 600) })
  },

  decorateComments(comments) {
    return (Array.isArray(comments) ? comments : []).map((item) => {
      const timestamp = Math.max(0, Number(item.createdAtSeconds) || 0) * 1000
      const date = timestamp ? new Date(timestamp) : null
      const createdAtText = date
        ? `${date.getFullYear()}-${pad2(date.getMonth() + 1)}-${pad2(date.getDate())} ${pad2(date.getHours())}:${pad2(date.getMinutes())}`
        : ''
      return Object.assign({}, item, {
        avatarText: String(item.nickname || '厨友').slice(0, 1),
        createdAtText
      })
    })
  },

  submitComment() {
    const content = String(this.data.content || '').trim()
    if (!content || this.data.submitting) return
    this.setData({ submitting: true })
    api.createComment(this.data.targetType, this.data.targetId, content, 0)
      .then((res) => {
        if (!res.ok) throw new Error(res.message || '评论发送失败')
        const comment = res.comment
        this.setData({
          content: '',
          submitting: false,
          comments: comment ? this.data.comments.concat(this.decorateComments([comment])) : this.data.comments
        })
        debugLog.info('COMMENTS', 'CREATE_OK', { targetType: this.data.targetType, targetId: this.data.targetId })
      })
      .catch((err) => {
        this.setData({ submitting: false })
        wx.showToast({ title: err.message || '评论发送失败', icon: 'none' })
      })
  }
})
