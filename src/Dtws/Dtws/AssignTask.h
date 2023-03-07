#pragma once
#include "CommonTask/CommonTaskAPI.h"

class AssignTask : public SleepBaseTask
{
public:
	AssignTask();

public:
	virtual AssignTask* copy();

public:
	void setAccountIndex(int32_t accountIndex);

protected:
	int32_t m_accountIndex;
};