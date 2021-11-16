#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class HeroTask : public CTask
{
public:
	HeroTask();

public:
	void DoTask();

	void setParam(int32_t code1, int32_t code2);

private:
	int32_t m_code1;
	int32_t m_code2;
};