// 所有裁剪坐标都在CSS像素空间；导出时才换算为图片像素。
function orientedSize(width, height, orientation) {
  const swapped = ['left', 'right', 'left-mirrored', 'right-mirrored'].includes(orientation)
  return { width: swapped ? height : width, height: swapped ? width : height }
}

function orientationMatrix(width, height, orientation) {
  const matrices = {
    up: [1, 0, 0, 1, 0, 0], 'up-mirrored': [-1, 0, 0, 1, width, 0],
    down: [-1, 0, 0, -1, width, height], 'down-mirrored': [1, 0, 0, -1, 0, height],
    left: [0, -1, 1, 0, 0, width], right: [0, 1, -1, 0, height, 0],
    'left-mirrored': [0, 1, 1, 0, 0, 0], 'right-mirrored': [0, -1, -1, 0, height, width]
  }
  return matrices[orientation] || matrices.up
}

function fit(width, height, viewportWidth, viewportHeight) {
  if (![width, height, viewportWidth, viewportHeight].every(value => Number.isFinite(value) && value > 0)) return null
  const scale = Math.min(viewportWidth / width, viewportHeight / height)
  return { x: (viewportWidth - width * scale) / 2, y: (viewportHeight - height * scale) / 2, width: width * scale, height: height * scale }
}

function constrain(crop, image, minimum) {
  const maximum = Math.min(image.width, image.height)
  const size = Math.max(Math.min(minimum, maximum), Math.min(maximum, Number(crop.size) || maximum))
  return {
    size,
    x: Math.max(image.x, Math.min(image.x + image.width - size, Number(crop.x) || 0)),
    y: Math.max(image.y, Math.min(image.y + image.height - size, Number(crop.y) || 0))
  }
}

function initial(image, minimum) {
  const size = Math.min(image.width, image.height) * 0.8
  return constrain({ x: image.x + (image.width - size) / 2, y: image.y + (image.height - size) / 2, size }, image, minimum)
}

function resize(crop, size, image, minimum, center) {
  const bounded = constrain(Object.assign({}, crop, { size }), image, minimum).size
  return constrain({ size: bounded, x: crop.x + (center ? (crop.size - bounded) / 2 : 0), y: crop.y + (center ? (crop.size - bounded) / 2 : 0) }, image, minimum)
}

function gesture(touches, mode, crop) {
  const points = (touches || []).map(point => ({ x: Number(point.clientX), y: Number(point.clientY) }))
  if (!points.length || points.some(point => !Number.isFinite(point.x) || !Number.isFinite(point.y))) return null
  const pinch = points.length > 1
  return { mode: pinch ? 'pinch' : mode, crop: Object.assign({}, crop),
    x: pinch ? (points[0].x + points[1].x) / 2 : points[0].x,
    y: pinch ? (points[0].y + points[1].y) / 2 : points[0].y,
    distance: pinch ? Math.hypot(points[1].x - points[0].x, points[1].y - points[0].y) : 0 }
}

function update(start, touches, image, minimum) {
  const next = gesture(touches, start.mode, start.crop)
  if (!next || next.mode !== start.mode) return start.crop
  const dx = next.x - start.x, dy = next.y - start.y
  if (start.mode === 'resize') return resize(start.crop, start.crop.size + (dx + dy) / 2, image, minimum, false)
  if (start.mode === 'pinch') {
    const size = start.distance > 0 ? start.crop.size * next.distance / start.distance : start.crop.size
    const changed = resize(start.crop, size, image, minimum, true)
    return constrain(Object.assign(changed, { x: changed.x + dx, y: changed.y + dy }), image, minimum)
  }
  return constrain(Object.assign({}, start.crop, { x: start.crop.x + dx, y: start.crop.y + dy }), image, minimum)
}

function draw(context, image, info, placement, selection, ratio) {
  context.save()
  context.setTransform(ratio, 0, 0, ratio, selection ? -selection.x * ratio : 0, selection ? -selection.y * ratio : 0)
  context.translate(placement.x, placement.y)
  const size = orientedSize(info.width, info.height, info.orientation)
  context.scale(placement.width / size.width, placement.height / size.height)
  context.transform(...orientationMatrix(info.width, info.height, info.orientation))
  context.drawImage(image, 0, 0, info.width, info.height)
  context.restore()
}

module.exports = { fit, constrain, initial, resize, gesture, update, draw, orientedSize, orientationMatrix }