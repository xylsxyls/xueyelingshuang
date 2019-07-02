#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
class MairubishengTask : public CTask
{
public:
	MairubishengTask();

public:
	void DoTask();

	void setParam(const std::string& stock, int32_t sleepTime, std::vector<std::string>* vecValidStock);

private:
	std::string m_stock;
	int32_t m_sleepTime;
	std::vector<std::string>* m_vecValidStock;
};