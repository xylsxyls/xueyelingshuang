#pragma once
#include "QtControls/DialogShow.h"

class StockDrawWidget : public DialogShow
{
public:
	/** 构造函数
	*/
	StockDrawWidget();

protected:
	/** 初始化
	*/
	void init();

public:
	/** 设置avg参数
	*/
	void setAvgParam();

protected:
	void resizeEvent(QResizeEvent* eve);
};