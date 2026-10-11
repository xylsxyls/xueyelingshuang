const { main } = require('./lib/runner')

main(process.argv.slice(2)).then(code => { process.exitCode = code }).catch(error => {
  process.stderr.write(error.stack + '\n')
  process.exitCode = 1
})