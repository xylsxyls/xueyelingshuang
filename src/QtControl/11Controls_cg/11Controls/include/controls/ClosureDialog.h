#pragma once
#include "BoxDialogBase.h"
#include "ControlsMacro.h"

class LockedAccountPanel;
class ControlsAPI ClosureDialog : public BoxDialogBase
{
public:
	/** 构造函数
	@param [in] parent 父窗口句柄
	*/
	ClosureDialog(QWindow* parent = nullptr);

	LockedAccountPanel* lockedAccountPanel();

private:
	LockedAccountPanel* mLockedAccountPanel;


	void resizeEvent(QResizeEvent *e);
	void layoutContorls();
};