#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class CWeaTask : public CTask
{
public:
	CWeaTask();

public:
	void DoTask();

	void StopTask();

	void setParam(int32_t vkCode);

private:
	int32_t m_vkCode;
	std::atomic<bool> m_exit;
};