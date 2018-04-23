#pragma once
#include "DialogShow.h"

class PopDialog : public DialogShow
{
    Q_OBJECT
public:
    PopDialog();
    /** ����Ĭ�Ͽؼ������йرհ�ť�����⣬�ָ��ߣ����С��������ɫ�������߿�
    @param [in] dialogWidth ���ڿ��
    @param [in] dialogHeight ���ڸ߶�
    */
    void init();

    /** �رմ���
    @param [in] result ���ڷ���ֵ
    */
    void done(int32_t result);

    /** ���ô��ڱ���
    */
    void setWindowTiTle(const QString& title,
                        const QColor& color = QColor(163, 175, 191, 255),
                        int32_t fontSize = 12,
                        Qt::Alignment align = Qt::AlignCenter,
                        int32_t origin = 0,
                        const QString& fontName = QString::fromStdWString(L"΢���ź�"));

protected:
    void paintEvent(QPaintEvent* eve);
    void showEvent(QShowEvent* eve);
    void resizeEvent(QResizeEvent* eve);

private slots:
    void onNcActiveChanged(const bool& ncActive);

private:
    bool m_highLight;
    Separator* m_separator;
};