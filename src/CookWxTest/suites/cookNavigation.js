const { freshAccount, ownedRecipe, cookPage, until, assert } = require('./helpers')

module.exports = {
  37: async context => {
    const user = await freshAccount(context)
    const recipe = await ownedRecipe(context, user.token)
    const sim = context.simulator
    const cook = await cookPage(context, user.token)
    await sim.tap(cook, '.control-bar .primary-btn')
    const started = await sim.data(cook, data => data.started && data.running && data.elapsedSeconds >= 1, '返回前真实计时')
    const stack = await sim.mini.evaluate(() => getCurrentPages().map(page => page.route))
    assert.deepEqual(stack.slice(-2), ['pages/menu/menu', 'pages/cook/cook'])

    // 操作真实原生左上角及询问弹窗；不替换 wx.enableAlertBeforeUnload 或 showModal。
    await sim.mini.native().navigateLeft()
    await sim.mini.native().cancelModal()
    assert.equal((await sim.mini.currentPage()).path, 'pages/cook/cook')
    await sim.data(cook, data => data.running && data.elapsedSeconds > started.elapsedSeconds, '取消退出后继续真实计时')
    await sim.mini.native().navigateLeft()
    await sim.mini.native().confirmModal()
    const menu = await until(async () => {
      const current = await sim.mini.currentPage()
      return current && current.path === 'pages/menu/menu' && current
    }, '原生返回确认后到点单', 15000, context.signal)
    await sim.data(menu, data => data.recipes.length && !data.starting && !data.previewing && !data.selectedRecipes.length && !data.plan.summary, '点单选择及旧预览复位')
    const cleared = await sim.mini.evaluate(() => {
      const data = getApp().globalData
      return !data.lastPlan && !data.lastPreview && !data.selectedRecipeIds.length
    })
    assert.equal(cleared, true)

    await sim.tap(menu, `.cat[data-name="${recipe.category}"]`)
    await sim.input(menu, '.search', recipe.title)
    await sim.data(menu, data => data.activeCategory === recipe.category && data.filteredRecipes.some(item => item.id === recipe.id), '返回后分类和搜索响应')
    await sim.tap(menu, `.plus[data-id="${recipe.id}"]`)
    await sim.data(menu, data => data.selectedRecipes.some(item => item.id === recipe.id) && !data.previewing && data.plan.summary, '返回后重新选菜与真实预览')
    await sim.tap(menu, `.minus[data-id="${recipe.id}"]`)
    await sim.data(menu, data => !data.selectedRecipes.length && !data.previewing && !data.plan.summary, '返回后减菜响应')
    await sim.tap(menu, `.plus[data-id="${recipe.id}"]`)
    await sim.data(menu, data => data.plan.summary && !data.previewing, '再次选菜后可排程')
    await sim.tap(menu, '.cart .primary-btn')
    await until(async () => (await sim.mini.currentPage()).path === 'pages/plan/plan', '返回后再次进入计划', 15000, context.signal)
    return { nativeCancelContinued: true, nativeConfirmReturned: true, interactiveMenu: ['category', 'search', 'add', 'remove', 'plan'], recipeId: recipe.id }
  }
}