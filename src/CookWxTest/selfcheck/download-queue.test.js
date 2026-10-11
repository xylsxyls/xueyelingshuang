const test = require('node:test')
const assert = require('node:assert/strict')
const { VideoDownloadQueue } = require('../../CookWx/utils/videoDownloadQueue')
const tick = () => new Promise(resolve => setImmediate(resolve))

test('同一片段共享下载，新定位替换旧读者而不取消共享任务', async () => {
  const queue = new VideoDownloadQueue()
  let generation = 1
  let finish
  let runs = 0
  let aborts = 0
  const run = job => new Promise(resolve => { runs += 1; finish = resolve; job.setAbort(() => { aborts += 1 }) })
  const old = queue.request('same', run, () => generation === 1)
  const oldResult = assert.rejects(old, /已取消/)
  await tick()
  generation = 2
  const current = queue.request('same', run, () => generation === 2, 100)
  finish('cached')
  assert.equal(await current, 'cached')
  await oldResult
  assert.equal(runs, 1)
  assert.equal(aborts, 0)
  assert.equal(Object.keys(queue.jobs).length, 0)
})

test('取消中的网络请求清理完成前不启动新下载，排队的过期片段不下载', async () => {
  const queue = new VideoDownloadQueue()
  let wanted = true
  let finish
  let started = 0
  let aborts = 0
  const first = queue.request('old', job => new Promise(resolve => {
    started += 1; finish = resolve; job.setAbort(() => { aborts += 1 })
  }), () => wanted)
  const oldResult = assert.rejects(first, /已取消/)
  await tick()
  const stale = queue.request('queued', () => { throw Error('must not run') }, () => wanted)
  const staleResult = assert.rejects(stale, /已取消/)
  wanted = false
  const latest = queue.request('latest', async () => { started += 1; return 'latest' }, () => true, 100)
  await tick()
  assert.equal(started, 1)
  assert.equal(aborts, 1)
  finish('aborted download cleaned')
  assert.equal(await latest, 'latest')
  await Promise.all([oldResult, staleResult])
  assert.equal(started, 2)
  assert.equal(queue.running.size, 0)
})

test('微任务开始前读者失效也清空队列；不留下无界 pending', async () => {
  const queue = new VideoDownloadQueue()
  let wanted = true
  const result = queue.request('unused', () => { throw Error('must not run') }, () => wanted)
  wanted = false
  await assert.rejects(result, /已取消/)
  assert.equal(Object.keys(queue.jobs).length, 0)
  assert.equal(queue.running.size, 0)
})

test('三百次快速定位保持下载并发有界，只保留最后请求', async () => {
  const queue = new VideoDownloadQueue()
  let generation = 0
  let running = 0
  let maximum = 0
  let completed = 0
  const results = []
  for (let index = 1; index <= 300; index += 1) {
    generation = index
    const request = queue.request(String(index), job => new Promise(resolve => {
      running += 1; maximum = Math.max(maximum, running)
      let stopped = false
      job.setAbort(() => { stopped = true })
      setImmediate(() => { running -= 1; if (!stopped) completed += 1; resolve(index) })
    }), () => generation === index, 100)
    results.push(request.then(value => ({ value }), error => ({ code: error.code })))
    if (index % 20 === 0) await Promise.resolve()
  }
  const outcomes = await Promise.all(results)
  assert.equal(outcomes.filter(result => result.value).length, 1)
  assert.equal(outcomes[299].value, 300)
  assert.equal(outcomes.filter(result => result.code === 'VIDEO_REQUEST_CANCELED').length, 299)
  assert.equal(maximum, 1)
  assert.equal(completed, 1)
  assert.equal(queue.waiting.length, 0)
  assert.equal(Object.keys(queue.jobs).length, 0)
})

test('短延迟合并连续定位，最终请求前不发HTTP，不为每个拖拽点下载', async () => {
  const queue = new VideoDownloadQueue(1, 30)
  let generation = 0
  const requests = []
  const results = []
  for (let index = 1; index <= 12; index += 1) {
    generation = index
    results.push(queue.request(String(index), async () => { requests.push(index); return index }, () => generation === index)
      .then(value => value, error => error.code))
    await new Promise(resolve => setTimeout(resolve, 1))
  }
  const outcomes = await Promise.all(results)
  assert.deepEqual(requests, [12])
  assert.equal(outcomes[11], 12)
  assert.equal(queue.timer, null)
})