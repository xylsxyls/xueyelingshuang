#pragma once
#include <QtWidgets/QDoubleSpinBox>
#include "ControlShow.h"
#include "ControlFont.h"
#include "ControlSelf.h"
#include "ControlBorder.h"
#include "ControlBackground.h"
#include "ControlSpinBox.h"
#include "QtControlsMacro.h"

/** 小数微调框控件，封装QDoubleSpinBox本体和上下按钮的常用样式
*/
class QtControlsAPI DoubleSpinBox :
	public ControlShow < QDoubleSpinBox >,
	public ControlFont < QDoubleSpinBox >,
	public ControlSelf < QDoubleSpinBox >,
	public ControlBorderForNormalHoverDisabled < QDoubleSpinBox >,
	public ControlBackgroundForNormalHoverDisabled < QDoubleSpinBox >,
	public ControlSpinBox < QDoubleSpinBox >
{
public:
	/** 构造函数
	@param [in] parent 父窗口指针
	*/
	DoubleSpinBox(QWidget* parent = nullptr);

	/** 析构函数
	*/
	virtual ~DoubleSpinBox();

public:
	/** 设置控件默认样式参数
	*/
	void setDefault();
};