#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "CommonTaskMacro.h"

class CommonTaskAPI KeyTask : public CTask
{
public:
	KeyTask();

public:
	void DoTask();

	void setParam(int32_t vkCode);

	void setParam(int32_t vkCode, bool isDown);

private:
	int32_t m_vkCode;
	int32_t m_type;
};