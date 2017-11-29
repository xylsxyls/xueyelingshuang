#include "stdafx.h"
#include "DataNeuron.h"
#include "FundHelper.h"

DataNeuron::DataNeuron()
{

}

double DataNeuron::ForecastData(int32_t days, double fluctuation)
{
	//起始点从昨天开始
}

void DataNeuron::AnalyzeData(const std::vector<DataNeuron>& vecDataNeuron, int32_t index, double& updata, std::map<double, IntDateTime>& mapUpData)
{
	updata += FundHelper::Square(vecDataNeuron[index].m_dayChg * 10000 - m_dayChg * 10000);
	++index;
	if (index < (int32_t)vecDataNeuron.size() && m_preData != nullptr)
	{
		m_preData->AnalyzeData(vecDataNeuron, index, updata, mapUpData);
	}
	else if (index == vecDataNeuron.size())
	{
		mapUpData[updata] = m_time;
	}
	if (m_preData != nullptr)
	{
		double preUpdata = 0;
		m_preData->AnalyzeData(vecDataNeuron, 0, preUpdata, mapUpData);
	}
}