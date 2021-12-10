#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class CwqTask : public CTask
{
public:
	CwqTask();

public:
	void DoTask();

	void setParam(bool isR, bool isF = true);

	void KeyPress(int32_t vkCode);

	void Sleep(int32_t sleepTime);

	void KeyPressE();

	void KeyPressF();

	void LockHero(bool edit);

	void Lock3();

private:
	bool m_isR;
	bool m_isF;
};