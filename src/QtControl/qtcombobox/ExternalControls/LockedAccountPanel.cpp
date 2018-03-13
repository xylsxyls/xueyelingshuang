#include "LockedAccountPanel.h"

#include <QPainter>
#include "../CGeneralStyle.h"
#include "../CTreeViewEx.h"
#include <QHeaderView>

LockedAccountPanel::LockedAccountPanel(QWidget *parent)
    :QWidget(parent)
    ,mName(QStringLiteral("一二三四五六七"))
    ,mTreeView(new CTreeViewEx(this))
    ,mModel(new QStandardItemModel(this))
{
    mTreeView->setModel(mModel);
    mTreeView->setEditTriggers(CTreeViewEx::NoEditTriggers);
    mTreeView->setIndentation(0);
    LockedAccountItemDelegate* tDelegate = new LockedAccountItemDelegate(mTreeView);
    mTreeView->setItemDelegate(tDelegate);
    mTreeView->header()->setDefaultAlignment(Qt::AlignCenter);
    mTreeView->header()->setStyleSheet(QStringLiteral("QHeaderView{border-top:1px solid #4a5980;}") +
                                       QStringLiteral("QHeaderView::section{background-color:#36415f;height:28px; border:none; text-align: center;font-style: 14px '微软雅黑'; color:#899ac7}"));


    QStringList labels;
    labels << QStringLiteral("封号时间") << QStringLiteral("封号原因") << QStringLiteral("封号天数");
    mModel->setHorizontalHeaderLabels(labels);

    LockedAccountItemList li;
    for(int i = 0; i < 10; i++)
    {
        LockedAccountItem* litem = new LockedAccountItem;
        litem->setStartDate("2019-03-07 11:11:11");
        litem->setReasion  (QStringLiteral("自己爆炸了"));
        litem->setDayCount (365);
        li << litem;
    }

    this->setLockedAccountItemList(li);
}

void LockedAccountPanel::paintEvent(QPaintEvent *e)
{
    QPainter p(this);
    p.fillRect(rect(), "#2c344a");

    QRect nameRect = rect().adjusted(0,14,0,0);
    QFont nameFont = CGeneralStyle::instance()->font();
    nameFont.setPixelSize(18);

    QColor nameColor = "#FFFFFF";
    p.setFont(nameFont);
    p.setPen(nameColor);

    p.drawText(nameRect, Qt::AlignTop|Qt::AlignHCenter, mName);
}

void LockedAccountPanel::resizeEvent(QResizeEvent *e)
{
    QWidget::resizeEvent(e);
    this->layoutControls();
}

void LockedAccountPanel::layoutControls()
{
    mTreeView->setGeometry(QRect(QPoint(7,50), QPoint(this->width() - 7, this->height() - 7)));
}

void LockedAccountPanel::setName(const QString &s)
{
    mName = s;
    this->update();
}

QString LockedAccountPanel::name()
{
    return mName;
}

void LockedAccountPanel::setLockedAccountItemList(const LockedAccountItemList &li)
{
    mModel->clear();
    QStringList labels;
    labels << QStringLiteral("封号时间") << QStringLiteral("封号原因") << QStringLiteral("封号天数");
    mModel->setHorizontalHeaderLabels(labels);

    for(int i = 0; i < li.count(); i++)
    {
        mModel->appendRow(li[i]);
    }

    mTreeView->header()->resizeSection(0,170);
    mTreeView->header()->resizeSection(1,110);
    mTreeView->header()->resizeSection(2,60);
}

LockedAccountItemDelegate::LockedAccountItemDelegate(QObject *parent)
    :QStyledItemDelegate(parent)
{

}

void LockedAccountItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    CTreeViewEx* view = qobject_cast<CTreeViewEx*>(parent());
    if(view == NULL)
        return;

    QStandardItemModel* model = (QStandardItemModel*)(view->model());
    LockedAccountItem* litem = (LockedAccountItem*)(model->item(index.row(),0));

    QFont tf = CGeneralStyle::instance()->font();
    painter->setFont(tf);

    QModelIndex i0 = index.sibling(index.row(),0);
    QModelIndex i1 = index.sibling(index.row(),1);
    QModelIndex i2 = index.sibling(index.row(),2);

    QRect rt0 = view->visualRect(i0);
    QRect rt1 = view->visualRect(i1);
    QRect rt2 = view->visualRect(i2);

    QRect rowRect(rt0.topLeft(), rt2.bottomRight());

    painter->setPen("#ffffff");
    painter->drawText(rt0.adjusted(7,0,0,0), Qt::AlignLeft|Qt::AlignVCenter, litem->startDate());
    painter->setPen("#fd874c");
    painter->drawText(rt1, Qt::AlignCenter, litem->reasion());
    painter->setPen("#ffffff");
    painter->drawText(rt2, Qt::AlignCenter, QString::number(litem->dayCount()));
    painter->setPen("#4a5980");

    QLine line(rowRect.bottomLeft() + QPoint(0,-1), rowRect.bottomRight() + QPoint(0,-1));
    painter->drawLine(line);
}
