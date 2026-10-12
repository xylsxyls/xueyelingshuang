const api = require('../../utils/api')
const media = require('../../utils/privateMediaCache')
const config = require('../../utils/config')
const assets = require('../../utils/recipeAssets')
Page({
  data: { active: false, videoId: '', videoUrl: '', playing: false, images: [], imageIndex: 0, image: '', loading: false, error: '', current: 0, duration: 0 },
  onLoad(options) { this.options = options; this.generation = 0; this.owner = getApp().globalData.userId },
  onShow() { this.visible = true; this.load() },
  onHide() { this.visible = false; ++this.generation; this.setData({ active: false }) },
  onUnload() { this.visible = false; ++this.generation },
  stale(g) { return !this.visible || g !== this.generation || this.owner !== getApp().globalData.userId },
  async load() {
    const g = ++this.generation
    this.setData({ loading: true, error: '' })
    try {
      let draftIds
      try { draftIds = this.options.mediaIds ? JSON.parse(this.options.mediaIds) : null } catch (_) {}
      const result = Array.isArray(draftIds) && draftIds.length <= 9 ? { ok: true, recipe: { mediaIds: draftIds } } : this.options.public === '1' ? await api.getRecipeDetail(this.options.recipeId, { isStale: () => this.stale(g) }) : await api.getRecipeReview(this.options.recipeId, this.options.reviewId, { isStale: () => this.stale(g) })
      if (this.stale(g)) return
      const recipe = this.options.previous === '1' ? result.previousApprovedRecipe : result.recipe
      if (!result.ok || !recipe) { this.setData({ error: result.message || '媒体不可查看' }); return }
      const ids = recipe.mediaIds && recipe.mediaIds.length ? recipe.mediaIds : recipe.videoId ? [recipe.videoId] : []
      if (!ids.length) { this.setData({ error: '这个版本没有原视频或图片组' }); return }
      const metadata = await api.getVideoDetail(ids[0], { isStale: () => this.stale(g) })
      if (this.stale(g)) return
      if (!metadata.ok || !metadata.item) { this.setData({ error: metadata.message || '媒体不可查看' }); return }
      if (metadata.item.mediaType === 'image') { this.setData({ images: ids, videoId: '', imageIndex: 0 }); await this.showImage(0, g) }
      else this.setData({ images: [], videoId: metadata.item.videoId || ids[0], videoUrl: assets.url(metadata.item.videoUrl) || config.BASE_URL + '/api/videos/file?videoId=' + encodeURIComponent(ids[0]), active: true, playing: this.data.videoId === ids[0] ? this.data.playing : true })
    } catch (error) { if (!this.stale(g)) this.setData({ error: error.message || '媒体加载失败' }) }
    finally { if (!this.stale(g)) this.setData({ loading: false }) }
  },
  async showImage(index, generation) {
    const id = this.data.images[index]
    if (!id) return
    this.setData({ loading: true, imageIndex: index })
    try { const file = await media.get(id, true); if (!this.stale(generation) && this.data.imageIndex === index) this.setData({ image: file }) }
    catch (error) { if (!this.stale(generation)) this.setData({ error: error.message || '图片加载失败' }) }
    finally { if (!this.stale(generation)) this.setData({ loading: false }) }
  },
  changeImage(event) { const index = this.data.imageIndex + Number(event.currentTarget.dataset.offset); if (index >= 0 && index < this.data.images.length) this.showImage(index, this.generation) },
  toggle() { this.setData({ playing: !this.data.playing }) },
  onTime(event) { this.setData({ current: Number(event.detail.currentTime) || 0, duration: Number(event.detail.duration) || this.data.duration }) },
  onMetadata(event) { this.setData({ duration: Number(event.detail.duration) || 0 }) },
  seek(event) { const player = this.selectComponent('#reviewVideo'); if (player) player.seek(Number(event.detail.value) || 0) },
  onError(event) { this.setData({ error: event.detail.message || '媒体播放失败' }) }
})