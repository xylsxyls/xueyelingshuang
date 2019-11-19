#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include <string>
#include "BaseTask.h"

class ClearTask : public BaseTask
{
public:
	void DoTask();
};