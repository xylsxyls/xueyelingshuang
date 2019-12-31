#pragma once
#include "StockDrawMacro.h"
#include "IntDateTime/IntDateTimeAPI.h"
#include "SolutionWidgetParam.h"
#include <QObject>

enum
{
	/** ��zijin
	*/
	TRADE_FUND = 1,

	/** jiaoyi��¼
	*/
	TRADE_NOTE,

	/** Ĭ�ϼ�¼��gupiao
	*/
	DEFAULT_STOCK_1,

	/** Ĭ�ϼ�¼��gupiao
	*/
	DEFAULT_STOCK_2,
};

class SolutionWidget;
class StockDrawAPI StockDraw : public QObject
{
	Q_OBJECT
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