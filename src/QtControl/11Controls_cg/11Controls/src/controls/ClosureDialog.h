#pragma once
#include "BoxDialogBase.h"
#include "ControlsMacro.h"

class LockedAccountPanel;
class ControlsAPI ClosureDialog : public BoxDialogBase
{
public:
	/** ���캯��
	*/
	ClosureDialog();

protected:
    void resizeEvent(QResizeEvent* eve);

private:
    bool check();

private:
	LockedAccountPanel* mLockedAccountPanel;

	
};