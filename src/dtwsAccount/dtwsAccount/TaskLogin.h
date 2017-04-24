#pragma once
#include "CTask/CTaskAPI.h"

class TaskLogin : public CTask{
public:
	string account;
	string password;
	string bigServerName;
	string smallServerName;
public:
	TaskLogin();
public:
	int Run();
	void SuspendRun();
};