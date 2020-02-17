#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class Three5Task : public CTask
{
public:
	void DoTask();

	void StopTask();

private:
	std::atomic<bool> m_exit;
};