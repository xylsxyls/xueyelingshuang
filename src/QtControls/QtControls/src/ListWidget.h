#pragma once
#include <QtWidgets/QListWidget>
#include "ControlFont.h"
#include "ControlItem.h"
#include "ControlSelf.h"
#include "ControlShow.h"
#include "ControlBorder.h"
#include "ControlBackground.h"
#include "QtControlsMacro.h"

/** 列表控件，封装列表自身和列表项的字体、边框、背景以及点击禁用控制
*/
class QtControlsAPI ListWidget :
	public ControlShow < QListWidget >,
	public ControlFont < QListWidget >,
	public ControlItem < QListWidget >,
	public ControlSelf < QListWidget >,
	public ControlBorderForNormal < QListWidget >,
	public ControlBackgroundForNormal < QListWidget >,
	public ControlItemBorderForNormalHoverDisabled < QListWidget >,
	public ControlItemBackgroundForNormalHoverDisabled < QListWidget >
{
	Q_OBJECT
public:
	/** 构造函数
	@param [in] parent 父窗口指针
	*/
	ListWidget(QWidget* parent = nullptr);

	/** 析构函数
	*/
	virtual ~ListWidget();

public:
	/** 设置最大高度
	@param [in] maxHeight 最大高度
	@param [in] rePaint 是否立即重画
	*/
	void setMaxHeight(qint32 maxHeight, bool rePaint = false);

	/** 是否使点击失效
	@param [in] enable 是否有效
	*/
	void setClickEnable(bool enable);

Q_SIGNALS:
	/** 列表项被鼠标按下时发送信号
	@param [in] item 被按下的列表项
	*/
	void itemPressed(QListWidgetItem* item);

protected:
	/** 处理鼠标按下事件，支持全局禁用点击后拦截列表项选择
	@param [in] eve Qt鼠标事件
	*/
	void mousePressEvent(QMouseEvent* eve);

private:
	// 当前列表是否允许鼠标点击
	bool m_click;
};