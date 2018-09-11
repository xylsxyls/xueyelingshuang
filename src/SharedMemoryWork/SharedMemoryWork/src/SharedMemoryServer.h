#pragma once
#include "SharedMemoryWorkMacro.h"
#include <string>
#include <stdint.h>

class ServerReceiveCallback;
/** �����ڴ�ͻ��ˣ��ͻ���ֻ�ܷ���
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