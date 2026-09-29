#pragma once
#include "Config.h"
#include "QtControls/ListWidget.h"
#include <QtWidgets/QStyledItemDelegate>
class QTimer;
class QPainter;
class QWheelEvent;

class PdfReaderThumbnailList : public ListWidget
{
    Q_OBJECT
public:
    /** 建立单列列表及长按拖动定时器
    @param [in] parent Qt父对象，可空，父子所有权由Qt管理
    */
    explicit PdfReaderThumbnailList(QWidget* parent = nullptr);

    /** 是否存在尚未释放的缩略图长按或拖动手势
    @return 按下源页到取消或释放期间返回true
    */
    bool gestureActive() const;
public slots:
    /** 停止长按及滚动定时器，清理蒙层和插入线；布局重建前也必须调用
    */
    void cancelDrag();

signals:
    /** 通知当前拖动提交的源页与目标页
    @param [in] fromRow 拖动源行
    @param [in] toRow 拖动目标行
    */
    void reordered(int fromRow, int toRow);

protected:
    /** 拖动期间将普通及Ctrl滚轮用于滚动，保留源页和插入位置
    @param [in] event Qt滚轮事件，同步借用
    */
    void wheelEvent(QWheelEvent* event) override;

    /** 列表滚动后刷新覆盖层，避免滚动复用旧像素留下或擦掉蓝线
    @param [in] dx 水平偏移，逻辑像素
    @param [in] dy 垂直偏移，逻辑像素
    */
    void scrollContentsBy(int dx, int dy) override;

    /** 记录单次点击或长按的唯一源项
    @param [in] event 当前Qt事件，调用期间借用
    */
    void mousePressEvent(QMouseEvent* event) override;

    /** 更新拖动目标，不产生第二个选择
    @param [in] event 当前Qt事件，调用期间借用
    */
    void mouseMoveEvent(QMouseEvent* event) override;

    /** 释放时提交有效重排并清理手势
    @param [in] event 当前Qt事件，调用期间借用
    */
    void mouseReleaseEvent(QMouseEvent* event) override;

    /** 绘制当前控件及必要交互标记
    @param [in] event 当前Qt事件，调用期间借用
    */
    void paintEvent(QPaintEvent* event) override;

    /** 处理Esc取消并转发其余按键
    @param [in] event 当前Qt事件，调用期间借用
    */
    void keyPressEvent(QKeyEvent* event) override;

    /** 失活或隐藏时收回拖动手势
    @param [in] event 当前Qt事件，调用期间借用
    @return 操作成功返回true；失败返回false
    */
    bool event(QEvent* event) override;

private slots:
    /** 按鼠标和可见条目更新插入间隙，滚动及鼠标移动共用
    */
    void updateDragTarget();

    /** 长按到期只进入拖动状态，不更改选择或页面顺序
    */
    void beginDrag();

    /** 拖动边缘滚动；插入位置由当前视图几何决定
    */
    void autoScrollDrag();

private:
    /** 返回鼠标对应的插入间隙，范围为 0 到 count
    */
    int insertionRow() const;

    // Qt 父子对象持有定时器；按下到释放期间源行保持不变
    QTimer* m_holdTimer;
    QTimer* m_scrollTimer;
    // 按下时的唯一源行，结束或模型重建前清除
    int m_sourceRow;
    // 蓝线与提交共用的插入间隙；初始等于源行，负数表示无有效横向落点
    int m_targetGap;
    // 高精度滚轮尚未累计到整像素的余量
    double m_wheelRemainder;
    // 已达到长按时限，允许蒙层及重排
    bool m_dragging;
    // 最近鼠标视口坐标，滚动后仍以此确定插入间隙
    QPoint m_dragPoint;
};