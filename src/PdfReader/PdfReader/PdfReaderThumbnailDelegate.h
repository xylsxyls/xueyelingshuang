#pragma once
#include "Config.h"
#include <QtWidgets/QListWidget>
#include <QtWidgets/QStyledItemDelegate>
class QTimer;
class QPainter;

class PdfReaderThumbnailDelegate : public QStyledItemDelegate
{
public:
    /** 建立不拥有模型的缩略图绘制委托
    @param [in] parent Qt父对象，可空，父子所有权由Qt管理
    */
    explicit PdfReaderThumbnailDelegate(QObject* parent = nullptr);

    /** 返回当前单列条目尺寸
    @param [in] option 当前条目布局和样式
    @param [in] index 从0开始的页码或插入点
    @return 当前状态或结果值；返回对象的所有权保持不变
    */
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

    /** 按条目实际中心绘制缩略图及编号
    @param [in] painter 借用当前绘制上下文
    @param [in] option 当前条目布局和样式
    @param [in] index 从0开始的页码或插入点
    */
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
};