#include "FriendDialogFrame.h"
#include "../COriginalButton.h"
#include "../CGeneralStyle.h"
#include "TreeWidget.h"
#include "NoFocusFrameDelegate.h"
#include "Label.h"
#include <QEvent>
#include <QWindow>
#include <QPainter>
#include <Windows.h>
#include <QPixmap>
#include <QBitmap>
//#include "D:\\SendToMessageTest.h"

FriendDialogFrame::FriendDialogFrame(QWindow* parent) :
m_highLight(false)
{
	setWindowFlags(windowFlags() | Qt::Tool);
	if (parent != nullptr)
	{
		setParentWindow(parent);
	}
	
	setFixedSize(256, 561);
	setWindowTitle(QString::fromStdWString(L"邀请好友"));

	m_friendTree = new TreeWidget(this);
	m_friendTree->setGeometry(1, 34, width() - 2, height() - 34 - 16 * 2 - 38);
	m_friendTree->initScrollBar();
	m_friendTree->setBackgroundColor(QColor(27, 37, 63, 255));
	m_friendTree->setBorderWidth(0);

	m_friendTree->setMouseTracking(true);
	m_friendTree->installEventFilter(this);

	Label* separator = new Label(this);
	if (separator == nullptr)
	{
		return;
	}

	separator->setGeometry(1, height() - 16 * 2 - 38, width() - 2, 2);
	separator->setBackgroundColor(QColor(42, 54, 88, 255));
	Label* label = new Label(separator);
	if (label == nullptr)
	{
		return;
	}
	label->setGeometry(0, 0, separator->width(), 1);
	label->setBackgroundColor(QColor(20, 28, 49, 255));
	
	m_exit = new COriginalButton(this);
	m_exit->setGeometry(width() - 24 - 9, 5, 24, 24);
	m_exit->setBkgImage(CGeneralStyle::instance()->war3lobbyResourcePath() + "/Image/Common/PopupView/NormalCloseButton.png");
	m_exit->setText("");
	QObject::connect(m_exit, &COriginalButton::clicked, this, &FriendDialogFrame::reject);

	m_bottomExit = new COriginalButton(this);
	m_bottomExit->setBkgImage(CGeneralStyle::instance()->platformResourcePath() + "/../W3L/res/Image/InviteFriendView/CloseButton.png");
	m_bottomExit->setGeometry((width() - 164) / 2, height() - 16 - 38, 164, 38);
	m_bottomExit->setText("");
	QObject::connect(m_bottomExit, &COriginalButton::clicked, this, &FriendDialogFrame::reject);
}

void FriendDialogFrame::popUp()
{
	initUIData();
	QTreeWidgetItem* item = m_friendTree->itemAt(0, 0);
	if (item != nullptr)
	{
		item->setExpanded(true);
	}
	setParentWindow(nullptr);
	show();
	return;
}

void FriendDialogFrame::initUIData()
{
	clear();

	const YY::W3L::UIData& uiData = YY::W3L::UIData::getInstance();

	auto friendDialogFrame = CLobbyMainFrame::instance()->friendDialogFrame();
	if (friendDialogFrame != nullptr)
	{
		// 好友分组
		FriendDialogFrame::Group groupData;
		groupData.m_groupId = 0;
		groupData.m_groupName = QString::fromStdWString(L"我的可邀请好友");

		const YY::W3L::FriendList& friendList = uiData.getFriendList();
		FriendDialogFrame::User user;

		int32_t index = -1;
		while (index++ != friendList.size() - 1)
		{
			auto& oneFriend = friendList[index];
			user.m_userId = oneFriend.friendId;
			user.m_userName = oneFriend.friendName;
			user.m_userPicPath = CGeneralStyle::instance()->getUserAvatar(oneFriend.friendId, oneFriend.friendImageId);
			user.m_isVip = oneFriend.friendVip;
			groupData.m_userList.push_back(user);
		}

		friendDialogFrame->addGroupList(groupData);
		
		QObject::connect(&uiData, &YY::W3L::UIData::friendInserted, friendDialogFrame, &FriendDialogFrame::onFriendInsert);
		QObject::connect(&uiData, &YY::W3L::UIData::friendUpdated, friendDialogFrame, &FriendDialogFrame::onFriendUpdate);
		QObject::connect(&uiData, &YY::W3L::UIData::friendRemoved, friendDialogFrame, &FriendDialogFrame::onFriendRemove);
	}
}

void FriendDialogFrame::addGroupList(const Group& groupList)
{
	QWidget* widget = new QWidget(this);
	if (widget == nullptr)
	{
		return;
	}
	m_friendTree->addWidget(32, widget);
	m_groupData[groupList.m_groupId] = widget;
	widget->setMouseTracking(true);
	widget->installEventFilter(this);
	widget->setStyleSheet(".QWidget{background-color:rgba(27,37,63,255);}");

	Label* groupName = new Label(widget);
	groupName->setObjectName("groupName");
	groupName->setText(groupList.m_groupName + QString::fromStdWString(CStringManager::Format(L" %d", groupList.m_userList.size())));
	groupName->setGeometry(24, 0, 200, 32);
	groupName->setTextColor(QColor(112, 123, 150, 255), QColor(112, 146, 229, 255));

	Label* separator = new Label(widget);
	if (separator == nullptr)
	{
		return;
	}

	separator->setGeometry(1, 30, width() - 2, 2);
	separator->setBackgroundColor(QColor(42, 54, 88, 255));
	Label* label = new Label(separator);
	if (label == nullptr)
	{
		return;
	}
	label->setGeometry(0, 0, separator->width(), 1);
	label->setBackgroundColor(QColor(20, 28, 49, 255));

	COriginalButton* indi = new COriginalButton(widget);
	indi->setBkgImage(CGeneralStyle::instance()->platformResourcePath() + "/../W3L/res/Image/InviteFriendView/group_indicator.png", 4, 1, 2, 1, 1, 3, 4, 3, 3);
	indi->setGeometry(6, 11, 10, 10);
	indi->setText("");
	indi->setObjectName("indi");
	indi->setCheckable(!!groupList.m_userList.size());
	QObject::connect(indi, &COriginalButton::clicked, this, &FriendDialogFrame::onIndiClicked);

	QObject::connect(m_friendTree, &TreeWidget::itemExpanded, this, &FriendDialogFrame::onItemExpanded);
	QObject::connect(m_friendTree, &TreeWidget::itemCollapsed, this, &FriendDialogFrame::onItemCollapsed);

	const std::list<User>& userList = groupList.m_userList;
	for (auto itUser = userList.begin(); itUser != userList.end(); ++itUser)
	{
		Label* hover = new Label;
		hover->setGeometry(0, 0, width() - 2, 40);
		hover->setBackgroundColor(QColor(39, 50, 83, 255));
		hover->setObjectName("hoverLabel");
		hover->setVisible(false);

		UserWidget* userWidget = new UserWidget(hover, this);

		Label* headPic = new Label(userWidget);
		headPic->setGeometry(23, 5, 30, 30);
		headPic->setBorderRadius(4);
		headPic->setObjectName("headPic");
		headPic->setBorderImage(itUser->m_userPicPath, 1, 1, 1, 1);

		Label* userName = new Label(userWidget);
		userName->setGeometry(65, 10, 146, 20);
		userName->setFontSize(13);
		if (itUser->m_isVip)
		{
			QString vipPath = CGeneralStyle::instance()->platformResourcePath() + "/../W3L/res/Image/MatchChat/vip.png";
			userName->setText("<img src = '" + vipPath + "' /><font color = 'red'>  " + itUser->m_userName + "</font>");
		}
		else
		{
			userName->setText(itUser->m_userName);
			userName->setTextColor(QColor(200, 200, 250, 255));
		}
		userName->setObjectName("userName");

		COriginalButton* invite = new COriginalButton(hover);
		invite->setBkgImage(CGeneralStyle::instance()->platformResourcePath() + "/../W3L/res/Image/InviteFriendView/InviteButton.png");
		invite->setGeometry(210, 5, 40, 30);
		invite->setText("");
		QObject::connect(invite, &COriginalButton::clicked, this, &FriendDialogFrame::onInviteClicked);

		m_friendTree->addWidget(40, userWidget, widget);
		m_userData[groupList.m_groupId][itUser->m_userId] = userWidget;
	}
}

void FriendDialogFrame::clear()
{
	while (m_groupData.size())
	{
		removeGroup(m_groupData.begin()->first);
	}
}

void FriendDialogFrame::updateUserInfo(int32_t groupId, const User& userInfo)
{
	auto itGroup = m_userData.find(groupId);
	if (itGroup == m_userData.end())
	{
		return;
	}

	auto& userMap = itGroup->second;
	auto itUser = userMap.find(userInfo.m_userId);
	if (itUser != userMap.end())
	{
		QWidget* updateWidget = itUser->second;
		Label* userName = updateWidget->findChild<Label*>("userName");
		if (userName != nullptr)
		{
			userName->setText(userInfo.m_userName);
		}
		Label* headPic = updateWidget->findChild<Label*>("headPic");
		if (headPic != nullptr)
		{
			headPic->setBackgroundImage(userInfo.m_userPicPath, 1, 1, 1, 1, true);
		}
	}
}

void FriendDialogFrame::addUserInfo(int32_t groupId, const User& userInfo)
{
	removeUser(userInfo.m_userId);

	auto itGroup = m_userData.find(groupId);
	if (itGroup == m_userData.end())
	{
		return;
	}
	auto& userMap = itGroup->second;
	auto itUser = userMap.find(userInfo.m_userId);
	if (itUser != userMap.end())
	{
		return;
	}

	Label* hover = new Label;
	hover->setGeometry(0, 0, width() - 2, 40);
	hover->setBackgroundColor(QColor(39, 50, 83, 255));
	hover->setObjectName("hoverLabel");
	hover->setVisible(false);

	UserWidget* userWidget = new UserWidget(hover, this);

	Label* headPic = new Label(userWidget);
	headPic->setGeometry(23, 5, 30, 30);
	headPic->setBorderRadius(4);
	headPic->setObjectName("headPic");
	headPic->setBorderImage(userInfo.m_userPicPath, 1, 1, 1, 1);

	Label* userName = new Label(userWidget);
	userName->setGeometry(65, 10, 146, 20);
	userName->setFontSize(13);
	userName->setObjectName("userName");
	if (userInfo.m_isVip)
	{
		QString vipPath = CGeneralStyle::instance()->platformResourcePath() + "/../W3L/res/Image/MatchChat/vip.png";
		userName->setText("<font color = 'red'>" + userInfo.m_userName + "</font><img src = '" + vipPath + "' />");
	}
	else
	{
		userName->setText(userInfo.m_userName);
		userName->setTextColor(QColor(200, 200, 250, 255));
	}

	COriginalButton* invite = new COriginalButton(hover);
	invite->setBkgImage(CGeneralStyle::instance()->platformResourcePath() + "/../W3L/res/Image/InviteFriendView/InviteButton.png");
	invite->setGeometry(210, 5, 40, 30);
	invite->setText("");
	invite->setMouseTracking(true);
	invite->installEventFilter(this);
	QObject::connect(invite, &COriginalButton::clicked, this, &FriendDialogFrame::onInviteClicked);

	m_friendTree->addWidget(40, userWidget, m_groupData[groupId]);
	m_userData[groupId][userInfo.m_userId] = userWidget;

	Label* groupName = m_groupData[groupId]->findChild<Label*>("groupName");
	if (groupName != nullptr)
	{
		std::vector<std::wstring> vecText = CStringManager::split(groupName->text().toStdWString(), L" ");
		if (vecText.size() == 2)
		{
			groupName->setText(QString::fromStdWString(CStringManager::Format((vecText[0] + L" %d").c_str(), _wtoi(vecText[1].c_str()) + 1)));
		}
	}

	COriginalButton* indi = m_groupData[groupId]->findChild<COriginalButton*>("indi");
	if (indi != nullptr)
	{
		indi->setCheckable(true);
	}
}

void FriendDialogFrame::removeUser(int32_t userId)
{
	int32_t groupId = -1;
	QWidget* removeWidget = nullptr;
	for (auto itGroup = m_userData.begin(); itGroup != m_userData.end(); ++itGroup)
	{
		auto& userMap = itGroup->second;
		auto itUser = userMap.find(userId);
		if (itUser != userMap.end())
		{
			groupId = itGroup->first;
			removeWidget = itUser->second;
			break;
		}
	}

	if (removeWidget == nullptr)
	{
		return;
	}
	
	Label* groupName = m_groupData[groupId]->findChild<Label*>("groupName");
	if (groupName != nullptr)
	{
		std::vector<std::wstring> vecText = CStringManager::split(groupName->text().toStdWString(), L" ");
		if (vecText.size() == 2)
		{
			groupName->setText(QString::fromStdWString(CStringManager::Format((vecText[0] + L" %d").c_str(), _wtoi(vecText[1].c_str()) - 1)));
		}
	}

	if (m_userData[groupId].size() == 1)
	{
		COriginalButton* indi = m_groupData[groupId]->findChild<COriginalButton*>("indi");
		if (indi != nullptr)
		{
			indi->setChecked(false);
			indi->setCheckable(false);
		}
	}

	m_userData[groupId].erase(m_userData[groupId].find(userId));

	m_friendTree->removeWidget(removeWidget);
}

void FriendDialogFrame::removeGroup(int32_t groupId)
{
	auto itGroup = m_groupData.find(groupId);
	if (itGroup == m_groupData.end())
	{
		return;
	}
	QWidget* removeWidget = itGroup->second;
	m_groupData.erase(itGroup);
	
	auto itUserGroup = m_userData.find(groupId);
	if (itUserGroup != m_userData.end())
	{
		m_userData.erase(itUserGroup);
	}

	m_friendTree->removeWidget(removeWidget);
}

void FriendDialogFrame::paintEvent(QPaintEvent* eve)
{
	CW3LModalFrame::paintEvent(eve);

	QPainter painter(this);
	if (m_highLight)
	{
		painter.setPen(QColor(50, 99, 190, 255));
	}
	else
	{
		painter.setPen(QColor("#4b5586"));
	}
	painter.drawRect(0, 0, width() - 1, height() - 1);
}

bool FriendDialogFrame::nativeEvent(const QByteArray& eventType, void* message, long* result)
{
	bool res = CW3LModalFrame::nativeEvent(eventType, message, result);
	if (eventType == "windows_generic_MSG" || eventType == "windows_dispatcher_MSG")
	{
		MSG* msg = static_cast<MSG*>(message);
		switch (msg->message)
		{
		case WM_NCACTIVATE:
		{
			activeChanged(msg->wParam);
		}
		break;
		default:
			break;
		}
	}
	return res;
}

void FriendDialogFrame::activeChanged(int32_t wParam)
{
	m_highLight = !!wParam;
	repaint();
}

void FriendDialogFrame::setParentWindow(QWindow* parent)
{
	if (parent != nullptr)
	{
		QWindow* window = windowHandle();
		if (window != nullptr)
		{
			setAttribute(Qt::WA_NativeWindow);
			setWindowModality(Qt::WindowModal);
			window->setTransientParent(parent);
			return;
		}
	}
	//去掉任务栏
	//setAttribute(Qt::WA_ShowModal);
	setWindowModality(Qt::ApplicationModal);
}

void FriendDialogFrame::onIndiClicked()
{
	if (m_friendTree == nullptr)
	{
		return;
	}

	QTreeWidgetItem* pClickItem = m_friendTree->itemAt(m_friendTree->mapFromGlobal(cursor().pos()));
	if (pClickItem == nullptr)
	{
		return;
	}
	pClickItem->setExpanded(!pClickItem->isExpanded());
	QWidget* clickWidget = m_friendTree->itemWidget(pClickItem, 0);
	if (clickWidget == nullptr)
	{
		return;
	}
	COriginalButton* indi = clickWidget->findChild<COriginalButton*>("indi");
	if (indi == nullptr)
	{
		return;
	}
	indi->setChecked(pClickItem->isExpanded() && pClickItem->childCount());
}

void FriendDialogFrame::onInviteClicked()
{
	QTreeWidgetItem* pClickItem = m_friendTree->itemAt(m_friendTree->mapFromGlobal(cursor().pos()));
	QWidget* clickWidget = m_friendTree->itemWidget(pClickItem, 0);
	int32_t groupId = -1;
	int32_t userId = -1;
	for (auto itUserMap = m_userData.begin(); itUserMap != m_userData.end(); ++itUserMap)
	{
		auto& userMap = itUserMap->second;
		for (auto itUser = userMap.begin(); itUser != userMap.end(); ++itUser)
		{
			if (itUser->second == clickWidget)
			{
				groupId = itUserMap->first;
				userId = itUser->first;
				break;
			}
		}
	}
	emit inviteClicked(groupId, userId);
	QList<qint64> idList;
	idList.append(userId);
	emit YY::W3L::UISignalSet::getInstance().gameInvitationSent(idList);
}

void FriendDialogFrame::onItemExpanded(QTreeWidgetItem* item)
{
	QWidget* expandedWidget = m_friendTree->itemWidget(item, 0);
	if (expandedWidget == nullptr)
	{
		return;
	}
	COriginalButton* indi = expandedWidget->findChild<COriginalButton*>("indi");
	if (indi == nullptr)
	{
		return;
	}
	bool xxx = indi->isChecked();
	bool xxxx = m_friendTree->isItemExpanded(item);
	indi->setChecked(m_friendTree->isItemExpanded(item));
}

void FriendDialogFrame::onItemCollapsed(QTreeWidgetItem* item)
{
	QWidget* expandedWidget = m_friendTree->itemWidget(item, 0);
	if (expandedWidget == nullptr)
	{
		return;
	}
	COriginalButton* indi = expandedWidget->findChild<COriginalButton*>("indi");
	if (indi == nullptr)
	{
		return;
	}
	indi->setChecked(m_friendTree->isItemExpanded(item));
}

void FriendDialogFrame::onFriendInsert(const YY::W3L::FriendData& data)
{
	FriendDialogFrame::User user;
	user.m_userId = data.friendId;
	user.m_userName = data.friendName;
	user.m_userPicPath = CGeneralStyle::instance()->getUserAvatar(data.friendId, data.friendImageId);
	addUserInfo(data.groupId, user);
}

void FriendDialogFrame::onFriendUpdate(const YY::W3L::FriendData& data)
{
	FriendDialogFrame::User user;
	user.m_userId = data.friendId;
	user.m_userName = data.friendName;
	user.m_userPicPath = CGeneralStyle::instance()->getUserAvatar(data.friendId, data.friendImageId);
	updateUserInfo(data.groupId, user);
}

void FriendDialogFrame::onFriendRemove(int32_t userId)
{
	removeUser(userId);
}