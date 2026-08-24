const voice = require('../../utils/voice')

Page({
  data: {
    voiceGender: 'female',
    providerLabel: '未启用'
  },

  onShow() {
    this.setData({
      voiceGender: voice.getGender(),
      providerLabel: voice.providerLabel()
    })
  },

  choose(e) {
    const value = e.currentTarget.dataset.value
    voice.setGender(value)
    this.setData({ voiceGender: value })
    const label = voice.genderLabel(value)
    wx.showToast({ title: `已选择${label}`, icon: 'success' })
    voice.speak(`已选择${label}。接下来做菜步骤会用这个音色播报。`, value)
      .then((ok) => {
        if (!ok) wx.showToast({ title: '语音暂未启用', icon: 'none' })
      })
  }
})
