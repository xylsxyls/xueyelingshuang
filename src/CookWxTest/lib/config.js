const fs = require('node:fs')
const path = require('node:path')

function loadConfig() {
  const projectRoot = path.resolve(__dirname, '..')
  const repositoryRoot = path.resolve(projectRoot, '../..')
  const local = path.join(projectRoot, 'config.local.json')
  const config = JSON.parse(fs.readFileSync(path.join(projectRoot, 'config.example.json'), 'utf8'))
  if (fs.existsSync(local)) Object.assign(config, JSON.parse(fs.readFileSync(local, 'utf8')))
  const toolsHome = path.join(process.env['ProgramFiles(x86)'] || 'C:/Program Files (x86)', 'Tencent', '微信web开发者工具')
  config.cliPath = config.cliPath || process.env.COOK_WX_CLI || path.join(toolsHome, 'cli.bat')
  config.nodePath = config.nodePath || process.execPath
  config.projectRoot = projectRoot
  config.repositoryRoot = repositoryRoot
  config.sourceRoot = path.join(repositoryRoot, 'src/CookWx')
  config.serverRoot = path.join(repositoryRoot, 'common/CookServer')
  config.serverTestRoot = path.join(repositoryRoot, 'common/CookServerTest')
  config.reportRoot = path.join(projectRoot, 'reports')
  config.voiceProvider = config.voiceProvider || 'disabled'
  if (!['disabled', 'windows_sapi'].includes(config.voiceProvider)) throw new Error('隔离语音仅支持 disabled/windows_sapi')
  for (const key of ['automationPort', 'serverPort', 'mysqlPort', 'redisPort']) {
    if (!Number.isInteger(config[key]) || config[key] < 1024 || config[key] > 65535) throw new Error(`无效端口 ${key}`)
  }
  if (new Set(['automationPort', 'serverPort', 'mysqlPort', 'redisPort'].map(key => config[key])).size !== 4) throw new Error('测试端口不能重复')
  for (const key of ['caseTimeoutMs', 'startupTimeoutMs', 'stressLoops', 'serverStressThreads', 'serverStressLoops']) {
    if (!Number.isInteger(config[key]) || config[key] < 1 || config[key] > 300000) throw new Error(`无效配置 ${key}`)
  }
  return Object.freeze(config)
}

module.exports = { loadConfig }