#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include <vector>
#include "Semaphore/SemaphoreAPI.h"

class AssignThreadTask : public CTask
{
public:
	void DoTask();

	void StopTask();

	void setParam(const std::vector<std::shared_ptr<CTask>>& vecSpDoTask);

private:
	Semaphore m_isInit;
	std::vector<uint32_t> m_vecThreadId;
	std::vector<std::shared_ptr<CTask>> m_vecSpDoTask;
};