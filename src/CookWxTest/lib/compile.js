const fs = require('node:fs')
const path = require('node:path')
const { execFileSync } = require('node:child_process')
const { loadConfig } = require('./config')
const { instrumentWxml } = require('./selectors')

// 使用本机开发者工具随包的官方编译器。语法编译不代替真实页面运行。
function compileProject(config = loadConfig()) {
  const directory = path.join(config.projectRoot, 'temp/compile')
  fs.mkdirSync(directory, { recursive: true })
  const tools = path.join(path.dirname(config.cliPath), 'code/package.nw/node_modules/wcc-exec')
  for (const executable of ['wcc.exe', 'wcsc.exe']) if (!fs.existsSync(path.join(tools, executable))) throw new Error('没有找到官方小程序编译器：' + executable)
  const files = []
  const visit = directory => {
    for (const entry of fs.readdirSync(directory, { withFileTypes: true })) {
      if (entry.isSymbolicLink()) continue
      const file = path.join(directory, entry.name)
      if (entry.isDirectory()) visit(file)
      else if (/\.(wxml|wxss)$/.test(entry.name)) files.push(path.relative(config.sourceRoot, file).replace(/\\/g, '/'))
    }
  }
  visit(config.sourceRoot)
  const result = { wxml: 0, instrumentedWxml: 0, wxss: 0 }
  const styles = files.filter(file => file.endsWith('.wxss'))
  execFileSync(path.join(tools, 'wcsc.exe'), ['-pc', String(styles.length), '-o', path.join(directory, 'style.js'), ...styles], { cwd: config.sourceRoot, windowsHide: true, timeout: 15000, maxBuffer: 16 * 1024 * 1024, stdio: ['pipe', 'pipe', 'pipe'] })
  result.wxss = styles.length
  for (const file of files.filter(file => file.endsWith('.wxml'))) {
    const compiler = path.join(tools, 'wcc.exe')
    const source = fs.readFileSync(path.join(config.sourceRoot, file), 'utf8')
    const args = ['-s', file, '-o', path.join(directory, 'wxml.js')]
    execFileSync(compiler, args, { cwd: config.sourceRoot, windowsHide: true, timeout: 15000, maxBuffer: 16 * 1024 * 1024, input: source, stdio: ['pipe', 'pipe', 'pipe'] })
    result.wxml++
    {
      execFileSync(compiler, args, { cwd: config.sourceRoot, windowsHide: true, timeout: 15000, maxBuffer: 16 * 1024 * 1024, input: instrumentWxml(source), stdio: ['pipe', 'pipe', 'pipe'] })
      result.instrumentedWxml++
    }
  }
  return result
}

module.exports = { compileProject }