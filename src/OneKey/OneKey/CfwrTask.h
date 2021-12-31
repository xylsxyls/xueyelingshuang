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

	void setParam(uint32_t editIndex);

private:
	uint32_t m_editIndex;
};