#pragma once

/** QApplication存活期间注册内容工厂，在应用窗口释放后回收全部托管弹窗
*/
class SplitViewerDialogSession
{
public:
    /** 设置安全初值，工厂由init显式注册
    */
    SplitViewerDialogSession();

    /** QApplication析构前释放DialogManager及其Qt对象
    */
    ~SplitViewerDialogSession();

    /** GUI线程注册两个工厂，重复初始化不重复转交所有权
    @return 注册是否完整成功，失败回滚本次注册
    */
    bool init();

    /** 在QApplication存活时回收本会话的窗口和管理器，允许重复调用
    */
    void uninit();

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

private:
    // 是否已成功注册并接管DialogManager生命周期
    bool m_initialized;
};