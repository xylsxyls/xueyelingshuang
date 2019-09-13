#pragma once
#include "QtControls/DialogShow.h"
#include "IntDateTime/IntDateTimeAPI.h"
#include "BigNumber/BigNumberAPI.h"

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
	@param [in] stock gupiao代码
	@param [in] beginDate 开始时间
	@param [in] endDate 结束时间
	*/
	void setAvgMarketParam(const std::string& stock, const IntDateTime& beginDate, const IntDateTime& endDate);

	/** 设置hangqing参数
	@param [in] stock gupiao代码
	@param [in] beginDate 开始时间
	@param [in] endDate 结束时间
	*/
	void setMarketParam(const std::string& stock, const IntDateTime& beginDate, const IntDateTime& endDate);

	/** 绘制一天的kxian图
	@param [in] painter 绘画类
	@param [in] x 起始位置
	@param [in] width 一天kxian图的宽度
	@param [in] y 最高可画的高度
	@param [in] height 可画的高度
	@param [in] rise shangzhang颜色
	@param [in] fall xiadie颜色
	@param [in] flat pingpan颜色
	@param [in] maxHigh 所有hangqing的zuigao
	@param [in] minLow 所有hangqing的zuidi
	@param [in] dayMarket 一天的hangqing
	*/
	void drawOneKLine(QPainter& painter,
		int32_t x,
		int32_t width,
		int32_t y,
		int32_t height,
		const QColor& rise,
		const QColor& fall,
		const QColor& flat,
		const BigNumber& maxHigh,
		const BigNumber& minLow,
		const std::vector<BigNumber>& dayMarket);

	/** 绘制kxian图
	@param [in] painter 绘画类
	@param [in] x 起始位置
	@param [in] width 一天kxian图的宽度
	@param [in] y 最高可画的高度
	@param [in] height 可画的高度
	@param [in] space kxian之间的间隔
	@param [in] rise shangzhang颜色
	@param [in] fall xiadie颜色
	@param [in] flat pingpan颜色
	@param [in] market hangqing
	*/
	void drawKLine(QPainter& painter,
		int32_t x,
		int32_t width,
		int32_t y,
		int32_t height,
		int32_t space,
		const QColor& rise,
		const QColor& fall,
		const QColor& flat,
		const std::map<IntDateTime, std::vector<BigNumber>>& market);

protected:
	void getMaxHighMinLow(BigNumber& maxHigh, BigNumber& minLow);

protected:
	void resizeEvent(QResizeEvent* eve);
	void paintEvent(QPaintEvent* eve);

private:
	std::string m_stock;
	std::map<IntDateTime, std::vector<BigNumber>> m_market;
	std::map<IntDateTime, std::vector<BigNumber>> m_avgMarket;
};