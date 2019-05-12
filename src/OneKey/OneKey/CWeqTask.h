#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class CWeqTask : public CTask
{
public:
	CWeqTask();

public:
	void DoTask();

	void setParam(int32_t sleepTime);

private:
	int32_t m_sleepTime;
};