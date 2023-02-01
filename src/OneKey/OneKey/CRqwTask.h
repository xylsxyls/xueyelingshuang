#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class CRqwTask : public CTask
{
public:
	CRqwTask();

public:
	void DoTask();

	void setParam(char press, bool hasS);

	void pressK(char realPress);

private:
	char m_press;
	bool m_hasS;
};