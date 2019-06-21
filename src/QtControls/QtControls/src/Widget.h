#pragma once
#include <QWidget>
#include "ControlShow.h"
#include "ControlFont.h"
#include "ControlSelf.h"
#include "ControlBorder.h"
#include "ControlBackground.h"
#include "QtControlsMacro.h"

/** 窗口控件，该控件的setBorder相关函数没有效果，setBackground函数只能设置颜色
*/
class QtControlsAPI Widget :
	public ControlShow < QWidget >,
	public ControlFont < QWidget >,
	public ControlSelf < QWidget >,
	public ControlBackgroundForNormal < QWidget >
{
	Q_OBJECT
public:
	/** 构造函数
	@param [in] parent 父窗口指针
	*/
	Widget(QWidget* parent = nullptr);

	/** 析构函数
	*/
	virtual ~Widget();
};