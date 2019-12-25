#pragma once
#include "StockDrawMacro.h"
#include "IntDateTime/IntDateTimeAPI.h"
#include "SolutionWidgetParam.h"

enum
{
	TRADE_FUND = 1,
	TRADE_NOTE_SIZE,
	TRADE_NOTE
};

class SolutionWidget;
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

protected:
	/** ��ʼ��
	*/
	void init();

public:
	/** ��ʾavg��kxianͼ
	@param [in] stock gupiao����
	@param [in] beginDate ��ʼʱ��
	@param [in] endDate ����ʱ��
	*/
	void showAvgKLine(const std::string& stock, const IntDateTime& beginDate, const IntDateTime& endDate);

	/** ��ʾ��������
	@param [in] solutionWidgetParam �����������
	*/
	void showSolution(const SolutionWidgetParam& solutionWidgetParam = SolutionWidgetParam());

protected:
	//��������
	SolutionWidget* m_solutionWidget;
};