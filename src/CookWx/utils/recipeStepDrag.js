const config = require('./config')
const order = require('./recipeStepOrder')

// 只测量一次静止卡片，移动时更新浮层/插入线，松手才提交整份顺序。
class RecipeStepDrag {
  constructor(page, platform) { this.page = page; this.platform = platform; this.serial = 0; this.scrollTop = 0 }
  allowed() { return this.page.canEdit() }
  point(event) {
    const touches = event.touches && event.touches.length ? event.touches : event.changedTouches || []
    if (touches.length > 1) return null
    const touch = touches.find(item => this.touchId == null || item.identifier === this.touchId)
    if (!touch) return null
    const y = Number(touch.clientY == null ? Number(touch.pageY) - this.scrollTop : touch.clientY)
    return Number.isFinite(y) ? { y, id: touch.identifier } : null
  }
  arm(event) {
    if (!this.allowed()) return
    if (this.active) { if (event.touches && event.touches.length > 1) this.cancel(); return }
    this.touchId = null
    this.pointer = this.point(event)
    if (this.pointer) this.touchId = this.pointer.id
  }
  valid(serial = this.serial) {
    return this.active && serial === this.serial && this.allowed() && this.ids.length === this.page.data.steps.length &&
      this.ids.every((id, index) => id === this.page.data.steps[index].id)
  }
  start(event) {
    if (!this.allowed() || this.active) return
    if (event.touches && event.touches.length > 1) { this.pointer = null; this.touchId = null; return }
    const id = event.currentTarget.dataset.id, index = this.page.data.steps.findIndex(item => item.id === id)
    const pointer = this.point(event) || this.pointer
    if (index < 0 || !pointer) return
    this.pointer = pointer; this.touchId = pointer.id
    this.id = id; this.from = index; this.ids = this.page.data.steps.map(item => item.id)
    this.active = true; this.ready = false
    const serial = ++this.serial
    this.measureTimer = setTimeout(() => this.fail(serial), config.RECIPE_STEP_DRAG_MEASURE_TIMEOUT_MS)
    this.page.setData({ dragging: true, dragId: id, dragLabel: '第' + (index + 1) + '步 · ' + this.page.data.steps[index].title,
      dragY: Math.max(0, pointer.y - 24), dropBeforeId: '', dropAfterId: '' }, () => {
      if (!this.valid(serial)) { this.cancelIfCurrent(serial); return }
      let started = false
      const measure = () => { if (started) return; if (!this.valid(serial)) { this.cancelIfCurrent(serial); return }; started = true; this.measure(serial) }
      try { if (this.platform.hideKeyboard) this.platform.hideKeyboard({ complete: measure }); else measure() }
      catch (error) { measure() }
    })
  }
  measure(serial) {
    try {
      let query = this.platform.createSelectorQuery()
      if (typeof query.in === 'function') query = query.in(this.page)
      query.selectAll('.creation-step-card').boundingClientRect()
      query.select('.creation-editor').boundingClientRect()
      query.selectViewport().scrollOffset()
      query.exec(results => {
        if (!this.valid(serial)) { this.cancelIfCurrent(serial); return }
        const rects = results && results[0], container = results && results[1], viewport = results && results[2]
        let info
        try { info = this.platform.getWindowInfo ? this.platform.getWindowInfo() : this.platform.getSystemInfoSync() }
        catch (error) { this.fail(serial); return }
        if (!Array.isArray(rects) || rects.length !== this.ids.length || !container || !Number.isFinite(container.height) || !viewport || !info || !Number.isFinite(info.windowHeight) || info.windowHeight <= 0 ||
            rects.some(rect => !rect || !Number.isFinite(rect.top) || !Number.isFinite(rect.bottom) || rect.bottom <= rect.top)) { this.fail(serial); return }
        this.scrollTop = Number(viewport.scrollTop) || 0
        this.height = info.windowHeight
        this.maxScroll = Math.max(0, container.height - this.height)
        this.rects = rects.map((rect, index) => ({ id: this.ids[index], center: (rect.top + rect.bottom) / 2 + this.scrollTop }))
        clearTimeout(this.measureTimer); this.measureTimer = null
        this.ready = true; this.render(); this.autoScroll()
      })
    } catch (error) { this.fail(serial) }
  }
  fail(serial) {
    if (!this.valid(serial)) { this.cancelIfCurrent(serial); return }
    this.cancel()
    this.platform.showToast({ title: '暂时无法拖动，请稍后重试', icon: 'none' })
  }
  update(event) {
    if (!this.active) { this.pointer = this.point(event); return }
    const point = this.point(event)
    if (!point || !this.valid()) { this.cancel(); return }
    this.pointer = point
    if (this.ready) { this.queueRender(); this.autoScroll() }
  }
  queueRender() {
    if (this.renderTimer) return
    const serial = this.serial
    this.renderTimer = setTimeout(() => { this.renderTimer = null; if (this.valid(serial)) this.render(); else this.cancelIfCurrent(serial) }, config.RECIPE_STEP_DRAG_FRAME_MS)
  }
  render() {
    if (!this.ready || !this.valid()) return
    const index = order.insertionIndex(this.rects, this.id, this.pointer.y + this.scrollTop)
    const others = this.rects.filter(rect => rect.id !== this.id)
    const before = index !== this.from && index < others.length ? others[index].id : ''
    const after = index !== this.from && !before && others.length ? others[others.length - 1].id : ''
    const values = { dragY: Math.round(Math.max(0, Math.min(this.height - 48, this.pointer.y - 24))) }
    if (this.page.data.dropBeforeId !== before) values.dropBeforeId = before
    if (this.page.data.dropAfterId !== after) values.dropAfterId = after
    this.page.setData(values)
  }
  onScroll(scrollTop) {
    this.scrollTop = Math.max(0, Number(scrollTop) || 0)
    if (this.active && this.ready) this.queueRender()
  }
  autoScroll() {
    if (!this.valid() || !this.ready || this.scrollTimer || !this.platform.pageScrollTo) return
    const edge = config.RECIPE_STEP_DRAG_EDGE_PX
    const direction = this.pointer.y < edge ? -1 : (this.pointer.y > this.height - edge ? 1 : 0)
    const target = Math.max(0, Math.min(this.maxScroll, this.scrollTop + direction * config.RECIPE_STEP_DRAG_SCROLL_STEP_PX))
    if (!direction || target === this.scrollTop) return
    const serial = this.serial
    this.scrollTimer = setTimeout(() => {
      this.scrollTimer = null
      if (!this.valid(serial)) { this.cancelIfCurrent(serial); return }
      const now = this.pointer.y < edge ? -1 : (this.pointer.y > this.height - edge ? 1 : 0)
      const next = Math.max(0, Math.min(this.maxScroll, this.scrollTop + now * config.RECIPE_STEP_DRAG_SCROLL_STEP_PX))
      if (!now || next === this.scrollTop) return
      try { this.platform.pageScrollTo({ scrollTop: next, duration: 0, complete: () => { if (this.valid(serial)) this.autoScroll() } }) }
      catch (error) { this.cancel() }
    }, config.RECIPE_STEP_DRAG_SCROLL_INTERVAL_MS)
  }
  finish(event) {
    if (!this.active) { this.pointer = null; this.touchId = null; return }
    const point = this.point(event)
    if (!point) { this.cancel(); return }
    this.pointer = point
    const position = this.ready && this.valid() ? order.insertionIndex(this.rects, this.id, this.pointer.y + this.scrollTop) : null
    const steps = position != null && position !== this.from ? order.move(this.page.data.steps, this.id, position) : null
    this.cancel()
    if (steps && this.allowed()) this.page.setData({ steps, dirty: true })
  }
  cancelIfCurrent(serial) { if (this.active && serial === this.serial) this.cancel() }
  cancel() {
    ++this.serial; this.active = false; this.ready = false; this.pointer = null; this.touchId = null
    clearTimeout(this.measureTimer); clearTimeout(this.renderTimer); clearTimeout(this.scrollTimer)
    this.measureTimer = this.renderTimer = this.scrollTimer = null
    if (this.page.alive && this.page.data.dragging) this.page.setData({ dragging: false, dragId: '', dropBeforeId: '', dropAfterId: '' })
  }
}

module.exports = RecipeStepDrag