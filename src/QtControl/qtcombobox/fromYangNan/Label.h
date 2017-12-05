#pragma once
#include <QtWidgets/QLabel>
#include "ControlShow.h"
#include "ControlFont.h"
#include "ControlBorder.h"
#include "ControlBackground.h"

class Label : 
	public ControlShow < QLabel >,
	public ControlFont < QLabel >,
	public ControlBorderForNormalHoverDisabled < QLabel >,
	public ControlBackgroundForNormalHoverDisabled < QLabel >
{
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