#include "AccountManagerDialog.h"
#include "QtControls/Label.h"
#include "AccountDialog.h"
#include "QtControls/COriginalButton.h"
#include "QtControls/CGeneralStyle.h"
#include "SubAccountPanel.h"
#include "QtControls/CExternalTextEdit.h"
#include <QWindow>
#include "ClosureDialog.h"
#include "DialogType.h"
#include "DialogHelper.h"

QString AccountManagerDialog::popAccountDialog()
{
    if (!check())
    {
        return "";
    }

	m_accountDialog->clearAccountEdit();
    m_accountDialog->setTransientWindow(windowHandle());
	qint32 result = m_accountDialog->exec();
	if (result != ACCEPT_BUTTON)
	{
		return "";
	}
	return m_accountDialog->accountEditText();
}

void AccountManagerDialog::popClosureDialog()
{
    if (!check())
    {
        return;
    }
	m_closureDialog->exec();
}

SubAccountPanel* AccountManagerDialog::subAccountPanelPtr()
{
    return m_subAccountPanel;
}

AccountDialog* AccountManagerDialog::accountDialogPtr()
{
    return m_accountDialog;
}

ClosureDialog* AccountManagerDialog::closureDialogPtr()
{
    return m_closureDialog;
}

AccountManagerDialog::AccountManagerDialog() :
m_accountDialog(nullptr),
m_closureDialog(nullptr),
m_subAccountPanel(nullptr)
{
    m_accountDialog = new AccountDialog();
    m_closureDialog = new ClosureDialog();
    m_subAccountPanel = new SubAccountPanel(this);

    if (!check())
    {
        return;
    }

	m_subAccountPanel->helpTip()->winId();
	m_subAccountPanel->helpTip()->windowHandle()->setTransientParent(windowHandle());

    m_exit->setBkgImage(CGeneralStyle::instance()->platformResourcePath() + "/Common/Image/NotificationView/CloseButton.png");

    setFixedSize(340, 482);
}

void AccountManagerDialog::resizeEvent(QResizeEvent* eve)
{
    BoxDialogBase::resizeEvent(eve);
    if (!check())
    {
        return;
    }
    m_subAccountPanel->setGeometry(DialogHelper::rectValid(rect().adjusted(1, customerTitleBarHeight() + 1, -1, -1)));
    m_exit->setGeometry(QRect(width() - 34, 1, 34, 31));
}

bool AccountManagerDialog::check()
{
    return m_accountDialog != nullptr &&
        m_closureDialog != nullptr &&
        m_subAccountPanel != nullptr &&
        BoxDialogBase::check();
}