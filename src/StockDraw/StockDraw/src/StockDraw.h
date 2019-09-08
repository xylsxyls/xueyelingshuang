#pragma once
#include "StockDrawMacro.h"

class StockDrawAPI StockDraw
{
protected:
	/** 构造函数
	*/
	StockDraw();

public:
	/** 单一实例
	@return 返回单一实例
	*/
	static StockDraw& instance();

public:
	/** 显示avg的kxian图
	*/
	void showAvgKLine();
};