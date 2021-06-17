#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include <vector>
#include <mutex>

class AssignThreadTask : public CTask
{
public:
	AssignThreadTask();

	~AssignThreadTask();

public:
	void DoTask();

	void StopTask();

	void setParam(const std::vector<std::shared_ptr<CTask>>& vecSpDoTask);

private:
	std::mutex m_isInit;
	std::vector<uint32_t> m_vecThreadId;
	std::vector<std::shared_ptr<CTask>> m_vecSpDoTask;
	std::atomic<bool> m_exit;
};