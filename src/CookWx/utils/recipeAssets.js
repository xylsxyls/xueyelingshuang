const config = require('./config')

const CATEGORIES = ['荤菜', '小荤', '蔬菜', '汤饮', '主食']

function url(value) {
  const text = String(value || '')
  return text.charAt(0) === '/' ? config.BASE_URL + text : text
}

function normalize(recipe) {
  const item = recipe || {}
  const priceAmount = Number(item.priceAmount || item.priceCoins || 0)
  const priceType = item.priceType || (priceAmount > 0 ? 'coin' : 'free')
  const free = priceType === 'free' || priceAmount <= 0
  const coverImageUrl = url(item.coverImageUrl)
  const posterUrl = url(item.posterUrl)
  const categories = { '肉菜': '荤菜', '素菜': '蔬菜', '汤': '汤饮', '汤品': '汤饮', '荤素': '小荤' }
  const category = categories[item.category] || item.category
  return Object.assign({}, item, {
    category: String(category || '小荤'),
    inMenu: item.inMenu === true,
    coverImageUrl, posterUrl, thumbnailUrl: coverImageUrl || posterUrl,
    videoUrl: url(item.videoUrl),
    priceAmount, priceType, free,
    accessible: !!item.owned || free,
    priceText: item.purchased ? '已拥有' : (free ? '免费菜谱' : `${priceAmount}${priceType === 'yuanbao' ? '元宝' : '金币'}`),
    canRemove: !!item.inMenu && !item.purchased && free,
    canBuy: !item.owned && !free
  })
}

function thumbnailFallback(item, failedUrl) {
  if (item.thumbnailUrl !== failedUrl) return item
  return Object.assign({}, item, { thumbnailUrl: failedUrl !== item.posterUrl ? item.posterUrl : '' })
}

function videoPage(item) {
  return `/pages/videoPlayer/videoPlayer?videoId=${encodeURIComponent(item.videoId || item.id || '')}&src=${encodeURIComponent(url(item.videoUrl))}&poster=${encodeURIComponent(url(item.posterUrl))}&title=${encodeURIComponent(item.title || '')}`
}

module.exports = { CATEGORIES, url, normalize, thumbnailFallback, videoPage }