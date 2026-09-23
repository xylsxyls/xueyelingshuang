#pragma once
#include <QtWidgets/QSplitter>
#include "QtControlsMacro.h"
#include "ControlShow.h"
#include "ControlSelf.h"
#include "ControlFont.h"
#include "ControlBorder.h"
#include "ControlBackground.h"

/** 可拖动分隔容器，保留 Qt 的尺寸分配和键盘行为。
*/
class QtControlsAPI Splitter :
	public ControlShow<QSplitter>,
	public ControlSelf<QSplitter>,
	public ControlFont<QSplitter>,
	public ControlBorderForNormalHoverDisabled<QSplitter>,
	public ControlBackgroundForNormalHoverDisabled<QSplitter>
{
public:
	/** 构造函数。
	@param [in] parent 父控件，接管对象所有权。
	*/
	explicit Splitter(QWidget* parent = nullptr);
};
