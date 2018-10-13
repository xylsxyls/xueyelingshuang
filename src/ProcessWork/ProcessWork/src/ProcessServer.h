#pragma once
#include "ProcessWorkMacro.h"
#include <string>
#include <stdint.h>

class ServerReceiveCallback;
/** 共享内存客户端，客户端只能发送
*/
class ProcessWorkAPI ProcessServer
{
public:
	ProcessServer();

public:
	void listen(const std::string& mapName, ServerReceiveCallback* receive);

protected:
	ServerReceiveCallback* m_callback;
};