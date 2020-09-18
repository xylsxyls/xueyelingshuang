#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class CpuTask : public CTask
{
public:
	CpuTask();

public:
	void DoTask();

	void StopTask();

	void setParam(uint32_t pid);

private:
	std::atomic<bool> m_exit;
	uint32_t m_pid;
};