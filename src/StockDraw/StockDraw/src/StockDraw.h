#pragma once
#include "StockDrawMacro.h"
#include "IntDateTime/IntDateTimeAPI.h"
#include "SolutionWidgetParam.h"
#include <QObject>

enum
{
	/** 总zijin
	*/
	TRADE_FUND = 1,

	/** jiaoyi记录
	*/
	TRADE_NOTE,

	/** 默认记录的gupiao
	*/
	DEFAULT_STOCK_1,

	/** 默认记录的gupiao
	*/
	DEFAULT_STOCK_2,
};

class SolutionWidget;
class StockDrawAPI StockDraw : public QObject
{
	Q_OBJECT
protected:
	/** 构造函数
	*/
	StockDraw();

public:
	/** 单一实例
	@return 返回单一实例
	*/
	static StockDraw& instance();

protected:
	/** 初始化
	*/
	void init();

public:
	/** 显示avg的kxian图
	@param [in] stock gupiao代码
	@param [in] beginDate 开始时间
	@param [in] endDate 结束时间
	*/
	void showAvgKLine(const std::string& stock, const IntDateTime& beginDate, const IntDateTime& endDate);

	/** 显示方案界面
	@param [in] solutionWidgetParam 方案界面参数
	*/
	void showSolution(const SolutionWidgetParam& solutionWidgetParam = SolutionWidgetParam());

protected:
	//方案界面
	SolutionWidget* m_solutionWidget;
};