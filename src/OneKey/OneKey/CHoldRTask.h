#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class CStopWatch;

class CHoldRTask : public CTask
{
public:
	CHoldRTask();

public:
	void DoTask();

	void setParam(bool isDown, CStopWatch* tabStopWatch);

private:
	bool m_isDown;
	CStopWatch* m_tabStopWatch;
};