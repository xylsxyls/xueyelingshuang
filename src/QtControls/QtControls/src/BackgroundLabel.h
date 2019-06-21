#pragma once
#include "Label.h"
#include "QtControlsMacro.h"

/** 透明窗口背景控件
*/
class QtControlsAPI BackgroundLabel : public Label
{
	Q_OBJECT
public:
	/** 构造函数
	@param [in] parent 父窗口指针
	*/
	BackgroundLabel(QWidget* parent = nullptr);

	/** 析构函数
	*/
	virtual ~BackgroundLabel();
};