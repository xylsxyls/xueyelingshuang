#pragma once
#include "StockDrawMacro.h"

class StockDrawAPI StockDraw
{
protected:
	/** ���캯��
	*/
	StockDraw();

public:
	/** ��һʵ��
	@return ���ص�һʵ��
	*/
	static StockDraw& instance();

public:
	/** ��ʾavg��kxianͼ
	*/
	void showAvgKLine();
};