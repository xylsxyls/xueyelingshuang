#pragma once
#include "BoxDialogBase.h"
#include "DialogManager.h"

class Label;
class ComboBox;
class AccountDialog : public BoxDialogBase
{
	Q_OBJECT
public:
	/** ���캯��
	@param [in] parent �����ھ��
	@param [in] acceptDone ����ȷ���ķ���ֵ
	@param [in] ignoreDone ����ȡ���ķ���ֵ
	*/
	AccountDialog(QWindow* parent = nullptr, int32_t acceptDone = ACCEPT_BUTTON, int32_t ignoreDone = IGNORE_BUTTON);

	/** �����˺Ŵ�����ʾ�Ƿ���ʾ
	@param [in] visible �Ƿ���ʾ
	*/
	void setErrorVisible(bool visible);

Q_SIGNALS:
	void errorVisible(bool visible);

protected:
	bool eventFilter(QObject* tar, QEvent* eve);

private slots:
	void accountAccept(QObject* tar, Qt::Key key);

private:
	Label* m_sep;
	Label* m_registerAlt;
	LineEdit* m_account;
	Label* m_errorAccount;
	COriginalButton* m_accept;
	COriginalButton* m_ignore;
};