const { freshAccount, account, recipes, ownedRecipe, recipePage, menu, pickRecipe, verifyPlan, cookPage, until, sleep, assert } = require('./helpers')

async function personalPage(context, recipeId) {
  const page = await context.simulator.page('/pages/personalization/personalization?id=' + encodeURIComponent(recipeId))
  await context.simulator.data(page, data => data.recipe && !data.loading && data.steps.length, '个性化真实加载')
  return page
}

async function savePersonal(context, page, token, recipeId, predicate) {
  await context.simulator.tap(page, '.toolbar .primary-btn')
  return until(async () => {
    const response = await context.http.json('/api/recipes/personalization/detail?recipeId=' + encodeURIComponent(recipeId), { token })
    return predicate(response.personalization) && response.personalization
  }, '个性化真实存储', 15000, context.signal)
}

module.exports = {
  5: async context => {
    const user = await freshAccount(context)
    const page = await menu(context)
    const initial = await page.data()
    const directory = await recipes(context, user.token)
    assert.deepEqual(initial.recipes.map(item => item.id).sort(), directory.map(item => item.id).sort())
    for (const category of initial.categories) {
      await context.simulator.tap(page, `.cat[data-name="${category}"]`)
      assert.equal((await page.data()).activeCategory, category)
    }
    const target = initial.recipes.find(item => item.owned)
    await context.simulator.tap(page, `.cat[data-name="${target.category}"]`)
    await context.simulator.input(page, '.search', target.title)
    const state = await page.data()
    assert.ok(state.filteredRecipes.some(item => item.id === target.id))
    assert.ok(state.filteredRecipes.every(item => (item.title + item.subtitle + item.category).includes(target.title)))
    await context.simulator.input(page, '.search', '__不存在的菜谱__')
    assert.equal((await page.data()).filteredRecipes.length, 0)
    return { categoryCount: initial.categories.length, recipeCount: directory.length, searchAndEmpty: true }
  },
  6: async context => {
    const user = await freshAccount(context)
    const recipe = await ownedRecipe(context, user.token)
    const page = await recipePage(context, recipe.id)
    const data = await page.data()
    assert.ok(data.recipe.tasks.length > 0)
    const expected = await context.http.json('/api/recipes/detail?recipeId=' + recipe.id, { token: user.token })
    assert.equal(data.recipe.title, expected.recipe.title)
    assert.equal(data.recipe.tasks.length, expected.recipe.tasks.length)
    assert.equal(await page.$('.action-bar button[bindtap="favorite"]'), null)
    const before = await context.simulator.mini.evaluate(() => getApp().globalData.selectedRecipeIds.slice())
    await context.simulator.tap(page, '.action-bar .primary-btn')
    const cart = await menu(context)
    await context.simulator.data(cart, item => item.recipes.some(value => value.id === recipe.id && value.inMenu), '加入后菜单含目标菜')
    assert.deepEqual(await context.simulator.mini.evaluate(() => getApp().globalData.selectedRecipeIds.slice()), before)
    return { recipeId: recipe.id, taskCount: data.recipe.tasks.length }
  },
  7: async context => {
    const user = await freshAccount(context)
    const recipe = await ownedRecipe(context, user.token, true)
    const before = await account(context, user.token)
    const page = await recipePage(context, recipe.id)
    await context.simulator.mock('showModal', { confirm: true, cancel: false })
    await context.simulator.tap(page, '.purchase .primary-btn')
    await context.simulator.data(page, data => data.recipe.owned && !data.buying, '购买后拥有菜谱')
    const after = await account(context, user.token)
    const price = Number(recipe.priceAmount || recipe.priceCoins)
    const field = recipe.priceType === 'yuanbao' ? 'yuanbao' : 'coins'
    assert.equal(after[field], before[field] - price)
    assert.ok(after.ownedRecipeIds.includes(recipe.id))
    const body = { recipeId: recipe.id, idempotencyKey: 'duplicate_' + user.account.userId }
    await context.http.json('/api/recipes/purchase', { token: user.token, body })
    await context.http.json('/api/recipes/purchase', { token: user.token, body })
    assert.equal((await account(context, user.token))[field], after[field])
    return { recipeId: recipe.id, price, field, repeatNoCharge: true }
  },
  8: async context => require('./menuSocial')[8](context),
  9: async context => {
    const user = await freshAccount(context), sim = context.simulator, page = await menu(context)
    const selected = (await page.data()).recipes.filter(item => item.accessible).slice(0, 2)
    assert.equal(selected.length, 2, '隔离菜单须有两道可制作的菜')
    for (const recipe of selected) await pickRecipe(context, page, recipe)
    await sim.tap(page, '.cart-info')
    await sim.data(page, data => data.cartOpen && data.selectedRecipes.length === 2, '展开本次选择')
    assert.equal((await page.$$('.cart-sheet .chosen-remove')).length, 2)
    await sim.tap(page, `.cart-sheet .chosen-remove[data-id="${selected[0].id}"]`)
    const remaining = await sim.data(page, data => data.selectedRecipes.length === 1 && data.selectedRecipes[0].id === selected[1].id && data.planRecipeKey === selected[1].id && data.plan.summary && !data.previewing, '减去一菜后计算剩余菜预览')
    assert.equal(remaining.cartOpen, true)
    assert.deepEqual(await sim.mini.evaluate(() => getApp().globalData.selectedRecipeIds.slice()), [selected[1].id])
    const card = `.recipe-card[data-id="${selected[0].id}"]`
    assert.equal(await (await sim.element(page, card + ' .plus')).property('disabled'), false)
    assert.equal(await (await sim.element(page, card + ' .minus')).property('disabled'), true)
    const expected = await context.http.json('/api/plan/preview', { token: user.token, body: { recipeIds: [selected[1].id], summaryOnly: true } })
    assert.equal(remaining.plan.summary.totalSeconds, expected.summary.totalSeconds)
    await sim.tap(page, `.cart-sheet .chosen-remove[data-id="${selected[1].id}"]`)
    const empty = await sim.data(page, data => !data.selectedRecipes.length && !data.previewing, '删完本次选择清理预览')
    assert.ok(!empty.plan.summary)
    assert.equal(empty.cartOpen, false)
    assert.equal(await page.$('.cart'), null)
    assert.equal(await page.$('.cart-sheet'), null)
    assert.deepEqual(await sim.mini.evaluate(() => getApp().globalData.selectedRecipeIds.slice()), [])
    const stored = await recipes(context, user.token)
    assert.ok(selected.every(recipe => stored.some(item => item.id === recipe.id && item.inMenu)), '本次减菜保留服务端长期菜单')
    await pickRecipe(context, page, selected[0])
    await sim.tap(page, `.recipe-card[data-id="${selected[0].id}"] .minus`)
    await sim.data(page, data => !data.selectedRecipes.length && !data.plan.summary, '原菜卡减号仍可移除')
    return { recipeIds: selected.map(item => item.id), sheetRemovalSynchronized: true, remainingPreviewSeconds: remaining.plan.summary.totalSeconds, membershipUnchanged: true, emptyCartClosed: true, cardMinusWorks: true }
  },
  10: async context => {
    const user = await freshAccount(context)
    const owned = (await recipes(context, user.token)).filter(item => item.owned).slice(0, 2)
    assert.equal(owned.length, 2)
    const page = await menu(context)
    for (const recipe of owned) await pickRecipe(context, page, recipe)
    const ids = owned.map(item => item.id)
    const expected = await context.http.json('/api/plan/preview', { token: user.token, body: { recipeIds: ids, summaryOnly: false } })
    verifyPlan(expected)
    const state = await page.data()
    assert.equal(state.plan.summary.totalSeconds, expected.summary.totalSeconds)
    assert.deepEqual(state.selectedRecipes.map(item => item.id).sort(), ids.sort())
    await context.simulator.tap(page, '.cart .primary-btn')
    const planPage = await until(async () => { const current = await context.simulator.mini.currentPage(); return current.path === 'pages/plan/plan' && current }, '计划页面', 15000, context.signal)
    await context.simulator.data(planPage, data => data.plan && data.plan.summary && data.plan.freeBlocks, '计划摘要与区间')
    assert.equal((await planPage.data()).plan.summary.totalSeconds, expected.summary.totalSeconds)
    return { recipeIds: ids, taskCount: expected.timeline.length, totalSeconds: expected.summary.totalSeconds }
  },
  11: async context => {
    const user = await freshAccount(context)
    const recipe = await ownedRecipe(context, user.token)
    const page = await personalPage(context, recipe.id)
    const step = (await page.data()).steps[0]
    const seconds = step.seconds === 59 ? 58 : step.seconds + 1
    await context.simulator.input(page, 'input[data-index="0"][data-field="seconds"]', seconds)
    const changed = await savePersonal(context, page, user.token, recipe.id, item => item.durationOverrides.some(value => value.taskId === (step.shortId || step.id) && value.durationSeconds === step.hours * 3600 + step.minutes * 60 + seconds))
    const reopened = await personalPage(context, recipe.id)
    assert.equal((await reopened.data()).steps[0].seconds, seconds)
    const plan = await context.http.json('/api/plan/preview', { token: user.token, body: { recipeIds: [recipe.id], summaryOnly: false } })
    assert.ok(plan.timeline.some(item => (item.id === step.id || item.shortId === step.shortId) && item.durationSeconds === step.hours * 3600 + step.minutes * 60 + seconds))
    return { recipeId: recipe.id, changedDuration: changed.durationOverrides }
  },
  12: async context => {
    const user = await freshAccount(context)
    const recipe = await ownedRecipe(context, user.token)
    const page = await personalPage(context, recipe.id)
    await context.simulator.tap(page, '.toolbar .ghost-btn')
    const editor = await until(async () => { const current = await context.simulator.mini.currentPage(); return current.path === 'pages/personalization/taskEditor' && current }, '新增步骤编辑器', 15000, context.signal)
    await context.simulator.input(editor, 'input[data-field="title"]', '测试新增步骤')
    await context.simulator.input(editor, 'input[data-field="seconds"]', 15)
    await context.simulator.tap(editor, '.primary-btn')
    await until(async () => (await context.simulator.mini.currentPage()).path === 'pages/personalization/personalization', '返回个性化', 15000, context.signal)
    const personal = await context.simulator.mini.currentPage()
    await context.simulator.data(personal, data => data.steps.some(item => item.added && item.title === '测试新增步骤'), '真实 eventChannel 新增回传')
    await context.simulator.tap(personal, '.toggle-btn[data-index="0"]')
    await savePersonal(context, personal, user.token, recipe.id, item => item.addedTasks.length === 1 && item.disabledTaskIds.length === 1)
    await context.simulator.mock('showModal', { confirm: true, cancel: false })
    await context.simulator.tap(personal, '.toolbar .danger')
    const reset = await until(async () => {
      const result = await context.http.json('/api/recipes/personalization/detail?recipeId=' + recipe.id, { token: user.token })
      return !result.personalization.addedTasks.length && !result.personalization.disabledTaskIds.length && result.personalization
    }, '重置真实存储', 15000, context.signal)
    return { recipeId: recipe.id, addedDisabledReset: true, reset }
  },
  13: async context => {
    const user = await freshAccount(context)
    const recipe = await ownedRecipe(context, user.token)
    const page = await personalPage(context, recipe.id)
    await context.simulator.tap(page, '.toolbar .ghost-btn')
    const editor = await until(async () => { const current = await context.simulator.mini.currentPage(); return current.path === 'pages/personalization/taskEditor' && current }, '安全步骤编辑器', 15000, context.signal)
    await context.simulator.input(editor, 'input[data-field="title"]', '危险后台测试')
    await context.simulator.input(editor, 'input[data-field="seconds"]', 30)
    await (await context.simulator.element(editor, 'picker[bindchange="onTaskModeChange"]')).trigger('change', { value: 1 })
    await (await context.simulator.element(editor, 'picker[bindchange="onSafetyChange"]')).trigger('change', { value: 2 })
    await (await context.simulator.element(editor, 'picker[bindchange="onWaitModeChange"]')).trigger('change', { value: 1 })
    await context.simulator.tap(editor, '.primary-btn')
    const personal = await until(async () => { const current = await context.simulator.mini.currentPage(); return current.path === 'pages/personalization/personalization' && current }, '安全步骤返回', 15000, context.signal)
    const data = await context.simulator.data(personal, value => value.steps.filter(item => item.added).length === 1, '新增后台步骤')
    assert.equal(data.steps.find(item => item.added).canLeaveKitchen, false)
    assert.equal(data.steps.find(item => item.added).backgroundWaitMode, 'watch')
    await savePersonal(context, personal, user.token, recipe.id, item => item.addedTasks.length === 1)
    const plan = await context.http.json('/api/plan/preview', { token: user.token, body: { recipeIds: [recipe.id] } })
    const task = plan.timeline.find(item => item.title === '危险后台测试')
    assert.ok(task)
    assert.equal(task.canLeaveKitchen, false)
    assert.equal(task.backgroundWaitMode, 'watch')
    return { clientAndServerForcedWatch: true }
  },
  14: async context => {
    const user = await freshAccount(context)
    const page = await cookPage(context, user.token)
    const sim = context.simulator
    if (!(await page.data()).running) await sim.tap(page, '.control-bar .primary-btn')
    await sim.data(page, data => data.started && data.running && data.elapsedSeconds >= 1, '指导真实计时')
    await sim.tap(page, '.control-bar .primary-btn')
    const paused = await sim.data(page, data => !data.running, '暂停状态')
    const value = paused.elapsedSeconds
    await sleep(1200, context.signal)
    assert.equal((await page.data()).elapsedSeconds, value)
    await sim.tap(page, '.control-bar .primary-btn')
    await sim.data(page, data => data.running && data.elapsedSeconds > value, '恢复指导计时')
    return { pauseAt: value, actualResume: true }
  },
  15: async context => {
    const user = await freshAccount(context)
    const page = await cookPage(context, user.token)
    const sim = context.simulator
    if (!(await page.data()).running) await sim.tap(page, '.control-bar .primary-btn')
    const start = await sim.data(page, data => data.started && data.currentTask.id && data.currentTask.canAddTime, '可调整的前台步骤')
    await sim.tap(page, '.current-actions button[bindtap="addTime"]')
    await sim.data(page, data => data.plan.summary.totalSeconds === start.plan.summary.totalSeconds + 10, '加10秒后实际计划')
    await sim.mock('showModal', { confirm: true, cancel: false })
    await sim.tap(page, '.current-actions button[bindtap="markDone"]')
    await sim.data(page, data => data.currentTask.id !== start.currentTask.id, '提前完成转入后续步骤')
    const state = await page.data()
    verifyPlan(state.plan)
    return { addedSeconds: 10, completedTask: start.currentTask.id, newTask: state.currentTask.id, incomplete: '后台跳过与全部安全属性组合尚未覆盖' }
  },
  16: async context => {
    const user = await freshAccount(context)
    const page = await cookPage(context, user.token)
    const sim = context.simulator
    if (!(await page.data()).running) await sim.tap(page, '.control-bar .primary-btn')
    const before = await sim.data(page, data => data.started && data.running && data.elapsedSeconds >= 1, '后台恢复前计时')
    await sim.mini.navigateTo('/pages/voice/voice')
    await sleep(2200, context.signal)
    await sim.mini.navigateBack()
    await sim.data(page, data => data.elapsedSeconds >= before.elapsedSeconds + 2 && !data.complete, '返回后基于实际时间继续')
    await sim.mock('showModal', { confirm: true, cancel: false })
    await sim.page('/pages/menu/menu', true)
    const current = await sim.mini.currentPage()
    assert.equal(current.path, 'pages/menu/menu')
    return { restoredWallClock: true, exitedToMenu: true, incomplete: '旧计时器/语音资源停止未独立观测；锁屏需真机验证' }
  }
}