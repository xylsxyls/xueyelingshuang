#pragma once
#include "DialogBase.h"
#include <QPropertyAnimation>

/** ���ڻ���ģ�ͣ�ʵ��ͨ�ò���
*/
class DialogShow : public DialogBase
{
	Q_OBJECT
public:
	/** ���캯��
	*/
	DialogShow();

	/** ��������
	*/
	virtual ~DialogShow();

	/** ����Ĭ�Ͽؼ������йرհ�ť�����⣬�ָ��ߣ����С��������ɫ�������߿�
	@param [in] dialogWidth ���ڿ��
	@param [in] dialogHeight ���ڸ߶�
	*/
	void initForExec(int32_t dialogWidth, int32_t dialogHeight);

	/** ����Ĭ�Ͽؼ������б�����
	@param [in] dialogWidth ���
	@param [in] dialogHeight �߶�
	@param [in] typeName ������
	*/
	void initForShow(int32_t dialogWidth, int32_t dialogHeight, const std::string& typeName);

	/** ���ùرհ�ť�Ƿ�ɼ�������Ի����йرհ�ť���ɼ���ͬʱ��ȡ��Esc��Alt+F4��ʹ��
	@param [in] visible �Ƿ�ɼ�
	*/
	void setExitVisible(bool visible);

	/** �رմ���
	@param [in] result ���ڷ���ֵ
	*/
	void done(int32_t result);

	/** �����϶�����߶�
	@param [in] titleHeight �϶��߶�
	*/
	void setMoveTitleHeight(int32_t titleHeight);

	/** ��ȡ�϶�����߶�
	@return �����϶�����߶�
	*/
	int32_t moveTitleHeight();

Q_SIGNALS:
	/** ��֪ͨ�򴰿ڹر�ʱ�����źţ��ڹرն���ִ��֮ǰ�����ź�
	@param [in] dialogId ����ID
	@param [in] result ���ڹر�ʱ�����ķ���ֵ
	@param [in] userType �û��Զ���ֵ
	*/
	void dialogDone(int32_t dialogId, int32_t result, int32_t userType);

	/** ���»س��Ϳո񶼱�ʾaccept
	*/
	void keyboardAccept(QObject* tar, Qt::Key key);

protected:
	void paintEvent(QPaintEvent* eve);
	//void mousePressEvent(QMouseEvent* eve);
	//void mouseMoveEvent(QMouseEvent* eve);
	//void mouseReleaseEvent(QMouseEvent* eve);
	long onNcHitTest(QPoint pt);
	void keyPressEvent(QKeyEvent* eve);
	void closeEvent(QCloseEvent* eve);
	bool nativeEvent(const QByteArray& eventType, void* message, long* result);
	void showEvent(QShowEvent* eve);
	void end();
	bool eventFilter(QObject* tar, QEvent* eve);

private slots:
	void timeUpdate(int32_t timeOut);

private:
	void ncActiveChanged(int32_t wParam);
	
protected:
	COriginalButton* m_exit;
	Label* m_title;
	Label* m_separator;
	Label* m_time;
	Label* m_titleBar;
	Label* m_icon;
	int32_t m_userType;
	bool m_inExit;

private:
	bool m_bPressed;
	bool m_highLight;
	QPoint m_point;
	bool m_cancelEscAltF4;
	bool m_isExec;
	int32_t m_titleHeight;
	QPropertyAnimation m_animation;
	int32_t m_result;
	QRect m_beginRect;
	QRect m_endRect;
	static std::wstring s_countDownString;
};