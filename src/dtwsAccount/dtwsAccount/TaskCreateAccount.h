#pragma once
#include "CTask/CTaskAPI.h"

class TaskCreateAccount : public CTask{
public:
	string job;
	string name;
public:
	int initRun();
	int Run();
	void SuspendRun();
	void ExceptionRun(int exception);
};