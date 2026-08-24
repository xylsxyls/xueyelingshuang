const api = require('./utils/api')
const voice = require('./utils/voice')

App({
  globalData: {
    userId: '',
    account: null,
    token: '',
    clientType: 'wechat_mini',
    clientVersion: '0.2.0',
    selectedRecipeIds: [],
    selectedRecipes: [],
    lastPreview: null,
    lastPreviewRecipeKey: '',
    lastPreviewNeedsTimeBlocks: false,
    lastPlan: null,
    voiceGender: 'female',
    currentCookVoiceGender: '',
    pendingCookVoiceTexts: [],
    exitCookFlowToMenu: false,
    mineInitialTab: ''
  },

  onLaunch() {
    const token = wx.getStorageSync(api.TOKEN_KEY) || ''
    const account = wx.getStorageSync(api.ACCOUNT_KEY) || null
    const voiceGender = wx.getStorageSync(voice.STORAGE_KEY) || 'female'
    this.globalData.token = token
    this.globalData.account = account
    this.globalData.userId = account && account.userId ? account.userId : ''
    this.globalData.voiceGender = voiceGender
    if (!token) {
      setTimeout(() => {
        wx.reLaunch({ url: '/pages/auth/login/login' })
      }, 0)
      return
    }
    api.session()
      .then((res) => {
        if (res.ok && res.account) {
          this.globalData.account = res.account
          this.globalData.userId = res.account.userId || ''
          wx.setStorageSync(api.ACCOUNT_KEY, res.account)
        }
      })
      .catch(() => {})
  }
})
