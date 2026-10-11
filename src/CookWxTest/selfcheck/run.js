const fs = require('node:fs')
const path = require('node:path')
const { spawn } = require('node:child_process')
const { escapeHtml } = require('../lib/report')

async function run() {
  const files = fs.readdirSync(__dirname).filter(name => name.endsWith('.test.js')).sort().map(name => path.join(__dirname, name))
  if (!files.length) throw new Error('设施自检用例为空')
  const root = path.resolve(__dirname, '../reports')
  const directory = path.join(root, 'selfcheck-' + new Date().toISOString().replace(/[:.]/g, '-') + '-' + process.pid)
  fs.mkdirSync(directory, { recursive: true })
  const outputFile = path.join(directory, 'selfcheck.tap')
  const log = fs.openSync(outputFile, 'wx')
  const child = spawn(process.execPath, ['--test', ...files], { windowsHide: true, stdio: ['ignore', 'pipe', 'pipe'] })
  let canceled = false
  let startupError
  const cancel = () => { canceled = true; child.kill() }
  process.once('SIGINT', cancel)
  process.once('SIGTERM', cancel)
  const receive = data => { fs.writeSync(log, data); process.stdout.write(data) }
  child.stdout.on('data', receive)
  child.stderr.on('data', receive)
  child.on('error', error => { startupError = error.message })
  const code = await new Promise(resolve => child.once('close', resolve))
  fs.closeSync(log)
  process.removeListener('SIGINT', cancel)
  process.removeListener('SIGTERM', cancel)
  const text = fs.readFileSync(outputFile, 'utf8')
  const count = pattern => Number((text.match(pattern) || [])[1] || 0)
  const passed = count(/^# pass (\d+)$/m)
  const total = count(/^# tests (\d+)$/m)
  const report = { state: canceled ? 'canceled' : code === 0 && total > 0 && passed === total ? 'passed' : 'failed', exitCode: code, total, passed, failed: count(/^# fail (\d+)$/m), startupError, node: process.version, files, boundary: 'Node平台契约与测试设施；不代替微信模拟器页面、真实解码或真机', finishedAt: new Date().toISOString() }
  fs.writeFileSync(path.join(directory, 'report.json'), JSON.stringify(report, null, 2))
  fs.writeFileSync(path.join(directory, 'report.html'), `<!doctype html><meta charset="utf-8"><title>CookWxTest设施自检</title><h1>CookWxTest设施自检</h1><pre>${escapeHtml(JSON.stringify(report, null, 2))}</pre><a href="selfcheck.tap">原始TAP报告</a>`)
  process.stdout.write('自检报告：' + path.join(directory, 'report.html') + '\n')
  return report.state === 'passed' ? 0 : 1
}

run().then(code => { process.exitCode = code }).catch(error => { process.stderr.write(error.stack + '\n'); process.exitCode = 1 })