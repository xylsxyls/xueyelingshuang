#pragma once
#include <QDialog>
#include <stdint.h>
#include "ControlsMacro.h"
#include "COriginalDialog.h"

class Label;
/** 弹出框基类，增加倒计时和Esc和Alt+F4屏蔽功能，exec增加堵塞模式
*/
class ControlsAPI DialogBase : public COriginalDialog
{
	Q_OBJECT
public:
	/** 构造函数
	*/
	DialogBase();

	/** 设置窗口含有句柄
    @param [in] hasHandle 是否含有句柄
	*/
	void setNativeWindow(bool hasHandle);

    /** 设置窗口弹出时是否抢焦点
    @param [in] focus 是否抢焦点
    */
    void setExistFocus(bool focus);

    /** 设置倒计时，以秒为单位
    @param [in] timeOut 倒计时
    */
    void setTimeRest(int32_t timeOut);

    /** 设置Esc和Alt+F4是否可用
    @param [in] enable 是否可用
    */
    void setEscAltF4Enable(bool enable);

	/** 展示窗口，如果倒计时设为0则此函数无效，会根据临时父窗口设置WindowModal或ApplicationModal
	@return 关闭窗口时给的返回值
	*/
	int32_t exec();

    /** 设置窗口是否可改变大小，暂时只可变为不可改变大小
    @param [in] enable 是否可改变大小
    */
    void setChangeSizeEnable(bool enable = false);

    /** 设置窗口标题
    */
    void setWindowTiTle(const QString& title,
                        const QColor& color = QColor(255, 0, 0, 255),
                        int32_t fontSize = 12,
                        Qt::Alignment align = Qt::AlignCenter,
                        int32_t origin = 0,
                        const QString& fontName = QString::fromStdWString(L"微软雅黑"));

Q_SIGNALS:
	void timeRest(int32_t seconds);
    /** 按下回车和空格都表示accept
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