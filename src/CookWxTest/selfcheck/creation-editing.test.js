const test = require('node:test')
const assert = require('node:assert/strict')
const fs = require('node:fs'), path = require('node:path'), vm = require('node:vm')
const editing = require('../../CookWx/utils/creationEditing')
const creationAssets = require('../../CookWx/utils/creationAssets')
const flush = () => new Promise(resolve => setImmediate(resolve))
const recipe = () => ({ id: 'r', currentRevisionId: 'r1', title: '我的菜', authorUserId: 'self', priceType: 'coin', priceAmount: 18,
  tasks: [{ id: 'r.cut', title: '切小块', detail: '均匀切块', durationSeconds: 10, active: true, resource: 'hands', dependsOn: [] }] })

function fixture(apiOverrides = {}, options = { id: 'r' }, userId = 'self') {
  const calls = [], app = { globalData: { userId } }, platform = { showToast: options => calls.push(options), showModal: options => { calls.modal = options }, navigateTo: options => calls.push(options) }
  const api = { getCreationRecipe: async () => ({ ok: true, recipe: recipe() }),
    saveCreationSteps: async body => { calls.push(body); return { ok: true, recipe: { ...recipe(), currentRevisionId: 'r2', reviewId: 'r2', reviewState: 'pending', tasks: body.steps, priceType: body.priceType, priceAmount: body.priceAmount } } },
    isStaleError: error => error && error.stale, ...apiOverrides }
  let page
  vm.runInNewContext(fs.readFileSync(path.resolve(__dirname, '../../CookWx/pages/creationEditor/creationEditor.js'), 'utf8'), {
    Page: value => { page = value }, getApp: () => app, wx: platform,
    require: name => name.endsWith('/api') ? api : require(path.resolve(__dirname, '../../CookWx/pages/creationEditor', name)), console
  })
  page.data = structuredClone(page.data)
  page.setData = values => { for (const [key, value] of Object.entries(values)) { const parts = key.replace(/\[(\d+)\]/g, '.$1').split('.'); let target = page.data; for (const part of parts.slice(0, -1)) target = target[part]; target[parts.at(-1)] = value } }
  page.onLoad(options); page.onShow()
  return { page, app, api, calls, platform }
}

test('本人创作是小图列表，按真实作者过滤；纯视频保留且不重复添加已关联菜谱', () => {
  const rows = creationAssets.build([recipe(), { ...recipe(), id: 'peer', authorUserId: 'peer' }],
    [{ id: 'v1', videoId: 'v1', recipeId: 'r', posterUrl: '/p1' }, { id: 'v2', videoId: 'v2', recipeId: '', posterUrl: '/p2' }], 'self', item => ({ id: 'recipe_' + item.id, recipeId: item.id }))
  assert.equal(rows.length, 2)
  assert.equal(rows[0].creation, true)
  assert.equal(rows[0].recipeId, 'r')
  assert.equal(rows[1].videoId, 'v2')
  assert.equal(rows[1].thumbnailUrl, '/p2')
  assert.equal(creationAssets.build([recipe()], [], '', item => item).length, 0)
})

test('作者保存发送原ID、版本、定价及步骤名说明时长，不发送作者依赖等只读字段', () => {
  const source = recipe(), steps = editing.steps(source)
  steps[0].title = '小火慢煮'; steps[0].detail = '保持小火'; steps[0].minutes = 2; steps[0].seconds = 3
  const body = editing.payload(source, steps)
  assert.deepEqual(Object.keys(body).sort(), ['currentRevisionId', 'editorVersion', 'priceAmount', 'priceType', 'recipeId', 'steps'])
  assert.equal(body.editorVersion, 2)
  assert.equal(body.priceType, 'coin')
  assert.equal(body.priceAmount, 18)
  assert.deepEqual(body.steps[0], { id: 'r.cut', title: '小火慢煮', detail: '保持小火', durationSeconds: 123, active: true })
  assert.equal(source.tasks[0].durationSeconds, 10)
})

test('免费金币元宝定价只接受收费正整数，免费提交零，边界与非法输入受控', () => {
  assert.deepEqual(editing.pricePayload({ priceType: 'free', priceAmount: 'bad' }), { priceType: 'free', priceAmount: 0 })
  for (const type of ['coin', 'yuanbao']) {
    assert.deepEqual(editing.pricePayload({ priceType: type, priceAmount: '2147483647' }), { priceType: type, priceAmount: 2147483647 })
    for (const value of ['', '0', '-1', '1.5', '1e3', 'abc', '2147483648', Infinity, NaN]) assert.throws(() => editing.pricePayload({ priceType: type, priceAmount: value }))
  }
  assert.throws(() => editing.pricePayload({ priceType: 'cash', priceAmount: 10 }))
})

test('页面读取服务端定价，切币种保存真实金额，撤销同时恢复定价和步骤', async () => {
  const f = fixture(); await flush()
  assert.equal(f.page.data.priceType, 'coin')
  assert.equal(f.page.data.priceAmount, '18')
  f.page.onPriceTypeChange({ detail: { value: 'yuanbao' } })
  f.page.onPriceInput({ detail: { value: '27' } })
  await f.page.save()
  assert.equal(f.calls[0].priceType, 'yuanbao')
  assert.equal(f.calls[0].priceAmount, 27)
  assert.equal(f.page.data.recipe.priceType, 'yuanbao')
  f.page.onPriceTypeChange({ detail: { value: 'free' } })
  f.page.data.steps[0].title = '新标题'
  f.page.resetDraft(); f.calls.modal.success({ confirm: true })
  assert.equal(f.page.data.priceType, 'yuanbao')
  assert.equal(f.page.data.priceAmount, '27')
  assert.notEqual(f.page.data.steps[0].title, '新标题')
  f.page.onPriceInput({ detail: { value: '0' } })
  const count = f.calls.length
  await f.page.save()
  assert.equal(f.calls.slice(count).filter(value => value.recipeId).length, 0)
  assert.equal(f.page.data.dirty, true)
})

test('线上旧服务器404显示版本提示并可重试，缺少菜谱ID或登录身份不留下空白加载', async () => {
  const error = Object.assign(new Error('API not found'), { statusCode: 404 })
  const f = fixture({ getCreationRecipe: async () => { throw error } }); await flush()
  assert.match(f.page.data.error, /更新 CookServer/)
  assert.equal(f.page.data.loading, false)
  f.api.getCreationRecipe = async () => ({ ok: true, recipe: recipe() })
  await f.page.load()
  assert.equal(f.page.data.recipe.id, 'r')
  const missing = fixture({}, {}); await flush()
  assert.match(missing.page.data.error, /菜谱不存在/)
  const guest = fixture({}, { id: 'r' }, ''); await flush()
  assert.match(guest.page.data.error, /登录状态/)
  assert.equal(guest.page.data.loading, false)
})

test('旧服务器忽略定价不能显示保存成功，草稿保留并使用实际返回的新版本重试', async () => {
  const bodies = []
  const f = fixture({ saveCreationSteps: async body => { bodies.push(body); return { ok: true, recipe: { ...recipe(), currentRevisionId: 'r2' } } } }); await flush()
  f.page.onPriceTypeChange({ detail: { value: 'yuanbao' } })
  f.page.onPriceInput({ detail: { value: '27' } })
  await f.page.save()
  assert.match(f.page.data.error, /服务器未保存菜谱定价/)
  assert.equal(f.page.data.dirty, true)
  assert.equal(f.page.data.priceType, 'yuanbao')
  assert.equal(f.page.data.priceAmount, '27')
  assert.equal(f.page.data.recipe.currentRevisionId, 'r2')
  assert.equal(f.calls.filter(value => value.icon === 'success').length, 0)
  await f.page.save()
  assert.equal(bodies[1].currentRevisionId, 'r2')
})

test('八字标题、说明及24小时边界校验，重复ID和无效时间不提交', () => {
  const source = recipe(), steps = editing.steps(source)
  steps[0].title = '一二三四五六七八'; steps[0].hours = 24; steps[0].seconds = 0
  assert.equal(editing.payload(source, steps).steps[0].durationSeconds, 86400)
  steps[0].seconds = 1; assert.throws(() => editing.payload(source, steps))
  steps[0].hours = 0; steps[0].seconds = 0; steps[0].title += '九'; assert.throws(() => editing.payload(source, steps))
  steps[0].title = '切菜'; steps[0].detail = '字'.repeat(2001); assert.throws(() => editing.payload(source, steps))
  steps[0].detail = '正常'; assert.throws(() => editing.payload(source, steps.concat(steps)))
  steps[0].seconds = NaN; assert.throws(() => editing.payload(source, steps))
})

test('页面输入后真实调用保存接口，新版本及步骤来自服务器响应', async () => {
  const f = fixture(); await flush()
  f.page.onStepInput({ currentTarget: { dataset: { index: 0, field: 'title' } }, detail: { value: '小火慢煮' } })
  f.page.onStepInput({ currentTarget: { dataset: { index: 0, field: 'detail' } }, detail: { value: '保持小火' } })
  f.page.onStepInput({ currentTarget: { dataset: { index: 0, field: 'minutes' } }, detail: { value: '2' } })
  await f.page.save()
  assert.equal(f.calls[0].steps[0].title, '小火慢煮')
  assert.equal(f.calls[0].steps[0].detail, '保持小火')
  assert.equal(f.calls[0].steps[0].durationSeconds, 130)
  assert.equal(f.page.data.recipe.currentRevisionId, 'r2')
  assert.equal(f.page.data.dirty, false)
  assert.equal(f.page.data.saving, false)
})

test('加载/保存失败保留编辑内容与重试入口，账号切换后不发送旧账号请求', async () => {
  const f = fixture({ saveCreationSteps: async () => ({ ok: false, message: '版本冲突' }) }); await flush()
  f.page.onStepInput({ currentTarget: { dataset: { index: 0, field: 'detail' } }, detail: { value: '保留草稿' } })
  await f.page.save()
  assert.equal(f.page.data.steps[0].detail, '保留草稿')
  assert.equal(f.page.data.dirty, true)
  f.app.globalData.userId = 'other'
  const count = f.calls.length
  await f.page.save()
  assert.equal(f.calls.length, count)
})

test('卸载后旧读取不写页面，旧撤销弹框不能清空新草稿', async () => {
  let resolve
  const f = fixture({ getCreationRecipe: () => new Promise(done => { resolve = done }) })
  f.page.onUnload(); resolve({ ok: true, recipe: recipe() }); await flush()
  assert.equal(f.page.data.recipe, null)
  const g = fixture(); await flush()
  g.page.data.dirty = true; g.page.resetDraft()
  g.page.generation++
  g.page.data.steps[0].title = '新草稿'
  g.calls.modal.success({ confirm: true })
  assert.equal(g.page.data.steps[0].title, '新草稿')
})

test('保存途中隐藏后仍识别成功，下次显示读取服务器新版本；卸载后的保存不更新账号状态', async () => {
  let resolve, options
  const f = fixture({ saveCreationSteps: (body, requestOptions) => { options = requestOptions; return new Promise(done => { resolve = done }) } })
  await flush()
  f.page.data.dirty = true
  const saved = f.page.save()
  f.page.onHide()
  assert.equal(options.isStale(), false)
  resolve({ ok: true, recipe: { ...recipe(), currentRevisionId: 'r2', reviewId: 'r2', reviewState: 'pending' } })
  await saved
  assert.equal(f.page.reloadOnShow, true)
  assert.equal(f.app.globalData.recipeStateVersion, 1)
  f.api.getCreationRecipe = async () => ({ ok: true, recipe: { ...recipe(), currentRevisionId: 'r2' } })
  f.page.onShow(); await flush()
  assert.equal(f.page.data.recipe.currentRevisionId, 'r2')
  assert.equal(f.page.data.dirty, false)
  const g = fixture({ saveCreationSteps: (body, requestOptions) => { options = requestOptions; return new Promise(done => { resolve = done }) } })
  await flush()
  const staleSave = g.page.save()
  g.page.onUnload()
  assert.equal(options.isStale(), true)
  resolve({ ok: true, recipe: recipe() }); await staleSave
  assert.equal(g.app.globalData.recipeStateVersion, undefined)
})

test('创作复用新增页，隐藏时事件按锚点插入、连续编号，删除确认与最后一步限制', async () => {
  const f = fixture(); await flush()
  let created, configuration
  f.platform.navigateTo = options => options.success({ eventChannel: { on: (name, callback) => { created = callback }, emit: (name, value) => { configuration = value } } })
  f.page.addStep()
  assert.equal(configuration.steps[0].key, 'r.cut')
  f.page.onHide()
  created({ id: 'added_a', title: '搅拌', detail: '慢慢搅拌', durationSeconds: 5, active: true, insertRelativeTo: 'r.cut', insertPosition: 'before' })
  assert.equal(f.page.data.steps[0].id, 'r.added_a')
  assert.deepEqual(Array.from(f.page.data.steps, item => item.index), [1, 2])
  f.page.onShow()
  f.page.deleteStep({ currentTarget: { dataset: { id: 'r.cut' } } })
  f.calls.modal.success({ confirm: false })
  assert.equal(f.page.data.steps.length, 2)
  f.page.deleteStep({ currentTarget: { dataset: { id: 'r.cut' } } }); f.calls.modal.success({ confirm: true })
  assert.equal(f.page.data.steps.length, 1)
  f.page.deleteStep({ currentTarget: { dataset: { id: 'r.added_a' } } })
  assert.equal(f.page.data.steps.length, 1)
  f.page.onModeChange({ currentTarget: { dataset: { index: 0 } }, detail: { value: '1' } })
  await f.page.save()
  const body = f.calls.find(value => value.recipeId)
  assert.equal(body.steps[0].active, false)
  assert.equal(body.steps[0].id, 'r.added_a')
})

test('旧页面新增回调和删除确认不影响重新加载后的步骤，旧服务忽略顺序不报告成功', async () => {
  const f = fixture(); await flush()
  let created
  f.platform.navigateTo = options => options.success({ eventChannel: { on: (name, callback) => { created = callback }, emit() {} } })
  f.page.addStep(); ++f.page.generation
  created({ id: 'added_old', title: '旧步骤', active: true, durationSeconds: 10, insertRelativeTo: 'r.cut' })
  assert.equal(f.page.data.steps.length, 1)
  const g = fixture({ saveCreationSteps: async body => ({ ok: true, recipe: { ...recipe(), currentRevisionId: 'r2', priceType: body.priceType, priceAmount: body.priceAmount } }) }); await flush()
  g.page.onModeChange({ currentTarget: { dataset: { index: 0 } }, detail: { value: 1 } })
  await g.page.save()
  assert.match(g.page.data.error, /服务器未保存步骤顺序或性质/)
  assert.equal(g.page.data.steps[0].active, false)
  assert.equal(g.page.data.dirty, true)
  assert.equal(g.page.data.recipe.currentRevisionId, 'r2')
})