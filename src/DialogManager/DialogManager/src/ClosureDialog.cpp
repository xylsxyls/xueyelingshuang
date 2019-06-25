#include "ClosureDialog.h"
#include "LockedAccountPanel.h"
#include "core/coreAPI.h"
#include "DialogHelper.h"

ClosureDialog::ClosureDialog():
mLockedAccountPanel(nullptr)
{
    mLockedAccountPanel = new LockedAccountPanel(this);
    if (!check())
    {
        return;
    }
    resize(372, 296);
}

LockedAccountPanel* ClosureDialog::lockedAccountPanel()
{
    return mLockedAccountPanel;
}

void ClosureDialog::resizeEvent(QResizeEvent* eve)
{
	BoxDialogBase::resizeEvent(eve);
    if (!check())
    {
        return;
    }
    mLockedAccountPanel->setGeometry(DialogHelper::rectValid(rect().adjusted(1, customerTitleBarHeight() + 1, -1, -1)));
}

bool ClosureDialog::check()
{
    return mLockedAccountPanel != nullptr && BoxDialogBase::check();
}
