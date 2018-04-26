#pragma once
#include "BoxDialogBase.h"
#include "ControlsMacro.h"

class LockedAccountPanel;
class ControlsAPI ClosureDialog : public BoxDialogBase
{
public:
	/** ¹¹Ôìº¯Êý
	*/
	ClosureDialog();

protected:
    void resizeEvent(QResizeEvent* eve);

private:
    bool check();

private:
	LockedAccountPanel* mLockedAccountPanel;

	
};