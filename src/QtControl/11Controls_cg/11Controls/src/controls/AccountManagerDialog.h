#pragma once
#include "BoxDialogBase.h"
#include "ControlsMacro.h"

class AccountDialog;
class ClosureDialog;
class SubAccountPanel;
/** �˺Ź����
*/
class ControlsAPI AccountManagerDialog : public BoxDialogBase
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

protected:
    void resizeEvent(QResizeEvent* eve);
    bool check();

private:
	AccountDialog* m_accountDialog;
	ClosureDialog* m_closureDialog;
	SubAccountPanel* m_subAccountPanel;
};