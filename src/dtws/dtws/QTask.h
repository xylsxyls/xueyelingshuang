#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class QTask : public CTask
{
public:
	QTask();

public:
	void DoTask();

	void StopTask();

private:
	std::atomic<bool> m_exit;
};