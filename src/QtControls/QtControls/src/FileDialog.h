#pragma once
#include <QtWidgets/QFileDialog>
#include "QtControlsMacro.h"
#include "ControlShow.h"
#include "ControlSelf.h"
#include "ControlFont.h"
#include "ControlBorder.h"
#include "ControlBackground.h"

/** 可嵌入的文件选择复合控件，由外部弹窗管理器负责显示和生命周期。
*/
class QtControlsAPI FileDialog :
	public ControlShow<QFileDialog>,
	public ControlSelf<QFileDialog>,
	public ControlFont<QFileDialog>,
	public ControlBorderForNormalHoverDisabled<QFileDialog>,
	public ControlBackgroundForNormalHoverDisabled<QFileDialog>
{
public:
	/** 构造函数。
	@param [in] parent 父控件，接管对象所有权。
	*/
	explicit FileDialog(QWidget* parent = nullptr);
};
