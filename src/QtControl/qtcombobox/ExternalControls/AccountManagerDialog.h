#pragma once
#include "BoxDialogBase.h"

class AccountDialog;
class ClosureDialog;
class COriginalButton;
class SubAccountPanel;
class AccountManagerDialog : public BoxDialogBase
{
public:
	/** ���캯��
	*/
	AccountManagerDialog();

	/** ����ע���˺Ŵ���
	@return ������������˺�����������ȡ���򷵻ؿ��ַ���
	*/
	QString popAccountDialog();

	/** ��ȡע���˺Ŵ���ָ��
	@return ����ע���˺Ŵ���ָ��
	*/
	AccountDialog* accountDialogPtr();

	/** ������Ŵ���
	*/
	void popClosureDialog();

	/** ��ȡ��Ŵ���ָ��
	@return ���ط�Ŵ���ָ��
	*/
	ClosureDialog* closureDialogPtr();

	//* �������˺��б�
	SubAccountPanel* subAccountPanelPtr();

private slots:
	void onHide();

private:
	void layoutContorls();
	void resizeEvent(QResizeEvent *e);

private:
	AccountDialog* m_accountDialog;
	ClosureDialog* m_closureDialog;
	COriginalButton* m_hide;
	SubAccountPanel* m_subAccountPanel;
};