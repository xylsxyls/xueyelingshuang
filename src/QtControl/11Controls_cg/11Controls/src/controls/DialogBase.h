#pragma once
#include <QDialog>
#include <stdint.h>
#include "ControlsMacro.h"
#include "COriginalDialog.h"

class Label;
/** ��������࣬���ӵ���ʱ��Esc��Alt+F4���ι��ܣ�exec���Ӷ���ģʽ
*/
class ControlsAPI DialogBase : public COriginalDialog
{
	Q_OBJECT
public:
	/** ���캯��
	*/
	DialogBase();

	/** ���ô��ں��о��
    @param [in] hasHandle �Ƿ��о��
	*/
	void setNativeWindow(bool hasHandle);

    /** ���ô��ڵ���ʱ�Ƿ�������
    @param [in] focus �Ƿ�������
    */
    void setExistFocus(bool focus);

    /** ���õ���ʱ������Ϊ��λ
    @param [in] timeOut ����ʱ
    */
    void setTimeRest(int32_t timeOut);

    /** ����Esc��Alt+F4�Ƿ����
    @param [in] enable �Ƿ����
    */
    void setEscAltF4Enable(bool enable);

	/** չʾ���ڣ��������ʱ��Ϊ0��˺�����Ч���������ʱ����������WindowModal��ApplicationModal
	@return �رմ���ʱ���ķ���ֵ
	*/
	int32_t exec();

    /** ���ô����Ƿ�ɸı��С����ʱֻ�ɱ�Ϊ���ɸı��С
    @param [in] enable �Ƿ�ɸı��С
    */
    void setChangeSizeEnable(bool enable = false);

    /** ���ô��ڱ���
    */
    void setWindowTiTle(const QString& title,
                        const QColor& color = QColor(255, 0, 0, 255),
                        int32_t fontSize = 12,
                        Qt::Alignment align = Qt::AlignCenter,
                        int32_t origin = 0,
                        const QString& fontName = QString::fromStdWString(L"΢���ź�"));

Q_SIGNALS:
	void timeRest(int32_t seconds);
    /** ���»س��Ϳո񶼱�ʾaccept
    */
    void keyboardAccept(QObject* tar, Qt::Key key);

protected:
	void showEvent(QShowEvent* eve);
	void timerEvent(QTimerEvent* eve);
    void keyPressEvent(QKeyEvent* eve);
    void closeEvent(QCloseEvent* eve);
    bool eventFilter(QObject* tar, QEvent* eve);
    void resizeEvent(QResizeEvent* eve);

private:
    bool check();
	
protected:
	int32_t m_timeId;
    bool m_escAltF4Enable;
    int32_t m_timeRest;
    Label* m_title;
};