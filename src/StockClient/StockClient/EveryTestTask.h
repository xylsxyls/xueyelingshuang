#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class EveryTestTask : public CTask
{
public:
	void DoTask();

	void setParam(const std::string& stock);

private:
	std::string m_stock;
};