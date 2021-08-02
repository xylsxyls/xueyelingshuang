#pragma once
#include <stdint.h>
#include <memory>
#include <string>
#include "LockFreeQueue/LockFreeQueueAPI.h"
#include "Area.h"

class CTaskThread;
typedef struct uv_tcp_s uv_tcp_t;
class LibuvTcp;
class Semaphore;

class NetWorkHelper
{
public:
	static char* send(LibuvTcp* libuvTcp, uv_tcp_t* dest, const char* buffer, int32_t length, int32_t type, int32_t head = -1);

	static void receive(uv_tcp_t* sender,
		const char* buffer,
		int32_t length,
		Area& receiveArea,
		LockFreeQueue<char*>* receiveQueue,
		Semaphore* receiveSemaphore,
		LibuvTcp* libuvTcp);

	//返回false表示全部收取到缓冲区但是还未达到规定个数
	static bool necessaryReceive(const char* buffer,
		int32_t length,
		int32_t receiveCount,
		int32_t& vernier,
		std::string& receiveArea);
};