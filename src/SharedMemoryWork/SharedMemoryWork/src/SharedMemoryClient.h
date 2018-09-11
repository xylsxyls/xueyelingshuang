#pragma once
#include "SharedMemoryWorkMacro.h"
#include <string>
#include <stdint.h>

/** 共享内存客户端，客户端只能发送
*/
class SharedMemoryWorkAPI SharedMemoryClient
{
public:
	void connect(const std::string& mapName);

	void send(char* buffer, int32_t length);
};