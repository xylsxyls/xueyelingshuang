const assets = require('./recipeAssets')
const review = require('./recipeReview')

// 作者页按菜谱列出创作；未关联菜谱的视频仍保留原视频入口。
function build(recipes, videos, owner, toRecipeAsset) {
  const own = (recipes || []).filter(recipe => owner && recipe.authorUserId === owner)
  const recipeIds = new Set(own.map(recipe => recipe.id))
  const rows = own.map(recipe => Object.assign(toRecipeAsset(recipe, review.status(recipe)), {
    creation: true, reviewReason: recipe.reviewReason || '', reviewId: recipe.reviewId || '', mediaId: recipe.coverImageId || recipe.videoId || (recipe.mediaIds || [])[0] || '', privateMedia: ['pending', 'rejected'].includes(recipe.reviewState), videoId: recipe.videoId || '', posterUrl: assets.normalize(recipe).posterUrl
  }))
  return rows.concat((videos || []).filter(video => !video.recipeId || !recipeIds.has(video.recipeId))
    .map(video => Object.assign({}, video, { thumbnailUrl: video.posterUrl, subtitle: video.subtitle || (video.recipeId ? '关联菜谱不属于你，点击播放原视频' : '未关联菜谱，点击播放原视频') })))
}

module.exports = { build }