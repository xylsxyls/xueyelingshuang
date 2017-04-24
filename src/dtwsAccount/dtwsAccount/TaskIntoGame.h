#pragma once
#include "CTask/CTaskAPI.h"

class TaskIntoGame : public CTask{
public:
	int Run();
	void SuspendRun();
};