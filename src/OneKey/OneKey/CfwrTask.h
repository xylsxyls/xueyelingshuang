#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class CfwrTask : public CTask
{
public:
	CfwrTask();

public:
	void DoTask();

	void StopTask();

	void KeyPressE();

	void KeyPressF();

	void KeyPressR();

	bool Sleep(int32_t sleepTime);

	void setParam(uint32_t editIndex);

private:
	uint32_t m_editIndex;
	std::atomic<bool> m_exit;
};