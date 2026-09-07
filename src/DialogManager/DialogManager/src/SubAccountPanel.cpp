#include "SubAccountPanel.h"
#include <QEvent>
#include <QHeaderView>
#include <QPainter>
#include "QtControls/ControlStyleManager.h"
#include "QtControls/COriginalButton.h"
#include "QtControls/CTreeViewEx.h"
#include "QtControls/CExternalTextEdit.h"
#include <QWindow>
#include "DialogHelper.h"

SubAccountItem::SubAccountItem()
{
    this->setSizeHint(QSize(0, 94));
    this->setLocked(false);
    this->setId(0);
    this->setIsCurrent(false);
}

void SubAccountItem::setName(const QString& name)
{
    setData(name, Role_Name);
}

QString SubAccountItem::name()
{
    return data(Role_Name).toString();
}

void SubAccountItem::setDate(const QString& date)
{
    setData(date, Role_Date);
}

QString SubAccountItem::date()
{
    return data(Role_Date).toString();
}

void SubAccountItem::setDesc(const QString& desc)
{
    setData(desc, Role_Desc);
}

QString SubAccountItem::desc()
{
    return data(Role_Desc).toString();
}

void SubAccountItem::setId(quint64 id)
{
    setData(id, Role_Id);
}

quint64 SubAccountItem::id()
{
    return data(Role_Id).toULongLong();
}

void SubAccountItem::setLocked(bool locked)
{
    setData(locked, Role_Locked);
}

bool SubAccountItem::locked()
{
    return data(Role_Locked).toBool();
}

void SubAccountItem::setIsCurrent(bool current)
{
    setData(current, Role_IsCurrent);
}

bool SubAccountItem::isCurrent()
{
    return data(Role_IsCurrent).toBool();
}

void SubAccountItem::setLockedTime(const QString& lockedTime)
{
    setData(lockedTime, Role_LockedTime);
}

QString SubAccountItem::lockedTime()
{
    return data(Role_LockedTime).toString();
}

void SubAccountItem::setLockedReasion(const QString& lockedReasion)
{
    setData(lockedReasion, Role_LockedReasion);
}

QString SubAccountItem::lockedReasion()
{
    return data(Role_LockedReasion).toString();
}

void SubAccountItem::setLockedDayCount(quint64 lockedDayCount)
{
    setData(lockedDayCount, Role_LockedDayCount);
}

quint64 SubAccountItem::lockedDayCount()
{
    return data(Role_LockedDayCount).toULongLong();
}

void SubAccountItem::setCanRename(bool canRename)
{
    setData(canRename, Role_CanRename);
}

bool SubAccountItem::canRename()
{
    return data(Role_CanRename).toBool();
}

SubAccountPanel::SubAccountPanel(QWidget* parent)
    :QWidget(parent)
    ,m_treeView(new CTreeViewEx(this))
    ,m_model(new QStandardItemModel(this))
    ,m_createSubAccountButton(new COriginalButton(this))
    ,m_helpButton(new COriginalButton(this))
    ,m_helpTip(new CExternalTextEdit)
    ,m_canCreateCount(0)
{
    m_treeView->setModel(m_model);
    m_treeView->setIndentation(0);
    m_treeView->header()->setVisible(false);
    m_treeView->setEditTriggers(CTreeViewEx::NoEditTriggers);
	m_treeView->setSelectionMode(CTreeViewEx::NoSelection);
    SubAccountItemDelegate* tDelegate = new SubAccountItemDelegate(m_treeView);
    m_treeView->setItemDelegate(tDelegate);


    m_createSubAccountButton->setText("");
    m_createSubAccountButton->setFixedSize(88,21);
    m_createSubAccountButton->setBkgImage(ControlStyleManager::instance().resourcePath("Image/SubAccount/create_subaccount_button.png"),
                                         4,1,2,3,4,1,2,3,4);
	connect(m_createSubAccountButton, &COriginalButton::clicked, this, &SubAccountPanel::createSubAccount);

    m_helpButton->setUnderline(true);
    m_helpButton->setFixedSize(80,21);
    m_helpButton->setText(QStringLiteral("[如何增加]"));
    m_helpButton->setFontColor(QColor(255,180,120),"white", "white",QColor(255,180,120),
                              QColor(255,180,120),"white", "white",QColor(255,180,120));

    m_helpButton->installEventFilter(this);
    this->installEventFilter(this);
    this->setMouseTracking(true);

    m_helpTip->setWindowFlags(m_helpTip->windowFlags() | Qt::FramelessWindowHint | Qt::Tool | Qt::WindowDoesNotAcceptFocus);




    m_helpTip->setStyleSheet(QStringLiteral("background-color:rgba(29,29,47);border: 1px solid rgba(132,142,168); font-size:14px; font-family:'微软雅黑';color:#abb3d3;"));
    m_helpTip->setText(QStringLiteral("1. 新建账户默认有一个账号<br><br>"                              )+
                      QStringLiteral("2. 允许立即创建第二个子账号<br><br>"                            )+
                      QStringLiteral("3. 平台等级达到5级，允许创建第三个子账号<br><br>"                 )+
                      QStringLiteral("4. 平台等级达到10级，允许创建第四个子账号<br><br>"                )+
                      QStringLiteral("5. 平台等级达到12级，允许创建第五个子账号<br><br>"                )+
                      QStringLiteral("6. <font color='#f33838'>平台VIP</font>可随时额外创建一个子账号"));


    SubAccountItemList li;
    for(int i = 0; i < 5; i++)
    {
        SubAccountItem* sitem = new SubAccountItem;
        sitem->setName(QStringLiteral("一二三四五六七"));
        sitem->setDate(QStringLiteral("登录时间:2018.03.07"));
        sitem->setDesc(QStringLiteral("DotaA天梯:68%胜率"));
        sitem->setId(i);
		li << sitem;
		if(i == 0 )
		{
			sitem->setIsCurrent(true);
		}
		else
		{
			sitem->setIsCurrent(false);
		}
        if(i > 3)
        {
            sitem->setLocked(true);
        }
    }

    this->setSubAccountList(li);
}

SubAccountPanel::~SubAccountPanel()
{

}

void SubAccountPanel::moveEvent(QMoveEvent* eve)
{
    QWidget::moveEvent(eve);
    this->layoutControls();
}

void SubAccountPanel::layoutControls()
{
    m_createSubAccountButton->move(this->width() - 38 - m_createSubAccountButton->width(),
                                  14);

    m_helpButton->move(this->width() - 24 - m_helpButton->width() - 6,56 - 6);

    m_treeView->setGeometry(QRect(8,90, this->width() - 16, this->height() - 90 - 8));

    m_helpTip->setGeometry(QRect(this->mapToGlobal(m_treeView->geometry().topLeft()),
                                this->mapToGlobal(m_treeView->geometry().bottomRight() + QPoint(1,1))));
}

void SubAccountPanel::resizeEvent(QResizeEvent* eve)
{
    QWidget::resizeEvent(eve);
    this->layoutControls();
}

void SubAccountPanel::paintEvent(QPaintEvent* eve)
{
    QWidget::paintEvent(eve);
    QPainter p(this);
    p.save();
    p.fillRect(rect(), "#2c344a");

    QRect fucRect(8, 45, width() - 16 < 0 ? 0 : (width() - 16), 32);
    p.setPen("#4a5980");
    p.drawRect(DialogHelper::rectValid(fucRect));

    QFont tf = ControlStyleManager::instance().defaultFont();
    p.setPen(QColor(192,200,218));
    p.setFont(tf);
    p.drawText(DialogHelper::rectValid(fucRect.adjusted(21, 0, 0, 0)), Qt::AlignVCenter | Qt::AlignLeft, QStringLiteral("可创建的子账号数："));


    QRect descRect(29,0,this->width(),45);
    p.setPen("#ffffff");
    QFont tf1 = ControlStyleManager::instance().defaultFont();
    tf1.setPixelSize(16);
    p.setFont(tf1);

    p.drawText(DialogHelper::rectValid(descRect), Qt::AlignVCenter | Qt::AlignLeft, QStringLiteral("子账号管理"));

    QRect countRect = fucRect;
    countRect.setRight(m_helpButton->geometry().left() - 10);
    p.drawText(DialogHelper::rectValid(countRect), Qt::AlignVCenter | Qt::AlignRight, QString::number(m_canCreateCount));

    p.restore();
    //p.drawRect(m_treeView->geometry());
}

bool SubAccountPanel::eventFilter(QObject* obj, QEvent* eve)
{
    bool res = QWidget::eventFilter(obj, eve);
	if (obj == nullptr || eve == nullptr)
	{
		return res;
	}

    if(obj == this)
    {
        if(eve->type() == QEvent::Enter || eve->type() == QEvent::Move)
        {
            m_helpTip->close();
        }
    }
    else if(obj == m_helpButton)
    {
        if(eve->type() == QEvent::Enter)
        {
			this->layoutControls();
            m_helpTip->show();
        }
        else if(eve->type() == QEvent::Leave)
        {
            m_helpTip->close();
        }
    }

    return res;
}

QList<QStandardItem *> SubAccountPanel::subAccountItemLiToStandardItemLi(const SubAccountItemList &li)
{
    QList<QStandardItem *> tli;
    for(int i = 0; i < li.count(); i++)
    {
        tli << (QStandardItem*)(li[i]);
    }

    return tli;
}

SubAccountItemList SubAccountPanel::standardItemLiToSubAccountItemLi(const QList<QStandardItem *> &li)
{
    SubAccountItemList tli;
    for(int i = 0; i < li.count(); i++)
    {
        tli << (SubAccountItem*)(li[i]);
    }
    return tli;
}

CExternalTextEdit* SubAccountPanel::helpTip()
{
	return m_helpTip;
}

quint64 SubAccountPanel::canCreateCount()
{
	return m_canCreateCount;
}

void SubAccountPanel::setCanCreateCount(quint64 count)
{
    m_canCreateCount = count;
    this->update();
}

void SubAccountPanel::setSubAccountList(const SubAccountItemList &li)
{
    m_model->clear();
    m_model->appendColumn(this->subAccountItemLiToStandardItemLi(li));
    for(int i = 0; i < m_model->rowCount(); i++)
    {
        m_treeView->openPersistentEditor(m_model->index(i,0));
    }

	//	m_model->setSortRole(SubAccountItem::Role_IsCurrent);
	//	m_model->sort(0, Qt::DescendingOrder);
}


SubAccountItemList SubAccountPanel::subAccountList()
{
	SubAccountItemList li;
	for(int i = 0; i < m_model->rowCount(); i++)
	{
		li << (SubAccountItem*)(m_model->item(i,0));
	}

	return li;
}

void SubAccountPanel::appendSubAccount(SubAccountItem* item)
{
	m_model->appendRow(item);
	m_treeView->openPersistentEditor(item->index());
}

SubAccountItem* SubAccountPanel::getSubAccountItemById(quint64 id)
{
	SubAccountItem* res = nullptr;
	SubAccountItemList allLi = subAccountList();
	for(int i = 0; i < allLi.count(); i++)
	{
		SubAccountItem* t = allLi[i];
		if(t->id() == id)
		{
			return t;
		}
	}

	return res;
}

void SubAccountPanel::removeSubAccountItem(quint64 id)
{
	SubAccountItem* sitem = getSubAccountItemById(id);
	if(sitem)
	{
		m_model->removeRow(sitem->row());
	}
}

SubAccountItemView::SubAccountItemView(QWidget* parent)
    :QWidget(parent)
    ,m_switchButton(new COriginalButton(this))
	,m_helpButton(new COriginalButton(this))
	,m_changeNameButton(new COriginalButton(this))
	,m_blocked(false)
    ,m_id(0)
{
	setBlocked(true);
    m_switchButton->setText("");
    m_switchButton->resize(51, 21);
    m_switchButton->setBkgImage(ControlStyleManager::instance().resourcePath("Image/SubAccount/switch_subaccount_button.png"),
                               4,1,2,3,4,1,2,3,4);

    connect(m_switchButton, &COriginalButton::clicked,
            this, &SubAccountItemView::onSwitchButtonClicked);


	m_helpButton->setText("");
	m_helpButton->resize(19,19);
	m_helpButton->setBkgImage(ControlStyleManager::instance().resourcePath("Image/SubAccount/help_button.png"),
		3,1,2,3,3,1,2,3,3);

	connect(m_helpButton, &COriginalButton::clicked, this, &SubAccountItemView::helpButtonClicked);

	m_changeNameButton->setText(QStringLiteral("改名"));
	m_changeNameButton->resize(51,21);
	m_changeNameButton->setBorderWidth(1);
	m_changeNameButton->setBorderStyle("solid");
	m_changeNameButton->setBorderColor(QColor(202,212,248),QColor(0,248,255), QColor(44,52,74), QColor(180,180,181));
	m_changeNameButton->setFontColor(QColor(202,212,248),QColor(0,248,255), QColor(44,52,74), QColor(180,180,181),QColor(202,212,248),QColor(0,248,255), QColor(44,52,74), QColor(180,180,181));

	connect(m_changeNameButton, &COriginalButton::clicked, this, &SubAccountItemView::onChangeNameButtonClicked);
}

void SubAccountItemView::resizeEvent(QResizeEvent* eve)
{
    QWidget::resizeEvent(eve);
    this->layoutControls();
}

void SubAccountItemView::layoutControls()
{
    m_switchButton->move(this->width() - m_switchButton->width() - 22, 17);
	m_changeNameButton->move(m_switchButton->x(), m_switchButton->y() + m_switchButton->height() + 10);
	m_helpButton->move(this->width() - m_helpButton->width() - 2, 2);
}

void SubAccountItemView::onSwitchButtonClicked()
{
    emit switchAccount(m_id);
}

void SubAccountItemView::onChangeNameButtonClicked()
{
	emit renameAccount(m_id);
}

void SubAccountItemView::setId(quint64 id)
{
    m_id = id;
}

void SubAccountItemView::showSwitchButton(bool show)
{
	m_switchButton->setVisible(show);
	//m_changeNameButton->setVisible(show);
}

void SubAccountItemView::setBlocked(bool blocked)
{
	m_blocked = blocked;
	m_switchButton->setEnabled(!blocked);
	m_changeNameButton->setEnabled(!blocked);
	m_helpButton->setVisible(blocked);

	this->update();
}

SubAccountItemDelegate::SubAccountItemDelegate(QObject* parent)
    :QStyledItemDelegate(parent)
{

}

void SubAccountItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    if (painter == nullptr)
    {
        return;
    }
    CTreeViewEx* view = qobject_cast<CTreeViewEx*> (parent());
    if(view == nullptr)
    {
        return ;
    }

    QStandardItemModel* model = (QStandardItemModel*)(view->model());
    SubAccountItem* sitem = (SubAccountItem*)( model->item(index.row(),0));
    if(sitem == nullptr)
    {
        return;
    }

    QColor bkgColor;
    QFont  tf  = ControlStyleManager::instance().defaultFont();
    QColor tc1 = "#ffffff";
    QColor tc2 = "#b3bfdc";
    QColor tc3 = "#efefef";

    int useTC = 1;
    if(option.state.testFlag(QStyle::State_Selected) || option.state.testFlag(QStyle::State_MouseOver) || sitem->isCurrent())
    {
        bkgColor = "#687fba";
        useTC = 1;
    }
    else
    {
        bkgColor = "#4a5980";
        useTC = 2;
    }

    if(sitem->locked())
    {
        bkgColor = "#777d8c";
        useTC = 3;
    }

    QRect tagRect = option.rect.adjusted(0,0,0,-10);

    painter->fillRect(DialogHelper::rectValid(tagRect), bkgColor);

    QRect nameRect = tagRect.adjusted(18,13,0,0);
    painter->setFont(tf);
    if(useTC != 3)
    {
        painter->setPen(tc3);
    }
    else
    {
        painter->setPen(tc1);
    }

    painter->drawText(DialogHelper::rectValid(nameRect), Qt::AlignLeft | Qt::AlignTop, sitem->name());

    QRect dateRect = tagRect.adjusted(18,36,0,0);
    QRect descRect = tagRect.adjusted(18,58,0,0);
    switch(useTC)
    {
    case 1:
    {
        painter->setPen(tc1);
        break;
    }
    case 2:
    {
        painter->setPen(tc2);
        break;
    }
    case 3:
    {
        painter->setPen(tc3);
        break;
    }
    default:
    {
        break;
    }
    }

    painter->drawText(DialogHelper::rectValid(dateRect), Qt::AlignLeft | Qt::AlignTop, sitem->date());
    painter->drawText(DialogHelper::rectValid(descRect), Qt::AlignLeft | Qt::AlignTop, sitem->desc());
}

QWidget* SubAccountItemDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	CTreeViewEx* view = qobject_cast<CTreeViewEx*> (this->parent());
	if(view == nullptr)
	{
		return QStyledItemDelegate::createEditor(parent, option, index);
	}

	SubAccountPanel* panel = qobject_cast<SubAccountPanel*> (view->parent());
	if(panel == nullptr)
	{
		return QStyledItemDelegate::createEditor(parent, option, index);
	}

    SubAccountItemView* itemView = new SubAccountItemView(parent);
	connect(itemView, &SubAccountItemView::switchAccount, panel, &SubAccountPanel::siwtchSubAccount);
	connect(itemView, &SubAccountItemView::helpButtonClicked, panel, &SubAccountPanel::helpButtonClicked);
	connect(itemView, &SubAccountItemView::renameAccount, panel, &SubAccountPanel::renameSubAccount);

    return itemView;
}

void SubAccountItemDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    CTreeViewEx* view = qobject_cast<CTreeViewEx*> (parent());
    if(view == nullptr)
    {
        return ;
    }

    QStandardItemModel* model = (QStandardItemModel*)(view->model());
    SubAccountItem* sitem = (SubAccountItem*)( model->item(index.row(),0));
    if(sitem == nullptr)
    {
        return;
    }

    SubAccountItemView* itemView = (SubAccountItemView*)(editor);
    itemView->setId(sitem->id());

    //itemView->setEnabled(!sitem->locked());
	itemView->setBlocked(sitem->locked());
	itemView->showSwitchButton(!sitem->isCurrent());
}

void SubAccountItemDelegate::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QRect tagRect = option.rect.adjusted(0,0,0,-10);
    editor->setGeometry(tagRect);
}