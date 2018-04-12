#pragma once
#include "BoxDialogBase.h"
#include "ControlsMacro.h"

class LockedAccountPanel;
class ControlsAPI ClosureDialog : public BoxDialogBase
{
public:
	/** ���캯��
	@param [in] parent �����ھ��
	*/
	ClosureDialog(QWindow* parent = nullptr);

	LockedAccountPanel* lockedAccountPanel();

private:
	LockedAccountPanel* mLockedAccountPanel;


	void resizeEvent(QResizeEvent *e);
	void layoutContorls();
};