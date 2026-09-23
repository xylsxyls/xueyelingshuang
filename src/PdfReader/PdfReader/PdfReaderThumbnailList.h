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
    explicit PdfReaderThumbnailList(QWidget* parent = nullptr, const Config& config = Config());

signals:
    void reordered(int fromRow, int toRow);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    bool event(QEvent* event) override;

private slots:
    /** 长按到期只进入拖动状态，不更改选择或页面顺序 */
    void beginDrag();
    /** 拖动边缘滚动；插入位置由当前视图几何决定 */
    void autoScrollDrag();

private:
    /** 返回鼠标对应的插入间隙，范围为 0 到 count */
    int insertionRow() const;
    /** 结束或取消手势，释放定时器状态 */
    void cancelDrag();
    // Qt 父子对象持有定时器；按下到释放期间源行保持不变
    const Config m_config;
    QTimer* m_holdTimer;
    QTimer* m_scrollTimer;
    int m_sourceRow;
    bool m_dragging;
    QPoint m_dragPoint;
};