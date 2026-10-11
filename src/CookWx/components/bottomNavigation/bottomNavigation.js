Component({
  data: { tabs: [
    { label: '首页', path: '/pages/home/home' }, { label: '菜单', path: '/pages/menu/menu' },
    { label: '+', path: '/pages/publish/publish' }, { label: '消息', path: '/pages/messages/messages' },
    { label: '我的', path: '/pages/mine/mine' }
  ] },
  methods: { switchTab(event) {
    const tab = this.data.tabs[Number(event.currentTarget.dataset.index)]
    if (tab) wx.switchTab({ url: tab.path })
  } }
})