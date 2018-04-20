#pragma once
#include "DialogShow.h"

class NotifyDialog : public DialogShow
{
    Q_OBJECT
public:
    NotifyDialog();
    /** ����Ĭ�Ͽؼ������б�����
    @param [in] typeName ������
    */
    void init(const std::string& typeName);

    /** �رմ���
    @param [in] result ���ڷ���ֵ
    */
    void done(int32_t result);

protected:
    void showEvent(QShowEvent* eve);
    void resizeEvent(QResizeEvent* eve);
    void end();

protected:
    QPropertyAnimation m_animation;
    QRect m_beginRect;
    QRect m_endRect;
    Label* m_titleBar;
    Label* m_icon;
};