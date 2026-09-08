#pragma once
#include <QtWidgets/QSpinBox>
#include "ControlShow.h"
#include "ControlFont.h"
#include "ControlSelf.h"
#include "ControlBorder.h"
#include "ControlBackground.h"
#include "ControlSpinBox.h"
#include "QtControlsMacro.h"

/** 整数微调框控件，封装QSpinBox本体和上下按钮的常用样式
*/
class QtControlsAPI SpinBox :
	public ControlShow < QSpinBox >,
	public ControlFont < QSpinBox >,
	public ControlSelf < QSpinBox >,
	public ControlBorderForNormalHoverDisabled < QSpinBox >,
	public ControlBackgroundForNormalHoverDisabled < QSpinBox >,
	public ControlSpinBox < QSpinBox >
{
public:
	/** 构造函数
	@param [in] parent 父窗口指针
	*/
	SpinBox(QWidget* parent = nullptr);

	/** 析构函数
	*/
	virtual ~SpinBox();

public:
	/** 设置控件默认样式参数
	*/
	void setDefault();
};