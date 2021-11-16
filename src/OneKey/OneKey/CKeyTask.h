#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class CKeyTask : public CTask
{
public:
	CKeyTask();

public:
	void DoTask();

	void setParam(int32_t vkCode, bool isDown);

private:
	int32_t m_vkCode;
	bool m_isDown;
};