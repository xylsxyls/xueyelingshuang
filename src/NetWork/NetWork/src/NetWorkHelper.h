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
	static char* send(uv_tcp_t* dest, const char* buffer, int32_t length, int32_t type);

	static void receive(uv_tcp_t* sender,
		const char* buffer,
		int32_t length,
		std::string& sendArea,
		uint32_t receiveThreadId,
		LibuvTcp* libuvTcp);
};