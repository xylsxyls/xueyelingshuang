#pragma once
#include "DialogManagerMacro.h"
#include "PopDialog.h"

/** �ȴ�������
*/
class WaitDialog : public PopDialog
{
	Q_OBJECT
public:
    /** ���캯��
    */
    WaitDialog();

    /** ������ʾ����
    @param [in] tip ��ʾ����
    */
    void setTip(const QString& tip);

protected:
    void resizeEvent(QResizeEvent* eve);
    bool check();

private:
	Label* m_tip;
};