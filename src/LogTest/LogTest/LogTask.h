#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "LockFreeQueue/LockFreeQueueAPI.h"
#include "LogTestMessage.pb.h"

class Semaphore;

class LogTask : public CTask
{
public:
	LogTask();

public:
	virtual void DoTask();

	void StopTask();

	void setParam(Semaphore* logSemaphore, LockFreeQueue<std::string>* logQueue);

private:
	Semaphore* m_logSemaphore;
	LockFreeQueue<std::string>* m_logQueue;
	std::string m_buffer;
	logtest::LogTestMessage m_message;
	std::atomic<bool> m_exit;
};