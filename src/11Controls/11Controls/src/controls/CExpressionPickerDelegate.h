#ifndef CEXPRESSIONPICKERDELEGATE_H
#define CEXPRESSIONPICKERDELEGATE_H

#include <QStyledItemDelegate>
#include "ControlsMacro.h"

class ControlsAPI CExpressionPickerDelegate : public QStyledItemDelegate
{
public:
    CExpressionPickerDelegate();
    ~CExpressionPickerDelegate();

    void  paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

#endif // CEXPRESSIONPICKERDELEGATE_H
