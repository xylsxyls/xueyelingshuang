#include "ClosureDialog.h"
#include "LockedAccountPanel.h"

ClosureDialog::ClosureDialog():
mLockedAccountPanel(nullptr)
{
    mLockedAccountPanel = new LockedAccountPanel(this);
    resize(372, 296);
}

void ClosureDialog::resizeEvent(QResizeEvent* eve)
{
	BoxDialogBase::resizeEvent(eve);
    mLockedAccountPanel->setGeometry(rect().adjusted(1, customerTitleBarHeight() + 1, -1, -1));
}