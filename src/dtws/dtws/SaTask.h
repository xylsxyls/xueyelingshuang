#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class SaTask : public CTask
{
public:
	SaTask();

public:
	void DoTask();

	void StopTask();

private:
	std::atomic<bool> m_exit;
};