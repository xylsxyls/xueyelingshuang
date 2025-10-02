#include "AnalyzeTask.h"
#include "Ctxt/CtxtAPI.h"
#include "CStringManager/CStringManagerAPI.h"

struct PriceInfo
{
	int32_t price;
	int32_t shares;
	char buy_sell;
	PriceInfo() :
		price(0),
		shares(0),
		buy_sell(0)
	{

	}
};

AnalyzeTask::AnalyzeTask():
m_exit(false)
{

}

void AnalyzeTask::DoTask()
{
	Ctxt txt(m_path);
	txt.LoadTxt(Ctxt::Load::SPLIT, "\t");
	for (size_t lineIndex = 0; lineIndex < txt.m_vectxt.size(); ++lineIndex)
	{
		if (lineIndex <= 1)
		{
			continue;
		}
		const std::vector<std::string>& line = txt.m_vectxt[lineIndex];
		if (line.size() < 5)
		{
			continue;
		}
		else if (line.size() <= 6)
		{
			std::string strTime = CStringManager::Replace(line[0].c_str(), ":", "");
			int time = atoi(strTime.c_str());
			PriceInfo info;
			std::string strPrice = CStringManager::Replace(line[1].c_str(), " ", "");
			strPrice = CStringManager::Replace(strPrice.c_str(), ".", "");
			info.price = atoi(strPrice.c_str());
			std::string strShares = CStringManager::Replace(line[2].c_str(), " ", "");
			info.shares = atoi(strShares.c_str());
			if (line.size() == 6 && !line[4].empty())
			{
				info.buy_sell = line[4][0];
			}
		}
		else
		{
			RCSend("analyze error, file = %s, line = %d", m_path.c_str(), (int32_t)lineIndex + 1);
		}
	}
	RCSend("end = %s", m_path.c_str());
}

void AnalyzeTask::StopTask()
{
	m_exit = true;
}

void AnalyzeTask::setParam(const std::string& path)
{
	m_path = path;
}
