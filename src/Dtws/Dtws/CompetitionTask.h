#pragma once
#include "AssignTask.h"

class CompetitionTask : public AssignTask
{
public:
	void DoTask();

	AssignTask* copy();
};