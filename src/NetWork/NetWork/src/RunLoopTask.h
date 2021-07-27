#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class LibuvTcp;

class RunLoopTask : public CTask
{
public:
	RunLoopTask();

public:
	void DoTask();

	void StopTask();

	void setParam(LibuvTcp* libuvTcp);
	
private:
	LibuvTcp* m_libuvTcp;
};