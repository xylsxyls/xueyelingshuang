#include "FriendDialog.h"
#include "../COriginalButton.h"
#include "../CGeneralStyle.h"

FriendDialog::FriendDialog()
{
	resize(256, 561);
	m_exit = new COriginalButton(this);
	m_exit->setGeometry(width() - 30 - 5, 5, 30, 40);
	QString s = CGeneralStyle::instance()->platformResourcePath();
	m_exit->setBkgImage(CGeneralStyle::instance()->platformResourcePath() + "/../W3L/res/Image/InviteFriendView/InviteButton.png");
	m_exit->setText("");
	m_bottomExit = new COriginalButton(this);
	m_bottomExit->setBkgImage(CGeneralStyle::instance()->platformResourcePath() + "/../W3L/res/Image/InviteFriendView/CloseButton.png");
	m_bottomExit->setGeometry((width() - 164) / 2, height() - 10 - 38, 164, 38);
	m_bottomExit->setText("");
}

