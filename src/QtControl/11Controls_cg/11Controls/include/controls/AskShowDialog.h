#pragma once
#include "DialogShow.h"
#include "ControlsMacro.h"
#include "NotifyDialog.h"

class COriginalButton;
/** ��ʾ��ʾ��
*/
class AskShowDialog : public NotifyDialog
{
	Q_OBJECT
public:
    /** ���캯��
    */
	AskShowDialog();

public:
    /** ������ʾ
    @param [in] tip ��ʾ����
    */
    void setTip(const QString& tip);

    /** ����ȷ����ť�������ݺͰ��µķ���ֵ
    @param [in] acceptText ��ť��������
    @param [in] acceptDone ���µķ���ֵ
    */
    void setAcceptButton(const QString& acceptText, int32_t acceptDone);

    /** ����ȡ����ť�������ݺͰ��µķ���ֵ
    @param [in] ignoreText ��ť��������
    @param [in] ignoreDone ���µķ���ֵ
    */
    void setIgnoreButton(const QString& ignoreText, int32_t ignoreDone);

protected:
    void resizeEvent(QResizeEvent* eve);
    bool check();

private:
	Label* m_tip;
	COriginalButton* m_accept;
	COriginalButton* m_ignore;
};