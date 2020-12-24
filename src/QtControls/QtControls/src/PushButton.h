#pragma once
#include <QtWidgets/QPushButton>
#include "ControlFont.h"
#include "ControlSelf.h"
#include "ControlBorder.h"
#include "ControlBackground.h"
#include "QtControlsMacro.h"

class QtControlsAPI PushButton :
	public ControlShow < QPushButton >,
	public ControlFont < QPushButton >,
	public ControlSelf < QPushButton >,
	public ControlBorderForNormalHoverPressedDisabled < QPushButton >,
	public ControlBackgroundForNormalHoverPressedDisabled < QPushButton >
{
	Q_OBJECT
public:
	/** 构造函数
	@param [in] parent 父窗口指针
	*/
	PushButton(QWidget* parent = nullptr);

	/** 析构函数
	*/
	virtual ~PushButton();
};