const http = require('node:http')

class TestHttpClient {
  constructor(port, signal) { this.port = port; this.signal = signal }

  async request(route, options = {}) {
    if (!route.startsWith('/api/')) throw new Error('只允许测试 API 相对路径')
    const body = options.body == null ? null : Buffer.isBuffer(options.body) ? options.body : Buffer.from(JSON.stringify(options.body))
    const headers = { ...(options.token ? { Authorization: `Bearer ${options.token}` } : {}), ...options.headers }
    if (body) { headers['Content-Type'] = headers['Content-Type'] || 'application/json'; headers['Content-Length'] = body.length }
    return new Promise((resolve, reject) => {
      const request = http.request({ host: '127.0.0.1', port: this.port, path: route, method: options.method || (body ? 'POST' : 'GET'), headers, signal: this.signal }, response => {
        let bytes = 0
        const chunks = []
        response.on('data', chunk => {
          bytes += chunk.length
          if (bytes > 64 * 1024 * 1024) { response.destroy(new Error('响应超过测试上限')); return }
          chunks.push(chunk)
        })
        response.on('error', reject)
        response.on('end', () => {
          const data = Buffer.concat(chunks)
          let json
          try { json = JSON.parse(data.toString('utf8')) } catch (_) {}
          resolve({ status: response.statusCode, headers: response.headers, data, json })
        })
      })
      request.on('error', reject)
      request.setTimeout(options.timeoutMs || 30000, () => request.destroy(new Error('HTTP 请求超时')))
      request.end(body)
    })
  }

  async json(route, options) {
    const response = await this.request(route, options)
    if (response.status !== 200 || !response.json || response.json.ok !== true) throw new Error(`${route} HTTP ${response.status}: ${response.json && response.json.message || '无有效成功 JSON'}`)
    return response.json
  }
}

module.exports = { TestHttpClient }