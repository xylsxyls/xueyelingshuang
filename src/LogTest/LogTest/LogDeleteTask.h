#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class LogDeleteTask : public CTask
{
public:
	LogDeleteTask();

public:
	void DoTask();

	void StopTask();

	void setParam(std::atomic<bool>* needSendDeleteLog, std::atomic<int32_t>* lastLogTime, const std::shared_ptr<CTaskThread>& logThread);

private:
	std::atomic<bool>* m_needSendDeleteLog;
	std::atomic<int32_t>* m_lastLogTime;
	std::shared_ptr<CTaskThread> m_logThread;
	std::atomic<bool> m_exit;
};