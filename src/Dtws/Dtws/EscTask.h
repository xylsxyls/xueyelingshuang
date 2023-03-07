#pragma once
#include "AssignTask.h"

class EscTask : public AssignTask
{
public:
	void DoTask();

	AssignTask* copy();
};