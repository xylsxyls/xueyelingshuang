#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include <atomic>

class CqTask : public CTask
{
public:
	CqTask();

public:
	void DoTask();

	void StopTask();

private:
	std::atomic<bool> m_exit;
};