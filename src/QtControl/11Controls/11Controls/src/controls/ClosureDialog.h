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

    /** ��ȡ�ڲ�����ָ��
    @return �����ڲ�����ָ��
    */
    LockedAccountPanel* lockedAccountPanel();

protected:
    void resizeEvent(QResizeEvent* eve);
    bool check();

private:
	LockedAccountPanel* mLockedAccountPanel;
};