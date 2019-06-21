#pragma once
#include "BoxDialogBase.h"
#include "DialogManagerMacro.h"

class LockedAccountPanel;
/** ��ſ�
*/
class DialogManagerAPI ClosureDialog : public BoxDialogBase
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