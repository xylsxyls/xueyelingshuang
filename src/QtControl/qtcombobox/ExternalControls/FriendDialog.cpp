#include "FriendDialog.h"
#include "../COriginalButton.h"
#include "../CGeneralStyle.h"
#include "TreeWidget.h"
#include "NoFocusFrameDelegate.h"
#include "Label.h"
#include <QEvent>
#include "D:\\SendToMessageTest.h"

FriendDialog::FriendDialog():
m_lastHoverWidget(nullptr)
{
	resize(256, 561);
	setWindowTitle(QString::fromStdWString(L"邀请好友"));

	m_friendTree = new TreeWidget(this);
	m_friendTree->setGeometry(1, 33, width() - 2, height() - 36 - 16 * 2 - 38);
	m_friendTree->setBackgroundColor(QColor(27, 37, 63, 255));
	m_friendTree->setBorderWidth(0);

	
	m_friendTree->setMouseTracking(true);
	m_friendTree->installEventFilter(this);
	
	//m_friendTree->setStyleSheet("QTreeWidget{background-color:(255,0,0,100);}");
	//m_friendTree->setFrameShape(QFrame::NoFrame);

	m_exit = new COriginalButton(this);
	m_exit->setGeometry(width() - 30 - 5, 5, 30, 40);
	QString s = CGeneralStyle::instance()->platformResourcePath();
	m_exit->setBkgImage(CGeneralStyle::instance()->platformResourcePath() + "/../W3L/res/Image/InviteFriendView/InviteButton.png");
	m_exit->setText("");
	m_bottomExit = new COriginalButton(this);
	m_bottomExit->setBkgImage(CGeneralStyle::instance()->platformResourcePath() + "/../W3L/res/Image/InviteFriendView/CloseButton.png");
	m_bottomExit->setGeometry((width() - 164) / 2, height() - 16 - 38, 164, 38);
	m_bottomExit->setText("");
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
	widget->setStyleSheet(".QWidget{background-color:rgba(27,37,63,255);}");

	Label* lab = new Label(widget);
	lab->setText(groupList.m_groupName + QString::fromStdWString(L" 11"));
	lab->setGeometry(18, 0, 200, 32);

	COriginalButton* indi = new COriginalButton(widget);
	indi->setBkgImage(CGeneralStyle::instance()->platformResourcePath() + "/../W3L/res/Image/InviteFriendView/FriendExpandButton.png", 2, 1, 2, 1, 1, 1, 2, 1, 1);
	indi->setGeometry(2, 12, 6, 10);
	indi->setText("");

	COriginalButton* invite = new COriginalButton(widget);
	invite->setBkgImage(CGeneralStyle::instance()->platformResourcePath() + "/../W3L/res/Image/InviteFriendView/InviteButton.png");
	invite->setGeometry(218, 6, 40, 30);
	invite->setText("");

	const std::list<User>& userList = groupList.m_userList;
	for (auto itUser = userList.begin(); itUser != userList.end(); ++itUser)
	{
		QWidget* userWidget = new QWidget(this);
		userWidget->setMouseTracking(true);
		userWidget->installEventFilter(this);

		Label* hover = new Label(userWidget);
		hover->setGeometry(0, 0, width() - 2, 40);
		hover->setBackgroundColor(QColor(255, 0, 0, 255));
		hover->setObjectName("hoverLabel");
		hover->setVisible(false);

		Label* headPic = new Label(userWidget);

		Label* userName = new Label(userWidget);
		userName->setText(itUser->m_userName);

		COriginalButton* invite = new COriginalButton(hover);
		invite->setBkgImage(CGeneralStyle::instance()->platformResourcePath() + "/../W3L/res/Image/InviteFriendView/InviteButton.png");
		invite->setGeometry(218, 6, 40, 30);
		invite->setText("");

		m_friendTree->addWidget(40, userWidget, widget);
		m_userData[groupList.m_groupId][itUser->m_userId] = userWidget;
	}
}

bool FriendDialog::eventFilter(QObject* obj, QEvent* eve)
{
	for (auto itGroup = m_userData.begin(); itGroup != m_userData.end(); ++itGroup)
	{
		auto& userData = itGroup->second;
		for (auto itUser = userData.begin(); itUser != userData.end(); ++itUser)
		{
			RCSend("%d",eve->type());
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
				}
				
				m_lastHoverWidget = itUser->second;
				Label* hoverLabel = itUser->second->findChild<Label*>("hoverLabel");
				if (hoverLabel != nullptr)
				{
					hoverLabel->setVisible(true);
				}
			}
		}
	}
	return CW3LModalFrame::eventFilter(obj, eve);
}