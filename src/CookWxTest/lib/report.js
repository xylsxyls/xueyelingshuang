const fs = require('node:fs')
const path = require('node:path')
const crypto = require('node:crypto')

function escapeHtml(value) {
  return String(value == null ? '' : value).replace(/[&<>"']/g, char => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;' }[char]))
}

function redact(value) {
  return String(value).replace(/Bearer\s+[A-Za-z0-9_.-]+/gi, 'Bearer [REDACTED]').replace(/("?(?:token|password|adToken)"?\s*[:=]\s*)"?[^",\s}]+"?/gi, '$1[REDACTED]')
}

function hashFile(file) { return crypto.createHash('sha256').update(fs.readFileSync(file)).digest('hex') }

class Reporter {
  constructor(config, catalog, requirements, selected) {
    this.runId = new Date().toISOString().replace(/[:.]/g, '-') + '-' + process.pid
    this.directory = path.join(config.reportRoot, this.runId)
    fs.mkdirSync(this.directory, { recursive: true })
    this.report = {
      version: 1, runId: this.runId, purpose: '开发期真实模拟器回归', startedAt: new Date().toISOString(),
      state: 'running', environment: { platform: process.platform, arch: process.arch, node: process.version },
      requirements, selectedIds: selected.map(item => item.id),
      cases: catalog.map(item => ({ ...item, state: 'not-run', reason: '本批未选择' }))
    }
    this.save()
  }

  event(message) {
    fs.appendFileSync(path.join(this.directory, 'events.log'), `${new Date().toISOString()} ${redact(message)}\n`)
    process.stdout.write(redact(message) + '\n')
  }

  result(id, fields) {
    Object.assign(this.report.cases.find(item => item.id === id), fields)
    this.save()
  }

  save() {
    const executed = this.report.cases.filter(item => ['passed', 'failed', 'canceled'].includes(item.state))
    this.report.summary = {
      executed: executed.length, instant: executed.filter(item => item.type === 'instant').length,
      stress: executed.filter(item => item.type === 'stress').length,
      passed: executed.filter(item => item.state === 'passed').length, failed: executed.filter(item => item.state === 'failed').length,
      canceled: executed.filter(item => item.state === 'canceled').length,
      notRun: this.report.cases.filter(item => item.state === 'not-run').length
    }
    this.report.coverage = this.report.requirements.map(requirement => {
      const cases = this.report.cases.filter(item => item.requirements.includes(requirement.id))
      const state = requirement.boundary || (cases.some(item => item.state === 'failed') ? 'failed' : cases.length && cases.every(item => item.state === 'passed' && !(item.evidence && item.evidence.incomplete)) ? 'passed' : 'incomplete')
      return { ...requirement, state, caseIds: cases.map(item => item.id) }
    })
    const jsonPath = path.join(this.directory, 'report.json')
    fs.writeFileSync(jsonPath + '.tmp', JSON.stringify(this.report, null, 2))
    fs.renameSync(jsonPath + '.tmp', jsonPath)
    const rows = this.report.cases.map(item => `<tr><td>${item.id}</td><td>${escapeHtml(item.name)}</td><td>${item.type}</td><td>${item.state}</td><td>${escapeHtml(item.reason || item.evidence && item.evidence.incomplete || '')}</td><td>${item.elapsedMs || 0}</td><td>${item.screenshot ? `<a href="${escapeHtml(item.screenshot)}">截图</a>` : ''}${item.evidence ? `<details><summary>实际结果</summary><pre>${escapeHtml(JSON.stringify(item.evidence, null, 2))}</pre></details>` : ''}</td></tr>`).join('')
    const coverage = this.report.coverage.map(item => `<tr><td>${escapeHtml(item.id)}</td><td>${escapeHtml(item.name)}</td><td>${item.state}</td><td>${item.caseIds.join(',')}</td></tr>`).join('')
    fs.writeFileSync(path.join(this.directory, 'report.html'), `<!doctype html><html lang="zh-CN"><meta charset="utf-8"><title>CookWxTest ${this.runId}</title><style>body{font:15px sans-serif;margin:24px}table{border-collapse:collapse;width:100%;margin-bottom:30px}td,th{border:1px solid #ccc;padding:8px;text-align:left}pre{white-space:pre-wrap}</style><h1>CookWxTest</h1><p>${escapeHtml(this.runId)} · ${escapeHtml(this.report.state)}</p><pre>${escapeHtml(JSON.stringify(this.report.summary, null, 2))}</pre><table><tr><th>ID</th><th>用例</th><th>类型</th><th>状态</th><th>原因</th><th>毫秒</th><th>证据</th></tr>${rows}</table><h2>需求覆盖</h2><table>${coverage}</table></html>`)
  }

  finish(state) {
    this.report.state = state
    this.report.finishedAt = new Date().toISOString()
    this.report.elapsedMs = Date.parse(this.report.finishedAt) - Date.parse(this.report.startedAt)
    this.save()
  }
}

module.exports = { Reporter, hashFile, redact, escapeHtml }