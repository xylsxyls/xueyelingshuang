#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class CwqTask : public CTask
{
public:
	CwqTask();

public:
	void DoTask();

	void setParam(bool isR);

	void KeyPress(int32_t vkCode);

	void Sleep(int32_t sleepTime);

	void KeyPressE(bool edit);

private:
	bool m_isR;
};