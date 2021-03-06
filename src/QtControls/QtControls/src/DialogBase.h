#pragma once
#include "QtControlsMacro.h"
#include "COriginalDialog.h"

class Label;
/** 弹出框基类，增加倒计时和Esc和Alt+F4屏蔽功能，exec增加堵塞模式
*/
class QtControlsAPI DialogBase : public COriginalDialog
{
	Q_OBJECT
public:
	/** 构造函数
	*/
	DialogBase();

    /** 析构函数
    */
    virtual ~DialogBase();

public:
	/** 设置窗口含有句柄
    @param [in] hasHandle 是否含有句柄
	*/
	void setNativeWindow(bool hasHandle);

    /** 设置倒计时，以秒为单位
    @param [in] timeOut 倒计时
    */
    void setTimeRest(qint32 timeOut);

    /** 设置Esc是否可用，Esc原本作用是隐藏
    @param [in] enable 是否可用
    */
    void setEscEnable(bool enable);

    /** 获取Esc是否可用
    @return 返回Esc是否可用
    */
    bool escEnable();

    /** 设置Esc和Alt+F4是否可用
    @param [in] enable 是否可用
    */
    void setEscAltF4Enable(bool enable);

	/** 展示窗口，如果倒计时设为0则此函数无效，会根据临时父窗口设置WindowModal或ApplicationModal
	@return 关闭窗口时给的返回值
	*/
	qint32 exec();

    /** 显示窗口
    */
    void show();

    /** 监听当前键是否被按下，按下则发送信号keyboardAccept
    @param [in] key 键盘码
    */
    void addListenKey(Qt::Key key);

    /** 设置窗口标题
    @param [in] title 窗口标题
    @param [in] color 窗口标题颜色
    @param [in] fontSize 字体大小
    @param [in] align 对齐方式
    @param [in] origin 当左对齐时窗口标题的偏移量
    @param [in] fontName 字体名
    */
    void setWindowTitle(const QString& title,
                        const QColor& color = QColor(255, 255, 255, 255),
                        qint32 fontSize = 12,
                        Qt::Alignment align = Qt::AlignCenter,
                        qint32 origin = 0,
                        const QString& fontName = QString::fromStdWString(L"微软雅黑"));

Q_SIGNALS:
    /** 倒计时时间剩余，每秒发送一次
    @param [in] seconds 当前还剩多少秒
    */
	void timeRest(qint32 seconds);

    /** 倒计时结束，这个信号在timeRest(0)之后发
    */
    void timeUp();

    /** 按下回车和空格都表示accept，需要使用Qt::QueuedConnection
    @param [in] tar 当前焦点所在的控件指针
    @param [in] key 按下键的值
    */
    void keyboardAccept(QObject* tar, Qt::Key key);

	/** 窗口已经执行过显示操作发送信号，使用队列方式连接则是在显示界面之后执行
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