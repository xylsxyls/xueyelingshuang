// 指定 filePath 避免产生不可 unlink 的微信临时文件；原始错误交给缓存层处理。
function downloadToPath(api, url, filePath, job, maxBytes, timeout) {
  return new Promise((resolve, reject) => {
    let headers = {}
    let oversize = false
    let task
    const stop = () => { if (task && typeof task.abort === 'function') task.abort() }
    task = api.downloadFile({
      url, filePath, timeout,
      success(response) {
        if (oversize) { reject(new Error('视频片段过大，已停止缓存')); return }
        if (!response || response.statusCode !== 200 || response.filePath !== filePath) {
          reject(new Error('视频片段下载失败')); return
        }
        resolve(headers)
      },
      fail(error) { reject(oversize ? new Error('视频片段过大，已停止缓存') : error) }
    })
    job.setAbort(stop)
    if (task && typeof task.onHeadersReceived === 'function') {
      task.onHeadersReceived(event => { headers = event && event.header || {} })
    }
    if (task && typeof task.onProgressUpdate === 'function') {
      task.onProgressUpdate(event => {
        if (Number(event.totalBytesWritten) > maxBytes || Number(event.totalBytesExpectedToWrite) > maxBytes) {
          oversize = true
          stop()
        }
      })
    }
  })
}

module.exports = downloadToPath