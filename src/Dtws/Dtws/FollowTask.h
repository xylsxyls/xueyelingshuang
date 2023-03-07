#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "AssignTask.h"

class FollowTask : public AssignTask
{
public:
	void DoTask();

	AssignTask* copy();
};