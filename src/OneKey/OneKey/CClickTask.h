#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class CClickTask : public CTask
{
public:
	CClickTask();

public:
	void DoTask();

	void setParam(int32_t ctrlCode, bool isSuper);

private:
	int32_t m_ctrlCode;
	bool m_isSuper;
};