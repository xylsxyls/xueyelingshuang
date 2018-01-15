#pragma once
#include <memory>
#include <string>
#include "IntDateTime/IntDateTimeAPI.h"
#include <vector>
#include <stdint.h>
#include <map>

enum
{
    FORECAST_DAYS = 3,
	HOLD,
	WAIT,
    BUY,
    SELL,
    SMALL,
    BIG,
    UP,
    DOWN
};

class DataNeuron
{
public:
	DataNeuron();

public:
	double ForecastData(int32_t days);

	bool Condition(double variance, DataNeuron* time, const IntDateTime& nowTime);
	
	double GetUpDownAccuracy(int32_t days, double base = 0);

	double GetUpDown(int32_t days);

	double GetUpDownHighest(int32_t days);

	int32_t GetUpDownDays();

	int32_t GetForecastFlatDays();

	int32_t GetDays(int32_t realDays);

	double GetMinMaxChg(int32_t days, bool isMin);

	double GetMinMaxUpDown_5(int32_t realDays, bool isMin);

	//bid 1，sell 2
	double GetBidSellAvg_5(int32_t type, int32_t bidSell);

	double GetAlways(DataNeuron* begin);

private:
	void AnalyzeData(const std::vector<DataNeuron>& vecDataNeuron,
					 int32_t index,
					 double& updata,
					 std::map<double, std::vector<DataNeuron*>>& mapUpData);

	DataNeuron* GetPreNeuron(int32_t days);

	DataNeuron* GetNextNeuron(int32_t days);

public:
	double m_dayChg = 0;
	
	IntDateTime m_time;
	DataNeuron* m_preData = nullptr;
	DataNeuron* m_nextData = nullptr;
	double m_unitNetValue = 0;
	double m_allNetValue = 0;

	double m_forecast1 = 0;
	double m_forecast2 = 0;
	double m_forecast3 = 0;

    //3天
    double m_upDown_3 = 0;

    double m_always_15days_3 = 0;
    double m_highest_15days_3 = 0;
    double m_bid_15days_3 = 0;
    double m_sell_15days_3 = 0;
    
    double m_always_30days_3 = 0;
    double m_highest_30days_3 = 0;
    double m_bid_30days_3 = 0;
    double m_sell_30days_3 = 0;
    
    double m_always_90days_3 = 0;
    double m_highest_90days_3 = 0;
    double m_bid_90days_3 = 0;
    double m_sell_90days_3 = 0;
    
    //一周
    double m_upDown_5 = 0;

    double m_always_15days_5 = 0;
    double m_highest_15days_5 = 0;
    double m_bid_15days_5 = 0;
    double m_sell_15days_5 = 0;
    
    double m_always_30days_5 = 0;
    double m_highest_30days_5 = 0;
    double m_bid_30days_5 = 0;
    double m_sell_30days_5 = 0;
    
    double m_always_90days_5 = 0;
    double m_highest_90days_5 = 0;
    double m_bid_90days_5 = 0;
    double m_sell_90days_5 = 0;
    
	double m_upDownForecast3 = 0;
	double m_upDownForecast5 = 0;
	double m_upDownHighest3 = 0;
	double m_upDownHighest5 = 0;
    double m_upDownHighest10 = 0;
	double m_upDownHighest15 = 0;
	double m_upDownHighest20 = 0;
	int32_t m_upDownInDays = 0;
	int32_t m_forecastPlatDays = 0;

	double m_avg_in_always3 = 0;
	double m_avg_in_always6 = 0;
	double m_avg_in_always12 = 0;

	//包括今天的
	double m_upDown_4 = 0;

private:
	std::string m_fundName;
	bool m_isOpenBuy;
	bool m_isOpenSell;
};