#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "LockFreeQueue/LockFreeQueueAPI.h"

struct StrategyResult;

class StrategyResultTask : public CTask
{
public:
	StrategyResultTask();

public:
	void DoTask();

	void StopTask();

	void setParam(LockFreeQueue<std::shared_ptr<StrategyResult>>* resultQueue,
		Semaphore* resultSemaphore, std::map<int32_t, std::vector<std::shared_ptr<StrategyResult>>>* resultMap,
		std::atomic<bool>* isComplete);

private:
	LockFreeQueue<std::shared_ptr<StrategyResult>>* m_resultQueue;
	Semaphore* m_resultSemaphore;
	std::map<int32_t, std::vector<std::shared_ptr<StrategyResult>>>* m_resultMap;
	std::atomic<bool>* m_isComplete;
	std::atomic<bool> m_exit;
};