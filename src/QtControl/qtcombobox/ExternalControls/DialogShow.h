#pragma once
#include "DialogBase.h"
#include <QPropertyAnimation>

/** 窗口基本模型，实现通用部分
*/
class DialogShow : public DialogBase
{
	Q_OBJECT
public:
	/** 构造函数
	*/
	DialogShow();

	/** 析构函数
	*/
	virtual ~DialogShow();

	/** 设置默认控件，含有关闭按钮，标题，分割线，框大小，背景颜色，背景边框
	@param [in] dialogWidth 窗口宽度
	@param [in] dialogHeight 窗口高度
	*/
	void initForExec(int32_t dialogWidth, int32_t dialogHeight);

	/** 设置默认控件，含有标题栏
	@param [in] dialogWidth 宽度
	@param [in] dialogHeight 高度
	@param [in] typeName 类型名
	*/
	void initForShow(int32_t dialogWidth, int32_t dialogHeight, const std::string& typeName);

	/** 设置关闭按钮是否可见，如果对话框中关闭按钮不可见则同时会取消Esc和Alt+F4的使用
	@param [in] visible 是否可见
	*/
	void setExitVisible(bool visible);

	/** 关闭窗口
	@param [in] result 窗口返回值
	*/
	void done(int32_t result);

	/** 设置拖动标题高度
	@param [in] titleHeight 拖动高度
	*/
	void setMoveTitleHeight(int32_t titleHeight);

	/** 获取拖动标题高度
	@return 返回拖动标题高度
	*/
	int32_t moveTitleHeight();

Q_SIGNALS:
	/** 当通知框窗口关闭时发送信号，在关闭动画执行之前发送信号
	@param [in] dialogId 窗口ID
	@param [in] result 窗口关闭时所给的返回值
	@param [in] userType 用户自定义值
	*/
	void dialogDone(int32_t dialogId, int32_t result, int32_t userType);

	/** 按下回车和空格都表示accept
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