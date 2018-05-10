#pragma once
#include "BoxDialogBase.h"
#include "DialogManager.h"
#include "ControlsMacro.h"

class Label;
class ComboBox;
class LineEdit;
/** ����˺ſ�
*/
class ControlsAPI AccountDialog : public BoxDialogBase
{
	Q_OBJECT
public:
	/** ���캯��
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

    /** ����ȷ����ť���µķ���ֵ
    @param [in] result ȷ����ť���µķ���ֵ
    */
    void setAcceptDown(DialogResult result);

    /** ����ȡ����ť���µķ���ֵ
    @param [in] result ȡ����ť���µķ���ֵ
    */
    void setIgnoreDown(DialogResult result);

Q_SIGNALS:
    /** ������ʾ�Ƿ���ʾ
    @param [in] visible �Ƿ���ʾ
    */
	void errorVisible(bool visible);

protected:
	bool eventFilter(QObject* tar, QEvent* eve);
    void resizeEvent(QResizeEvent* eve);
    bool check();

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