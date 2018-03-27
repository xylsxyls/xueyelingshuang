#pragma once
#include "BoxDialogBase.h"

class LockedAccountPanel;

class ClosureDialog : public BoxDialogBase
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