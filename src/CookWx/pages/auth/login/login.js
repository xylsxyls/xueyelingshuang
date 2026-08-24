const api = require('../../../utils/api')

Page({
  data: {
    mode: 'login',
    account: '',
    password: '',
    nickname: '',
    submitting: false
  },

  onAccount(e) {
    this.setData({ account: e.detail.value })
  },

  onPassword(e) {
    this.setData({ password: e.detail.value })
  },

  onNickname(e) {
    this.setData({ nickname: e.detail.value })
  },

  switchMode() {
    this.setData({ mode: this.data.mode === 'login' ? 'register' : 'login' })
  },

  submit() {
    if (this.data.submitting) return
    const account = this.data.account.trim()
    const password = this.data.password
    const nickname = this.data.nickname.trim()
    if (!account || !password) {
      wx.showToast({ title: '请输入账号和密码', icon: 'none' })
      return
    }
    this.setData({ submitting: true })
    const action = this.data.mode === 'login'
      ? api.login(account, password)
      : api.register(account, password, nickname || account)
    action
      .then((res) => {
        if (!res.ok) throw new Error(res.message)
        this.setData({ submitting: false })
        wx.switchTab({ url: '/pages/home/home' })
      })
      .catch((err) => {
        this.setData({ submitting: false })
        wx.showToast({ title: err.message || '登录失败', icon: 'none' })
      })
  }
})
