#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include <stdint.h>

class BaseTask : public CTask
{
public:
	BaseTask();

public:
	void setStep(int32_t step);

protected:
	int32_t m_step;
};