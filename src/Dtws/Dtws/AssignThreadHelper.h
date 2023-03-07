#pragma once
#include <memory>
#include "AssignTask.h"

class AssignThreadHelper
{
public:
	static void postEveryAssignTask(const std::shared_ptr<AssignTask>& spTask);
};