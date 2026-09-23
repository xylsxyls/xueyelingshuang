#pragma once
#include <QtWidgets/QToolBar>
#include "QtControlsMacro.h"
#include "ControlShow.h"
#include "ControlSelf.h"
#include "ControlFont.h"
#include "ControlBorder.h"
#include "ControlBackground.h"

/** 工具栏复合控件，集中承接 Qt 的动作布局与溢出行为。
*/
class QtControlsAPI ToolBar :
	public ControlShow<QToolBar>,
	public ControlSelf<QToolBar>,
	public ControlFont<QToolBar>,
	public ControlBorderForNormalHoverDisabled<QToolBar>,
	public ControlBackgroundForNormalHoverDisabled<QToolBar>
{
public:
	/** 构造函数。
	@param [in] parent 父控件，接管对象所有权。
	*/
	explicit ToolBar(QWidget* parent = nullptr);
};
