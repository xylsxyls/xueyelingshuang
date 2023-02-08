#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class ATask : public CTask
{
public:
	ATask();

public:
	void DoTask();

	void StopTask();

	virtual bool ReExecute();

	virtual CTask* Clone();

private:
	std::atomic<bool> m_exit;
};