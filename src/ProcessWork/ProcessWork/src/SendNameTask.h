#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "LockFreeQueue/LockFreeQueueAPI.h"

class Semaphore;

class SendNameTask : public CTask
{
public:
	SendNameTask();

public:
	void DoTask();

	void StopTask();

	void setParam(std::atomic<bool>* waitEndPostName, LockFreeQueue<char*>* postNameQueue, Semaphore* postNameSemaphore);

private:
	std::atomic<bool>* m_waitEndPostName;
	LockFreeQueue<char*>* m_postNameQueue;
	Semaphore* m_postNameSemaphore;
	std::atomic<bool> m_exit;
};