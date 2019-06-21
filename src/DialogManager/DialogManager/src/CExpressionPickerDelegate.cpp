#include "CExpressionPickerDelegate.h"
#include <QPainter>
#include <QDebug>
#include "CExpressionPicker.h"
#include "DialogHelper.h"

CExpressionPickerDelegate::CExpressionPickerDelegate()
{

}

CExpressionPickerDelegate::~CExpressionPickerDelegate()
{

}

void CExpressionPickerDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    painter->save();

    QColor borderColor("#4A5677");
    QColor hoverdBorderColor("#3187DD");
    QColor bkgColor("#2C344A");


    //fill hoverd rect
    QColor hoveredColor = borderColor;
    if(option.state.testFlag(QStyle::State_MouseOver))
    {
        hoveredColor = hoverdBorderColor;
    }
    else
    {
        hoveredColor = borderColor;
    }

    painter->fillRect(DialogHelper::rectValid(option.rect), hoveredColor);


    //fill bkg
    CExpressionPicker* picker = qobject_cast<CExpressionPicker*>(this->parent());
    QRect bkgRect;
    if(picker->isShowIconRect())
    {
        bkgRect  = option.rect.adjusted(1,1,-1,-1);
    }
    else
    {
        bkgRect = option.rect;
    }

    painter->fillRect(DialogHelper::rectValid(bkgRect), bkgColor);

    QString icon = index.data(CExpressionPicker::ExpressionRole_FileName).toString();
    QPixmap pix;
    pix.load(icon);
    painter->drawPixmap(bkgRect.adjusted(1,1,-1,-1), pix);

    painter->restore();
}

