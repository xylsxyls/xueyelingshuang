#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include <atomic>

class IntoGameTask : public CTask
{
public:
	IntoGameTask();

public:
	void DoTask();

	void StopTask();

	void setParam(int32_t time);

private:
	std::atomic<bool> m_exit;
	int32_t m_time;
};