#include "SubAccountPanel.h"
#include <QEvent>
#include <QHeaderView>
#include <QPainter>
#include "CGeneralStyle.h"
#include "COriginalButton.h"
#include "CTreeViewEx.h"
#include "CExternalTextEdit.h"
#include <QWindow>
#include "../core/CSystem.h"

SubAccountPanel::SubAccountPanel(QWidget *parent)
    :QWidget(parent)
    ,mCreateSubAccountButton(new COriginalButton(this))
    ,mHelpButton(new COriginalButton(this))
    ,mTreeView(new CTreeViewEx(this))
    ,mModel(new QStandardItemModel(this))
    ,mCanCreateCount(0)
    ,mHelpTip(new CExternalTextEdit)
{
    mTreeView->setModel(mModel);
    mTreeView->setIndentation(0);
    mTreeView->header()->setVisible(false);
    mTreeView->setEditTriggers(CTreeViewEx::NoEditTriggers);
	mTreeView->setSelectionMode(CTreeViewEx::NoSelection);
    SubAccountItemDelegate* tDelegate = new SubAccountItemDelegate(mTreeView);
    mTreeView->setItemDelegate(tDelegate);


    mCreateSubAccountButton->setText("");
    mCreateSubAccountButton->setFixedSize(88,21);
    mCreateSubAccountButton->setBkgImage(CGeneralStyle::instance()->war3lobbyResourcePath() + "/Image/SubAccount/create_subaccount_button.png",
                                         4,1,2,3,4,1,2,3,4);
	connect(mCreateSubAccountButton, &COriginalButton::clicked, this, &SubAccountPanel::createSubAccount);

    mHelpButton->setUnderline(true);
    mHelpButton->setFixedSize(80,21);
    mHelpButton->setText(QStringLiteral("[如何增加]"));
    mHelpButton->setFontColor(QColor(255,180,120),"white", "white",QColor(255,180,120),
                              QColor(255,180,120),"white", "white",QColor(255,180,120));

    mHelpButton->installEventFilter(this);
    this->installEventFilter(this);
    this->setMouseTracking(true);

    mHelpTip->setWindowFlags(mHelpTip->windowFlags() | Qt::FramelessWindowHint | Qt::Tool | Qt::WindowDoesNotAcceptFocus);

    
    

    mHelpTip->setStyleSheet(QStringLiteral("background-color:rgba(29,29,47);border: 1px solid rgba(132,142,168); font-size:14px; font-family:'微软雅黑';color:#abb3d3;"));
    mHelpTip->setText(QStringLiteral("1. 新建账户默认有一个账号<br><br>"                              )+
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
			sitem->setIsCurrent(true);
		else
			sitem->setIsCurrent(false);
        if(i > 3)
            sitem->setLocked(true);
    }

    this->setSubAccountList(li);
}

SubAccountPanel::~SubAccountPanel()
{

}

void SubAccountPanel::moveEvent(QMoveEvent *e)
{
    QWidget::moveEvent(e);
    this->layoutContorls();
}

void SubAccountPanel::layoutContorls()
{
    mCreateSubAccountButton->move(this->width() - 38 - mCreateSubAccountButton->width(),
                                  14);

    mHelpButton->move(this->width() - 24 - mHelpButton->width() - 6,56 - 6);

    mTreeView->setGeometry(QRect(8,90, this->width() - 16, this->height() - 90 - 8));

    mHelpTip->setGeometry(QRect(this->mapToGlobal(mTreeView->geometry().topLeft()),
                                this->mapToGlobal(mTreeView->geometry().bottomRight() + QPoint(1,1))));
}

void SubAccountPanel::resizeEvent(QResizeEvent *e)
{
    QWidget::resizeEvent(e);
    this->layoutContorls();
}

void SubAccountPanel::paintEvent(QPaintEvent *e)
{
    QPainter p(this);
    p.save();
    p.fillRect(rect(), "#2c344a");

    QRect fucRect(8, 45, width() - 16 < 0 ? 0 : (width() - 16), 32);
    p.setPen("#4a5980");
    p.drawRect(CSystem::rectValid(fucRect));

    QFont tf = CGeneralStyle::instance()->font();
    p.setPen(QColor(192,200,218));
    p.setFont(tf);
    p.drawText(CSystem::rectValid(fucRect.adjusted(21, 0, 0, 0)), Qt::AlignVCenter | Qt::AlignLeft, QStringLiteral("可创建的子账号数："));


    QRect descRect(29,0,this->width(),45);
    p.setPen("#ffffff");
    QFont tf1 = CGeneralStyle::instance()->font();
    tf1.setPixelSize(16);
    p.setFont(tf1);

    p.drawText(CSystem::rectValid(descRect), Qt::AlignVCenter | Qt::AlignLeft, QStringLiteral("子账号管理"));

    QRect countRect = fucRect;
    countRect.setRight(mHelpButton->geometry().left() - 10);
    p.drawText(CSystem::rectValid(countRect), Qt::AlignVCenter | Qt::AlignRight, QString::number(mCanCreateCount));

    p.restore();
    //p.drawRect(mTreeView->geometry());
}

bool SubAccountPanel::eventFilter(QObject *obj, QEvent *e)
{
    bool res = QWidget::eventFilter(obj, e);
	if (obj == nullptr || e == nullptr)
	{
		return res;
	}

    if(obj == this)
    {
        if(e->type() == QEvent::Enter || e->type() == QEvent::Move)
        {
            mHelpTip->close();
        }
    }
    else if(obj == mHelpButton)
    {
        if(e->type() == QEvent::Enter)
        {
			this->layoutContorls();
            mHelpTip->show();
        }
        else if(e->type() == QEvent::Leave)
        {
            mHelpTip->close();
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
	return mHelpTip;
}

quint64 SubAccountPanel::canCreateCount()
{
	return mCanCreateCount;
}

void SubAccountPanel::setCanCreateCount(quint64 v)
{
    mCanCreateCount = v;
    this->update();
}

void SubAccountPanel::setSubAccountList(const SubAccountItemList &li)
{
    mModel->clear();
    mModel->appendColumn(this->subAccountItemLiToStandardItemLi(li));
    for(int i = 0; i < mModel->rowCount(); i++)
    {
        mTreeView->openPersistentEditor(mModel->index(i,0));
    }

	//	mModel->setSortRole(SubAccountItem::Role_IsCurrent);
	//	mModel->sort(0, Qt::DescendingOrder);
}


SubAccountItemList SubAccountPanel::subAccountList()
{
	SubAccountItemList li;
	for(int i = 0; i < mModel->rowCount(); i++)
	{
		li << (SubAccountItem*)(mModel->item(i,0));
	}

	return li;
}

void SubAccountPanel::appendSubAccount(SubAccountItem* item)
{
	mModel->appendRow(item);
	mTreeView->openPersistentEditor(item->index());
}

SubAccountItem* SubAccountPanel::getSubAccountItemById(quint64 id)
{
	SubAccountItem* res = NULL;
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
		mModel->removeRow(sitem->row());
	}
}

SubAccountItemView::SubAccountItemView(QWidget *parent)
    :QWidget(parent)
    ,mId(0)
    ,mSwitchButton(new COriginalButton(this))
	,mHelpButton(new COriginalButton(this))
	,mChangeNameButton(new COriginalButton(this))
{
	setBlocked(true);
    mSwitchButton->setText("");
    mSwitchButton->resize(51, 21);
    mSwitchButton->setBkgImage(CGeneralStyle::instance()->war3lobbyResourcePath() + "/Image/SubAccount/switch_subaccount_button.png",
                               4,1,2,3,4,1,2,3,4);

    connect(mSwitchButton, &COriginalButton::clicked,
            this, &SubAccountItemView::onSwitchButtonClicked);


	mHelpButton->setText("");
	mHelpButton->resize(19,19);
	mHelpButton->setBkgImage(CGeneralStyle::instance()->war3lobbyResourcePath() + "/Image/SubAccount/help_button.png",
		3,1,2,3,3,1,2,3,3);

	connect(mHelpButton, &COriginalButton::clicked, this, &SubAccountItemView::helpButtonClicked);

	mChangeNameButton->setText(QStringLiteral("改名"));
	mChangeNameButton->resize(51,21);
	mChangeNameButton->setBorderWidth(1);
	mChangeNameButton->setBorderStyle("solid");
	mChangeNameButton->setBorderColor(QColor(202,212,248),QColor(0,248,255), QColor(44,52,74), QColor(180,180,181));
	mChangeNameButton->setFontColor(QColor(202,212,248),QColor(0,248,255), QColor(44,52,74), QColor(180,180,181),QColor(202,212,248),QColor(0,248,255), QColor(44,52,74), QColor(180,180,181));

	connect(mChangeNameButton, &COriginalButton::clicked, this, &SubAccountItemView::onChangeNameButtonClicked);
}

void SubAccountItemView::resizeEvent(QResizeEvent *e)
{
    QWidget::resizeEvent(e);
    this->layoutControls();
}

void SubAccountItemView::layoutControls()
{
    mSwitchButton->move(this->width() - mSwitchButton->width() - 22, 17);
	mChangeNameButton->move(mSwitchButton->x(), mSwitchButton->y() + mSwitchButton->height() + 10);
	mHelpButton->move(this->width() - mHelpButton->width() - 2, 2);
}

void SubAccountItemView::onSwitchButtonClicked()
{
    emit switchAccount(mId);
}

void SubAccountItemView::onChangeNameButtonClicked()
{
	emit renameAccount(mId);
}

void SubAccountItemView::showSwitchButton(bool s)
{
	mSwitchButton->setVisible(s);
	//mChangeNameButton->setVisible(s);
}

void SubAccountItemView::setBlocked(bool s)
{
	mBlocked = s;
	mSwitchButton->setEnabled(!s);
	mChangeNameButton->setEnabled(!s);
	mHelpButton->setVisible(s);

	this->update();
}

SubAccountItemDelegate::SubAccountItemDelegate(QObject *parent)
    :QStyledItemDelegate(parent)
{

}

void SubAccountItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (painter == nullptr)
    {
        return;
    }
    CTreeViewEx* view = qobject_cast<CTreeViewEx*> (parent());
    if(view == NULL)
        return ;

    QStandardItemModel* model = (QStandardItemModel*)(view->model());
    SubAccountItem* sitem = (SubAccountItem*)( model->item(index.row(),0));
    if(sitem == NULL)
        return;

    QColor bkgColor;
    QFont  tf  = CGeneralStyle::instance()->font();
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

    painter->fillRect(CSystem::rectValid(tagRect), bkgColor);

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

    painter->drawText(CSystem::rectValid(nameRect), Qt::AlignLeft | Qt::AlignTop, sitem->name());

    QRect dateRect = tagRect.adjusted(18,36,0,0);
    QRect descRect = tagRect.adjusted(18,58,0,0);
    switch(useTC)
    {
    case 1:
        painter->setPen(tc1);
        break;
    case 2:
        painter->setPen(tc2);
        break;
    case 3:
        painter->setPen(tc3);
        break;
    }

    painter->drawText(CSystem::rectValid(dateRect), Qt::AlignLeft | Qt::AlignTop, sitem->date());
    painter->drawText(CSystem::rectValid(descRect), Qt::AlignLeft | Qt::AlignTop, sitem->desc());
}

QWidget *SubAccountItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	CTreeViewEx* view = qobject_cast<CTreeViewEx*> (this->parent());
	if(view == NULL)
	{
		return QStyledItemDelegate::createEditor(parent, option, index);
	}

	SubAccountPanel* panel = qobject_cast<SubAccountPanel*> (view->parent());
	if(panel == NULL)
	{
		return QStyledItemDelegate::createEditor(parent, option, index);
	}

    SubAccountItemView* itemView = new SubAccountItemView(parent);
	connect(itemView, &SubAccountItemView::switchAccount, panel, &SubAccountPanel::siwtchSubAccount);
	connect(itemView, &SubAccountItemView::helpButtonClicked, panel, &SubAccountPanel::helpButtonClicked);
	connect(itemView, &SubAccountItemView::renameAccount, panel, &SubAccountPanel::renameSubAccount);

    return itemView;
}

void SubAccountItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    CTreeViewEx* view = qobject_cast<CTreeViewEx*> (parent());
    if(view == NULL)
        return ;

    QStandardItemModel* model = (QStandardItemModel*)(view->model());
    SubAccountItem* sitem = (SubAccountItem*)( model->item(index.row(),0));
    if(sitem == NULL)
        return;

    SubAccountItemView* itemView = (SubAccountItemView*)(editor);
    itemView->setId(sitem->id());

    //itemView->setEnabled(!sitem->locked());
	itemView->setBlocked(sitem->locked());
	itemView->showSwitchButton(!sitem->isCurrent());
}

void SubAccountItemDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QRect tagRect = option.rect.adjusted(0,0,0,-10);
    editor->setGeometry(tagRect);
}
