#pragma once
#include "AssignTask.h"

class SubmitTask : public AssignTask
{
public:
	void DoTask();

	AssignTask* copy();
};