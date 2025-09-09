#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class CEscapeR : public CTask
{
public:
	CEscapeR();

public:
	void DoTask();

	void StopTask();

private:
	std::atomic<bool> m_exit;
};