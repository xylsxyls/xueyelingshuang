#pragma once
#include <stdint.h>
#include <memory>
#include <string>

class CTaskThread;
typedef struct uv_tcp_s uv_tcp_t;
class LibuvTcp;

class NetWorkHelper
{
public:
	static void receive(uv_tcp_t* sender,
		const char* buffer,
		int32_t length,
		std::string& sendArea,
		uint32_t receiveThreadId,
		LibuvTcp* libuvTcp);
};