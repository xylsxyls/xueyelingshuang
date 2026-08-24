const api = require('../../utils/api')
const tabBar = require('../../utils/tabBar')

Page({
  data: {
    mediaPath: '',
    mediaType: '',
    caption: '',
    ownedRecipes: [],
    selectedMap: {}
  },

  onShow() {
    tabBar.showHome()
    this.pageActive = true
    const app = getApp()
    const selectedMap = {}
    ;(app.globalData.selectedRecipeIds || []).forEach((id) => { selectedMap[id] = true })
    this.setData({ selectedMap })
    this.loadRecipes()
  },

  onHide() {
    this.pageActive = false
    this.loadRecipesRequestId = (this.loadRecipesRequestId || 0) + 1
  },

  onUnload() {
    this.pageActive = false
    this.loadRecipesRequestId = (this.loadRecipesRequestId || 0) + 1
  },

  loadRecipes() {
    this.loadRecipesRequestId = (this.loadRecipesRequestId || 0) + 1
    const requestId = this.loadRecipesRequestId
    const isStale = () => requestId !== this.loadRecipesRequestId || this.pageActive === false
    api.getRecipes({ isStale })
      .then((res) => {
        if (!res.ok) throw new Error(res.message)
        if (isStale()) return
        this.setData({ ownedRecipes: this.decorateRecipes((res.recipes || []).filter((item) => item.owned)) })
      })
      .catch((err) => {
        if (api.isStaleError(err) || isStale()) return
        wx.showToast({ title: '菜谱加载失败', icon: 'none' })
      })
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
      title: '确认发布',
      content: `发布并挂载 ${ids.length} 个菜谱？`,
      success: (res) => {
        if (!res.confirm) return
        api.uploadVideoMeta({
          mediaType: this.data.mediaType || 'video',
          filePath: this.data.mediaPath,
          fileSizeBytes: 0
        })
          .then((uploadRes) => {
            if (!uploadRes.ok) throw new Error(uploadRes.message)
            return api.publishVideo({
              videoId: uploadRes.video ? uploadRes.video.id : '',
              caption: this.data.caption,
              recipeIds: ids
            })
          })
          .then((publishRes) => {
            if (!publishRes.ok) throw new Error(publishRes.message)
            wx.showToast({ title: '发布成功', icon: 'success' })
            this.setData({ mediaPath: '', mediaType: '', caption: '', selectedMap: {} })
          })
          .catch((err) => wx.showToast({ title: err.message || '发布失败', icon: 'none' }))
      }
    })
  }
})
