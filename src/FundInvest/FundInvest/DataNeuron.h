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
};

class DataNeuron
{
public:
	DataNeuron();

public:
	double ForecastData(int32_t days);

    bool Condition(double variance, DataNeuron* time);

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

private:
	std::string m_fundName;
	bool m_isOpenBuy;
	bool m_isOpenSell;
};