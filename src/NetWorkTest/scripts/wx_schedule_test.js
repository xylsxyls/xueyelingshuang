'use strict'

const fs = require('node:fs')
const path = require('node:path')
const vm = require('node:vm')
const assert = require('node:assert/strict')
const { createRequire } = require('node:module')

// A ready connection must wake at once for business data/ACKs, even if it has
// already armed a long heartbeat timer. Fake time checks this contract directly.
for (const [file, className, method, timerKey] of [
  ['NetWxClient.js', 'NetWxClient', 'schedulePump', 'pumpTimer'],
  ['NetWxGatewaySocket.js', 'NetWxGatewaySocket', 'schedule', 'timer']
]) {
  let now = 0, serial = 0, calls = 0
  const pending = new Map()
  const microtasks = []
  const filename = path.resolve(__dirname, 'host', file)
  const module = { exports: {} }
  vm.runInNewContext(fs.readFileSync(filename, 'utf8'), { module, exports: module.exports,
    require: createRequire(filename), console,
    Promise: { resolve: () => ({ then(callback) { microtasks.push(callback) } }) },
    setTimeout(callback, delay) { const id = ++serial; pending.set(id, { at: now + delay, callback }); return id },
    clearTimeout(id) { pending.delete(id) }
  }, { filename })
  const value = Object.create(module.exports[className].prototype)
  Object.assign(value, { options: { clock: () => now }, disposed: false, closed: false,
    [timerKey]: null, pumpTimerAt: 0, timerAt: 0, pumpQueued: false, pump() { calls++ } })
  if (method === 'schedulePump') {
    // This is the exact legacy idle state established by pump().
    value[timerKey] = ++serial
    value.pumpTimerAt = 1000
    pending.set(serial, { at: 1000, callback() { value[timerKey] = null; value.pump() } })
  } else value[method](25)
  value[method](0)
  assert.equal(pending.size, 0, file + ': replaced timer must be cancelled')
  assert.equal(microtasks.length, 1, file + ': immediate work must not depend on a clamped timer')
  value[method](0)
  assert.equal(microtasks.length, 1, file + ': repeated wakeups must coalesce')
  microtasks.shift()()
  assert.equal(calls, 1)
  assert.equal(value[timerKey], null)
  value[method](25)
  value[method](10)
  value[method](50)
  assert.equal(pending.size, 1)
  assert.equal(Array.from(pending.values())[0].at, 10, 'Later retries must not postpone an earlier deadline')
  value[method](0)
  value.disposed = value.closed = true
  microtasks.shift()()
  assert.equal(calls, 1, 'Disposed connection must ignore an already queued microtask')
  console.log('PASS ' + file + ' timer-independent wakeup, coalescing, earliest retry and disposal')
}