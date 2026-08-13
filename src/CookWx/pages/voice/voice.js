Page({
  data: {
    voiceGender: 'female'
  },

  onShow() {
    this.setData({ voiceGender: getApp().globalData.voiceGender || 'female' })
  },

  choose(e) {
    const value = e.currentTarget.dataset.value
    getApp().globalData.voiceGender = value
    this.setData({ voiceGender: value })
    wx.showToast({ title: value === 'male' ? '已选择男声' : '已选择女声', icon: 'success' })
  }
})