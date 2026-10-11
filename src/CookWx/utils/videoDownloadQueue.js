function canceled() {
  const error = new Error('视频片段请求已取消')
  error.code = 'VIDEO_REQUEST_CANCELED'
  return error
}

// 同片段共享任务；队列仅保留仍有使用者的请求，取消完成后才释放并发名额。
class VideoDownloadQueue {
  constructor(concurrency = 1, delayMs = 0) {
    this.concurrency = Math.max(1, Math.min(2, Math.floor(concurrency) || 1))
    this.jobs = Object.create(null)
    this.waiting = []
    this.running = new Set()
    this.delayMs = Math.max(0, Math.min(1000, Number(delayMs) || 0))
    this.timer = null
  }

  request(key, run, wanted = () => true, priority = 0) {
    if (!wanted()) return Promise.reject(canceled())
    let job = this.jobs[key]
    if (!job || job.canceled) {
      job = { key, run, consumers: [], priority, readyAt: Date.now() + this.delayMs, canceled: false, abort: null }
      job.isWanted = () => !job.canceled && job.consumers.some(consumer => consumer.wanted())
      job.setAbort = abort => { job.abort = abort; if (!job.isWanted()) abort() }
      this.jobs[key] = job
      this.waiting.push(job)
    }
    job.priority = Math.max(job.priority, priority)
    const result = new Promise((resolve, reject) => job.consumers.push({ wanted, resolve, reject }))
    // 先登记新使用者再取消旧使用者，避免同片段定位相互取消。
    this.prune()
    return result
  }

  prune() {
    const jobs = new Set([...this.waiting, ...this.running])
    jobs.forEach(job => {
      job.consumers = job.consumers.filter(consumer => {
        if (consumer.wanted()) return true
        consumer.reject(canceled())
        return false
      })
      if (job.consumers.length || job.canceled) return
      job.canceled = true
      if (this.jobs[job.key] === job) delete this.jobs[job.key]
      if (job.abort) job.abort()
    })
    this.waiting = this.waiting.filter(job => !job.canceled)
    Promise.resolve().then(() => this.pump())
  }

  pump() {
    clearTimeout(this.timer)
    this.timer = null
    while (this.running.size < this.concurrency && this.waiting.length) {
      this.waiting.sort((left, right) => right.priority - left.priority)
      const delay = this.waiting[0].readyAt - Date.now()
      if (delay > 0) { this.timer = setTimeout(() => this.pump(), delay); return }
      const job = this.waiting.shift()
      if (!job.isWanted()) {
        job.canceled = true
        if (this.jobs[job.key] === job) delete this.jobs[job.key]
        job.consumers.forEach(consumer => consumer.reject(canceled()))
        job.consumers = []
        continue
      }
      this.running.add(job)
      Promise.resolve().then(() => job.run(job)).then(
        entry => this.finish(job, null, entry), error => this.finish(job, error)
      )
    }
  }

  finish(job, error, entry) {
    if (this.jobs[job.key] === job) delete this.jobs[job.key]
    job.consumers.forEach(consumer => {
      if (!consumer.wanted() || job.canceled) consumer.reject(canceled())
      else if (error) consumer.reject(error)
      else consumer.resolve(entry)
    })
    job.consumers = []
    this.running.delete(job)
    // 让读者先固定交接中的文件，再启动下一项清理/下载。
    Promise.resolve().then(() => this.pump())
  }
}

module.exports = { VideoDownloadQueue, canceled }