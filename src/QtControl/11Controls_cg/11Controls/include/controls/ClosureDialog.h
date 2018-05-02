#pragma once
#include "BoxDialogBase.h"
#include "ControlsMacro.h"

class LockedAccountPanel;
/** ·âºÅ¿ò
*/
class ControlsAPI ClosureDialog : public BoxDialogBase
{
public:
	/** ¹¹Ôìº¯Êý
	*/
	ClosureDialog();

protected:
    void resizeEvent(QResizeEvent* eve);
    bool check();

private:
	LockedAccountPanel* mLockedAccountPanel;
};