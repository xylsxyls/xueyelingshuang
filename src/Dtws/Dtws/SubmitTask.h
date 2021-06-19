#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class SubmitTask : public CTask
{
public:
	SubmitTask();

public:
	void DoTask();

	void setParam(int32_t sleepTime);

private:
	int32_t m_sleepTime;
};