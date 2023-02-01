#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class CKeyTask : public CTask
{
public:
	CKeyTask();

public:
	void DoTask();

	void setParam(int32_t vkCode, bool isDown);

	void setParam(int32_t vkCode);

private:
	int32_t m_vkCode;
	int32_t m_type;
};