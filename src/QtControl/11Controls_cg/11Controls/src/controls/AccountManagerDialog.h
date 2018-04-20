#pragma once
#include "BoxDialogBase.h"
#include "ControlsMacro.h"

class AccountDialog;
class ClosureDialog;
class SubAccountPanel;
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

private:
    void resizeEvent(QResizeEvent* eve);

private:
	AccountDialog* m_accountDialog;
	ClosureDialog* m_closureDialog;
	SubAccountPanel* m_subAccountPanel;
};