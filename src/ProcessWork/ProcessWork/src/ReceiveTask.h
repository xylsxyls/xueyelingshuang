#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class ProcessWork;

class ReceiveTask : public CTask
{
public:
	ReceiveTask();

public:
	virtual void DoTask();

	virtual void StopTask();

public:
	void setClient(ProcessWork* client);

private:
	HANDLE m_hClientReadKey;
	ProcessWork* m_client;
	bool m_exit;
};