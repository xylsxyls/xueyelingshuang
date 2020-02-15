#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class CrFlashTask : public CTask
{
public:
	CrFlashTask();

public:
    void DoTask();

	void setParam(bool isFlash);

private:
	bool m_isFlash;
};