#pragma once
#include "DialogShow.h"
#include "ControlsMacro.h"
#include "PopDialog.h"

class LineEdit;
/** ���뵯����
*/
class InputDialog : public PopDialog
{
	Q_OBJECT
public:
    /** ���캯��
    */
	InputDialog();

public:
    /** �������������
    @param [in] defaultText �����Ĭ����������
    @param [out] editText ����ر�ʱͨ����ָ��������������
    @param [in] maxLength �������������󳤶�
    */
    void setLineEdit(const QString& defaultText, QString* editText, int32_t maxLength = -1);

    /** ������ʾ
    @param [in] tip ��ʾ����
    */
    void setTip(const QString& tip);

    /** ����ȷ����ť����
    @param [in] acceptText ȷ����ť��������
    @param [in] acceptDone ȷ����ť���·���ֵ
    */
    void setAcceptButton(const QString& acceptText, int32_t acceptDone);

protected:
    void resizeEvent(QResizeEvent* eve);
    void closeEvent(QCloseEvent* eve);
    bool check();

private:
	Label* m_editTip;
	COriginalButton* m_accept;
	LineEdit* m_edit;
	QString* m_editText;
};