#ifndef SPLITVIEWER_CANVAS_H
#define SPLITVIEWER_CANVAS_H

#include "QtControls/Widget.h"

class SplitViewer;

class SplitViewerCanvas : public Widget
{
public:
    /** 初始化对象及其默认状态。
    @param [in] owner 父区域或事件接收者
    @param [in] parent Qt宿主窗口
    */
    explicit SplitViewerCanvas(SplitViewer* owner, QWidget* parent = NULL);

protected:
    /** 窗口尺寸变化后刷新工作区布局。
    @param [in] event 输入事件
    */
    void resizeEvent(QResizeEvent* event);
    void paintEvent(QPaintEvent* event);
    void mousePressEvent(QMouseEvent* event);
    void mouseMoveEvent(QMouseEvent* event);
    void mouseReleaseEvent(QMouseEvent* event);
    void mouseDoubleClickEvent(QMouseEvent* event);
    void wheelEvent(QWheelEvent* event);
    void contextMenuEvent(QContextMenuEvent* event);
    void dragEnterEvent(QDragEnterEvent* event);
    void dropEvent(QDropEvent* event);

private:
    SplitViewer* m_owner;
};

#endif