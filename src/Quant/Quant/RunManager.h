#pragma once
#include <stdint.h>

class RunManager
{
private:
	RunManager();

	~RunManager();

public:
	static RunManager& instance();

public:
	void init();

	void uninit();

	void simulateRun(uint32_t beginTime, uint32_t endTime, const std::vector<std::string>& stocks, int32_t initialFund);

private:
	uint32_t m_threadId;
};