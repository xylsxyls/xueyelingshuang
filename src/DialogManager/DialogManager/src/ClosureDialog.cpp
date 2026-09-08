#include "ClosureDialog.h"
#include "LockedAccountPanel.h"
#include "DialogHelper.h"

ClosureDialog::ClosureDialog():
m_lockedAccountPanel(nullptr)
{
    m_lockedAccountPanel = new LockedAccountPanel(this);
    if (!check())
    {
        return;
    }
    resize(372, 296);
}

LockedAccountPanel* ClosureDialog::lockedAccountPanel()
{
    return m_lockedAccountPanel;
}

void ClosureDialog::resizeEvent(QResizeEvent* eve)
{
	if (eve == nullptr)
	{
		return;
	}
	BoxDialogBase::resizeEvent(eve);
    if (!check())
    {
        return;
    }
    m_lockedAccountPanel->setGeometry(DialogHelper::rectValid(rect().adjusted(1, customerTitleBarHeight() + 1, -1, -1)));
}

bool ClosureDialog::check()
{
    return m_lockedAccountPanel != nullptr && BoxDialogBase::check();
}