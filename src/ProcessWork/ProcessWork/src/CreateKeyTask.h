#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class ProcessWork;

class CreateKeyTask : public CTask
{
public:
	CreateKeyTask();

public:
	virtual void DoTask();

	virtual void StopTask();

	void setClient(ProcessWork* client);

protected:
	ProcessWork* m_client;
	bool m_exit;
	HANDLE m_hCreateKey;
};