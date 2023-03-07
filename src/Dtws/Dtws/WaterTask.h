#pragma once
#include "AssignTask.h"

class WaterTask : public AssignTask
{
public:
	void DoTask();

	AssignTask* copy();
};