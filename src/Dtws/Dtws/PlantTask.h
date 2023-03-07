#pragma once
#include "AssignTask.h"

class PlantTask : public AssignTask
{
public:
	void DoTask();

	AssignTask* copy();
};