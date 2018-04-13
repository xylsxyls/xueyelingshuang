#include "ClosureDialog.h"
#include "LockedAccountPanel.h"

ClosureDialog::ClosureDialog(QWindow* parent) :
BoxDialogBase(372, 296, parent)
	,mLockedAccountPanel(new LockedAccountPanel(this))
{

}

LockedAccountPanel* ClosureDialog::lockedAccountPanel()
{
	return mLockedAccountPanel;
}

void ClosureDialog::resizeEvent(QResizeEvent *e)
{
	BoxDialogBase::resizeEvent(e);
	this->layoutContorls();
}

void ClosureDialog::layoutContorls()
{
	mLockedAccountPanel->setGeometry(rect().adjusted(1,this->moveTitleHeight() + 1,-1,-1));
}
