const api = require('../../utils/api')
const debugLog = require('../../utils/debugLog')
const tabBar = require('../../utils/tabBar')

Page({
  data: {
    messages: [],
    loading: false,
    errorText: ''
  },

  onShow() {
    tabBar.showHome()
    this.pageActive = true
    this.load()
  },

  onHide() {
    this.pageActive = false
    this.loadRequestId = (this.loadRequestId || 0) + 1
  },

  onUnload() {
    this.pageActive = false
    this.loadRequestId = (this.loadRequestId || 0) + 1
  },

  load() {
    this.loadRequestId = (this.loadRequestId || 0) + 1
    const requestId = this.loadRequestId
    const isStale = () => requestId !== this.loadRequestId || this.pageActive === false
    debugLog.info('MESSAGES', 'LOAD_START', { requestId })
    this.setData({ loading: true, errorText: '' })
    api.listMessages({ isStale })
      .then((res) => {
        if (!res.ok) throw new Error(res.message)
        if (isStale()) return
        const messages = this.decorate(res.messages || [])
        this.setData({ messages, loading: false, errorText: '' })
        debugLog.info('MESSAGES', 'LOAD_OK', { requestId, count: messages.length })
      })
      .catch((err) => {
        if (api.isStaleError(err) || isStale()) {
          debugLog.info('MESSAGES', 'LOAD_STALE', {
            requestId,
            error: err && err.message ? err.message : String(err)
          })
          return
        }
        debugLog.warn('MESSAGES', 'LOAD_FAIL', {
          requestId,
          error: err && err.message ? err.message : String(err)
        })
        this.setData({
          loading: false,
          errorText: '消息加载失败，正在显示本地提醒。',
          messages: [
            { id: 'm1', icon: '火', title: '安全提醒', text: '大火收汁、油锅和爆炒步骤会要求你留意厨房。' },
            { id: 'm2', icon: '菜', title: '新菜谱', text: '服务端暂时不可用，正在显示本地兜底消息。' }
          ]
        })
      })
  },

  decorate(messages) {
    return messages.map((item, index) => Object.assign({}, item, {
      id: item.id || `message_${index}`,
      title: item.title || '系统消息',
      text: item.text || '',
      icon: item.icon || (item.type === 'follow' ? '关' : (item.type === 'comment' ? '评' : (item.type === 'cook_result' ? '秀' : '菜')))
    }))
  }
})
