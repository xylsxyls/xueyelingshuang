#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class NetClient;
class HeartTask : public CTask
{
public:
	HeartTask();

public:
	void DoTask();

	void StopTask();

	void setParam(int32_t time, NetClient* netClient);

private:
	int32_t m_time;
	NetClient* m_netClient;
	std::atomic<bool> m_stop;
};