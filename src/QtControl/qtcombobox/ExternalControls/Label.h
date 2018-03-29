#pragma once
#include <QtWidgets/QLabel>
#include "ControlShow.h"
#include "ControlFont.h"
#include "ControlSelf.h"
#include "ControlBorder.h"
#include "ControlBackground.h"

/** 字条或图片控件，该控件的setBackground相关函数没有分割效果，必须使用setBorder相关函数
*/
class Label : 
	public ControlShow < QLabel >,
	public ControlFont < QLabel >,
	public ControlSelf < QLabel >,
	public ControlBorderForNormalHoverDisabled < QLabel >,
	public ControlBackgroundForNormalHoverDisabled < QLabel >
{
	Q_OBJECT
public:
	/** 构造函数
	@param [in] parent 父窗口指针
	*/
	Label(QWidget* parent = nullptr);

	/** 析构函数
	*/
	virtual ~Label();

public:
	
};