#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class FollowTask : public CTask
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