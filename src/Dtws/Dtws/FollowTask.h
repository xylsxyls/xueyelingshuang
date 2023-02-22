#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "SleepBaseTask.h"

class FollowTask : public SleepBaseTask
{
public:
	FollowTask();

public:
	void DoTask();

	void StopTask();

	void setParam(bool isHeal);

private:
	bool m_isHeal;
	std::atomic<bool> m_exit;
};