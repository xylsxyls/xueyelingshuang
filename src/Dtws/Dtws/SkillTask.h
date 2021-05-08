#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class SkillTask : public CTask
{
public:
	SkillTask();

public:
	void DoTask();

	void StopTask();

private:
	std::atomic<bool> m_exit;
};