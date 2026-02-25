#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class RunTask : public CTask
{
public:
	RunTask();

public:
	void DoTask();

	void setParam(uint32_t beginTime, uint32_t endTime, std::vector<std::string> stocks, int32_t initialFund);

private:
	uint32_t m_beginTime;
	uint32_t m_endTime;
	std::vector<std::string> m_stocks;
	int32_t m_initialFund;
};