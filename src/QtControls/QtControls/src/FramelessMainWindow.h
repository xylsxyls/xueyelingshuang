#pragma once
#include "MainWindow.h"
#include <QPointer>
#include <QPoint>
#include <QRect>

class Widget;

/** 可选自定义标题主窗口；扩展新类型，保留原MainWindow对象布局及默认行为。 */
class QtControlsAPI FramelessMainWindow : public MainWindow
{
public:
    /** 创建无原生标题的窗口，标题控件由调用方提供。
    @param [in] parent Qt拥有者
    */
    explicit FramelessMainWindow(QWidget* parent = nullptr);
    /** 安装标题和边缘命中范围，控件所有权交QMainWindow。
    @param [in] titleBar 标题控件，不可空
    @param [in] resizeBorder 逻辑像素，1至16
    */
    void setTitleBar(Widget* titleBar, int resizeBorder);
protected:
    /** Windows原生移动/缩放与工作区最大化。
    @param [in] eventType Qt平台事件类型
    @param [in] message 原生消息，按平台宏解释
    @param [out] result 原生返回值
    @return 是否已处理
    */
    bool nativeEvent(const QByteArray& eventType, void* message, long* result) override;
    /** 显示时保留Windows系统缩放/窗口操作能力。
    @param [in] event 显示事件
    */
    void showEvent(QShowEvent* event) override;
    /** 全屏时收起标题和外边距。
    @param [in] event 窗口状态事件
    */
    void changeEvent(QEvent* event) override;
    /** 非Windows边缘缩放的鼠标接纳。
    @param [in] event 鼠标事件
    */
    void mousePressEvent(QMouseEvent* event) override;
    /** 更新边缘提示和有约束缩放。
    @param [in] event 鼠标事件
    */
    void mouseMoveEvent(QMouseEvent* event) override;
    /** 结束边缘手势。
    @param [in] event 鼠标事件
    */
    void mouseReleaseEvent(QMouseEvent* event) override;
private:
    /** 计算左/上/右/下边缘位掩码。
    @param [in] point 客户区逻辑坐标
    @return 1左、2上、4右、8下；0不缩放
    */
    int edges(const QPoint& point) const;
    // 由Qt父子关系持有，QPointer避免标题被替换后悬空。
    QPointer<Widget> m_titleBar;
    // 当前窗口的边缘命中宽度。
    int m_resizeBorder;
    // 跨平台后备缩放状态，只在GUI线程访问。
    int m_resizeEdges;
    QPoint m_resizeStart;
    QRect m_resizeGeometry;
};