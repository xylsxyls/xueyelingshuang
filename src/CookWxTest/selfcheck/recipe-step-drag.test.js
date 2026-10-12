const test = require('node:test'), assert = require('node:assert/strict'), fs = require('node:fs'), path = require('node:path'), vm = require('node:vm')
const order = require('../../CookWx/utils/recipeStepOrder')
const entries = () => ['a','b','c'].map((id, index) => ({ id, title: id, index: index + 1, active: index !== 1, detail: '说明' + id }))
function fixture(options = {}) {
  let callback, now = 0, serial = 0
  const timers = new Map(), calls = [], scrolls = []
  const page = { alive: true, editable: true, data: { steps: entries(), dragging: false }, canEdit() { return this.alive && this.editable }, setData(value, done) { calls.push(value); Object.assign(this.data, value); if (done) done() } }
  const platform = { hideKeyboard({ complete }) { complete() }, getWindowInfo: () => ({ windowHeight: 400 }), showToast: value => calls.push(value),
    createSelectorQuery() { return { in() { return this }, selectAll() { return this }, select() { return this }, selectViewport() { return this }, boundingClientRect() { return this }, scrollOffset() { return this }, exec(done) { callback = done } } },
    pageScrollTo(value) { scrolls.push(value.scrollTop); drag.onScroll(value.scrollTop); value.complete() } }
  const module = { exports: {} }
  vm.runInNewContext(fs.readFileSync(path.resolve(__dirname, '../../CookWx/utils/recipeStepDrag.js'), 'utf8'), { module,
    require: name => name === './config' ? { RECIPE_STEP_DRAG_MEASURE_TIMEOUT_MS: 1000, RECIPE_STEP_DRAG_FRAME_MS: 32, RECIPE_STEP_DRAG_EDGE_PX: 56, RECIPE_STEP_DRAG_SCROLL_STEP_PX: 16, RECIPE_STEP_DRAG_SCROLL_INTERVAL_MS: 50 } : order,
    setTimeout: (fn, ms) => { const id = ++serial; timers.set(id, { fn, at: now + ms }); return id }, clearTimeout: id => timers.delete(id) })
  const drag = new module.exports(page, platform)
  function advance(ms) { const end = now + ms; for (;;) { const ready = [...timers].filter(([, timer]) => timer.at <= end).sort((a,b) => a[1].at-b[1].at)[0]; if (!ready) break; now = ready[1].at; timers.delete(ready[0]); ready[1].fn() } now = end }
  function measure(scrollTop = 0) { callback([[{ top: 100-scrollTop, bottom: 200-scrollTop }, { top: 200-scrollTop, bottom: 300-scrollTop }, { top: 300-scrollTop, bottom: 400-scrollTop }], { height: 1000 }, { scrollTop }]) }
  const event = (id, y, identifier = 1) => ({ currentTarget: { dataset: { id } }, touches: [{ identifier, clientY: y }] })
  function start(id = 'a', y = 150) { const e = event(id,y); drag.arm(e); drag.start(e); if (!options.delayed) measure() }
  return { drag, page, calls, scrolls, timers, event, advance, measure, start }
}
test('长按前普通触摸不改顺序，真实卡片中点决定前后插入，松手才重排并编号', () => {
  const f = fixture(), original = f.page.data.steps
  f.drag.arm(f.event('a', 150)); f.drag.update(f.event('a', 320)); assert.equal(f.page.data.dragging, false)
  f.start(); f.drag.update(f.event('a', 330)); f.advance(32)
  assert.equal(f.page.data.dropBeforeId, 'c')
  assert.equal(f.page.data.steps, original)
  f.drag.finish({ changedTouches: [{ identifier: 1, clientY: 390 }] })
  assert.deepEqual(Array.from(f.page.data.steps, item => item.id), ['b','c','a'])
  assert.deepEqual(Array.from(f.page.data.steps, item => item.index), [1,2,3])
  assert.equal(f.page.data.steps[0].active, false)
  assert.equal(f.timers.size, 0)
})
test('快速反向移动使用最后位置；移动一百次只排队一个渲染且不传整个步骤数组', () => {
  const f = fixture(); f.start('c',330)
  const count = f.calls.length
  for (let i=0;i<100;i++) f.drag.update(f.event('c', 160+i%3))
  assert.equal(f.timers.size, 1)
  f.advance(32)
  assert.ok(f.calls.slice(count).every(item => !Object.hasOwn(item,'steps')))
  f.drag.finish({ changedTouches: [{ identifier: 1, clientY: 110 }] })
  assert.deepEqual(Array.from(f.page.data.steps, item => item.id), ['c','a','b'])
})
test('未测量就松手、取消及隐藏后的晚测量不能启动或提交拖动', () => {
  const f = fixture({ delayed: true }); f.start()
  f.drag.finish({ changedTouches: [{ identifier: 1, clientY: 390 }] }); f.measure()
  assert.equal(f.page.data.dragging, false); assert.equal(f.page.data.steps[0].id, 'a')
  const g = fixture({ delayed: true }); g.start(); g.page.editable=false; g.drag.cancel(); g.measure()
  assert.equal(g.page.data.dragging, false); assert.equal(g.timers.size, 0)
})
test('多指、页面数据更换、测量异常和超时都取消，不误提交旧对象', () => {
  const f=fixture(); f.start(); f.drag.update({ touches: [{identifier:1,clientY:10},{identifier:2,clientY:20}] })
  assert.equal(f.page.data.dragging,false)
  const g=fixture(); g.start(); g.page.data.steps=entries().reverse(); g.drag.finish({changedTouches:[{identifier:1,clientY:350}]})
  assert.equal(g.page.data.steps[0].id,'c'); assert.equal(g.page.data.dirty,undefined)
  const h=fixture({delayed:true}); h.start(); h.advance(1000); assert.equal(h.page.data.dragging,false); assert.equal(h.timers.size,0)
  const multi=fixture(); multi.drag.arm(multi.event('a',150)); multi.drag.start({currentTarget:{dataset:{id:'a'}},touches:[{identifier:1,clientY:150},{identifier:2,clientY:160}]})
  assert.equal(multi.page.data.dragging,false); assert.equal(multi.timers.size,0)
  const stale=fixture({delayed:true}); stale.start(); stale.page.data.steps=entries().reverse(); stale.measure()
  assert.equal(stale.page.data.dragging,false); assert.equal(stale.timers.size,0)
  const owner=fixture({delayed:true}); owner.start(); owner.page.editable=false; owner.advance(1000)
  assert.equal(owner.page.data.dragging,false); assert.equal(owner.timers.size,0)
})
test('边缘滚动坐标与页面真实滚动同步；移回中间和结束停止滚动', () => {
  const f=fixture(); f.start(); f.drag.update(f.event('a',390)); f.advance(50)
  assert.equal(f.scrolls[0],16)
  f.drag.update(f.event('a',200)); f.advance(100)
  const count=f.scrolls.length; f.advance(100); assert.equal(f.scrolls.length,count)
  f.drag.cancel(); assert.equal(f.timers.size,0)
  const g=fixture(); g.start(); g.drag.onScroll(100); g.drag.update(g.event('a',390)); g.advance(32)
  assert.equal(g.page.data.dropAfterId,'c'); g.drag.cancel()
})
test('步骤操作只调整目标身份，新增删除不会改别的步骤，重复ID和最后一步拒绝', () => {
  const original=entries(), added=order.insert(original,{id:'d',title:'新'},'b','after')
  assert.deepEqual(added.map(item=>item.id),['a','b','d','c'])
  assert.equal(original.length,3); assert.equal(order.insert(original,{id:'b'},'a','before'),null)
  assert.equal(order.remove([{id:'a'}],'a'),null)
  assert.deepEqual(order.remove(added,'b').map(item=>item.id),['a','d','c'])
})