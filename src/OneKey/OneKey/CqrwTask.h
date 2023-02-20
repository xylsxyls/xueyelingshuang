#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class CqrwTask : public CTask
{
public:
	CqrwTask();

public:
	void DoTask();

	void rClick();
};