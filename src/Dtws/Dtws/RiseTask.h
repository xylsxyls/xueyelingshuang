#pragma once
#include "AssignTask.h"

class RiseTask : public AssignTask
{
public:
	void DoTask();

	AssignTask* copy();
};