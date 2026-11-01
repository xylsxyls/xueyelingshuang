#pragma once
#include "Widget.h"
#include "WindowTitleBarConfig.h"
#include <QIcon>
#include <QPointer>

class FramelessMainWindow;
class Label;
class PushButton;
class QHBoxLayout;

/** 可插入业务动作的标题栏；窗口操作、图标及拖动供各产品复用。 */
class QtControlsAPI WindowTitleBar : public Widget
{
public:
    /** 创建标题及系统操作，父窗口拥有本控件。
    @param [in] host 对应的主窗口，不可空
    @param [in] config 已确认的样式/尺寸
    */
    WindowTitleBar(FramelessMainWindow* host, const WindowTitleBarConfig& config);
    /** 在系统按钮之前加入产品动作，Qt接管控件。
    @param [in] action 不可空，不在控件库解释其业务
    */
    void addActionWidget(QWidget* action);
protected:
    /** 非Windows后备标题拖动接纳。
    @param [in] event 鼠标按下事件
    */
    void mousePressEvent(QMouseEvent* event) override;
    /** 后备移动保留全局鼠标与原窗口偏移。
    @param [in] event 鼠标移动事件
    */
    void mouseMoveEvent(QMouseEvent* event) override;
    /** 结束移动手势。
    @param [in] event 鼠标释放事件
    */
    void mouseReleaseEvent(QMouseEvent* event) override;
    /** 标题双击切换最大化，操作按钮不进入此入口。
    @param [in] event 双击事件
    */
    void mouseDoubleClickEvent(QMouseEvent* event) override;
private:
    /** 绘制简单的窗口操作线图标。
    @param [in] kind 0最小化、1最大化、2关闭
    @return 透明背景图标
    */
    QIcon icon(int kind) const;
    /** 创建标准窗口按钮，业务按钮由调用方提供。
    @param [in] kind 操作图标种类
    @return 由本标题栏持有的按钮
    */
    PushButton* makeButton(int kind);
    // 父窗口持有本标题，指针仅借用且自动置空。
    QPointer<FramelessMainWindow> m_host;
    // 样式快照，不能引用应用全局Config。
    WindowTitleBarConfig m_config;
    // 父子关系拥有，业务动作插入此布局。
    QHBoxLayout* m_actions;
    // 纯GUI拖动状态，不作为配置。
    bool m_dragging;
    QPoint m_dragOffset;
};