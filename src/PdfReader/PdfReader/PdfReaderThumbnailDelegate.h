#pragma once
#include "Config.h"
#include <QtWidgets/QListWidget>
#include <QtWidgets/QStyledItemDelegate>
class QTimer;
class QPainter;

class PdfReaderThumbnailDelegate : public QStyledItemDelegate
{
public:
    explicit PdfReaderThumbnailDelegate(QObject* parent = nullptr, const Config& config = Config());

    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
private:
    const Config m_config;
};