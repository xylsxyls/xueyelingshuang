#pragma once
#include "DialogManager/DialogManagerAPI.h"
#include <QPointer>

class LumaPlayer;

/** DialogManager面板参数，弱引用播放器，不延长其生命周期
*/
struct LumaPlayerMicrophoneParam : public CustomDialogParam
{
    /** 初始化LumaPlayerMicrophoneParam对象的状态，不转移输入借用对象的所有权
    @param [in] player 借用播放器，使用QPointer观察其销毁，不延长生命周期
    */
    explicit LumaPlayerMicrophoneParam(LumaPlayer* player);
    // 借用播放器的弱引用，不延长Qt对象生命周期
    QPointer<LumaPlayer> m_player;
};

/** DialogManager owns this tool window; it only borrows the player
*/
class LumaPlayerMicrophoneView : public CustomViewBase
{
public:
    /** 初始化LumaPlayerMicrophoneView对象的状态，不转移输入借用对象的所有权
    @param [in] player 借用播放器，使用QPointer观察其销毁，不延长生命周期
    */
    explicit LumaPlayerMicrophoneView(LumaPlayer* player);

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
    @param [in] watched 事件来源对象，借用至返回
    @param [in] event Qt事件，借用至事件处理返回，不保留指针
    @return true表示事件已拦截，false继续按基类规则处理
    */
    virtual bool eventFilter(QObject* watched, QEvent* event);

    /** 绘制当前控件内容，不执行长时间业务操作
    @param [in] event Qt事件，借用至事件处理返回，不保留指针
    */
    virtual void paintEvent(QPaintEvent* event);

    /** 处理控件鼠标按下并分发对应命令
    @param [in] event Qt事件，借用至事件处理返回，不保留指针
    */
    virtual void mousePressEvent(QMouseEvent* event);

    /** 处理控件双击，避免重复触发单击行为
    @param [in] event Qt事件，借用至事件处理返回，不保留指针
    */
    virtual void mouseDoubleClickEvent(QMouseEvent* event);

    /** 处理控件鼠标移动或滑轨拖动
    @param [in] event Qt事件，借用至事件处理返回，不保留指针
    */
    virtual void mouseMoveEvent(QMouseEvent* event);

    /** 完成控件鼠标释放交互
    @param [in] event Qt事件，借用至事件处理返回，不保留指针
    */
    virtual void mouseReleaseEvent(QMouseEvent* event);

    /** 处理控件键盘输入并保留相应悬停范围
    @param [in] event Qt事件，借用至事件处理返回，不保留指针
    */
    virtual void keyPressEvent(QKeyEvent* event);

private:
    // 借用播放器的弱引用，不延长Qt对象生命周期
    QPointer<LumaPlayer> m_player;
};

/** 向DialogManager创建麦克风内容，返回对象归管理器所有
*/
class LumaPlayerMicrophoneFactory : public CustomViewFactory
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