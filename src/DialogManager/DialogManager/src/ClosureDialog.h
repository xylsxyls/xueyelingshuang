#pragma once
#include "BoxDialogBase.h"
#include "DialogManagerMacro.h"

class LockedAccountPanel;
/** 封号框
*/
class DialogManagerAPI ClosureDialog : public BoxDialogBase
{
public:
	/** 构造函数
	*/
	ClosureDialog();

    /** 获取内部界面指针
    @return 返回内部界面指针
    */
    LockedAccountPanel* lockedAccountPanel();

protected:
    void resizeEvent(QResizeEvent* eve);
    bool check();

private:
	LockedAccountPanel* mLockedAccountPanel;
};