#pragma once
#include <QtWidgets/QSlider>
#include "ControlShow.h"
#include "ControlFont.h"
#include "ControlSelf.h"
#include "ControlItem.h"
#include "ControlBorder.h"
#include "ControlBackground.h"
#include "QtControlsMacro.h"

/** 滑块控件，封装滑槽和滑块节点的字体、边框、背景样式
*/
class QtControlsAPI Slider :
	public ControlShow < QSlider >,
	public ControlFont < QSlider >,
	public ControlSelf < QSlider >,
	public ControlItem < QSlider >,
	public ControlBorderForNormalHoverDisabled < QSlider >,
	public ControlBackgroundForNormalHoverDisabled < QSlider >,
	public ControlItemBorderForNormalHoverDisabled < QSlider >,
	public ControlItemBackgroundForNormalHoverDisabled < QSlider >
{
	Q_OBJECT
public:
	/** 构造函数
	@param [in] parent 父窗口指针
	*/
	Slider(QWidget* parent = nullptr);

	/** 析构函数
	*/
	virtual ~Slider();
};