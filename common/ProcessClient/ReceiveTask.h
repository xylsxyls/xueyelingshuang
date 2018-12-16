#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class ProcessClient;

class ReceiveTask : public CTask
{
public:
	ReceiveTask();

public:
	virtual void DoTask();

	virtual void StopTask();

public:
	void setClient(ProcessClient* client);

private:
	HANDLE m_hClientReadKey;
	ProcessClient* m_client;
	bool m_exit;
};