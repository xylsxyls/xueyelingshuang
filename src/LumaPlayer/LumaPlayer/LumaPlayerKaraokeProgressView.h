#pragma once
#include "DialogManager/DialogManagerAPI.h"
#include "LumaPlayerKaraokeJob.h"
#include <QPointer>

struct LumaPlayerKaraokeProgressParam : public CustomDialogParam
{
public:
    // The caller owns the job; queued window disposal must not extend the borrow.
    QPointer<LumaPlayerKaraokeJob> m_task;

public:
    /** 创建借用任务的进度弹窗参数
    @param [in] task 调用方拥有的任务，可为空，QPointer不延长其生命周期
    */
    explicit LumaPlayerKaraokeProgressParam(LumaPlayerKaraokeJob* task);
};

/** DialogManager-owned progress content; the modal caller owns and joins its worker
*/
class LumaPlayerKaraokeProgressView : public CustomViewBase
{
public:
    /** 初始化LumaPlayerKaraokeProgressView对象的状态，不转移输入借用对象的所有权
    @param [in] task 由调用方拥有的任务，弹窗处理期间保持有效
    */
    explicit LumaPlayerKaraokeProgressView(LumaPlayerKaraokeJob* task);

    /** 按DialogManager参数初始化内容控件
    @param [in] param DialogManager传入的参数，借用至返回
    @return true表示内容初始化完成，false表示参数无效
    */
    virtual bool initView(const DialogParam& param);

    /** 提供弹窗内容的首选逻辑像素尺寸
    @return 内容区域逻辑像素尺寸
    */
    virtual QSize preferredSize() const;

protected:
    /** 拦截弹窗事件并保持任务与窗口生命周期一致
    @param [in] object 事件来源对象，借用至返回
    @param [in] event Qt事件，借用至事件处理返回，不保留指针
    @return true表示事件已拦截，false继续按基类规则处理
    */
    virtual bool eventFilter(QObject* object, QEvent* event);

private:
    // 进度弹窗借用的任务，调用方负责等待其退出
    LumaPlayerKaraokeJob* m_task;
};

/** 向DialogManager创建进度内容，返回对象归管理器所有
*/
class LumaPlayerKaraokeProgressFactory : public CustomViewFactory
{
public:
    /** 创建与参数类型匹配的内容，由DialogManager取得所有权
    @param [in] param DialogManager传入的参数，借用至返回
    @return 新内容对象，参数不匹配时为空，由DialogManager销毁
    */
    virtual CustomViewBase* createView(const DialogParam& param);

    /** 释放工厂对象，由DialogManager在注销后调用
    @param [in] factory 由DialogManager转交销毁的工厂，不可再使用
    */
    static void destroy(CustomViewFactory* factory);
};