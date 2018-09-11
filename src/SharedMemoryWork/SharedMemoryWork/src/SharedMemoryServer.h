#pragma once
#include "SharedMemoryWorkMacro.h"
#include <string>
#include <stdint.h>

class ServerReceiveCallback;
/** 共享内存客户端，客户端只能发送
*/
class SharedMemoryWorkAPI SharedMemoryServer
{
public:
	SharedMemoryServer();

public:
	void listen(const std::string& mapName, int32_t bufferMaxLength, ServerReceiveCallback* receive);

protected:
	ServerReceiveCallback* m_callback;
};