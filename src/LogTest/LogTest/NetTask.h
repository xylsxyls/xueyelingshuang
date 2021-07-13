#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "LockFreeQueue/LockFreeQueueAPI.h"

class Semaphore;

class NetTask : public CTask
{
public:
	NetTask();

public:
	virtual void DoTask();

	void StopTask();

	void setParam(Semaphore* netSemaphore, LockFreeQueue<std::string>* netQueue);

private:
	Semaphore* m_netSemaphore;
	LockFreeQueue<std::string>* m_netQueue;
	std::string m_buffer;
	std::atomic<bool> m_exit;
};