#pragma once
#include "DialogShow.h"
#include "ControlsMacro.h"
#include "PopDialog.h"

/** ��ʾ������
*/
class TipDialog : public PopDialog
{
	Q_OBJECT
public:
    /** ���캯��
    */
	TipDialog();

public:
    /** ������ʾ
    @param [in] tip ��ʾ����
    */
    void setTip(const QString& tip);

    /** ����ȷ����ť����
    @param [in] acceptText ȷ����ť��������
    @param [in] acceptDone ȷ����ť���·���ֵ
    */
    void setAcceptButton(const QString& acceptText, DialogResult acceptDone);

protected:
    void resizeEvent(QResizeEvent* eve);
    bool check();

private:
	Label* m_tip;
	COriginalButton* m_accept;
};