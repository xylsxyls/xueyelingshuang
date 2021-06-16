#pragma once
#include "GoFindClickTask.h"

class JidiTask : public GoFindClickTask
{
public:
	void setParam(int32_t preSleepTime, int32_t clientIndex);
};