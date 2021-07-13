#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "LockFreeQueue/LockFreeQueueAPI.h"
#include "LogTestMessage.pb.h"

class Semaphore;

class ScreenTask : public CTask
{
public:
	ScreenTask();

public:
	virtual void DoTask();

	void StopTask();

	void setParam(Semaphore* screenSemaphore, LockFreeQueue<std::string>* screenQueue);

private:
	Semaphore* m_screenSemaphore;
	LockFreeQueue<std::string>* m_screenQueue;
	std::string m_buffer;
	logtest::LogTestMessage m_message;
	std::atomic<bool> m_exit;
};