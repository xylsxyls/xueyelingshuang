Page({
    data: { report: '' },
    onShow() {
        const update = () => this.setData({ report: JSON.stringify(getApp().globalData.netInterfaceReport, null, 2) });
        update();
        this.reportTimer = setInterval(update, 500);
    },
    onHide() { clearInterval(this.reportTimer); },
    onUnload() { clearInterval(this.reportTimer); }
});