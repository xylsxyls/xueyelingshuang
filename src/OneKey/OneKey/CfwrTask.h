#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class CfwrTask : public CTask
{
public:
	CfwrTask();

public:
	void DoTask();

	void KeyPressE();

	void KeyPressF();

	void KeyPressR();

	void setParam(int32_t editIndex);

private:
	int32_t m_editIndex;
};