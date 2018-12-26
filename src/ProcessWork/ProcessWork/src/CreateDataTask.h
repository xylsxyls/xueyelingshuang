#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class ProcessWork;

class CreateDataTask : public CTask
{
public:
	CreateDataTask();

public:
	virtual void DoTask();

	virtual void StopTask();

	void setClient(ProcessWork* client);

protected:
	ProcessWork* m_client;
	bool m_exit;
	HANDLE m_hCreateData;
};