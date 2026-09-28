#pragma once
#include "Config.h"
#include "QtControls/ListWidget.h"
#include <QtWidgets/QStyledItemDelegate>
class QTimer;
class QPainter;

class PdfReaderThumbnailList : public ListWidget
{
    Q_OBJECT
public:
    /** 建立单列列表及长按拖动定时器
    @param [in] parent Qt父对象，可空，父子所有权由Qt管理
    */
    explicit PdfReaderThumbnailList(QWidget* parent = nullptr);

signals:
    /** 通知当前拖动提交的源页与目标页
    @param [in] fromRow 拖动源行
    @param [in] toRow 拖动目标行
    */
    void reordered(int fromRow, int toRow);

protected:
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

    /** 结束或取消手势，释放定时器状态
    */
    void cancelDrag();
    // Qt 父子对象持有定时器；按下到释放期间源行保持不变
    QTimer* m_holdTimer;
    QTimer* m_scrollTimer;
    int m_sourceRow;
    bool m_dragging;
    QPoint m_dragPoint;
};