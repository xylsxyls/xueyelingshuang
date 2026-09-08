#include "LockedAccountPanel.h"
#include <QPainter>
#include "QtControls/ControlStyleManager.h"
#include "QtControls/CTreeViewEx.h"
#include <QHeaderView>
#include "DialogHelper.h"

LockedAccountItem::LockedAccountItem()
{
    setSizeHint(QSize(0, 30));
}

void LockedAccountItem::setStartDate(const QString& startDate)
{
    setData(startDate, Role_StartDate);
}

QString LockedAccountItem::startDate()
{
    return data(Role_StartDate).toString();
}

void LockedAccountItem::setReasion(const QString& reasion)
{
    setData(reasion, Role_Reasion);
}

QString LockedAccountItem::reasion()
{
    return data(Role_Reasion).toString();
}

void LockedAccountItem::setDayCount(quint64 dayCount)
{
    setData(dayCount, Role_DayCount);
}

quint64 LockedAccountItem::dayCount()
{
    return data(Role_DayCount).toULongLong();
}

LockedAccountPanel::LockedAccountPanel(QWidget* parent)
    :QWidget(parent)
    ,m_treeView(new CTreeViewEx(this))
    ,m_model(new QStandardItemModel(this))
    ,m_name(QStringLiteral("被封禁账号列表"))
{
    m_treeView->setModel(m_model);
    m_treeView->setEditTriggers(CTreeViewEx::NoEditTriggers);
    m_treeView->setIndentation(0);
    LockedAccountItemDelegate* tDelegate = new LockedAccountItemDelegate(m_treeView);
    m_treeView->setItemDelegate(tDelegate);
    m_treeView->header()->setDefaultAlignment(Qt::AlignCenter);
    m_treeView->header()->setStyleSheet(QStringLiteral("QHeaderView{border-top:1px solid #4a5980;}") +
                                       QStringLiteral("QHeaderView::section{background-color:#36415f;height:28px; border:none; text-align: center;font-style: 14px '微软雅黑'; color:#899ac7}"));

    QStringList labels;
    labels << QStringLiteral("封号时间") << QStringLiteral("封号原因") << QStringLiteral("封号天数");
    m_model->setHorizontalHeaderLabels(labels);

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

void LockedAccountPanel::paintEvent(QPaintEvent* eve)
{
	if (eve == nullptr)
	{
		return;
	}
    QWidget::paintEvent(eve);
    QPainter p(this);
    p.save();
    p.fillRect(rect(), "#2c344a");

    QRect nameRect = rect().adjusted(0,14,0,0);
    QFont nameFont = ControlStyleManager::instance().defaultFont();
    nameFont.setPixelSize(18);

    QColor nameColor = "#FFFFFF";
    p.setFont(nameFont);
    p.setPen(nameColor);

    p.drawText(DialogHelper::rectValid(nameRect), Qt::AlignTop|Qt::AlignHCenter, m_name);
    p.restore();
}

void LockedAccountPanel::resizeEvent(QResizeEvent* eve)
{
	if (eve == nullptr)
	{
		return;
	}
    QWidget::resizeEvent(eve);
    this->layoutControls();
}

void LockedAccountPanel::layoutControls()
{
	if (m_treeView == nullptr)
	{
		return;
	}
    m_treeView->setGeometry(QRect(QPoint(7,50), QPoint(this->width() - 7, this->height() - 7)));
}

void LockedAccountPanel::setName(const QString& name)
{
    m_name = name;
    this->update();
}

QString LockedAccountPanel::name()
{
    return m_name;
}

void LockedAccountPanel::setLockedAccountItemList(const LockedAccountItemList& li)
{
	if (m_model == nullptr || m_treeView == nullptr || m_treeView->header() == nullptr)
	{
		return;
	}
    m_model->clear();
    QStringList labels;
    labels << QStringLiteral("封号时间") << QStringLiteral("封号原因") << QStringLiteral("封号天数");
    m_model->setHorizontalHeaderLabels(labels);

    for(int i = 0; i < li.count(); i++)
    {
		if (li[i] != nullptr)
		{
			m_model->appendRow(li[i]);
		}
    }

    m_treeView->header()->resizeSection(0,170);
    m_treeView->header()->resizeSection(1,110);
    m_treeView->header()->resizeSection(2,60);
}

void LockedAccountPanel::appendLockedAccountItem(LockedAccountItem* item)
{
	if (m_model == nullptr || item == nullptr)
	{
		return;
	}
	m_model->appendRow(item);
}

void LockedAccountPanel::clearLockedAccountItem()
{
	if (m_model == nullptr || m_treeView == nullptr || m_treeView->header() == nullptr)
	{
		return;
	}
	m_model->clear();

	QStringList labels;
	labels << QStringLiteral("封号时间") << QStringLiteral("封号原因") << QStringLiteral("封号天数");
	m_model->setHorizontalHeaderLabels(labels);

	m_treeView->header()->resizeSection(0,170);
	m_treeView->header()->resizeSection(1,110);
	m_treeView->header()->resizeSection(2,60);
}

LockedAccountItemDelegate::LockedAccountItemDelegate(QObject* parent)
    :QStyledItemDelegate(parent)
{

}

void LockedAccountItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	if (painter == nullptr || !index.isValid())
	{
		return;
	}
    CTreeViewEx* view = qobject_cast<CTreeViewEx*>(parent());
    if(view == nullptr)
    {
        return;
    }

    QStandardItemModel* model = qobject_cast<QStandardItemModel*>(view->model());
	if (model == nullptr)
	{
		return;
	}
    LockedAccountItem* litem = dynamic_cast<LockedAccountItem*>(model->item(index.row(),0));
    if (litem == nullptr)
    {
        return;
    }

    QFont tf = ControlStyleManager::instance().defaultFont();
    painter->setFont(tf);

    QModelIndex i0 = index.sibling(index.row(),0);
    QModelIndex i1 = index.sibling(index.row(),1);
    QModelIndex i2 = index.sibling(index.row(),2);

    QRect rt0 = view->visualRect(i0);
    QRect rt1 = view->visualRect(i1);
    QRect rt2 = view->visualRect(i2);

    QRect rowRect(rt0.topLeft(), rt2.bottomRight());

	QFontMetrics fm = painter->fontMetrics();
	QString startDateString = fm.elidedText(litem->startDate(), Qt::ElideRight,rt0.adjusted(7,0,0,0).width());
	QString reasionString = fm.elidedText(litem->reasion(), Qt::ElideRight,rt1.width());
	QString dayCountString = fm.elidedText(QString::number(litem->dayCount()), Qt::ElideRight,rt2.width());

    painter->setPen("#ffffff");
    painter->drawText(DialogHelper::rectValid(rt0.adjusted(7, 0, 0, 0)), Qt::AlignLeft | Qt::AlignVCenter, startDateString);
    painter->setPen("#fd874c");
    painter->drawText(DialogHelper::rectValid(rt1), Qt::AlignCenter, reasionString);
    painter->setPen("#ffffff");
    painter->drawText(DialogHelper::rectValid(rt2), Qt::AlignCenter, dayCountString);
    painter->setPen("#4a5980");

    if (rowRect.left() >= 1)
    {
        QLine line(rowRect.bottomLeft() + QPoint(0, -1), rowRect.bottomRight() + QPoint(0, -1));
        painter->drawLine(line);
    }
}