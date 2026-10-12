function renumber(items) { return items.map((item, index) => Object.assign({}, item, { index: index + 1 })) }

function move(items, id, position) {
  const from = items.findIndex(item => item.id === id)
  if (from < 0 || !Number.isInteger(position) || position < 0 || position >= items.length) return null
  const result = items.slice(), item = result.splice(from, 1)[0]
  result.splice(position, 0, item)
  return renumber(result)
}

function insert(items, task, anchor, position) {
  if (!task || !task.id || items.some(item => item.id === task.id)) return null
  const index = items.findIndex(item => item.id === anchor)
  if (items.length && index < 0) return null
  const result = items.slice()
  result.splice(index < 0 ? 0 : index + (position === 'before' ? 0 : 1), 0, task)
  return renumber(result)
}

function remove(items, id) {
  if (items.length <= 1 || !items.some(item => item.id === id)) return null
  return renumber(items.filter(item => item.id !== id))
}

function insertionIndex(rects, id, pageY) {
  if (!Number.isFinite(pageY)) return null
  return rects.filter(rect => rect.id !== id && pageY > rect.center).length
}

module.exports = { renumber, move, insert, remove, insertionIndex }