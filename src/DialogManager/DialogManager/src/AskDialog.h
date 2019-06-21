#pragma once
#include "DialogManagerMacro.h"
#include "PopDialog.h"

/** ѯ�ʵ�����
*/
class AskDialog : public PopDialog
{
	Q_OBJECT
public:
    /** ���캯��
    */
    AskDialog();

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

    /** ����ȡ����ť�������ݺͰ��µķ���ֵ
    @param [in] ignoreText ��ť��������
    @param [in] ignoreDone ���µķ���ֵ
    */
    void setIgnoreButton(const QString& ignoreText, DialogResult ignoreDone);

protected:
    void resizeEvent(QResizeEvent* eve);
    bool check();

private:
	Label* m_tip;
	COriginalButton* m_accept;
	COriginalButton* m_ignore;
};