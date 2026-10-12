const assert = require('node:assert/strict')
const fs = require('node:fs')
const storage = require('../lib/isolatedStorage')
const rectangle = Buffer.from([137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,2,0,0,0,1,8,2,0,0,0,123,64,232,221,0,0,0,11,73,68,65,84,120,156,99,104,0,3,0,10,135,3,1,196,198,31,102,0,0,0,0,73,69,78,68,174,66,96,130])
async function upload(http, token, mediaType, bytes) {
  const start = await http.json('/api/videos/upload/start', { token, body: { mediaType, fileName: mediaType === 'video' ? 'review.mp4' : 'review.png', fileSizeBytes: bytes.length } })
  for (let offset = 0; offset < bytes.length; offset += 512 * 1024) await http.json('/api/videos/upload/chunk?uploadId=' + start.uploadId + '&offset=' + offset, { token, body: bytes.subarray(offset, offset + 512 * 1024), headers: { 'Content-Type': 'application/octet-stream' } })
  return (await http.json('/api/videos/upload/complete', { token, body: { uploadId: start.uploadId } })).video.id
}
async function run(context, admin) {
  const { http } = context
  const user = await http.json('/api/auth/register', { body: { account: 'media_' + context.reporter.runId.replace(/[^a-z0-9]/gi, '').slice(-30), password: 'MediaReview123!', nickname: '媒体测试' } })
  const token = user.token, bytes = fs.readFileSync(context.environment.fixture)
  const first = await upload(http, token, 'video', bytes)
  const source = { id: 'video-source', schemaVersion: '1.0', title: '审核视频作品', category: '小荤', priceType: 'coin', priceAmount: 2,
    mediaIds: [first], tasks: [{ id: 'cut', title: '切小块', active: true, durationSeconds: 10 }] }
  let recipe = (await http.json('/api/recipes/custom/publish', { token, body: { recipe: source } })).recipe
  assert.equal((await http.request('/api/videos/file?videoId=' + first)).status, 404)
  assert.equal((await http.request('/api/videos/poster?videoId=' + first, { token: admin.token })).status, 200)
  assert.equal((await http.request('/api/videos/poster?videoId=' + first)).status, 404, '生成后的私有封面不能从共享内存缓存泄露')
  const segment = await http.request('/api/videos/segment?videoId=' + first + '&startSeconds=0&durationSeconds=3', { token: admin.token })
  assert.equal(segment.status, 200)
  assert.match(segment.headers['cache-control'], /private/)
  assert.equal((await http.request('/api/videos/segment?videoId=' + first + '&startSeconds=0&durationSeconds=3')).status, 404)
  await http.json('/api/recipes/reviews/decide', { token: admin.token, body: { reviewId: recipe.reviewId, decision: 'approved' } })
  const second = await upload(http, token, 'video', bytes)
  recipe = (await http.json('/api/recipes/creation/save', { token, body: { recipeId: recipe.id, currentRevisionId: recipe.currentRevisionId, editorVersion: 2, steps: recipe.tasks, mediaIds: [second] } })).recipe
  const detail = await http.json('/api/recipes/reviews/detail?recipeId=' + recipe.id, { token: admin.token })
  assert.deepEqual(detail.previousApprovedRecipe.mediaIds, [first]); assert.deepEqual(detail.recipe.mediaIds, [second])
  const bypass = await http.request('/api/videos/publish', { token, body: { videoId: second, recipeIds: [] } })
  assert.equal(bypass.json.ok, false)
  await http.json('/api/recipes/reviews/decide', { token: admin.token, body: { reviewId: recipe.reviewId, decision: 'rejected', reason: '视频待完善' } })
  const photoA = await upload(http, token, 'photo', rectangle), photoB = await upload(http, token, 'photo', rectangle)
  source.id = 'photo-source'; source.mediaIds = [photoA, photoB]
  const photos = (await http.json('/api/recipes/custom/publish', { token, body: { recipe: source } })).recipe
  const poster = await http.request('/api/videos/poster?videoId=' + photoA, { token: admin.token })
  assert.equal(poster.status, 200, '普通PNG图片组需能生成封面，不能依赖未随包的FFmpeg PNG解码')
  assert.equal(poster.data[0], 255); assert.equal(poster.data[1], 216)
  assert.equal((await http.request('/api/videos/poster?videoId=' + photoA)).status, 404)
  assert.deepEqual((await http.json('/api/recipes/reviews/detail?recipeId=' + photos.id, { token: admin.token })).recipe.mediaIds, [photoA, photoB])
  await storage.stopOwnedBackend(context); await storage.startOwnedBackend(context)
  const oldVideo = await http.request('/api/videos/file?videoId=' + first)
  assert.equal(oldVideo.status, 200); assert.deepEqual(oldVideo.data, bytes)
  assert.equal((await http.request('/api/videos/file?videoId=' + second)).status, 404)
  assert.deepEqual((await http.request('/api/videos/file?videoId=' + second, { token: admin.token })).data, bytes)
  assert.deepEqual((await http.request('/api/videos/file?videoId=' + photoA, { token: admin.token })).data, rectangle)
  return { twoVideosPreserved: true, rectangularPhotoGroupPreserved: true, privatePosterCacheProtected: true, privateSegmentProtected: true, publishingBypassRejected: true }
}
module.exports = { run }