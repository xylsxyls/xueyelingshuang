#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "KeyPackage.h"

class ProcessWork;

class WorkTask : public CTask
{
public:
	WorkTask();

public:
	virtual void DoTask();

	void setKey(const KeyPackage& keyPackage);

	void setClient(ProcessWork* client);

private:
	KeyPackage m_keyPackage;
	ProcessWork* m_client;
	HANDLE m_hDeleteEnd;
	bool m_exit;
};