#pragma once
#include "NetWorkMacro.h"
#include <stdint.h>
#include <string>
#include <map>
#include <vector>
#include <list>
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "LockFreeQueue/LockFreeQueueAPI.h"

class LibuvTcp;
class CallbackBase;
typedef struct uv_tcp_s uv_tcp_t;

class ReceiveNetTask : public CTask
{
public:
	ReceiveNetTask();

public:
	void DoTask();

	void StopTask();

	void setParam(LockFreeQueue<char*>* receiveQueue, Semaphore* receiveSemaphore, LibuvTcp* libuvTcp);
	
private:
	LockFreeQueue<char*>* m_receiveQueue;
	Semaphore* m_receiveSemaphore;
	LibuvTcp* m_libuvTcp;
	std::atomic<bool> m_exit;
};