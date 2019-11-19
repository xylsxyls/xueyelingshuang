#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include <string>
#include "BaseTask.h"

class MainTask : public BaseTask
{
public:
	void DoTask();
};