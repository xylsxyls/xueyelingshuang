#pragma once
#include "DialogManagerMacro.h"
#include "NotifyDialog.h"

/** ��ʾ��ʾ��
*/
class TipShowDialog : public NotifyDialog
{
	Q_OBJECT
public:
    /** ���캯��
    */
	TipShowDialog();

public:
    /** ������ʾ
    @param [in] tip ��ʾ����
    */
    void setTip(const QString& tip);

    /** ����ȷ����ť�������ݺͰ��µķ���ֵ
    @param [in] acceptText ��ť��������
    @param [in] acceptDone ���µķ���ֵ
    */
    void setAcceptButton(const QString& acceptText, DialogResult acceptDone);

protected:
    void resizeEvent(QResizeEvent* eve);
    bool check();

private:
	Label* m_tip;
	COriginalButton* m_accept;
};