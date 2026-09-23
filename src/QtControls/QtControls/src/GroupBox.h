#pragma once
#include <QtWidgets/QGroupBox>
#include "QtControlsMacro.h"
#include "ControlShow.h"
#include "ControlSelf.h"
#include "ControlFont.h"
#include "ControlBorder.h"
#include "ControlBackground.h"

/** 分组容器，统一封装标题和子控件布局。
*/
class QtControlsAPI GroupBox :
	public ControlShow<QGroupBox>,
	public ControlSelf<QGroupBox>,
	public ControlFont<QGroupBox>,
	public ControlBorderForNormalHoverDisabled<QGroupBox>,
	public ControlBackgroundForNormalHoverDisabled<QGroupBox>
{
public:
	/** 构造函数。
	@param [in] parent 父控件，接管对象所有权。
	*/
	explicit GroupBox(QWidget* parent = nullptr);
};
