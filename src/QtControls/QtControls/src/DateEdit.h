#pragma once
#include <QtWidgets/QDateEdit>
#include "ControlShow.h"
#include "ControlFont.h"
#include "ControlSelf.h"
#include "ControlBorder.h"
#include "ControlBackground.h"
#include "ControlSpinBox.h"
#include "QtControlsMacro.h"

/** 日期编辑控件，封装QDateEdit本体、上下按钮和日历弹出开关
*/
class QtControlsAPI DateEdit :
	public ControlShow < QDateEdit >,
	public ControlFont < QDateEdit >,
	public ControlSelf < QDateEdit >,
	public ControlBorderForNormalHoverDisabled < QDateEdit >,
	public ControlBackgroundForNormalHoverDisabled < QDateEdit >,
	public ControlSpinBox < QDateEdit >
{
public:
	/** 构造函数
	@param [in] parent 父窗口指针
	*/
	DateEdit(QWidget* parent = nullptr);

	/** 析构函数
	*/
	virtual ~DateEdit();

public:
	/** 设置控件默认样式参数
	*/
	void setDefault();
};