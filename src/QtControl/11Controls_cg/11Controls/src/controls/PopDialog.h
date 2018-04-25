#pragma once
#include "DialogShow.h"

class PopDialog : public DialogShow
{
    Q_OBJECT
public:
    /** ���캯��
    */
    PopDialog();

    /** ���ô��ڱ���
    @param [in] title ���ڱ���
    @param [in] color ���ڱ�����ɫ
    @param [in] fontSize �����С
    @param [in] align ���뷽ʽ
    @param [in] origin �������ʱ���ڱ����ƫ����
    @param [in] fontName ������
    */
    void setWindowTiTle(const QString& title,
                        const QColor& color = QColor(163, 175, 191, 255),
                        int32_t fontSize = 12,
                        Qt::Alignment align = Qt::AlignCenter,
                        int32_t origin = 0,
                        const QString& fontName = QString::fromStdWString(L"΢���ź�"));

public slots:
    void endDialog();
    void onKeyboardAccept(QObject* tar, Qt::Key key);
    void onAltF4Pressed();

protected:
    void paintEvent(QPaintEvent* eve);
    void showEvent(QShowEvent* eve);
    void resizeEvent(QResizeEvent* eve);
    void closeEvent(QCloseEvent* eve);

private slots:
    void onNcActiveChanged(const bool& ncActive);
    void onTimeUp();

private:
    void init();

private:
    bool m_highLight;
    Separator* m_separator;
};