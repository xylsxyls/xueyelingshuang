#pragma once
#include "BoxDialogBase.h"
#include "DialogManagerMacro.h"

class AccountDialog;
class ClosureDialog;
class SubAccountPanel;
/** �˺Ź����
*/
class DialogManagerAPI AccountManagerDialog : public BoxDialogBase
{
public:
	/** ���캯��
	*/
	AccountManagerDialog();

	/** ����ע���˺Ŵ���
	@return ������������˺�����������ȡ���򷵻ؿ��ַ���
	*/
	QString popAccountDialog();

	/** ������Ŵ���
	*/
	void popClosureDialog();

    /** ��ȡ�ڲ�����ָ��
    @return �����ڲ�����ָ��
    */
    SubAccountPanel* subAccountPanelPtr();

    /** ��ȡ�˺ſ�ָ��
    @return �����˺ſ�ָ��
    */
    AccountDialog* accountDialogPtr();

    /** ��ȡ��Ŵ���ָ��
    @return ���ط�Ŵ���ָ��
    */
    ClosureDialog* closureDialogPtr();

protected:
    void resizeEvent(QResizeEvent* eve);
    bool check();

private:
	AccountDialog* m_accountDialog;
	ClosureDialog* m_closureDialog;
	SubAccountPanel* m_subAccountPanel;
};