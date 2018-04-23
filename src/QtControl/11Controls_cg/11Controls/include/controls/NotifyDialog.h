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

    /** ���ô��ڱ���
    */
    void setWindowTiTle(const QString& title,
                        const QColor& color = QColor(221, 213, 198, 255),
                        int32_t fontSize = 12,
                        Qt::Alignment align = Qt::AlignVCenter | Qt::AlignLeft,
                        int32_t origin = 0,
                        const QString& fontName = QString::fromStdWString(L"΢���ź�"));

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
    bool m_isShow;
};