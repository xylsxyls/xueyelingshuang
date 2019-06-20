#pragma once
#include "DialogShow.h"

/** ֪ͨ�����
*/
class NotifyDialog : public DialogShow
{
    Q_OBJECT
public:
    /** ���캯��
    */
    NotifyDialog();

    /** ����Ĭ�Ͽؼ������б�����
    @param [in] typeName ������
    */
    void init(const std::string& typeName);

    /** ��ʼ׼���ر�
    */
    void prepareExit();

    /** ���ô��ڱ���
    @param [in] title ���ڱ���
    @param [in] color ���ڱ�����ɫ
    @param [in] fontSize �����С
    @param [in] align ���뷽ʽ
    @param [in] origin �������ʱ���ڱ����ƫ����
    @param [in] fontName ������
    */
    void setWindowTitle(const QString& title,
                        const QColor& color = QColor(221, 213, 198, 255),
                        qint32 fontSize = 12,
                        Qt::Alignment align = Qt::AlignVCenter | Qt::AlignLeft,
                        qint32 origin = 0,
                        const QString& fontName = QString::fromStdWString(L"΢���ź�"));

protected:
    void showEvent(QShowEvent* eve);
    void resizeEvent(QResizeEvent* eve);
    void end();
    bool check();

private:
    DialogResult buttonResult(COriginalButton* button);

    /** ��ʼִ�йرն���
    */
    void beginExitAnimation();

private slots:
    void onTimeUp();

protected:
    QPropertyAnimation m_animation;
    QRect m_beginRect;
    QRect m_endRect;
    Label* m_titleBar;
    Label* m_icon;
    bool m_isShow;
};