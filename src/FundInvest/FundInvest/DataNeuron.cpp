#include "stdafx.h"
#include "DataNeuron.h"
#include "FundHelper.h"
#include "CStringManager/CStringManagerAPI.h"

DataNeuron::DataNeuron()
{

}

double DataNeuron::ForecastData(int32_t days)
{
	if (m_preData == nullptr)
	{
		return 0;
	}
	std::vector<DataNeuron> vecDataNeuron;
	int32_t dayBk = days;
	DataNeuron* thisNeuron = this;
	while (dayBk-- != 0)
	{
		vecDataNeuron.push_back(*thisNeuron);
		if (thisNeuron->m_preData == nullptr)
		{
			return 0;
		}
		thisNeuron = thisNeuron->m_preData;
	}

	double updata = 0;
	//获得的是当时预测的数据
	std::map<double, std::vector<DataNeuron*>> mapUpData;
	m_preData->AnalyzeData(vecDataNeuron, 0, updata, mapUpData);
	double all = 0;
    int32_t sampleCount = 0;
	//取出1以下的方差
	for (auto itUpdata = mapUpData.begin(); itUpdata != mapUpData.end(); ++itUpdata)
	{
        std::vector<DataNeuron*>& vecNeuron = itUpdata->second;
        int32_t index = -1;
        while (index++ != vecNeuron.size() - 1)
        {
            if (Condition(itUpdata->first, vecNeuron[index], m_time))
            {
                ++sampleCount;
                all += (1 / itUpdata->first) * itUpdata->second.size();
            }
        }
	}

    if (sampleCount < 5)
    {
        return 0;
    }

	double forecast = 0;
	for (auto itUpdata = mapUpData.begin(); itUpdata != mapUpData.end(); ++itUpdata)
	{
        std::vector<DataNeuron*>& vecNeuron = itUpdata->second;
        int32_t index = -1;
        while (index++ != vecNeuron.size() - 1)
        {
            if (Condition(itUpdata->first, vecNeuron[index], m_time))
            {
                forecast += vecNeuron[index]->m_dayChg * ((1 / itUpdata->first) / all);
            }
        }
	}
    
    //AfxMessageBox(CStringManager::Format("%.2lf%%", forecast * 100).c_str());
	int x = 3;
    return forecast;
}

bool DataNeuron::Condition(double variance, DataNeuron* forecastNeuron, const IntDateTime& nowTime)
{
    return variance < 1 && (nowTime - forecastNeuron->m_time < 365 * 24 * 60 * 60);
}

double DataNeuron::GetUpDownAccuracy(int32_t days, double base)
{
    DataNeuron* now = this;
    int32_t count = 0;
    int32_t success = 0;
    while (days-- != 0)
    {
        double real = now->m_dayChg;
        now = now->m_preData;
        if (now != nullptr)
        {
            double forecast = now->m_forecast1;
            if (forecast > base)
            {
                ++count;
                if (real > 0)
                {
                    ++success;
                }
            }
        }
    }
    return success / (double)count;
}

double DataNeuron::GetUpDown(int32_t days)
{
	DataNeuron* data = this;
	double all = 0;
	if (days < 0)
	{
		days = -days;
		while (days-- != 0)
		{
			all += data->m_dayChg;
			if (data->m_preData == nullptr)
			{
				break;
			}
			data = data->m_preData;
		}
	}
	else
	{
		while (days-- != 0)
		{
			if (data->m_nextData == nullptr)
			{
				break;
			}
			data = data->m_nextData;
			all += data->m_dayChg;
		}
	}
	
	return all;
}

double DataNeuron::GetUpDownHighest(int32_t days)
{
	double result = -10000;
	++days;
	while (days-- != 1)
	{
		double upDown = GetUpDown(days);
		if (upDown > result)
		{
			result = upDown;
		}
	}
	return result;
}

int32_t DataNeuron::GetUpDownDays()
{
	DataNeuron* data = this;
	int32_t days = 0;
	while (data->m_preData)
	{
		data = data->m_preData;
		if (data->m_dayChg * m_dayChg > 0)
		{
			if (m_dayChg > 0)
			{
				++days;
			}
			else if (m_dayChg < 0)
			{
				--days;
			}
		}
		else
		{
			break;
		}
	}
	return days;
}

int32_t DataNeuron::GetForecastFlatDays()
{
	int32_t days = 0;
	double fund = 100 * (1 - 0.15 / 100);
	DataNeuron* nowNeuron = this;
	while (nowNeuron->m_nextData)
	{
		nowNeuron = nowNeuron->m_nextData;
		fund = fund * (1 + nowNeuron->m_dayChg);
		++days;
		if (fund * (1 - 0.5 / 100) > 100)
		{
			break;
		}
	}
	if (nowNeuron->m_nextData == nullptr)
	{
		return -1;
	}
	return days;
}

int32_t DataNeuron::GetDays(int32_t realDays)
{
	IntDateTime preTime = m_time - realDays * 24 * 60 * 60;
	int32_t count = 1;
	DataNeuron* nowNeuron = this;
	while (nowNeuron->m_preData)
	{
		nowNeuron = nowNeuron->m_preData;
		if (nowNeuron->m_time < preTime)
		{
			break;
		}
		++count;
	}
	return count;
}

double DataNeuron::GetMinMaxChg(int32_t days, bool isMin)
{
	double min = 10000;
    double max = -10000;
    DataNeuron* nowNeuron = this;
    while (days-- != 0)
    {
		if (isMin)
		{
			if (nowNeuron->m_dayChg < min)
			{
				min = nowNeuron->m_dayChg;
			}
		}
		else
		{
			if (nowNeuron->m_dayChg > max)
			{
				max = nowNeuron->m_dayChg;
			}
		}
        
        if (nowNeuron->m_preData == nullptr)
        {
            break;
        }
        nowNeuron = nowNeuron->m_preData;
    }
	if (isMin)
	{
		return min;
	}
	else
	{
		return max;
	}
}

double DataNeuron::GetMinMaxUpDown_5(int32_t realDays, bool isMin)
{
	double min = 10000;
    double max = -10000;
    DataNeuron* nowNeuron = this;
	int32_t days = GetDays(realDays);
    while (days-- != 0)
    {
		if (isMin)
		{
			if (nowNeuron->m_upDown_5 < min)
			{
				min = nowNeuron->m_upDown_5;
			}
		}
		else
		{
			if (nowNeuron->m_upDown_5 > max)
			{
				max = nowNeuron->m_upDown_5;
			}
		}
        if (nowNeuron->m_preData == nullptr)
        {
            break;
        }
        nowNeuron = nowNeuron->m_preData;
    }
	if (isMin)
	{
		return min;
	}
	else
	{
		return max;
	}
}

double DataNeuron::GetBidSellAvg_5(int32_t type, int32_t bidSell)
{
    DataNeuron* nowNeuron = this;
    double result = 0;
	int32_t days = GetDays(type);
    int32_t dayCount = days;
    while (dayCount-- != 0)
    {
		if (type == 15)
        {
			if (bidSell == 1)
			{
				result += nowNeuron->m_bid_15days_5;
			}
			else if (bidSell == 2)
			{
				result += nowNeuron->m_sell_15days_5;
			}
        }
		else if (type == 30)
        {
			if (bidSell == 1)
			{
				result += nowNeuron->m_bid_30days_5;
			}
			else if (bidSell == 2)
			{
				result += nowNeuron->m_sell_30days_5;
			}
        }
        else
        {
			if (bidSell == 1)
			{
				result += nowNeuron->m_bid_90days_5;
			}
			else if (bidSell == 2)
			{
				result += nowNeuron->m_sell_90days_5;
			}
        }
        if (nowNeuron->m_preData == nullptr)
        {
            break;
        }
        nowNeuron = nowNeuron->m_preData;
    }
	return result / days;
}

void DataNeuron::AnalyzeData(const std::vector<DataNeuron>& vecDataNeuron,
							 int32_t index,
							 double& updata,
							 std::map<double, std::vector<DataNeuron*>>& mapUpData)
{
	updata += FundHelper::Square(vecDataNeuron[index].m_dayChg * 100 - m_dayChg * 100);
	++index;
	//如果到末尾了就上送
	if (index == (int32_t)vecDataNeuron.size())
	{
		DataNeuron* upNeuron = GetNextNeuron((int32_t)vecDataNeuron.size());
		if (upNeuron != nullptr)
		{
			mapUpData[updata].push_back(upNeuron);
		}
	}
	//如果没到末尾就用上一个神经元来调用
	else
	{
		if (m_preData == nullptr)
		{
			return;
		}
		m_preData->AnalyzeData(vecDataNeuron, index, updata, mapUpData);
		//如果index是1，那么才传递到上一个神经元，让神经元从头算
		if (index == 1)
		{
			double newUpdata = 0;
			m_preData->AnalyzeData(vecDataNeuron, 0, newUpdata, mapUpData);
		}
	}
}

DataNeuron* DataNeuron::GetPreNeuron(int32_t days)
{
	DataNeuron* thisNeuron = this;
	while (days-- != 0)
	{
		if (thisNeuron->m_preData == nullptr)
		{
			return nullptr;
		}
		thisNeuron = thisNeuron->m_preData;
	}
	return thisNeuron;
}

DataNeuron* DataNeuron::GetNextNeuron(int32_t days)
{
	DataNeuron* thisNeuron = this;
	while (days-- != 0)
	{
		if (thisNeuron->m_nextData == nullptr)
		{
			return nullptr;
		}
		thisNeuron = thisNeuron->m_nextData;
	}
	return thisNeuron;
}