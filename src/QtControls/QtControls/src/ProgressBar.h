#pragma once
#include <QtWidgets/QProgressBar>
#include "ControlShow.h"
#include "ControlFont.h"
#include "ControlSelf.h"
#include "ControlItem.h"
#include "ControlBorder.h"
#include "ControlBackground.h"
#include "QtControlsMacro.h"

class QtControlsAPI ProgressBar :
	public ControlShow < QProgressBar >,
	public ControlFont < QProgressBar >,
	public ControlSelf < QProgressBar >,
	public ControlItem < QProgressBar >,
	public ControlBorderForNormalHoverDisabled < QProgressBar >,
	public ControlBackgroundForNormalHoverDisabled < QProgressBar >,
	public ControlItemBorderForNormalHoverDisabled < QProgressBar >,
	public ControlItemBackgroundForNormalHoverDisabled < QProgressBar >
{
	Q_OBJECT
public:
	/** 构造函数
	@param [in] parent 父窗口指针
	*/
	ProgressBar(QWidget* parent = nullptr);

	/** 析构函数
	*/
	virtual ~ProgressBar();
};