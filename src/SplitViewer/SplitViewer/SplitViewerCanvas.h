#ifndef SPLITVIEWER_CANVAS_H
#define SPLITVIEWER_CANVAS_H

#include "QtControls/Widget.h"

class SplitViewer;
class QKeyEvent;

class SplitViewerCanvas : public Widget
{
public:
    /** 初始化对象及其默认状态
    @param [in] owner 父区域或事件接收者
    @param [in] parent Qt宿主窗口
    */
    explicit SplitViewerCanvas(SplitViewer* owner, QWidget* parent = nullptr);

protected:
    /** 窗口尺寸变化后刷新工作区布局
    @param [in] event 输入事件
    */
    void resizeEvent(QResizeEvent* event) override;

    /** 将画布绘制交给产品渲染入口
    @param [in] event Qt绘制事件
    */
    void paintEvent(QPaintEvent* event) override;

    /** 聚焦画布并开始命中交互
    @param [in] event Qt鼠标按下事件
    */
    void mousePressEvent(QMouseEvent* event) override;

    /** 更新产品拖动或悬停
    @param [in] event Qt鼠标移动事件
    */
    void mouseMoveEvent(QMouseEvent* event) override;

    /** 结束产品拖动
    @param [in] event Qt鼠标释放事件
    */
    void mouseReleaseEvent(QMouseEvent* event) override;

    /** 转交有效左键双击
    @param [in] event Qt双击事件
    */
    void mouseDoubleClickEvent(QMouseEvent* event) override;

    /** 转交画布图片缩放
    @param [in] event Qt滚轮事件
    */
    void wheelEvent(QWheelEvent* event) override;

    /** 处理图层删除，其余按键交回基类
    @param [in] event Qt按键事件
    */
    void keyPressEvent(QKeyEvent* event) override;

    /** 在命中位置显示操作菜单
    @param [in] event Qt上下文菜单事件
    */
    void contextMenuEvent(QContextMenuEvent* event) override;

    /** 接受可供产品打开的文件拖放
    @param [in] event Qt拖入事件
    */
    void dragEnterEvent(QDragEnterEvent* event) override;

    /** 把首个本地文件交给实际命中的分屏
    @param [in] event Qt释放拖放事件
    */
    void dropEvent(QDropEvent* event) override;

private:
    // 借用父级产品窗口，画布由该窗口的Qt对象树持有。
    SplitViewer* m_owner;
};

#endif