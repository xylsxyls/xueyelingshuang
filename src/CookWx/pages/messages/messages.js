const api = require('../../utils/api')
const config = require('../../utils/config')
const debugLog = require('../../utils/debugLog')
const tabBar = require('../../utils/tabBar')

const CONTACT_TABS = [
  { key: 'messages', title: '消息' },
  { key: 'following', title: '关注' },
  { key: 'friends', title: '好友' }
]

function firstDisplayChar(text, fallback) {
  const value = text === undefined || text === null ? '' : String(text).trim()
  if (!value) return fallback || ''
  return value.charAt(0)
}

Page({
  data: {
    contactTabs: CONTACT_TABS,
    activeContactTab: 'messages',
    messages: [],
    followingContacts: [],
    friendContacts: [],
    contactItems: [],
    selectedContact: {},
    sendText: '',
    sending: false,
    unfollowingUserId: '',
    loading: false,
    errorText: ''
  },

  onShow() {
    tabBar.showHome()
    this.pageActive = true
    const app = getApp()
    const initialTab = app.globalData.messageInitialTab || ''
    this.pendingTargetUserId = app.globalData.messageTargetUserId || ''
    app.globalData.messageInitialTab = ''
    app.globalData.messageTargetUserId = ''
    if (initialTab) {
      this.setData({ activeContactTab: initialTab }, () => {
        this.load()
        this.startMessagePolling()
      })
      return
    }
    this.load()
    this.startMessagePolling()
  },

  onHide() {
    this.pageActive = false
    this.stopMessagePolling()
    this.loadRequestId = (this.loadRequestId || 0) + 1
  },

  onUnload() {
    this.pageActive = false
    this.stopMessagePolling()
    this.loadRequestId = (this.loadRequestId || 0) + 1
  },

  load(options) {
    const safeOptions = options || {}
    const background = safeOptions.background === true
    if (background && this.backgroundLoading) return
    if (background) this.backgroundLoading = true
    this.loadRequestId = (this.loadRequestId || 0) + 1
    const requestId = this.loadRequestId
    const isStale = () => requestId !== this.loadRequestId || this.pageActive === false
    debugLog.info('MESSAGES', 'LOAD_START', { requestId, activeTab: this.data.activeContactTab, background })
    if (!background) this.setData({ loading: true, errorText: '' })
    api.listMessages({ isStale })
      .then((res) => {
        if (background) this.backgroundLoading = false
        if (!res.ok) throw new Error(res.message)
        if (isStale()) return
        const messages = this.decorateMessages(res.messages || [])
        const followingContacts = this.decorateContacts(res.followingUsers || [])
        const friendContacts = this.decorateContacts(res.friendUsers || [])
        const contactItems = this.contactsForTab(this.data.activeContactTab, followingContacts, friendContacts)
        const selectedContact = this.pickSelectedContact(contactItems, this.pendingTargetUserId, this.data.selectedContact)
        this.pendingTargetUserId = ''
        this.setData({
          messages,
          followingContacts,
          friendContacts,
          contactItems,
          selectedContact,
          loading: false,
          errorText: ''
        })
        debugLog.info('MESSAGES', 'LOAD_OK', {
          requestId,
          messageCount: messages.length,
          followingCount: followingContacts.length,
          friendCount: friendContacts.length,
          background
        })
      })
      .catch((err) => {
        if (background) this.backgroundLoading = false
        if (api.isStaleError(err) || isStale()) {
          debugLog.info('MESSAGES', 'LOAD_STALE', {
            requestId,
            error: err && err.message ? err.message : String(err)
          })
          return
        }
        if (background) {
          debugLog.warn('MESSAGES', 'POLL_FAIL', {
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
            { id: 'm1', icon: '!', title: '安全提醒', text: '大火收汁、油锅和爆炒步骤会要求你留意厨房。' },
            { id: 'm2', icon: '+', title: '新菜谱', text: '服务端暂时不可用，正在显示本地兜底消息。' }
          ],
          contactItems: [],
          selectedContact: {}
        })
      })
  },

  startMessagePolling() {
    this.stopMessagePolling()
    const intervalMs = Number(config.MESSAGE_POLL_INTERVAL_MS || 0)
    if (intervalMs <= 0) return
    this.messagePollTimer = setInterval(() => {
      if (!this.pageActive || this.data.loading || this.data.sending || this.backgroundLoading) return
      this.load({ background: true })
    }, Math.max(3000, intervalMs))
    debugLog.info('MESSAGES', 'POLL_START', { intervalMs: Math.max(3000, intervalMs) })
  },

  stopMessagePolling() {
    if (this.messagePollTimer) {
      clearInterval(this.messagePollTimer)
      this.messagePollTimer = null
      debugLog.info('MESSAGES', 'POLL_STOP', {})
    }
    this.backgroundLoading = false
  },

  decorateMessages(messages) {
    return messages.map((item, index) => Object.assign({}, item, {
      id: item.id || `message_${index}`,
      title: item.title || '系统消息',
      text: item.text || '',
      icon: item.icon || this.messageIcon(item.type)
    }))
  },

  decorateContacts(contacts) {
    return contacts.map((item, index) => {
      const name = item.nickname || item.account || item.userId || '厨友'
      return Object.assign({}, item, {
        id: item.userId || `contact_${index}`,
        name,
        avatarText: firstDisplayChar(name, '厨'),
        relationText: item.friend ? '互相关注' : '已关注',
        summary: `${item.friend ? '互相关注' : '已关注'} · 粉丝 ${Number(item.followerCount || 0)} · 关注 ${Number(item.followingCount || 0)}`
      })
    })
  },

  messageIcon(type) {
    if (type === 'follow') return '+'
    if (type === 'comment') return '评'
    if (type === 'cook_result') return '秀'
    if (type === 'chat') return '私'
    return '菜'
  },

  contactsForTab(tabKey, followingContacts, friendContacts) {
    if (tabKey === 'friends') return friendContacts || []
    if (tabKey === 'following') return followingContacts || []
    return []
  },

  pickSelectedContact(contactItems, targetUserId, oldContact) {
    if (!contactItems.length) return {}
    if (targetUserId) {
      for (let i = 0; i < contactItems.length; i += 1) {
        if ((contactItems[i].userId || '') === targetUserId) return contactItems[i]
      }
    }
    if (oldContact && oldContact.userId) {
      for (let i = 0; i < contactItems.length; i += 1) {
        if ((contactItems[i].userId || '') === oldContact.userId) return contactItems[i]
      }
    }
    return contactItems[0]
  },

  switchContactTab(e) {
    const key = e.currentTarget.dataset.key || 'messages'
    debugLog.info('MESSAGES', 'SWITCH_TAB', { from: this.data.activeContactTab, to: key })
    const contactItems = this.contactsForTab(key, this.data.followingContacts, this.data.friendContacts)
    this.setData({
      activeContactTab: key,
      contactItems,
      selectedContact: this.pickSelectedContact(contactItems, '', {})
    })
  },

  selectContact(e) {
    const index = Number(e.currentTarget.dataset.index)
    const selectedContact = this.data.contactItems[index] || {}
    debugLog.info('MESSAGES', 'SELECT_CONTACT', {
      activeTab: this.data.activeContactTab,
      userId: selectedContact.userId || ''
    })
    this.setData({ selectedContact })
  },

  unfollowContact(e) {
    const index = Number(e.currentTarget.dataset.index)
    const contact = this.data.contactItems[index] || {}
    const targetUserId = contact.userId || ''
    debugLog.info('MESSAGES', 'UNFOLLOW_TAP', {
      activeTab: this.data.activeContactTab,
      targetUserId
    })
    if (!targetUserId || this.data.unfollowingUserId) return
    wx.showModal({
      title: '取消关注',
      content: `不再关注 ${contact.name || '这个厨友'}？`,
      confirmText: '取消关注',
      confirmColor: '#b45232',
      success: (modalRes) => {
        if (!modalRes.confirm) return
        this.setData({ unfollowingUserId: targetUserId })
        api.toggleFollow(targetUserId)
          .then((res) => {
            if (!res.ok) throw new Error(res.message)
            if (res.following) {
              this.setData({ unfollowingUserId: '' })
              this.recordGlobalFollowState(targetUserId, true)
              wx.showToast({ title: '关注状态已恢复', icon: 'none' })
              this.load()
              return
            }
            this.recordGlobalFollowState(targetUserId, false)
            const followingContacts = (this.data.followingContacts || []).filter((item) => (item.userId || '') !== targetUserId)
            const friendContacts = (this.data.friendContacts || []).filter((item) => (item.userId || '') !== targetUserId)
            const contactItems = this.contactsForTab(this.data.activeContactTab, followingContacts, friendContacts)
            const selectedContact = this.pickSelectedContact(contactItems, '', this.data.selectedContact && this.data.selectedContact.userId === targetUserId ? {} : this.data.selectedContact)
            this.setData({
              followingContacts,
              friendContacts,
              contactItems,
              selectedContact,
              unfollowingUserId: ''
            })
            wx.showToast({ title: '已取消关注', icon: 'none' })
            debugLog.info('MESSAGES', 'UNFOLLOW_OK', { targetUserId })
          })
          .catch((err) => {
            this.setData({ unfollowingUserId: '' })
            debugLog.warn('MESSAGES', 'UNFOLLOW_FAIL', {
              targetUserId,
              error: err && err.message ? err.message : String(err)
            })
            wx.showToast({ title: err.message || '取消关注失败', icon: 'none' })
          })
      }
    })
  },

  recordGlobalFollowState(userId, following) {
    if (!userId) return
    const app = getApp()
    if (!app.globalData.followStateChanges) app.globalData.followStateChanges = {}
    app.globalData.followStateChanges[userId] = !!following
    app.globalData.socialStateVersion = Number(app.globalData.socialStateVersion || 0) + 1
  },

  onSendInput(e) {
    this.setData({ sendText: e.detail.value || '' })
  },

  sendContactMessage() {
    if (this.data.sending) return
    const targetUserId = this.data.selectedContact.userId || ''
    const text = String(this.data.sendText || '').trim()
    debugLog.info('MESSAGES', 'SEND_TAP', {
      targetUserId,
      textLength: text.length
    })
    if (!targetUserId) {
      wx.showToast({ title: '请选择联系人', icon: 'none' })
      return
    }
    if (!text) {
      wx.showToast({ title: '请输入消息', icon: 'none' })
      return
    }
    this.setData({ sending: true })
    api.sendMessage(targetUserId, text)
      .then((res) => {
        if (!res.ok) throw new Error(res.message)
        wx.showToast({ title: res.message || '已发送', icon: 'success' })
        this.setData({ sendText: '', sending: false })
        this.load()
        debugLog.info('MESSAGES', 'SEND_OK', { targetUserId })
      })
      .catch((err) => {
        this.setData({ sending: false })
        debugLog.warn('MESSAGES', 'SEND_FAIL', {
          targetUserId,
          error: err && err.message ? err.message : String(err)
        })
        wx.showToast({ title: err.message || '发送失败', icon: 'none' })
      })
  }
})