#pragma once
#include "ControlSelf.h"
#include "ControlFont.h"
#include "ControlBorder.h"
#include "ControlBackground.h"
#include <QtWidgets/QLineEdit>

class LineEdit :
	public ControlSelf < QLineEdit >,
	public ControlFont < QLineEdit >,
	public ControlBorderForNormalHoverDisabled < QLineEdit >,
	public ControlBackgroundForNormalHoverDisabled < QLineEdit >
{
public:
	/** 构造函数
	@param [in] parent 父窗口指针
	*/
	LineEdit(QWidget* parent = NULL);

	/** 析构函数
	*/
	virtual ~LineEdit();

public:

private:
	/** 常用初始化
	*/
	void init();
};