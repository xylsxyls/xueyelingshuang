#pragma once
#include <QtWidgets/QStatusBar>
#include "QtControlsMacro.h"
#include "ControlShow.h"
#include "ControlSelf.h"
#include "ControlFont.h"
#include "ControlBorder.h"
#include "ControlBackground.h"

/** 状态栏复合控件，显示临时和持久状态信息。
*/
class QtControlsAPI StatusBar :
	public ControlShow<QStatusBar>,
	public ControlSelf<QStatusBar>,
	public ControlFont<QStatusBar>,
	public ControlBorderForNormalHoverDisabled<QStatusBar>,
	public ControlBackgroundForNormalHoverDisabled<QStatusBar>
{
public:
	/** 构造函数。
	@param [in] parent 父控件，接管对象所有权。
	*/
	explicit StatusBar(QWidget* parent = nullptr);
};
