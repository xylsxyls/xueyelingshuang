#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "LockFreeQueue/LockFreeQueueAPI.h"

class Semaphore;

class LogDeleteTask : public CTask
{
public:
	LogDeleteTask();

public:
	void DoTask();

	void StopTask();

	void setParam(Semaphore* logSemaphore,
		LockFreeQueue<std::string>* logQueue,
		std::atomic<int32_t>* lastLogTime);

private:
	Semaphore* m_logSemaphore;
	LockFreeQueue<std::string>* m_logQueue;
	std::atomic<int32_t>* m_lastLogTime;
	std::atomic<bool> m_exit;
};