#include "AccountManagerDialog.h"
#include "Label.h"
#include "AccountDialog.h"
#include "COriginalButton.h"
#include "CGeneralStyle.h"
#include "SubAccountPanel.h"
#include "CExternalTextEdit.h"
#include <QWindow>
#include "ClosureDialog.h"

QString AccountManagerDialog::popAccountDialog()
{
	if (m_accountDialog == nullptr)
	{
		return -1;
	}

	m_accountDialog->clearAccountEdit();
	int32_t result = m_accountDialog->exec();
	if (result != ACCEPT_BUTTON)
	{
		return "";
	}
	return m_accountDialog->accountEditText();
}

void AccountManagerDialog::popClosureDialog()
{
	if (m_closureDialog == nullptr)
	{
		return;
	}
	m_closureDialog->exec();
}

AccountManagerDialog::AccountManagerDialog() :
m_accountDialog(nullptr),
m_closureDialog(nullptr),
m_subAccountPanel(nullptr)
{
    m_accountDialog = new AccountDialog();
    m_closureDialog = new ClosureDialog();
    m_subAccountPanel = new SubAccountPanel(this);

	m_subAccountPanel->helpTip()->winId();
	m_subAccountPanel->helpTip()->windowHandle()->setTransientParent(this->windowHandle());

	m_title->setText(QStringLiteral("11对战平台 - 账号管理页面"));

    m_exit->setBkgImage(CGeneralStyle::instance()->platformResourcePath() + "/Common/Image/NotificationView/CloseButton.png");

    resize(340, 482);
}

void AccountManagerDialog::resizeEvent(QResizeEvent* eve)
{
    BoxDialogBase::resizeEvent(eve);
    m_subAccountPanel->setGeometry(rect().adjusted(1, customerTitleBarHeight() + 1, -1, -1));
    m_exit->setGeometry(QRect(width() - 34, 1, 34, 31));
}
