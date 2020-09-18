#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class MemoryTask : public CTask
{
public:
	MemoryTask();

public:
	void DoTask();

	void StopTask();

	void setParam(uint32_t pid);

private:
	std::atomic<bool> m_exit;
	uint32_t m_pid;
};