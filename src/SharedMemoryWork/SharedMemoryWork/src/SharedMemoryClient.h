#pragma once
#include "SharedMemoryWorkMacro.h"
#include <string>
#include <stdint.h>

/** �����ڴ�ͻ��ˣ��ͻ���ֻ�ܷ���
*/
class SharedMemoryWorkAPI SharedMemoryClient
{
public:
	void connect(const std::string& mapName);

	void send(char* buffer, int32_t length);
};