#pragma once
#include "DialogShow.h"

/** �������
*/
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
                        qint32 fontSize = 12,
                        Qt::Alignment align = Qt::AlignCenter,
                        qint32 origin = 0,
                        const QString& fontName = QString::fromStdWString(L"΢���ź�"));

    

    /** ���ó�̬�͸����߿���ɫ��������paintEvent�����ڲ����Ի����߿���
    @param [in] normalColor ��̬��ɫ
    @param [in] highLightColor ������ɫ
    */
    void setBorderNormalHighLightColor(const QColor& normalColor, const QColor& highLightColor = QColor(0, 0, 0, 0));

public slots:
	/** ��ť���֮�󴰿ڹر�
	*/
    void endDialog();

protected:
    void paintEvent(QPaintEvent* eve);
    void showEvent(QShowEvent* eve);
    void resizeEvent(QResizeEvent* eve);
    void closeEvent(QCloseEvent* eve);
    bool check();

private slots:
    void onNcActiveChanged(const bool& ncActive);
    void onTimeUp();

private:
    void init();

protected:
    Separator* m_separator;

private:
    bool m_highLight;
    QColor m_normalColor;
    QColor m_highLightColor;
};