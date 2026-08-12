const api = require('../../utils/api')

Page({
  data: {
    mediaPath: '',
    mediaType: '',
    caption: '',
    ownedRecipes: [],
    selectedMap: {}
  },

  onShow() {
    const app = getApp()
    const selectedMap = {}
    ;(app.globalData.selectedRecipeIds || []).forEach((id) => { selectedMap[id] = true })
    this.setData({ selectedMap })
    this.loadRecipes()
  },

  loadRecipes() {
    api.getRecipes()
      .then((res) => {
        if (!res.ok) throw new Error(res.message)
        this.setData({ ownedRecipes: this.decorateRecipes((res.recipes || []).filter((item) => item.owned)) })
      })
      .catch(() => wx.showToast({ title: '菜谱加载失败', icon: 'none' }))
  },

  decorateRecipes(recipes) {
    const selectedMap = this.data.selectedMap
    return recipes.map((item) => Object.assign({}, item, {
      selected: !!selectedMap[item.id]
    }))
  },

  chooseMedia() {
    wx.chooseMedia({
      count: 1,
      mediaType: ['image', 'video'],
      sourceType: ['album', 'camera'],
      success: (res) => {
        const file = res.tempFiles && res.tempFiles[0]
        if (!file) return
        this.setData({
          mediaPath: file.tempFilePath,
          mediaType: file.fileType || (file.tempFilePath.indexOf('.mp4') >= 0 ? 'video' : 'image')
        })
      },
      fail: () => wx.showToast({ title: '未选择素材', icon: 'none' })
    })
  },

  onCaption(e) {
    this.setData({ caption: e.detail.value })
  },

  toggleRecipe(e) {
    const id = e.currentTarget.dataset.id
    const selectedMap = Object.assign({}, this.data.selectedMap)
    if (selectedMap[id]) delete selectedMap[id]
    else selectedMap[id] = true
    this.setData({ selectedMap }, () => {
      this.setData({ ownedRecipes: this.decorateRecipes(this.data.ownedRecipes) })
    })
  },

  publish() {
    const ids = Object.keys(this.data.selectedMap)
    if (!this.data.mediaPath) {
      wx.showToast({ title: '先选择视频或图片', icon: 'none' })
      return
    }
    if (!ids.length) {
      wx.showToast({ title: '至少挂载一个菜谱', icon: 'none' })
      return
    }
    wx.showModal({
      title: '示例发布成功',
      content: `已挂载 ${ids.length} 个菜谱。正式版这里会上传到服务器并进入审核。`,
      showCancel: false
    })
  }
})