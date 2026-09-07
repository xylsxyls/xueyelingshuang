#pragma once
#include <QtWidgets/QLabel>
#include "ControlShow.h"
#include "ControlFont.h"
#include "ControlSelf.h"
#include "ControlBorder.h"
#include "ControlBackground.h"
#include "QtControlsMacro.h"

/** 字条或图片控件，该控件的setBackground相关函数没有分割效果，必须使用setBorder相关函数
*/
class QtControlsAPI Label :
	public ControlShow < QLabel >,
	public ControlFont < QLabel >,
	public ControlSelf < QLabel >,
	public ControlBorderForNormalHoverDisabled < QLabel >,
	public ControlBackgroundForNormalHoverDisabled < QLabel >
{
	Q_OBJECT
public:
	/** 构造函数
	@param [in] parent 父窗口指针
	*/
	Label(QWidget* parent = nullptr);

	/** 析构函数
	*/
	virtual ~Label();

public:
	/** 设置行距
	@param [in] lineHeight 行距
	*/
	void setLineHeight(qint32 lineHeight);

	/** 设置双击全屏，一旦设置不可退回
	*/
	void setDoubleClickFullScreen();

Q_SIGNALS:
	/** 左键单击发送信号
	*/
	void leftClicked();

	/** 右键单击发送信号
	*/
	void rightClicked();

	/** 双击之后发送信号
	*/
	void doubleClicked();

	/** 鼠标移动信号
	@param [in] point 相对于父窗口的鼠标位置
	*/
	void mouseMoved(QPoint point);

protected:
	/** 处理鼠标移动，发出mouseMoved信号
	@param [in] eve Qt鼠标事件
	*/
	void mouseMoveEvent(QMouseEvent* eve);

	/** 过滤鼠标点击和双击事件，统一发出点击信号
	@param [in] tar 事件目标对象
	@param [in] eve Qt事件对象
	@return 返回true表示事件已处理，false表示继续分发
	*/
	bool eventFilter(QObject* tar, QEvent* eve);

protected:
	/** 执行双击全屏或还原逻辑
	*/
	void onDoubleClicked();

private:
	// 当前是否全屏
	bool m_full;
	// 是否已经设置过双击全屏功能
	bool m_hasSetFullScreen;
	// 全屏之前的窗口区域
	QRect m_rect;
};