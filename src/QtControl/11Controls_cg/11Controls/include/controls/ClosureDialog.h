#pragma once
#include "BoxDialogBase.h"
#include "ControlsMacro.h"

class LockedAccountPanel;
/** ��ſ�
*/
class ControlsAPI ClosureDialog : public BoxDialogBase
{
public:
	/** ���캯��
	*/
	ClosureDialog();

protected:
    void resizeEvent(QResizeEvent* eve);
    bool check();

private:
	LockedAccountPanel* mLockedAccountPanel;
};