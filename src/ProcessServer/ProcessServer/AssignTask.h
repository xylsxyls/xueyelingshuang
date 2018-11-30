#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class ProcessServer;

/** ∑÷≈‰»ŒŒÒ
*/
class AssignTask : public CTask
{
public:
	AssignTask();

public:
	virtual void DoTask();

	virtual void StopTask();

	void setServer(ProcessServer* server);

private:
	ProcessServer* m_server;
	HANDLE m_hReadClientKeyEnd;
	bool m_exit;
	HANDLE m_hAssign;
	int32_t m_readKeyIndex;
	int32_t m_readKeyPosition;
};