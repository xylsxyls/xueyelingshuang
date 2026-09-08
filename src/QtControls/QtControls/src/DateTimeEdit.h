#pragma once
#include <QtWidgets/QDateTimeEdit>
#include "ControlShow.h"
#include "ControlFont.h"
#include "ControlSelf.h"
#include "ControlBorder.h"
#include "ControlBackground.h"
#include "ControlSpinBox.h"
#include "QtControlsMacro.h"

/** 日期时间编辑控件，封装QDateTimeEdit本体和上下按钮的常用样式
*/
class QtControlsAPI DateTimeEdit :
	public ControlShow < QDateTimeEdit >,
	public ControlFont < QDateTimeEdit >,
	public ControlSelf < QDateTimeEdit >,
	public ControlBorderForNormalHoverDisabled < QDateTimeEdit >,
	public ControlBackgroundForNormalHoverDisabled < QDateTimeEdit >,
	public ControlSpinBox < QDateTimeEdit >
{
public:
	/** 构造函数
	@param [in] parent 父窗口指针
	*/
	DateTimeEdit(QWidget* parent = nullptr);

	/** 析构函数
	*/
	virtual ~DateTimeEdit();

public:
	/** 设置控件默认样式参数
	*/
	void setDefault();
};