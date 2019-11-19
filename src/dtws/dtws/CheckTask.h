#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include <string>
#include <atomic>
#include "BaseTask.h"

class CheckTask : public BaseTask
{
public:
	CheckTask();

public:
	void DoTask();

	void StopTask();

	bool Sleep(int32_t sleepTime);

private:
	std::atomic<bool> m_exit;
};