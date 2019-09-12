#pragma once
#include "StockDrawMacro.h"
#include "IntDateTime/IntDateTimeAPI.h"

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
	@param [in] stock gupiao����
	@param [in] beginDate ��ʼʱ��
	@param [in] endDate ����ʱ��
	*/
	void showAvgKLine(const std::string& stock, const IntDateTime& beginDate, const IntDateTime& endDate);
};