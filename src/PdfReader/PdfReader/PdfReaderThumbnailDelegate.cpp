#include "PdfReaderThumbnailDelegate.h"
#include <QPainter>
#include <QStyle>

PdfReaderThumbnailDelegate::PdfReaderThumbnailDelegate(QObject* parent) :
QStyledItemDelegate(parent)
{

}

QSize PdfReaderThumbnailDelegate::sizeHint(const QStyleOptionViewItem& option,
                                           const QModelIndex& index) const
{
    const QVariant value = index.data(Qt::UserRole + 2);
    if (value.isValid())
    {
        return QSize(option.rect.width() > 0 ? option.rect.width() : g_config.m_thumbnailWidth + g_config.m_thumbnailSidePadding, value.toInt());
    }
    return QStyledItemDelegate::sizeHint(option, index);
}

void PdfReaderThumbnailDelegate::paint(QPainter* painter,
                                       const QStyleOptionViewItem& option,
                                       const QModelIndex& index) const
{
    if (painter == nullptr)
    {
        return;
    }

    const QRect rect = option.rect;
    const bool selected = (option.state & QStyle::State_Selected) != 0;
    painter->fillRect(rect, selected ? g_config.m_thumbnailSelectedColor : g_config.m_thumbnailBackground);

    const QSize pageSize = index.data(Qt::UserRole + 1).toSize();
    const int labelHeight = g_config.m_thumbnailLabelHeight;
    const int usableHeight = qMax(1, rect.height() - labelHeight);
    const int pageWidth = qMin(pageSize.width(), qMax(1, rect.width() - g_config.m_thumbnailImagePaddingX));
    const int pageHeight = qMin(pageSize.height(), qMax(1, usableHeight - g_config.m_thumbnailImagePaddingY));
    const QRect pageRect(rect.center().x() - pageWidth / 2,
                         rect.top() + (usableHeight - pageHeight) / 2,
                         pageWidth,
                         pageHeight);
    const QIcon icon = index.data(Qt::DecorationRole).value<QIcon>();
    const QPixmap pixmap = icon.pixmap(QSize(pageWidth, pageHeight), QIcon::Normal, QIcon::On);
    if (!pixmap.isNull())
    {
        painter->drawPixmap(pageRect, pixmap);
    }
    else
    {
        painter->fillRect(pageRect, Qt::white);
    }
    painter->setPen(selected ? g_config.m_selectionColor : g_config.m_thumbnailBorderColor);
    painter->drawRect(pageRect.adjusted(0, 0, -1, -1));

    painter->setPen(g_config.m_thumbnailTextColor);
    painter->drawText(QRect(rect.left(), rect.bottom() - labelHeight + 1,
                            rect.width(), labelHeight - 1),
                      Qt::AlignHCenter | Qt::AlignVCenter,
                      index.data(Qt::DisplayRole).toString());
}