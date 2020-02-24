#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class CEwqr1Task : public CTask
{
public:
	CEwqr1Task();

public:
	void DoTask();

	void setParam(bool isFlash, bool isR);

private:
	bool m_isFlash;
	bool m_isR;
};