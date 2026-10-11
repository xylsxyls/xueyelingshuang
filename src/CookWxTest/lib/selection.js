function parseIds(text, catalog) {
  if (!text || !/^[1-9]\d*(?:-[1-9]\d*)?(?:,[1-9]\d*(?:-[1-9]\d*)?)*$/.test(text)) throw new Error('用例格式为 1,3,8-12（正整数闭区间）')
  const ids = new Set()
  for (const part of text.split(',')) {
    const [start, end = start] = part.split('-').map(Number)
    if (!Number.isSafeInteger(start) || !Number.isSafeInteger(end) || start > end || end - start > 10000) throw new Error('无效用例区间')
    for (let id = start; id <= end; id += 1) {
      if (!catalog.some(item => item.id === id)) throw new Error(`未登记用例 ${id}`)
      ids.add(id)
    }
  }
  return [...ids].sort((a, b) => a - b)
}

function selectCases(args, catalog) {
  const modes = args.filter(arg => arg === '--instant' || arg === '--stress' || arg === '--all' || arg.startsWith('--ids='))
  if (args.some(arg => !modes.includes(arg) && !['--list', '--prepare', '--backend-only'].includes(arg))) throw new Error('存在未知参数')
  if (modes.length > 1) throw new Error('一次只能选择一个测试方式')
  const mode = modes[0] || '--instant'
  const ids = mode.startsWith('--ids=') ? parseIds(mode.slice(6), catalog) : null
  const selected = catalog.filter(item => ids ? ids.includes(item.id) : mode === '--all' || item.type === mode.slice(2))
  if (!selected.length) throw new Error('有效用例为空')
  return mode === '--all' ? selected.sort((a, b) => (a.type === b.type ? a.id - b.id : a.type === 'instant' ? -1 : 1)) : selected
}

module.exports = { parseIds, selectCases }