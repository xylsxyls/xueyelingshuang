#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
class MairubishengTask : public CTask
{
public:
	MairubishengTask();

public:
	void DoTask();

	void setParam(const std::string& stock, std::vector<std::string>* vecValidStock);

private:
	std::string m_stock;
	std::vector<std::string>* m_vecValidStock;
};