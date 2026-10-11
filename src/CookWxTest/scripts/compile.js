try {
  const result = require('../lib/compile').compileProject()
  process.stdout.write('官方编译器语法检查：' + JSON.stringify(result) + '\n')
} catch (error) {
  process.stderr.write((error.stderr ? error.stderr.toString() : error.stack) + '\n')
  process.exitCode = 1
}