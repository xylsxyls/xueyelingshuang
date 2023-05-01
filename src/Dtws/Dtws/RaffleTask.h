#pragma once
#include "AssignTask.h"

class RaffleTask : public AssignTask
{
public:
	void DoTask();

	AssignTask* copy();
};