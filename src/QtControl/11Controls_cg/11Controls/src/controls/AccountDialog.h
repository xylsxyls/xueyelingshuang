#pragma once
#include "BoxDialogBase.h"
#include "DialogManager.h"
#include "ControlsMacro.h"

class Label;
class ComboBox;
class LineEdit;
class ControlsAPI AccountDialog : public BoxDialogBase
{
	Q_OBJECT
public:
	/** ���캯��
	@param [in] parent �����ھ��
	@param [in] acceptDone ����ȷ���ķ���ֵ
	@param [in] ignoreDone ����ȡ���ķ���ֵ
	*/
	AccountDialog();

	/** �����˺Ŵ�����ʾ�Ƿ���ʾ
	@param [in] visible �Ƿ���ʾ
	*/
	void setErrorVisible(bool visible);

	/** ��������
	*/
	void clearAccountEdit();

	/** ��ȡ���������
	@return �������������
	*/
	QString accountEditText();

    void setAcceptDown(int32_t result);

    void setIgnoreDown(int32_t result);

Q_SIGNALS:
	void errorVisible(bool visible);

protected:
	bool eventFilter(QObject* tar, QEvent* eve);
    void resizeEvent(QResizeEvent* eve);

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