#pragma once

/** QApplication存活期间注册内容工厂，在应用窗口释放后回收全部托管弹窗 */
class SplitViewerDialogSession
{
public:
    /** GUI线程注册文件选择工厂，失败抛出异常 */
    SplitViewerDialogSession();

    /** QApplication析构前释放DialogManager及其Qt对象 */
    ~SplitViewerDialogSession();

private:
    /** 禁止复制生命周期所有者
    @param [in] other 被禁用的来源
    */
    SplitViewerDialogSession(const SplitViewerDialogSession& other);

    /** 禁止赋值生命周期所有者
    @param [in] other 被禁用的来源
    @return 当前对象引用，函数不可调用
    */
    SplitViewerDialogSession& operator=(const SplitViewerDialogSession& other);
};