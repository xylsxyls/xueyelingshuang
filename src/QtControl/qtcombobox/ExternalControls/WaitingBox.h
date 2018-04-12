#pragma once
#include <QDialog>
#include <stdint.h>
#include "../COriginalDialog.h"

class Label;
/** 定做类，不阻塞代码但是阻塞界面的等待框
*/
class WaitingBox : public COriginalDialog
{
	Q_OBJECT
public:
	/** 构造函数
	@param [in] parent 父窗口指针
	*/
	WaitingBox(QDialog* parent = nullptr);

	/** 析构函数
	*/
	virtual ~WaitingBox();

public:
	enum ExitEnum
	{
		/** 代码关闭
		*/
		CODE_EXIT = -1,

		/** 超时关闭
		*/
		TIMEOUT_EXIT = 0,

		/** 用户使用Esc或Alt+F4关闭
		*/
		USER_EXIT = 1
	};

public:
	/** 设置默认控件，含有关闭按钮，标题，分割线，框大小，背景颜色，背景边框
	@param [in] dialogWidth 窗口宽度
	@param [in] dialogHeight 窗口高度
	*/
	void initForWaitingBox(int32_t dialogWidth, int32_t dialogHeight);

	/** 将本窗口的临时父窗口设为传入窗口的顶层临时父窗口，点击外部时会响应高亮闪烁的父窗口，传入窗口会执行winId
	@param [in] parent 传入窗口的句柄
	*/
	void setParentWindow(QWindow* parent);

	/** 设置标题
	@param [in] title 标题
	*/
	void setTitle(const QString& title);

	/** 设置提示
	@param [in] tip 提示
	*/
	void setTip(const QString& tip);

	/** 设置等待时间
	@param [in] timeRest 剩余时间（毫秒）
	*/
	void setTimeRest(int32_t timeRest);

	/** 设置用户是否可以通过Esc和Alt+F4关闭
	@param [in] userExit 用户是否可以主动关闭
	*/
	void setUserExit(bool userExit);

	/** 调用代码关闭，支持多线程
	*/
	void exit();

Q_SIGNALS:
	void exitSignal();
	void waitingBoxDone(WaitingBox* box, WaitingBox::ExitEnum exitEnum);

protected:
	/** 新增标签文字并设置
	@param [in] text 文本内容
	@param [in] rect 标签相对父窗口所在位置
	@param [in] textColor 文字颜色
	@return 返回标签指针
	*/
	Label* addLabel(const QString& text, const QRect& rect, const QColor& textColor);

	/** 新增提示文字并设置（多行）
	@param [in] text 文本内容
	@param [in] rect 标签相对父窗口所在位置
	@param [in] textColor 文字颜色
	@return 返回标签指针
	*/
	Label* addTip(const QString& text, const QRect& rect, const QColor& textColor);

	/** 添加分割线
	@param [in] point 起始点
	@param [in] length 长度
	@param [in] isHorizon 是否水平
	@param [in] upColor 上层颜色
	@param [in] downColor 下层颜色
	@return 返回分割线指针
	*/
	Label* addSeparator(const QPoint& point, int32_t length, bool isHorizon, const QColor upColor, const QColor downColor);

	bool check();

protected:
	void showEvent(QShowEvent* eve);
	void timerEvent(QTimerEvent* eve);
	void keyPressEvent(QKeyEvent* eve);
	void closeEvent(QCloseEvent* eve);
	void paintEvent(QPaintEvent* eve);

private slots:
	void onNcActiveChanged(const bool& ncActive);
	void onExitSignal();

private:
	QWindow* topWindowHandle(QWindow* parent);

protected:
	int32_t m_timeId;
	Label* m_title;
	Label* m_tip;
	Label* m_sep;
	bool m_cancelEscAltF4;

private:
	int32_t m_timeRest;
	bool m_highLight;
	std::map<void*, int32_t> m_mapResult;
};