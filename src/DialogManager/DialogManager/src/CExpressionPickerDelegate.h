#ifndef CEXPRESSIONPICKERDELEGATE_H
#define CEXPRESSIONPICKERDELEGATE_H

#include <QStyledItemDelegate>

class CExpressionPickerDelegate : public QStyledItemDelegate
{
public:
    CExpressionPickerDelegate();
    ~CExpressionPickerDelegate();

    void  paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

#endif // CEXPRESSIONPICKERDELEGATE_H
