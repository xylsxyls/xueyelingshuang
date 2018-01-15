#include "FriendDialog.h"
#include "../COriginalButton.h"
#include "../CGeneralStyle.h"
#include "TreeWidget.h"
#include "NoFocusFrameDelegate.h"
#include "Label.h"
#include <QEvent>
#include <QWindow>
#include <QPainter>
#include <Windows.h>

FriendDialog::FriendDialog(QWidget* parent) :
m_lastHoverWidget(nullptr),
m_highLight(false)
{
	if (parent != nullptr)
	{
		setAttribute(Qt::WA_NativeWindow);
		setResponseHighlightDialog(parent);
	}
	
	resize(256, 561);
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
	
	//m_friendTree->setStyleSheet("QTreeWidget{background-color:(255,0,0,100);}");
	//m_friendTree->setFrameShape(QFrame::NoFrame);

	m_exit = new COriginalButton(this);
	m_exit->setGeometry(width() - 30 - 5, 5, 30, 40);
	m_exit->setBkgImage(CGeneralStyle::instance()->platformResourcePath() + "/../W3L/res/Image/InviteFriendView/InviteButton.png");
	m_exit->setText("");
	QObject::connect(m_exit, &COriginalButton::clicked, this, &FriendDialog::reject);

	m_bottomExit = new COriginalButton(this);
	m_bottomExit->setBkgImage(CGeneralStyle::instance()->platformResourcePath() + "/../W3L/res/Image/InviteFriendView/CloseButton.png");
	m_bottomExit->setGeometry((width() - 164) / 2, height() - 16 - 38, 164, 38);
	m_bottomExit->setText("");
	QObject::connect(m_bottomExit, &COriginalButton::clicked, this, &FriendDialog::reject);
}

void FriendDialog::addGroupList(const Group& groupList)
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
	groupName->setGeometry(18, 0, 200, 32);
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
	indi->setBkgImage(CGeneralStyle::instance()->platformResourcePath() + "/../W3L/res/Image/InviteFriendView/FriendExpandButton.png", 2, 1, 2, 1, 1, 1, 2, 1, 1);
	indi->setGeometry(6, 11, 6, 10);
	indi->setText("");
	QObject::connect(indi, &COriginalButton::clicked, this, &FriendDialog::onIndiClicked);

	const std::list<User>& userList = groupList.m_userList;
	for (auto itUser = userList.begin(); itUser != userList.end(); ++itUser)
	{
		QWidget* userWidget = new QWidget(this);
		userWidget->setMouseTracking(true);
		userWidget->installEventFilter(this);

		Label* hover = new Label(userWidget);
		hover->setGeometry(0, 0, width() - 2, 40);
		hover->setBackgroundColor(QColor(39, 50, 83, 255));
		hover->setObjectName("hoverLabel");
		hover->setVisible(false);

		Label* headPic = new Label(userWidget);
		headPic->setGeometry(23, 5, 30, 30);
		headPic->setBorderRadius(4);
		headPic->setObjectName("headPic");
		headPic->setBackgroundImage(itUser->m_userPicPath, 1, 1, 1, 1);

		Label* userName = new Label(userWidget);
		userName->setGeometry(65, 10, 146, 20);
		userName->setFontSize(13);
		userName->setText(itUser->m_userName);
		userName->setObjectName("userName");
		userName->setTextColor(QColor(200, 200, 250, 255));

		COriginalButton* invite = new COriginalButton(hover);
		invite->setBkgImage(CGeneralStyle::instance()->platformResourcePath() + "/../W3L/res/Image/InviteFriendView/InviteButton.png");
		invite->setGeometry(210, 5, 40, 30);
		invite->setText("");
		QObject::connect(invite, &COriginalButton::clicked, this, &FriendDialog::onInviteClicked);

		m_friendTree->addWidget(40, userWidget, widget);
		m_userData[groupList.m_groupId][itUser->m_userId] = userWidget;
	}
}

void FriendDialog::updateUserInfo(int32_t groupId, const User& userInfo)
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

void FriendDialog::addUserInfo(int32_t groupId, const User& userInfo)
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
		return;
	}

	QWidget* userWidget = new QWidget(this);
	userWidget->setMouseTracking(true);
	userWidget->installEventFilter(this);

	Label* hover = new Label(userWidget);
	hover->setGeometry(0, 0, width() - 2, 40);
	hover->setBackgroundColor(QColor(39, 50, 83, 255));
	hover->setObjectName("hoverLabel");
	hover->setVisible(false);

	Label* headPic = new Label(userWidget);
	headPic->setGeometry(23, 5, 30, 30);
	headPic->setBorderRadius(4);
	headPic->setObjectName("headPic");
	headPic->setBackgroundImage(userInfo.m_userPicPath, 1, 1, 1, 1);

	Label* userName = new Label(userWidget);
	userName->setGeometry(65, 10, 146, 20);
	userName->setFontSize(13);
	userName->setText(userInfo.m_userName);
	userName->setObjectName("userName");
	userName->setTextColor(QColor(200, 200, 250, 255));

	COriginalButton* invite = new COriginalButton(hover);
	invite->setBkgImage(CGeneralStyle::instance()->platformResourcePath() + "/../W3L/res/Image/InviteFriendView/InviteButton.png");
	invite->setGeometry(210, 5, 40, 30);
	invite->setText("");
	QObject::connect(invite, &COriginalButton::clicked, this, &FriendDialog::onInviteClicked);

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
}

void FriendDialog::removeUser(int32_t groupId, int32_t userId)
{
	auto itGroup = m_userData.find(groupId);
	if (itGroup == m_userData.end())
	{
		return;
	}
	auto& userMap = itGroup->second;
	auto itUser = userMap.find(userId);
	if (itUser == userMap.end())
	{
		return;
	}

	QWidget* removeWidget = itUser->second;
	m_friendTree->removeWidget(removeWidget);
	Label* groupName = m_groupData[groupId]->findChild<Label*>("groupName");
	if (groupName != nullptr)
	{
		std::vector<std::wstring> vecText = CStringManager::split(groupName->text().toStdWString(), L" ");
		if (vecText.size() == 2)
		{
			groupName->setText(QString::fromStdWString(CStringManager::Format((vecText[0] + L" %d").c_str(), _wtoi(vecText[1].c_str()) - 1)));
		}
	}
	userMap.erase(itUser);
}

void FriendDialog::removeGroup(int32_t groupId)
{
	auto itGroup = m_groupData.find(groupId);
	if (itGroup == m_groupData.end())
	{
		return;
	}
	m_friendTree->removeWidget(itGroup->second);
	m_groupData.erase(itGroup);
	
	auto itUserGroup = m_userData.find(groupId);
	if (itUserGroup != m_userData.end())
	{
		m_userData.erase(itUserGroup);
	}
}

bool FriendDialog::eventFilter(QObject* obj, QEvent* eve)
{
	for (auto itGroup = m_userData.begin(); itGroup != m_userData.end(); ++itGroup)
	{
		auto& userData = itGroup->second;
		for (auto itUser = userData.begin(); itUser != userData.end(); ++itUser)
		{
			//RCSend("%d", eve->type());
			if (eve->type() == QEvent::Leave)
			{
				QWidget* leaveWidget = (QWidget*)obj;
				Label* hoverLabel = leaveWidget->findChild<Label*>("hoverLabel");
				if (hoverLabel != nullptr)
				{
					hoverLabel->setVisible(false);
				}
				Label* userName = leaveWidget->findChild<Label*>("userName");
				if (userName != nullptr)
				{
					userName->setTextColor(QColor(200, 200, 250, 255), QColor(200, 200, 250, 255), QColor(200, 200, 250, 255), true);
				}
			}
			//说明到了子节点
			if (itUser->second == obj && eve->type() == QEvent::MouseMove)
			{
				if (m_lastHoverWidget != nullptr)
				{
					Label* hoverLabel = m_lastHoverWidget->findChild<Label*>("hoverLabel");
					if (hoverLabel != nullptr)
					{
						hoverLabel->setVisible(false);
					}
					Label* userName = m_lastHoverWidget->findChild<Label*>("userName");
					if (userName != nullptr)
					{
						userName->setTextColor(QColor(200, 200, 250, 255), QColor(200, 200, 250, 255), QColor(200, 200, 250, 255), true);
					}
				}
				
				m_lastHoverWidget = itUser->second;
				Label* hoverLabel = itUser->second->findChild<Label*>("hoverLabel");
				if (hoverLabel != nullptr)
				{
					hoverLabel->setVisible(true);
				}
				Label* userName = m_lastHoverWidget->findChild<Label*>("userName");
				if (userName != nullptr)
				{
					userName->setTextColor(QColor(255, 255, 255, 255), QColor(255, 255, 255, 255), QColor(255, 255, 255, 255), true);
				}
			}
		}
	}
	return CW3LModalFrame::eventFilter(obj, eve);
}

void FriendDialog::paintEvent(QPaintEvent* eve)
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

bool FriendDialog::nativeEvent(const QByteArray& eventType, void* message, long* result)
{
	bool res = CW3LModalFrame::nativeEvent(eventType, message, result);
	if (eventType == "windows_generic_MSG" || eventType == "windows_dispatcher_MSG")
	{
		MSG* msg = static_cast<MSG*>(message);
		switch (msg->message)
		{
		case WM_NCACTIVATE:
		{
			ncActiveChanged(msg->wParam);
		}
		break;
		default:
			break;
		}
	}
	return res;
}

void FriendDialog::ncActiveChanged(int32_t wParam)
{
	m_highLight = !!wParam;
	CW3LModalFrame::repaint();
}

void FriendDialog::setResponseHighlightDialog(QWidget* parent)
{
	if (parent == nullptr)
	{
		return;
	}
	QWindow* handle = parent->windowHandle();
	if (handle != nullptr)
	{
		setWindowModality(Qt::WindowModal);
		windowHandle()->setTransientParent(handle);
	}
}

void FriendDialog::onIndiClicked()
{
	if (m_friendTree == nullptr)
	{
		return;
	}

	QTreeWidgetItem* pClickItem = m_friendTree->itemAt(m_friendTree->mapFromGlobal(cursor().pos()));
	pClickItem->setExpanded(!pClickItem->isExpanded());
}

void FriendDialog::onInviteClicked()
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
}
