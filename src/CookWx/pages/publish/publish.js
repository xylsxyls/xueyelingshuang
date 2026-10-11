const api = require('../../utils/api')
const assets = require('../../utils/recipeAssets')
const tabBar = require('../../utils/tabBar')
const videoUpload = require('../../utils/videoUpload')
const personalizationSync = require('../../utils/cookPersonalizationSync')

Page({
  data: {
    coverPath: '',
    coverFileSize: 0,
    uploadedCoverId: '',
    mediaPath: '',
    mediaFileName: '',
    mediaFileSizeBytes: 0,
    mediaType: '',
    uploadedVideoId: '',
    caption: '',
    ownedRecipes: [],
    selectedMap: {},
    publishing: false,
    uploadProgress: 0
  },

  onShow() {
    tabBar.showHome()
    this.pageActive = true
    personalizationSync.showPending(getApp(), wx, api, { isActive: () => this.pageActive !== false })
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
    api.getRecipes({ isStale, catalog: true })
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
    return recipes.map((item) => Object.assign({}, assets.normalize(item), {
      selected: !!selectedMap[item.id]
    }))
  },

  chooseCover() {
    if (this.data.publishing) return
    wx.chooseMedia({ count: 1, mediaType: ['image'], sourceType: ['album'], success: result => {
      const file = (result.tempFiles || [])[0]
      if (!file) return
      if (!file.size || file.size > 4 * 1024 * 1024) { wx.showToast({ title: '封面不得超过4MB', icon: 'none' }); return }
      wx.getImageInfo({ src: file.tempFilePath, success: info => {
        if (info.width !== info.height || info.width > 4096 || !/^(png|jpeg|jpg)$/i.test(info.type || '')) { wx.showToast({ title: '请选择正方形PNG或JPEG图片', icon: 'none' }); return }
        this.setData({ coverPath: file.tempFilePath, coverFileSize: Number(file.size), uploadedCoverId: '' })
      }, fail: () => wx.showToast({ title: '无法读取封面图片', icon: 'none' }) })
    } })
  },

  removeCover() { if (!this.data.publishing) this.setData({ coverPath: '', coverFileSize: 0, uploadedCoverId: '' }) },

  chooseMedia() {
    wx.chooseMedia({
      count: 1,
      mediaType: ['video'],
      sourceType: ['album', 'camera'],
      success: (res) => {
        const file = res.tempFiles && res.tempFiles[0]
        if (!file) return
        this.setData({
          mediaPath: file.tempFilePath,
          mediaFileName: file.tempFilePath.split(/[\\/]/).pop(),
          mediaFileSizeBytes: Number(file.size) || 0,
          mediaType: 'video',
          uploadedVideoId: '',
          uploadProgress: 0
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
    if (this.data.publishing) return
    const ids = Object.keys(this.data.selectedMap)
    if (!this.data.mediaPath) {
      wx.showToast({ title: '先选择视频', icon: 'none' })
      return
    }
    if (this.data.coverPath && (!ids.length || ids.some(id => !this.data.ownedRecipes.some(recipe => recipe.id === id && recipe.authorUserId === getApp().globalData.userId)))) {
      wx.showToast({ title: '封面只可用于自己的菜谱', icon: 'none' }); return
    }
    wx.showModal({
      title: '确认发布',
      content: ids.length ? `发布并挂载 ${ids.length} 个菜谱？` : '发布不挂载菜谱的视频？',
      success: (res) => {
        if (!res.confirm) return
        this.setData({ publishing: true, uploadProgress: 0 })
        const upload = this.data.uploadedVideoId
          ? Promise.resolve({ id: this.data.uploadedVideoId })
          : videoUpload.uploadVideoFile(this.data.mediaPath, this.data.mediaFileName, this.data.mediaFileSizeBytes, (progress) => {
            if (this.pageActive !== false) this.setData({ uploadProgress: progress })
          })
        upload
          .then(async (uploadedVideo) => {
            this.setData({ uploadedVideoId: uploadedVideo.id, uploadProgress: 100 })
            let coverImageId = this.data.uploadedCoverId
            if (this.data.coverPath && !coverImageId) {
              const cover = await videoUpload.uploadVideoFile(this.data.coverPath, this.data.coverPath.split(/[\\/]/).pop(), this.data.coverFileSize, null, 'image')
              coverImageId = cover.id
              this.setData({ uploadedCoverId: coverImageId })
            }
            return api.publishVideo({
              coverImageId,
              videoId: uploadedVideo.id,
              caption: this.data.caption,
              recipeIds: ids
            })
          })
          .then((publishRes) => {
            if (!publishRes.ok) throw new Error(publishRes.message)
            wx.showToast({ title: '发布成功', icon: 'success' })
            this.setData({ coverPath: '', coverFileSize: 0, uploadedCoverId: '', mediaPath: '', mediaFileName: '', mediaFileSizeBytes: 0, mediaType: '', uploadedVideoId: '', caption: '', selectedMap: {}, uploadProgress: 0 })
          })
          .catch((err) => wx.showToast({ title: err.message || '发布失败', icon: 'none' }))
          .then(() => this.setData({ publishing: false }), () => this.setData({ publishing: false }))
      }
    })
  }
})