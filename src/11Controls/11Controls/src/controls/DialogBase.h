#pragma once
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

    /** ��������
    */
    virtual ~DialogBase();

public:
	/** ���ô��ں��о��
    @param [in] hasHandle �Ƿ��о��
	*/
	void setNativeWindow(bool hasHandle);

    /** ���õ���ʱ������Ϊ��λ
    @param [in] timeOut ����ʱ
    */
    void setTimeRest(qint32 timeOut);

    /** ����Esc�Ƿ���ã�Escԭ������������
    @param [in] enable �Ƿ����
    */
    void setEscEnable(bool enable);

    /** ��ȡEsc�Ƿ����
    @return ����Esc�Ƿ����
    */
    bool escEnable();

    /** ����Esc��Alt+F4�Ƿ����
    @param [in] enable �Ƿ����
    */
    void setEscAltF4Enable(bool enable);

	/** չʾ���ڣ��������ʱ��Ϊ0��˺�����Ч���������ʱ����������WindowModal��ApplicationModal
	@return �رմ���ʱ���ķ���ֵ
	*/
	qint32 exec();

    /** ��ʾ����
    */
    void show();

    /** ������ǰ���Ƿ񱻰��£����������ź�keyboardAccept
    @param [in] key ������
    */
    void addListenKey(Qt::Key key);

    /** ���ô��ڱ���
    @param [in] title ���ڱ���
    @param [in] color ���ڱ�����ɫ
    @param [in] fontSize �����С
    @param [in] align ���뷽ʽ
    @param [in] origin �������ʱ���ڱ����ƫ����
    @param [in] fontName ������
    */
    void setWindowTiTle(const QString& title,
                        const QColor& color = QColor(255, 255, 255, 255),
                        qint32 fontSize = 12,
                        Qt::Alignment align = Qt::AlignCenter,
                        qint32 origin = 0,
                        const QString& fontName = QString::fromStdWString(L"΢���ź�"));

Q_SIGNALS:
    /** ����ʱʱ��ʣ�࣬ÿ�뷢��һ��
    @param [in] seconds ��ǰ��ʣ������
    */
	void timeRest(qint32 seconds);

    /** ����ʱ����������ź���timeRest(0)֮��
    */
    void timeUp();

    /** ���»س��Ϳո񶼱�ʾaccept����Ҫʹ��Qt::QueuedConnection
    @param [in] tar ��ǰ�������ڵĿؼ�ָ��
    @param [in] key ���¼���ֵ
    */
    void keyboardAccept(QObject* tar, Qt::Key key);

	/** �����Ѿ�ִ�й���ʾ���������źţ�ʹ�ö��з�ʽ������������ʾ����֮��ִ��
	*/
	void alreadyShown();

protected:
	void showEvent(QShowEvent* eve);
	void timerEvent(QTimerEvent* eve);
    void keyPressEvent(QKeyEvent* eve);
    bool eventFilter(QObject* tar, QEvent* eve);
    void resizeEvent(QResizeEvent* eve);
    virtual void escEvent();
    bool check();

private:
    void listenAllControls();
	
protected:
	qint32 m_timeId;
    bool m_escEnable;
    qint32 m_timeRest;
    Label* m_title;
    std::vector<Qt::Key> m_listenKey;
};