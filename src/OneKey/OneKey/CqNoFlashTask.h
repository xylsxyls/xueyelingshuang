#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class CqNoFlashTask : public CTask
{
public:
	CqNoFlashTask();

public:
    void DoTask();

	void setParam(int32_t sleepTime);

protected:
	int32_t m_sleepTime;
};