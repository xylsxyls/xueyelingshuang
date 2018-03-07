#include "AccountManagerDialog.h"
#include "Label.h"
#include "AccountDialog.h"
#include "../COriginalButton.h"
#include "../CGeneralStyle.h"

void AccountManagerDialog::popAccountDialog()
{
	if (m_accountDialog == nullptr)
	{
		return;
	}

	int32_t dialogId = 0;
	m_accountDialog->exec(dialogId);
}

AccountManagerDialog::AccountManagerDialog() :
BoxDialogBase(340, 482, nullptr)
{
	m_accountDialog = new AccountDialog(nullptr);
	m_title->setText(QString::fromStdWString(L"11对战平台 - 账号管理页面"));
	m_exit->setVisible(true);

	m_hide = new COriginalButton(this);
	m_hide->setText("");
	m_hide->setGeometry(QRect(width() - 34, 1, 34, 31));
	m_hide->setBkgImage(CGeneralStyle::instance()->platformResourcePath() + "/Common/Image/NotificationView/CloseButton.png");
	m_hide->setBkgMargins(0, 0);
	m_hide->setBorderRadius(0);
	m_hide->setMouseTracking(true);
	m_hide->installEventFilter(this);
	QObject::connect(m_hide, &COriginalButton::clicked, this, &AccountManagerDialog::onHide);
}

void AccountManagerDialog::onHide()
{
	setVisible(false);
}