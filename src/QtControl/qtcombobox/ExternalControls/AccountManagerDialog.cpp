#include "AccountManagerDialog.h"
#include "Label.h"
#include "AccountDialog.h"
#include "../COriginalButton.h"
#include "../CGeneralStyle.h"
#include "SubAccountPanel.h"
#include "../CExternalTextEdit.h"
#include <QWindow>
#include "ClosureDialog.h"

QString AccountManagerDialog::popAccountDialog()
{
	if (m_accountDialog == nullptr)
	{
		return -1;
	}

	m_accountDialog->clearAccountEdit();
	int32_t dialogId = 0;
	int32_t result = m_accountDialog->exec(dialogId);
	if (result != ACCEPT_BUTTON)
	{
		return "";
	}
	return m_accountDialog->accountEditText();
}

AccountDialog* AccountManagerDialog::accountDialogPtr()
{
	return m_accountDialog;
}

SubAccountPanel* AccountManagerDialog::subAccountPanelPtr()
{
	return m_subAccountPanel;
}

void AccountManagerDialog::popClosureDialog()
{
	if (m_closureDialog == nullptr)
	{
		return;
	}

	int32_t dialogId = 0;
	m_closureDialog->exec(dialogId);
}

ClosureDialog* AccountManagerDialog::closureDialogPtr()
{
	return m_closureDialog;
}

AccountManagerDialog::AccountManagerDialog() :
BoxDialogBase(340, 482, nullptr),
m_accountDialog(nullptr),
m_closureDialog(nullptr),
m_subAccountPanel(new SubAccountPanel(this))
{
	m_subAccountPanel->helpTip()->winId();
	m_subAccountPanel->helpTip()->windowHandle()->setTransientParent(this->windowHandle());

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

	m_closureDialog = new ClosureDialog(nullptr);

	layoutContorls();
}

void AccountManagerDialog::onHide()
{
	setVisible(false);
}

void AccountManagerDialog::layoutContorls()
{
	m_subAccountPanel->setGeometry(rect().adjusted(1,this->moveTitleHeight() + 1,-1,-1));
}

void AccountManagerDialog::resizeEvent(QResizeEvent *e)
{
	BoxDialogBase::resizeEvent(e);
	layoutContorls();
}
