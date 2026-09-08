#pragma once
#include <QtWidgets/QTimeEdit>
#include "ControlShow.h"
#include "ControlFont.h"
#include "ControlSelf.h"
#include "ControlBorder.h"
#include "ControlBackground.h"
#include "ControlSpinBox.h"
#include "QtControlsMacro.h"

/** 时间编辑控件，封装QTimeEdit本体和上下按钮的常用样式
*/
class QtControlsAPI TimeEdit :
	public ControlShow < QTimeEdit >,
	public ControlFont < QTimeEdit >,
	public ControlSelf < QTimeEdit >,
	public ControlBorderForNormalHoverDisabled < QTimeEdit >,
	public ControlBackgroundForNormalHoverDisabled < QTimeEdit >,
	public ControlSpinBox < QTimeEdit >
{
public:
	/** 构造函数
	@param [in] parent 父窗口指针
	*/
	TimeEdit(QWidget* parent = nullptr);

	/** 析构函数
	*/
	virtual ~TimeEdit();

public:
	/** 设置控件默认样式参数
	*/
	void setDefault();
};